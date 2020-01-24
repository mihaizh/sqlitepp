#include "sqlitepp_db.h"

namespace sqlitepp
{

database::database(const char* db)
{
    assert(open(db, SQLITE_OPEN_READWRITE) == SQLITE_OK);
}

database::database(const std::string& db)
{
    assert(open(db, SQLITE_OPEN_READWRITE) == SQLITE_OK);
}

database::database(const char* db, int& result)
{
    result = open(db, SQLITE_OPEN_READWRITE);
}

database::database(const std::string& db, int& result)
{
    result = open(db, SQLITE_OPEN_READWRITE);
}

database::database(database&& other) noexcept
{
    other.m_handle = nullptr;
}

database& database::operator=(database&& other) noexcept
{
    if (this != &other)
    {
        m_handle = other.m_handle;
        other.m_handle = nullptr;
    }

    return *this;
}

database::~database()
{
    close();
}

int database::open(const std::string& db, int flags)
{
    return open(db.c_str(), flags);
}

int database::open(const char* db, int flags)
{
    return sqlite3_open_v2(db, &m_handle, flags, nullptr);
}

int database::close()
{
    if (m_handle != nullptr)
    {
        const auto code = sqlite3_close(m_handle);
        if (code == SQLITE_OK)
        {
            m_handle = nullptr;
        }

        return code;
    }

    return SQLITE_OK;
}

statement database::prepare(const char* query)
{
    statement stmt;
    const char* query_tail = nullptr;

    sqlite3_prepare_v3(m_handle, query, (int)strlen(query) + 1, 0, &stmt.m_handle, &query_tail);

    return stmt;
}

int database::execute(const char* query)
{
    return sqlite3_exec(m_handle, query, nullptr, nullptr, nullptr);
}

int database::toggle_extended_result_codes()
{
    m_extended_result_codes = !m_extended_result_codes;
    return sqlite3_extended_result_codes(m_handle, m_extended_result_codes ? 1 : 0);
}

bool database::is_using_extended_result_codes() const
{
    return m_extended_result_codes;
}

} // sqlitepp
