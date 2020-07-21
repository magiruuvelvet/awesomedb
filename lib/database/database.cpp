#include "database.hpp"

#include <map>
#include <array>
#include <tuple>
#include <sstream>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

#include <fmt/format.h>

// workaround to avoid including QSqlDatabase in header file
static std::map<std::uintptr_t, QSqlDatabase> dbpool;

#define selfptr reinterpret_cast<std::uintptr_t>(this)
#define self dbpool[selfptr]

// query wrapper
template<typename... Args>
static std::tuple<std::shared_ptr<QSqlQuery>, std::string> query(const Database *db, bool &error, const std::string &query, Args&&... args)
{
    QString str;
    if constexpr (sizeof...(args) == 0)
    {
        str = QString::fromStdString(query);
    }
    else
    {
        str = QString::fromStdString(query).arg(std::forward<Args>(args)...);
    }

    fmt::print("running query: {}\n", str.toStdString());

    QSqlQuery q(dbpool[reinterpret_cast<std::uintptr_t>(db)]);
    if (!q.exec(str))
    {
        error = true;
        return {nullptr, q.lastError().text().toStdString()};
    }
    else
    {
        error = false;
        return {std::make_shared<QSqlQuery>(q), {}};
    }
}

#define RETURN(value) \
    self.close();     \
    return value

Database::Database(const DatabaseConfig &config)
    : _config(config)
{
    // setup database connection
    dbpool.insert({
        selfptr,
        QSqlDatabase::addDatabase("QMYSQL", QString::number(selfptr))
    });
    this->dbptr = &self;
    self.setHostName(QString::fromStdString(this->_config.host));
    self.setPort(this->_config.port);
    self.setUserName(QString::fromStdString(this->_config.username));
    self.setPassword(QString::fromStdString(this->_config.password));
    self.setDatabaseName(QString::fromStdString(this->_config.database));
}

Database::~Database()
{
    // ensure database connection is closed and remove it from the pool
    self.close();
    dbpool.erase(selfptr);
    QSqlDatabase::removeDatabase(QString::number(selfptr));
    this->dbptr = nullptr;
}

bool Database::execute(const std::string &_query)
{
    const std::lock_guard lock{this->_mutex};
    if (!this->open()) return false;

    bool qerror;
    const auto res = query(this, qerror, _query);
    if (qerror)
    {
        this->_lastErrorMessage = std::get<1>(res);
        RETURN(false);
    }
    else
    {
        this->_lastErrorMessage.clear();
    }

    RETURN(true);
}

const std::list<std::string> Database::tables() const
{
    const std::lock_guard lock{this->_mutex};
    if (!this->open()) return {};

    const auto qt = self.tables();
    std::list<std::string> t;
    for (auto&& table : qt)
    {
        t.emplace_back(table.toStdString());
    }
    RETURN(t);
}

bool Database::createTable(const DatabaseTable &table, bool errorWhenExists)
{
    const std::lock_guard lock{this->_mutex};
    if (!this->open()) return false;
    const auto status = this->internal_create_table(table, errorWhenExists);
    RETURN(status);
}

bool Database::dropTable(const std::string &tableName)
{
    return this->execute(fmt::format("DROP TABLE `{}`;", tableName));
}

bool Database::truncateTable(const std::string &tableName)
{
    return this->execute(fmt::format("TRUNCATE TABLE `{}`;", tableName));
}

bool Database::canConnect() const
{
    const std::lock_guard lock{this->_mutex};
    const auto status = this->open();
    RETURN(status);
}

bool Database::saveRecord(Model *model)
{
    const std::lock_guard lock{this->_mutex};
    if (!this->open()) return false;
    const auto status = model->save(this);
    RETURN(status);
}

bool Database::deleteRecord(Model *model)
{
    const std::lock_guard lock{this->_mutex};
    if (!this->open()) return false;
    const auto status = model->remove(this);
    RETURN(status);
}

