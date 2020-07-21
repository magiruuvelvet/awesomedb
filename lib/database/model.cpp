#include "model.hpp"

#include <database/database.hpp>
#include <utils/any_comparator.hpp>
#include <utils/any_formatter.hpp>
#include <utils/qvariant_mapper.hpp>
#include <utils/qvariant_converter.hpp>
#include <utils/list.hpp>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

Model::Model()
{
    this->make_model_attribute<id_t>("id", 0);
}

Model::Model(const Query *query, const Database *db)
    : Model()
{
}

Model::~Model()
{
}

QVariant Model::Query::value(const std::string &fieldName) const
{
    if (!this->query) return QVariant();
    return this->query->value(QString::fromStdString(fieldName));
}

bool Model::has_model_attributes() const
{
    // remove id column
    auto columns = this->_columns;
    columns.erase(std::find(columns.begin(), columns.end(), "id"));

    // check if empty
    return !columns.empty();
}

const std::string Model::generate_insert_query() const
{
    // remove id column
    auto columns = this->_columns;
    columns.erase(std::find(columns.begin(), columns.end(), "id"));

    const auto format = utils::list_join(columns, ",");
    const auto values = ":" + utils::list_join(columns, ",:");

    return fmt::format("INSERT INTO `{}` ({}) VALUES ({});",
        this->table_name(), format, values);
}

const std::string Model::generate_update_query() const
{
    // remove id column
    auto columns = this->_columns;
    columns.erase(std::find(columns.begin(), columns.end(), "id"));

    std::list<std::string> query_pairs;
    for (auto&& attr : this->_attributes)
    {
        if (attr.first == "id") continue;

        // only update changed values
        if (std::get<1>(attr.second))
        {
            query_pairs.emplace_back(fmt::format("{}=:{}", attr.first, attr.first));
        }
    }

    // nothing to update
    if (query_pairs.empty())
    {
        return {};
    }

    return fmt::format("UPDATE `{}` SET {} WHERE id={};",
        this->table_name(), utils::list_join(query_pairs, ","), this->id());
}

void Model::construct_default(const Query *query)
{
    // iterate and fetch data on a best guess basis
    for (auto&& attr : this->_columns)
    {
        utils::any_from_qvariant(
            this->get_attribute(attr),
            query->query->value(QString::fromStdString(attr)));
    }

    // mark model as unchanged
    this->reset_changed_state();
}

bool Model::has_changes() const
{
    for (auto&& attr : this->_attributes)
    {
        if (std::get<1>(attr.second))
        {
            return true;
        }
    }

    return false;
}

bool Model::is_new_record() const
{
    return this->id() == 0;
}

void Model::reset_changed_state()
{
    for(auto&& attr : this->_attributes)
    {
        std::get<1>(attr.second) = false;
    }
}

bool Model::compare_helper(const Model &other) const
{
    // attribute count must match
    if (this->_attributes.size() != other._attributes.size())
    {
        return false;
    }

    for (auto&& attr_self : this->_attributes)
    {
        for (auto&& attr_other : other._attributes)
        {
            if (attr_self.first == attr_other.first)
            {
                bool success;
                const auto equal = utils::compare_any(
                    std::get<0>(attr_self.second),
                    std::get<0>(attr_other.second),
                    &success);

                if (success == false)
                {
                    // unregistered type, assume model is no longer equal
                    return false;
                }

                if (!equal)
                {
                    // no longer equal, stop and return false
                    return false;
                }
            }
        }
    }

    return true;
}

bool Model::save(Database *db, id_t *last_insert_id)
{
    // database connection is open here

    // first do client-side model validation
    std::string error_message;
    if (!this->is_valid(&error_message))
    {
        db->_lastErrorMessage = error_message;
        return false;
    }

    // check if model has attributes
    if (!this->has_model_attributes())
    {
        db->_lastErrorMessage = "model is empty, please add some attributes first";
        return false;
    }

    QString statement;
    bool did_insert = false;

    // record not present in database, insert it
    if (this->is_new_record())
    {
        // insert query can't be empty
        statement = QString::fromStdString(this->generate_insert_query());
        did_insert = true;
    }

    // record present in database, update it
    else
    {
        // update query can be empty
        statement = QString::fromStdString(this->generate_update_query());
        if (statement.isEmpty())
        {
            // nothing to do, simulate success
            return true;
        }
    }

    // bind values and execute generated query
    QSqlQuery q(*static_cast<QSqlDatabase*>(db->dbptr));
    if (!q.prepare(statement))
    {
        db->_lastErrorMessage = q.lastError().text().toStdString();
        return false;
    }

    for (auto&& attr : this->_attributes)
    {
        q.bindValue(
            QString::fromStdString(":" + attr.first),
            utils::qvariant_from_any(std::get<0>(attr.second)));
    }

    fmt::print("running prepared query: {}\n", statement.toStdString());
    fmt::print("bound values: ");
    qDebug() << q.boundValues();

    if (!q.exec())
    {
        db->_lastErrorMessage = q.lastError().text().toStdString();
        return false;
    }

    // obtain last insert id when new record was saved
    // and store it in the current model instance
    if (did_insert)
    {
        const id_t newId = q.lastInsertId().toULongLong();
        if (newId != 0)
        {
            this->set_id(newId);

            if (last_insert_id)
            {
                (*last_insert_id) = newId;
            }
        }
    }

    return true;
}

bool Model::remove(Database *db)
{
    if (this->is_new_record())
    {
        return true;
    }

    const auto statement = fmt::format("DELETE FROM `{}` WHERE id={};", this->table_name(), this->id());

    fmt::print("running query: {}\n", statement);

    QSqlQuery q(*static_cast<QSqlDatabase*>(db->dbptr));
    if (!q.exec(QString::fromStdString(statement)))
    {
        db->_lastErrorMessage = q.lastError().text().toStdString();
        return false;
    }

    this->set_id(0);
    return true;
}

const std::string Model::to_string() const
{
    // header
    std::string str = fmt::format("{}({}) {{\n    ",
        this->type_name(), this->is_new_record() ? "new" : std::to_string(this->id()));

    // attributes
    std::list<std::string> formatted_attrs;
    for (auto&& attr : this->_columns)
    {
        if (attr == "id") continue;

        bool success;
        const auto fmt = utils::format_any(std::get<0>(this->_attributes.at(attr)), &success);
        if (success)
        {
            formatted_attrs.emplace_back(fmt::format("{} = {}", attr, fmt));
        }
        else
        {
            formatted_attrs.emplace_back(fmt::format("{}={{unsupported}}", attr));
        }
    }
    str += utils::list_join(formatted_attrs, ",\n    ");
    str += ",\n}";
    return str;
}
