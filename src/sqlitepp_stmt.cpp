#include "sqlitepp_stmt.h"

namespace sqlitepp
{

namespace detail
{
    int bind(sqlite3_stmt* stmt, int index, int32_t value)
    {
        return sqlite3_bind_int(stmt, index, value);
    }

    int bind(sqlite3_stmt* stmt, int index, int64_t value)
    {
        return sqlite3_bind_int64(stmt, index, value);
    }

    int bind(sqlite3_stmt* stmt, int index, double value)
    {
        return sqlite3_bind_double(stmt, index, value);
    }

    int bind(sqlite3_stmt* stmt, int index, const char* value)
    {
        return sqlite3_bind_text(stmt, index, value, (int)strlen(value), SQLITE_STATIC);
    }

    int bind(sqlite3_stmt* stmt, int index, const std::string& value)
    {
        return sqlite3_bind_text(stmt, index, value.data(), (int)value.length(), SQLITE_STATIC);
    }

    int bind(sqlite3_stmt* stmt, int index, const std::vector<char>& value)
    {
        return sqlite3_bind_blob(stmt, index, value.data(), (int)value.size(), SQLITE_STATIC);
    }

    int bind(sqlite3_stmt* stmt, int index, const void* src_ptr, size_t length)
    {
        return sqlite3_bind_blob(stmt, index, src_ptr, (int)length, SQLITE_STATIC);
    }

    int read(sqlite3_stmt* stmt, int index, int32_t& value)
    {
        if (sqlite3_column_type(stmt, index) != SQLITE_INTEGER)
            return SQLITE_MISMATCH;

        value = sqlite3_column_int(stmt, index);
        return SQLITE_OK;
    }

    int read(sqlite3_stmt* stmt, int index, int64_t& value)
    {
        if (sqlite3_column_type(stmt, index) != SQLITE_INTEGER)
            return SQLITE_MISMATCH;

        value = sqlite3_column_int64(stmt, index);
        return SQLITE_OK;
    }

    int read(sqlite3_stmt* stmt, int index, double& value)
    {
        if (sqlite3_column_type(stmt, index) != SQLITE_FLOAT)
            return SQLITE_MISMATCH;

        value = sqlite3_column_double(stmt, index);
        return SQLITE_OK;
    }

    int read(sqlite3_stmt* stmt, int index, std::string& value)
    {
        if (sqlite3_column_type(stmt, index) != SQLITE_TEXT)
            return SQLITE_MISMATCH;

        value = reinterpret_cast<const char*>(sqlite3_column_text(stmt, index));
        return SQLITE_OK;
    }

    int read(sqlite3_stmt* stmt, int index, std::vector<char>& value)
    {
        if (sqlite3_column_type(stmt, index) != SQLITE_BLOB)
            return SQLITE_MISMATCH;

        const auto* ptr = sqlite3_column_blob(stmt, index);
        if (ptr != nullptr)
        {
            const auto size = sqlite3_column_bytes(stmt, index);
            value.resize(size);
            memcpy(value.data(), ptr, size);
        }

        return SQLITE_OK;
    }

    int read(sqlite3_stmt* stmt, int index, void* dst_ptr, size_t length)
    {
        if (sqlite3_column_type(stmt, index) != SQLITE_BLOB)
            return SQLITE_MISMATCH;

        const auto size = sqlite3_column_bytes(stmt, index);
        if (size != length)
            return SQLITE_MISUSE;

        const auto* src_ptr = sqlite3_column_blob(stmt, index);
        if (src_ptr != nullptr)
        {
            memcpy(dst_ptr, src_ptr, size);
        }

        return SQLITE_OK;
    }
}

statement::statement(statement&& other) noexcept
    : m_handle(other.m_handle),
    m_bind_index(other.m_bind_index),
    m_read_index(other.m_read_index),
    m_exec_status(other.m_exec_status),
    m_exec_before_next_row(other.m_exec_before_next_row)
{
    other.m_handle = nullptr;
}

statement& statement::operator=(statement&& other) noexcept
{
    if (this != &other)
    {
        m_handle = other.m_handle;
        other.m_handle = nullptr;

        m_bind_index = other.m_bind_index;
        m_read_index = other.m_read_index;
        m_exec_status = other.m_exec_status;
        m_exec_before_next_row = other.m_exec_before_next_row;
    }

    return *this;
}

int statement::bind_text(int index, const char* text)
{
    return detail::bind(m_handle, index, text);
}

int statement::bind_text(int index, const std::string& text)
{
    return detail::bind(m_handle, index, text);
}

int statement::bind_blob(int index, const std::vector<char>& blob)
{
    return detail::bind(m_handle, index, blob);
}

int statement::bind_blob(int index, const void* ptr, size_t length)
{
    return detail::bind(m_handle, index, ptr, length);
}

int statement::get_argument_index(const char* name) const
{
    return sqlite3_bind_parameter_index(m_handle, name);
}

bool statement::next_row()
{
    if (!m_exec_before_next_row)
    {
        execute();
    }

    m_exec_before_next_row = false;
    return (m_exec_status == SQLITE_ROW);
}

int statement::read_text(int index, std::string& text) const
{
    return detail::read(m_handle, index, text);
}

int statement::read_blob(int index, std::vector<char>& value) const
{
    return detail::read(m_handle, index, value);
}

int statement::read_blob(int index, void* ptr, size_t length) const
{
    return detail::read(m_handle, index, ptr, length);
}

int statement::get_column_count() const
{
    return sqlite3_column_count(m_handle);
}

const char* statement::get_column_name(int index) const
{
    return sqlite3_column_name(m_handle, index);
}

int statement::execute()
{
    m_exec_status = sqlite3_step(m_handle);
    m_exec_before_next_row = true;

    return ((m_exec_status == SQLITE_ROW) ||
            (m_exec_status == SQLITE_DONE))
        ? SQLITE_OK
        : m_exec_status;
}

int statement::finalize()
{
    int code = SQLITE_OK;
    if (m_handle != nullptr)
    {
        code = sqlite3_finalize(m_handle);
        if (code == SQLITE_OK)
        {
            m_handle = nullptr;
        }
    }

    return code;
}

statement::~statement()
{
    finalize();
}

} // sqlitepp
