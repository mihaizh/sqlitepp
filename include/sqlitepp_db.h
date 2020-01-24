#ifndef SQLITEPP_DATABASE_H
#define SQLITEPP_DATABASE_H

#include <memory>
#include <string>
#include <cassert>

#include "sqlite3_inc.h"
#include "sqlitepp_stmt.h"

namespace sqlitepp
{

class database
{
public:
    database() noexcept = default;
    database(const char* db);
    database(const std::string& db);
    database(const char* db, int& result);
    database(const std::string& db, int& result);
    database(const database&) = delete;
    database(database&&) noexcept;
    database& operator=(const database&) = delete;
    database& operator=(database&&) noexcept;
    ~database();

    int open(const char* db, int flags);
    int open(const std::string& db, int flags);
    int close();

    template <typename... Args>
    statement prepare(const char* query, const Args&... args) const;
    statement prepare(const char* query) const;
    int execute(const char* query) const;

    int toggle_extended_result_codes();
    bool is_using_extended_result_codes() const;

private:
    sqlite3* m_handle = nullptr;
    bool m_extended_result_codes = false;

}; // database

template <typename... Args>
statement database::prepare(const char* query, const Args&... args) const
{
    statement stmt = prepare(query);
    stmt.bind(args...);

    return stmt;
}

} // sqlitepp

#endif // SQLITEPP_DATABASE_H
