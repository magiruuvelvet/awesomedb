#pragma once

#include "model.hpp"
#include "table.hpp"
#include "registrar.hpp"

#include <string>
#include <cstdint>
#include <mutex>
#include <memory>

#include "config.hpp"

/**
 * Database Abstraction Library
 */
class Database final
{
public:

    // forward id type
    using id_t = Model::id_t;

#define DATABSE_ENABLE_IF_MODEL \
    typename = std::enable_if_t<std::is_base_of<Model, ModelType>::value>

    /**
     * Registers a new model in the database registrar for automatic construction.
     * Unregistered models will not be constructed when using any of the database
     * find functions. Unregistered models return a default constructed empty instance
     * of the model.
     *
     * Usage: Database::registerModel<MyModel>();
     */
    template<typename ModelType, DATABSE_ENABLE_IF_MODEL>
    static void registerModel()
    {
        DatabaseRegistrar::register_model<ModelType>(
            [](const Model::Query *query, const Database *db){
                return std::make_shared<ModelType>(ModelType(query, db));
        });
    }

    /**
     * Initializes a new database with the given configuration.
     */
    explicit Database(const DatabaseConfig &config = {});

    /**
     * Closes the database connection and cleans up all allocated resources.
     */
    ~Database();

    /**
     * Execute raw SQL query. Fetching of data isn't possible.
     */
    bool execute(const std::string &query);

    /**
     * Returns a list of all tables in the database.
     */
    const std::list<std::string> tables() const;

    /**
     * Creates a new table in the database.
     * Returns true when the creation succeeded or when the table already
     * existed before. If you want this to return false when the table
     * already exists, set the errorWhenExists parameter to true.
     */
    bool createTable(const DatabaseTable &table, bool errorWhenExists = false);

    /**
     * Drops a table from the database.
     */
    bool dropTable(const std::string &tableName);

    /**
     * Truncates a table in the database by removing all its records
     * and resetting the auto incremental field back to zero.
     */
    bool truncateTable(const std::string &tableName);

    /**
     * Checks if a connection to the database is possible.
     * Sets the last error message when an error occurs.
     */
    bool canConnect() const;

    /**
     * Receives the last error message from the database server.
     */
    constexpr inline const auto &lastErrorMessage() const
    { return this->_lastErrorMessage; }

    /**
     * Saves the given model back to the database.
     */
    bool saveRecord(Model *model);

    /**
     * Deletes the given model from the database.
     */
    bool deleteRecord(Model *model);

    /**
     * Finds the given model record for the given id.
     */
    template<typename ModelType, DATABSE_ENABLE_IF_MODEL>
    ModelType findRecord(id_t id, bool *error = nullptr) const
    {
        const std::lock_guard lock{this->_mutex};
        if (!this->open(error)) return {};
        const auto result = this->internal_find(ModelType(), &id, nullptr, std::any(ModelType()), error);
        this->close();

        return result ? (*dynamic_cast<const ModelType*>(result.get())) : ModelType{};
    }

    /**
     * Finds a single record using a filter pattern.
     * The filter pattern is NOT injection protected!! Don't use user data for the filter.
     */
    template<typename ModelType, DATABSE_ENABLE_IF_MODEL>
    ModelType findRecord(const std::string filter, bool *error = nullptr) const
    {
        const std::lock_guard lock{this->_mutex};
        if (!this->open(error)) return {};
        const auto result = this->internal_find(ModelType(), nullptr, &filter, std::any(ModelType()), error);
        this->close();

        return result ? (*dynamic_cast<const ModelType*>(result.get())) : ModelType{};
    }

    /**
     * Finds the entire table of the given model.
     */
    template<typename ModelType, DATABSE_ENABLE_IF_MODEL>
    std::list<ModelType> findAll(bool *error = nullptr) const
    {
        const std::lock_guard lock{this->_mutex};
        if (!this->open(error)) return {};
        const auto results = this->internal_find_all(ModelType(), nullptr, std::any(ModelType()), error);
        this->close();

        std::list<ModelType> casted_results;
        for (auto&& res : results)
        {
            casted_results.emplace_back(*dynamic_cast<const ModelType*>(res.get()));
        }
        return casted_results;
    }

    /**
     * Finds the entire table of the given model using a filter pattern.
     * The filter pattern is NOT injection protected!! Don't use user data for the filter.
     */
    template<typename ModelType, DATABSE_ENABLE_IF_MODEL>
    std::list<ModelType> findAll(const std::string &filter, bool *error = nullptr) const
    {
        const std::lock_guard lock{this->_mutex};
        if (!this->open(error)) return {};
        const auto results = this->internal_find_all(ModelType(), &filter, std::any(ModelType()), error);
        this->close();

        std::list<ModelType> casted_results;
        for (auto&& res : results)
        {
            casted_results.emplace_back(*dynamic_cast<const ModelType*>(res.get()));
        }
        return casted_results;
    }

private:
    friend class Model;

    // disable copy
    Database(const Database &other) = delete;
    Database &operator= (const Database &other) = delete;

    DatabaseConfig _config;
    mutable std::string _lastErrorMessage;
    mutable std::recursive_mutex _mutex;
    void *dbptr = nullptr; // pointer to QSqlDatabase for internal use

    // internal helper functions
    bool open(bool *error = nullptr) const;
    void close() const;
    void set_error(bool *error = nullptr, bool = true) const;

    const std::shared_ptr<Model> internal_find(const Model &model, const id_t *id, const std::string *filter, const std::any &type, bool *error = nullptr) const;
    const std::list<std::shared_ptr<Model>> internal_find_all(const Model &model, const std::string *filter, const std::any &type, bool *error = nullptr) const;
    bool internal_create_table(const DatabaseTable &table, bool errorWhenExists = false);
};
