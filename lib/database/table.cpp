#include "table.hpp"

#include <sstream>

DatabaseTable::DatabaseTable(const std::string &name, const std::list<Field> &fields)
    : _name(name),
      _fields(fields)
{
}

void DatabaseTable::addField(const Field &field)
{
    this->_fields.emplace_back(field);
}

bool DatabaseTable::empty() const
{
    return this->_fields.empty();
}

const std::string DatabaseTable::generateSqlStatement(bool includeIfNotExists) const
{
    std::string statement;
    {
        // generated query buffers
        std::ostringstream query;
        std::ostringstream append;
        bool has_append = false;

        const std::string ifNotExists = "IF NOT EXISTS ";

        query << "CREATE TABLE " << (includeIfNotExists ? ifNotExists : "") << "`" << this->_name << "` (";
        for (auto&& field : this->_fields)
        {
            // generate field
            query << "`" << field.name << "` " << field.type;

            // field is not nullable
            if (!field.nullable)
            {
                query << " NOT NULL";
            }

            // field is auto incrementable
            if (field.auto_increment)
            {
                query << " AUTO_INCREMENT";
            }

            // field has default value
            if (!field.default_value.empty())
            {
                query << " DEFAULT " << field.default_value;
            }

            // end generate field
            query << ",";

            // append primary key
            if (field.pk)
            {
                append << "PRIMARY KEY (`" << field.name << "`),";
                has_append = true;
            }

            // append foreign key
            if (field.fk)
            {
                append << "FOREIGN KEY (`" << field.name << "`) REFERENCES ";
                append << field.references_table << "(`" << field.references_field << "`),";
                has_append = true;
            }

            // append unique key
            if (field.uk)
            {
                append << "UNIQUE KEY (`" << field.name << "`),";
                has_append = true;
            }
        }

        // remove last trailing comma because MariaDB can't handle this
        if (has_append)
        {
            append.seekp(-1, std::ios_base::cur);
        }
        else
        {
            query.seekp(-1, std::ios_base::cur);
            query << " ";
        }

        // finalize query
        append << ");";

        // build final query
        statement = query.str() + append.str();
    }

    return statement;
}
