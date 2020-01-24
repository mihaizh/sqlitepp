#ifndef SQLITEPP_STMT_H
#define SQLITEPP_STMT_H

#include <cstdint>
#include <vector>
#include <string>
#include <cstring>
#include "sqlite3_inc.h"

namespace sqlitepp
{

struct skip_arg
{
};

struct const_blob
{
    const void* ptr;
    size_t length;
};

struct blob
{
    void* ptr;
    size_t length;
};

namespace detail
{
    int bind(sqlite3_stmt* stmt, int index, int32_t value);
    int bind(sqlite3_stmt* stmt, int index, int64_t value);
    int bind(sqlite3_stmt* stmt, int index, double value);
    int bind(sqlite3_stmt* stmt, int index, const char* value);
    int bind(sqlite3_stmt* stmt, int index, const std::string& value);
    int bind(sqlite3_stmt* stmt, int index, const std::vector<char>& value);
    int bind(sqlite3_stmt* stmt, int index, const void* src_ptr, size_t length);

    int read(sqlite3_stmt* stmt, int index, int32_t& value);
    int read(sqlite3_stmt* stmt, int index, int64_t& value);
    int read(sqlite3_stmt* stmt, int index, double& value);
    int read(sqlite3_stmt* stmt, int index, std::string& value);
    int read(sqlite3_stmt* stmt, int index, std::vector<char>& value);
    int read(sqlite3_stmt* stmt, int index, void* dst_ptr, size_t length);

    template<class T>
    struct is_c_str
        : std::integral_constant<
        bool,
        std::is_same<char const*, typename std::decay<T>::type>::value ||
        std::is_same<char*, typename std::decay<T>::type>::value>
    {
    };

    template<class T>
    struct is_void
        : std::integral_constant<
        bool,
        std::is_same<void const*, typename std::decay<T>::type>::value ||
        std::is_same<void*, typename std::decay<T>::type>::value ||
        std::is_void<T>::value>
    {
    };

    template <typename Arg>
    typename std::enable_if<std::is_same<Arg, std::nullptr_t>::value, int>::type
        bind_if(sqlite3_stmt* stmt, int index, const Arg& arg)
    {
        return sqlite3_bind_null(stmt, index);
    }

    template <typename Arg>
    typename std::enable_if<std::is_same<Arg, const_blob>::value ||
                            std::is_same<Arg, blob>::value>::type
        bind_if(sqlite3_stmt* stmt, int index, const Arg& arg)
    {
        return bind(stmt, index, arg.ptr, arg.length);
    }

    template <typename Arg>
    typename std::enable_if<std::is_same<Arg, skip_arg>::value, int>::type
        bind_if(sqlite3_stmt* stmt, int index, const Arg& arg)
    {
        return SQLITE_OK; // skip this
    }

    template <typename Arg>
    typename std::enable_if<!std::is_same<Arg, skip_arg>::value, int>::type
        bind_if(sqlite3_stmt* stmt, int index, const Arg& arg)
    {
        return bind(stmt, index, arg);
    }

    template <typename Arg>
    typename std::enable_if<std::is_same<Arg, std::nullptr_t>::value, int>::type
        read_if(sqlite3_stmt* stmt, int index, Arg& arg)
    {
        return SQLITE_OK; // do not read into nullptr
    }

    template <typename Arg>
    typename std::enable_if<std::is_same<Arg, blob>::value>::type
        read_if(sqlite3_stmt* stmt, int index, Arg& arg)
    {
        return read(stmt, index, arg.ptr, arg.length);
    }

    template <typename Arg>
    typename std::enable_if<std::is_same<Arg, skip_arg>::value, int>::type
        read_if(sqlite3_stmt* stmt, int index, Arg& arg)
    {
        return SQLITE_OK; // skip this
    }

    template <typename Arg>
    typename std::enable_if<!std::is_same<Arg, skip_arg>::value, int>::type
        read_if(sqlite3_stmt* stmt, int index, Arg& arg)
    {
        return read(stmt, index, arg);
    }
}

class statement
{
public:
    statement(statement&&) noexcept;
    statement(const statement&) = delete;
    statement& operator=(statement&&) noexcept;
    statement& operator=(const statement&) = delete;
    ~statement() noexcept;

