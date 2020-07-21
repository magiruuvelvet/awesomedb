#pragma once

#include <string>
#include <list>

struct DatabaseTable final
{
    /**
     * Represent a single table column.
     */
    struct Field final
    {
        const std::string name;
        const std::string type;
        const bool pk = false;        // is primary key?
        const bool fk = false;        // is foreign key?
        const bool uk = false;        // is unique key?
        const bool nullable = false;  // is nullable?
        const bool auto_increment = false; // is auto increment?

        // foreign key config
        const std::string references_table;
        const std::string references_field;

        const std::string default_value;

        const bool operator== (const Field &other) const
        {
            return name == other.name &&
                   type == other.type &&
                   pk == other.pk &&
                   fk == other.fk &&
                   uk == other.uk &&
                   nullable == other.nullable &&
                   auto_increment == other.auto_increment &&
                   references_table == other.references_table &&
                   references_field == other.references_field &&
                   default_value == other.default_value;
        }
        const bool operator!= (const Field &other) const
        {
            return !this->operator==(other);
        }
    };

    /**
     * Constructs a new database table.
     */
    DatabaseTable(const std::string &name, const std::list<Field> &fields = {});

    /**
     * Constructs a default id field for use with the model abstraction.
     */
    static const Field idField()
    {
        return Field{.name="id", .type="bigint", .pk=true, .auto_increment=true};
    }

    /**
     * Returns the table name.
     */
    constexpr inline const auto &name() const
    { return this->_name; }

    /**
     * Add a new column to the table.
     */
    void addField(const Field &field);

    /**
     * Checks if the table has any properties.
     */
    bool empty() const;

    /**
     * Generate a "CREATE TABLE" SQL statement.
     */
    const std::string generateSqlStatement(bool includeIfNotExists = false) const;

    const bool operator== (const DatabaseTable &other) const
    {
        if (this->_fields.size() != other._fields.size())
        {
            return false;
        }

        const bool equal = std::equal(this->_fields.begin(), this->_fields.end(), other._fields.begin());
        return equal && this->_name == other._name;
    }
    const bool operator!= (const DatabaseTable &other) const
    {
        return !this->operator==(other);
    }

private:
    const std::string _name;
    std::list<Field> _fields;
};