bool Database::open(bool *error) const
{
    this->_lastErrorMessage.clear();

    if (self.open())
    {
        // open success
        this->set_error(error, false);
        return true;
    }
    else
    {
        // open failed
        this->set_error(error, true);
        this->_lastErrorMessage = self.lastError().text().toStdString();
        return false;
    }
}

void Database::close() const
{
    self.close();
}

void Database::set_error(bool *error, bool b) const
{
    if (error)
    {
        (*error) = b;
    }
}

const std::shared_ptr<Model> Database::internal_find(const Model &model, const id_t *id, const std::string *filter, const std::any &type, bool *error) const
{
    // note: db must be open already, function does not close db after work is done

    const std::lock_guard lock{this->_mutex};

    std::string statement;
    if (id)
    {
        statement = fmt::format("SELECT * FROM `{}` WHERE id={};", model.table_name(), *id);
    }
    else
    {
        statement = fmt::format("SELECT * FROM `{}` WHERE {} LIMIT 1;", model.table_name(), *filter);
    }

    bool e;
    const auto res = query(this, e, statement);
    if (e)
    {
        this->set_error(error, true);
        this->_lastErrorMessage = std::get<1>(res);
        return nullptr;
    }

    // try to seek to first result
    if (!std::get<0>(res)->next())
    {
        this->set_error(error, true);
        this->_lastErrorMessage = fmt::format("empty result set for {}", model.table_name());
        return nullptr;
    }

    this->set_error(error, false);

    // look if current model is registered in the registrar and call the constructor
    if (const auto it = DatabaseRegistrar::model_registrar.find(std::type_index(type.type()));
        it != DatabaseRegistrar::model_registrar.cend())
    {
        const auto q = Model::Query(std::get<0>(res).get());
        return it->second(&q, this);
    }

    // model not registered
    this->set_error(error, true);
    this->_lastErrorMessage = fmt::format("unsupported model type: {}", model.type_name());
    return nullptr;
}

const std::list<std::shared_ptr<Model>> Database::internal_find_all(const Model &model, const std::string *filter, const std::any &type, bool *error) const
{
    // note: db must be open already, function does not close db after work is done

    const std::lock_guard lock{this->_mutex};

    std::string statement;
    if (filter)
    {
        statement = fmt::format("SELECT * FROM `{}` WHERE {};", model.table_name(), *filter);
    }
    else
    {
        statement = fmt::format("SELECT * FROM `{}`;", model.table_name());
    }

    bool e;
    const auto res = query(this, e, statement);
    if (e)
    {
        this->set_error(error, true);
        this->_lastErrorMessage = std::get<1>(res);
        return {};
    }

    this->set_error(error, false);

    std::list<std::shared_ptr<Model>> results;
    while (std::get<0>(res)->next())
    {
        // look if current model is registered in the registrar and call the constructor
        if (const auto it = DatabaseRegistrar::model_registrar.find(std::type_index(type.type()));
            it != DatabaseRegistrar::model_registrar.cend())
        {
            const auto q = Model::Query(std::get<0>(res).get());
            results.emplace_back(it->second(&q, this));
        }
        // if model isn't registered, cancel iteration and return empty list
        else
        {
            this->set_error(error, true);
            this->_lastErrorMessage = fmt::format("unsupported model type: {}", model.type_name());
            return {};
        }
    }

    return results;
}

bool Database::internal_create_table(const DatabaseTable &table, bool errorWhenExists)
{
    if (table.empty())
    {
        this->_lastErrorMessage = fmt::format("{}: no fields specified", table.name());
        return false;
    }

    bool qerror;
    const auto res = query(this, qerror, table.generateSqlStatement(!errorWhenExists));
    if (qerror)
    {
        this->_lastErrorMessage = std::get<1>(res);
        return false;
    }
    else
    {
        this->_lastErrorMessage.clear();
    }

    return true;
}
