#ifndef SQLITEPP_STMT_H
#define SQLITEPP_STMT_H

#include <cstdint>
#include <vector>
#include <string>
#include <cstring>
#include "sqlite3_inc.h"

namespace sqlitepp
{

namespace detail
{
    int bind(sqlite3_stmt* stmt, int index, int32_t value) noexcept;
    int bind(sqlite3_stmt* stmt, int index, int64_t value) noexcept;
    int bind(sqlite3_stmt* stmt, int index, double value) noexcept;
    int bind(sqlite3_stmt* stmt, int index, const char* value) noexcept;
    int bind(sqlite3_stmt* stmt, int index, std::string_view value) noexcept;
    int bind(sqlite3_stmt* stmt, int index, const std::string& value) noexcept;
    int bind(sqlite3_stmt* stmt, int index, const std::vector<char>& value) noexcept;
    int bind(sqlite3_stmt* stmt, int index, const void* src_ptr, size_t length) noexcept;

    int read(sqlite3_stmt* stmt, int index, int32_t& value) noexcept;
    int read(sqlite3_stmt* stmt, int index, int64_t& value) noexcept;
    int read(sqlite3_stmt* stmt, int index, double& value) noexcept;
    int read(sqlite3_stmt* stmt, int index, std::string& value) noexcept;
    int read(sqlite3_stmt* stmt, int index, std::vector<char>& value) noexcept;
    int read(sqlite3_stmt* stmt, int index, void* dst_ptr, size_t length) noexcept;

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
}

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

class statement
{
public:
    statement(statement&&) noexcept;
    statement(const statement&) = delete;
    statement& operator=(statement&&) noexcept;
    statement& operator=(const statement&) = delete;
    ~statement() noexcept;

    template <typename Arg, typename... Args>
    int bind(const Arg& first, const Args&... args) noexcept;
    template <typename Arg>
    int bind(const Arg& last) noexcept;

    template <typename Arg>
    int bind_at(int index, const Arg& arg) noexcept;

    int bind_text(int index, const char* text) noexcept;
    int bind_text(int index, std::string_view text) noexcept;
    int bind_text(int index, const std::string& text) noexcept;
    int bind_blob(int index, const std::vector<char>& value) noexcept;
    int bind_blob(int index, const void* ptr, size_t length) noexcept;
    template <typename Arg>
    int bind_blob(int index, const Arg& value) noexcept;

    int get_argument_index(const char* name) noexcept;

    int execute() noexcept;

    template <typename... Args>
    bool read(Args&... args) noexcept;

    template <typename Arg, typename... Args>
    int read_row(Arg& arg, Args&... args) noexcept;
    template <typename Arg>
    int read_row(Arg& arg) noexcept;
    template <typename Arg>
    int read_row_at(int index, Arg& arg) noexcept;

    int read_text(int index, std::string& text) noexcept;
    int read_blob(int index, std::vector<char>& value) noexcept;
    int read_blob(int index, void* ptr, size_t length) noexcept;
    template <typename Arg>
    int read_blob(int index, Arg& value) noexcept;

    int get_column_count() noexcept;
    const char* get_column_name(int index) noexcept;

    int finalize() noexcept;

    bool ok() noexcept
    {
        return m_handle != nullptr;
    }

private:
    statement() noexcept = default;

    sqlite3_stmt* m_handle = nullptr;
    int m_bind_index = 0;
    int m_read_index = 0;

    int m_exec_status = SQLITE_OK;

    friend class database;
};

template <typename Arg>
int statement::bind(const Arg& last) noexcept
{
    ++m_bind_index;
    return bind_at(m_bind_index, last);
}

template <typename Arg, typename... Args>
int statement::bind(const Arg& first, const Args&... args) noexcept
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
int statement::bind_at(int index, const Arg& arg) noexcept
{
    static_assert(!detail::is_void<Arg>::value,
        "You can't bind void types. There's no information about their size. "
        "It is recommended to use std::vector<char> to bind blobs. You can, "
        "however, pass 'const_blob' or 'blob' to bind a void type.");

    int code = SQLITE_OK;
    if constexpr (std::is_same_v<Arg, std::nullptr_t>)
    {
        code = sqlite3_bind_null(m_handle, m_bind_index);
    }
    if constexpr (std::is_same_v<Arg, const_blob> ||
                  std::is_same_v<Arg, blob>)
    {
        code = detail::bind(m_handle, m_bind_index, arg.ptr, arg.length);
    }
    else if constexpr (!std::is_same_v<Arg, skip_arg>)
    {
        code = detail::bind(m_handle, m_bind_index, arg);
    }

    return code;
}

template <typename Arg>
int statement::bind_blob(int index, const Arg& value) noexcept
{
    return detail::bind(m_handle, index, reinterpret_cast<const void*>(&value), sizeof(Arg));
}

template <typename... Args>
bool statement::read(Args&... args) noexcept
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
int statement::read_row(Arg& arg, Args&... args) noexcept
{
    ++m_read_index;
    auto code = read_row_at(m_read_index, arg);

    return (code == SQLITE_OK)
        ? read_row(args...)
        : code;
}

template <typename Arg>
int statement::read_row(Arg& arg) noexcept
{
    ++m_read_index;
    return read_row_at(m_read_index, arg);
}

template <typename Arg>
int statement::read_row_at(int index, Arg& arg) noexcept
{
    static_assert(!detail::is_void<Arg>::value,
        "You can't read void types. There's no information about their size. "
        "Use std::vector<char> to read blobs. Check 'read_blob' too.");

    static_assert(!detail::is_c_str<Arg>::value,
        "Text needs to be read into a std::string type.");

    int code = SQLITE_OK;
    if constexpr (std::is_same_v<Arg, std::nullptr_t>)
    {
        // Skip nullptr
    }
    else if constexpr (std::is_same_v<Arg, blob>)
    {
        code = detail::read(m_handle, m_read_index, arg.ptr, arg.length);
    }
    else if constexpr (!std::is_same_v<Arg, skip_arg>)
    {
        code = detail::read(m_handle, m_read_index, arg);
    }

    return code;
}

template <typename Arg>
int statement::read_blob(int index, Arg& value) noexcept
{
    return detail::read(m_handle, index, reinterpret_cast<void*>(&value), sizeof(Arg));
}

} // sqlitepp

#endif // SQLITEPP_STMT_H