    template <typename Arg, typename... Args>
    int bind(const Arg& first, const Args&... args);
    template <typename Arg>
    int bind(const Arg& last);

    template <typename Arg>
    int bind_at(int index, const Arg& arg);

    int bind_text(int index, const char* text);
    int bind_text(int index, const std::string& text);
    int bind_blob(int index, const std::vector<char>& value);
    int bind_blob(int index, const void* ptr, size_t length);
    template <typename Arg>
    int bind_blob(int index, const Arg& value);

    int get_argument_index(const char* name);

    int execute();

    template <typename... Args>
    bool read(Args&... args);

    template <typename Arg, typename... Args>
    int read_row(Arg& arg, Args&... args);
    template <typename Arg>
    int read_row(Arg& arg);
    template <typename Arg>
    int read_row_at(int index, Arg& arg);

    int read_text(int index, std::string& text);
    int read_blob(int index, std::vector<char>& value);
    int read_blob(int index, void* ptr, size_t length);
    template <typename Arg>
    int read_blob(int index, Arg& value);

    int get_column_count();
    const char* get_column_name(int index);

    int finalize();

    bool ok() const
    {
        return m_handle != nullptr;
    }

    int execution_status() const
    {
        return m_exec_status;
    }

private:
    statement() = default;

    sqlite3_stmt* m_handle = nullptr;
    int m_bind_index = 0;
    int m_read_index = 0;

    int m_exec_status = SQLITE_OK;

    friend class database;
};

template <typename Arg>
int statement::bind(const Arg& last)
{
    ++m_bind_index;
    return bind_at(m_bind_index, last);
}

template <typename Arg, typename... Args>
int statement::bind(const Arg& first, const Args&... args)
{
    // increase index counter
    ++m_bind_index;
    // bind current parameter
    const auto code = bind_at(m_bind_index, first);

    // bind next parameters
    return (code == SQLITE_OK)
        ? bind(args...)
        : code;
}

template <typename Arg>
int statement::bind_at(int index, const Arg& arg)
{
    static_assert(!detail::is_void<Arg>::value,
        "You can't bind void types. There's no information about their size. "
        "It is recommended to use std::vector<char> to bind blobs. You can, "
        "however, pass 'const_blob' or 'blob' to bind a void type.");

    return detail::bind_if<Arg>(m_handle, m_bind_index, arg);
}

template <typename Arg>
int statement::bind_blob(int index, const Arg& value)
{
    return detail::bind(m_handle, index, reinterpret_cast<const void*>(&value), sizeof(Arg));
}

template <typename... Args>
bool statement::read(Args&... args)
{
    m_read_index = -1;

    auto code = read_row(args...);
    if (code != SQLITE_OK)
    {
        // stop execution
        return false;
    }

    // go to next row
    execute();

    return (m_exec_status == SQLITE_ROW) ||
           (m_exec_status == SQLITE_DONE);
}

template <typename Arg, typename... Args>
int statement::read_row(Arg& arg, Args&... args)
{
    ++m_read_index;
    auto code = read_row_at(m_read_index, arg);

    return (code == SQLITE_OK)
        ? read_row(args...)
        : code;
}

template <typename Arg>
int statement::read_row(Arg& arg)
{
    ++m_read_index;
    return read_row_at(m_read_index, arg);
}

template <typename Arg>
int statement::read_row_at(int index, Arg& arg)
{
    static_assert(!detail::is_void<Arg>::value,
        "You can't read void types. There's no information about their size. "
        "Use std::vector<char> to read blobs. Check 'read_blob' too.");

    static_assert(!detail::is_c_str<Arg>::value,
        "Text needs to be read into a std::string type.");

    return detail::read_if<Arg>(m_handle, m_read_index, arg);
}

template <typename Arg>
int statement::read_blob(int index, Arg& value)
{
    return detail::read(m_handle, index, reinterpret_cast<void*>(&value), sizeof(Arg));
}

} // sqlitepp

#endif // SQLITEPP_STMT_H
