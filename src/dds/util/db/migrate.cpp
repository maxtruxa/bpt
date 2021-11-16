#include "./migrate.hpp"

#include "./db.hpp"

#include <dds/error/result.hpp>

#include <fmt/format.h>
#include <neo/sqlite3/database.hpp>
#include <neo/sqlite3/error.hpp>
#include <neo/sqlite3/statement.hpp>
#include <neo/sqlite3/transaction.hpp>

using namespace dds;
namespace nsql = neo::sqlite3;

result<void> detail::do_migrations_1(unique_database&                                 db,
                                     std::string_view                                 tablename,
                                     std::initializer_list<detail::erased_migration*> migrations) {
    auto init_meta_table = fmt::format(
        R"(
        CREATE TABLE IF NOT EXISTS "{}" AS
            WITH init (version) AS (VALUES (0))
            SELECT * FROM init
        )",
        tablename);
    auto st = db.raw_database().prepare(init_meta_table);
    if (!st.has_value()) {
        return new_error(e_migration_error{
            fmt::format("Failed to prepare initialize-meta-table statement for '{}': {}: {}",
                        tablename,
                        nsql::make_error_code(st.errc()).message(),
                        db.raw_database().error_message())});
    }
    auto step_rc = st->step();
    if (step_rc != neo::sqlite3::errc::done) {
        return new_error(
            e_migration_error{fmt::format("Failed to initialize migration meta-table '{}': {}: {}",
                                          tablename,
                                          nsql::make_error_code(st.errc()).message(),
                                          db.raw_database().error_message())});
    }

    // Check the migration version
    BOOST_LEAF_AUTO(version, get_migration_version(db, tablename));
    if (version < 0) {
        return new_error(e_migration_error{"Database migration value is negative"});
    }
    if (version > static_cast<int>(migrations.size())) {
        return new_error(e_migration_error{"Database migration is too new"});
    }

    // Wrap migrations in a transaction
    neo::sqlite3::transaction_guard tr{db.raw_database()};

    // Apply individual migrations
    auto it = migrations.begin() + version;
    for (; it != migrations.end(); ++it) {
        try {
            (*it)->apply(db);
        } catch (const neo::sqlite3::error& err) {
            tr.rollback();
            return new_error(e_migration_error{std::string(err.what())});
        }
    }

    // Update the version in the meta table
    std::string query = fmt::format("UPDATE \"{}\" SET version = {}", tablename, migrations.size());
    st                = *db.raw_database().prepare(query);
    step_rc           = st->step();
    if (step_rc != neo::sqlite3::errc::done) {
        tr.rollback();
        return new_error(
            e_migration_error{fmt::format("Failed to update migration version on '{}': {}: {}",
                                          tablename,
                                          nsql::make_error_code(step_rc.errc()).message(),
                                          db.raw_database().error_message())});
    }

    return {};
}

result<int> dds::get_migration_version(unique_database& db, std::string_view tablename) {
    auto q  = fmt::format("SELECT version FROM \"{}\"", tablename);
    auto st = db.raw_database().prepare(q);
    if (!st.has_value()) {
        return new_error(e_migration_error{
            fmt::format("Failed to find version for migrations table '{}': {}: {}",
                        tablename,
                        nsql::make_error_code(st.errc()).message(),
                        db.raw_database().error_message())});
    }
    auto step_rc = st->step();
    if (step_rc != neo::sqlite3::errc::row) {
        return new_error("Failed to find version for migrations table '{}': {}: {}",
                         tablename,
                         nsql::make_error_code(step_rc.errc()).message(),
                         db.raw_database().error_message());
    }
    auto r = st->row()[0];
    if (!r.is_integer()) {
        return new_error(e_migration_error{
            fmt::format("Invalid 'version' in meta table '{}': Value must be an integer",
                        tablename)});
    }
    return static_cast<int>(r.as_integer());
}
