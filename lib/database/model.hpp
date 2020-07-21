#pragma once

#include <string>
#include <string_view>
#include <map>
#include <list>
#include <tuple>
#include <any>
#include <cstdint>
#include <memory>

#include <QVariant>

#include <fmt/format.h>

#define MODEL_STRING_FMT(type)                                  \
template<> struct fmt::formatter<type> {                        \
    constexpr auto parse(format_parse_context &ctx)             \
    { return ctx.begin(); }                                     \
    template<typename FormatContext>                            \
    auto format(const type &var, FormatContext &ctx) {          \
        return format_to(ctx.out(), "{}", var.to_string()); } }

// declares a new public model attribute
#define MODEL_ATTRIBUTE(name, type)                       \
    public: inline const type &name() const               \
    { return this->get_attribute_value<type>(#name); }    \
    public: inline void set_##name(const type &value)     \
    { this->set_attribute_value<type>(#name, value); }    \
    private: // set visibility back to private

// declares a new model attribute with protected setter and public getter
#define MODEL_ATTRIBUTE_PROTECTED(name, type)             \
    public: inline const type &name() const               \
    { return this->get_attribute_value<type>(#name); }    \
    protected: inline void set_##name(const type &value)  \
    { this->set_attribute_value<type>(#name, value); }    \
    private: // set visibility back to private

// base model declaration
#define MODEL_DECL(name, __table_name)                         \
    public: name();                                            \
    private: name(const Query *query, const Database *db);     \
    public: name(const name &other) = default;                 \
    public: name &operator= (const name &other) = default;     \
    public: inline bool operator== (const name &other) const   \
    { return this->compare_helper(other); }                    \
    public: inline bool operator!= (const name &other) const   \
    { return !this->compare_helper(other); }                   \
    private: friend class Database;                            \
    private: friend class Model;                               \
    public: bool is_valid(std::string *error_message = nullptr) const override; \
    public: inline const std::string table_name() const override {              \
        return __table_name; }                                 \
    public: static inline const std::string_view tableName() { \
        return __table_name; }                                 \
    public: inline const std::string type_name() const override { \
        return #name; }                                        \
    public: static inline const std::string_view typeName() {  \
        return #name; }                                        \
    private: // set visibility back to private

#define MODEL(name) \
    class name final : public Model

#define MODEL_DEFAULT_VALID_IMPL(name)        \
    bool name::is_valid(std::string *) const  \
    { return true; }

#define MODEL_CUSTOM_SAVE() \
    bool save(Database *db, id_t *last_insert_id = nullptr) override

#define MODEL_INTERNAL_FIND_CAST(type, var) \
    (*dynamic_cast<const type*>(var.get()))

// forward declarations
class Database;
class QSqlQuery;

/**
 * Abstract base model representing a record from a database table.
 */
class Model
{
public:
    virtual ~Model();

    /**
     * Query wrapper struct because <QSqlQuery> is not available outside
     * of the library.
     */
    struct Query
    {
        // construct query from QSqlQuery pointer
        Query(const QSqlQuery *query)
            : query(query)
        {}

        // return pointer to itself
        const Query *self() const { return this; }

        // QSqlQuery::value();
        QVariant value(const std::string &fieldName) const;

    private:
        friend class Model;
        const QSqlQuery *query = nullptr;
    };

    // type aliases
    using id_t = std::uint64_t;
    using key_t = std::string;
    using value_t = std::any;
    using modified_t = bool;
    using attribute_t = std::tuple<value_t, modified_t>;

    // comparison operators
    inline bool operator== (const Model &other) const
    { return this->compare_helper(other); }
    inline bool operator!= (const Model &other) const
    { return !this->compare_helper(other); }

    /**
     * Receive a list of all columns of the database model.
     *
     */
    constexpr inline const auto &columns() const
    { return this->_columns; }

    /**
     * Returns the table name of the model.
     */
    virtual const std::string table_name() const = 0;

    /**
     * Returns the class name of the model.
     */
    virtual const std::string type_name() const = 0;

    /**
     * String format function for {fmtlib}.
     */
    const std::string to_string() const;

public:

    // primary key
    MODEL_ATTRIBUTE_PROTECTED(id, id_t);

public:

    /**
     * Checks if the model has changed since it was loaded from the database.
     */
    bool has_changes() const;

    /**
     * Checks if the model is a new unsaved record not present in the database.
     */
    bool is_new_record() const;

    /**
     * Function which determines if the model qualifies as being valid.
     * User-defined error messages are supported.
     *
     * The save() method fails when this function returns false.
     * If you don't need model validation use the MODEL_DEFAULT_VALID_IMPL()
     * macro to auto generate a default stub validator which just returns true.
     */
    virtual bool is_valid(std::string *error_message = nullptr) const = 0;

protected:

    /**
     * Creates a new model attribute.
     */
    template<typename ValueType>
    inline void make_model_attribute(const std::string &name, const ValueType &value = {})
    {
        this->_attributes.insert({name, std::make_tuple(ValueType{value}, false)});
        this->_columns.emplace_back(name);
    }

    /**
     * Removes a attribute from the model.
     */
    inline void remove_model_attribute(const std::string &name)
    {
        this->_attributes.erase(name);
        this->_columns.erase(std::find(this->_columns.begin(), this->_columns.end(), name));
    }

    /**
     * Sets the given attribute value.
     */
    template<typename ValueType>
    inline void set_attribute_value(const std::string &key, const ValueType &value)
    {
        (*std::any_cast<ValueType>(&std::get<0>(this->_attributes[key]))) = value;
        std::get<1>(this->_attributes[key]) = true;
    }

    /**
     * Returns a read-only reference to the given attribute.
     */
    template<typename ValueType>
    inline const ValueType &get_attribute_value(const std::string &key) const
    {
        return (*std::any_cast<ValueType>(&std::get<0>(this->_attributes.at(key))));
    }

    /**
     * Returns a read-write reference to the given attribute.
     * For model building from query data only.
     */
    inline std::any &get_attribute(const std::string &key)
    {
        return std::get<0>(this->_attributes[key]);
    }

    /**
     * Marks the model as unchanged again once the data has been loaded from the database.
     */
    void reset_changed_state();

protected:
    Model();

    /**
     * If no special model construction is needed, call this
     * method in your models query constructor to automatically
     * load all data by iterating over all attributes and
     * attempt to fetch a value from it.
     */
    void construct_default(const Query *query);

    /**
     * Write model changes back to the database.
     * This function can be overwritten to be extended with
     * custom save logic.
     */
    virtual bool save(Database *db, id_t *last_insert_id = nullptr);

    /**
     * Delete model from database.
     */
    virtual bool remove(Database *db);

    // make model copy protected
    // actual subclassed models have public copy
    Model(const Model &other) = default;
    Model &operator= (const Model &other) = default;

    /**
     * Helper function to compare model attributes.
     */
    bool compare_helper(const Model &other) const;

    // MariaDB data type validation helpers
    static bool validate_varchar_length(std::uint64_t length, const std::string &text);
    static bool validate_mediumtext_length(const std::string &text);

private:
    friend class Database;

    // model attribute map
    std::map<key_t, attribute_t> _attributes;
    std::list<key_t> _columns;

    /**
     * Constructs a model directly from a database query result.
     */
    Model(const Query *query, const Database *db);

    // prepared query generators for save()
    const std::string generate_insert_query() const;
    const std::string generate_update_query() const;

    // check if the model has any attributes other than the PK
    bool has_model_attributes() const;
};

MODEL_STRING_FMT(Model);
