# awesomedb++

Model-based Database Abstraction Library using QtSql for C++

Work with databases in C++ without writing a single line of SQL
or dealing with data type nightmares. Just create a model matching
your database table and let the magic happen.

## Example

```cpp
#include <database/model.hpp>

// declare your model in the header file
MODEL(Project)
{
    // string "projects" is the table name in the database
    MODEL_DECL(Project, "projects");
    MODEL_ATTRIBUTE(name, std::string);
    MODEL_ATTRIBUTE(description, std::string);
};

MODEL_STRING_FMT(Project);

// implement it in the source file
Project::Project()
{
    this->make_model_attribute<std::string>("name");
    this->make_model_attribute<std::string>("description");
}

Project::Project(const Query *query, const Database *db)
    : Project()
{
    this->construct_default(query);
}

MODEL_DEFAULT_VALID_IMPL(Project);

// usage examples
Database::registerModel<Project>(); // make database aware of the model
Database db;
Project project;
// project.id() == 0
project.set_name("my project");
project.set_description("this is a project");

// saves the record in the database
bool res = db.saveRecord(&project);
// project.id() != 0 (id auto increment)
project.set_description("updated description");

// updates the existing record in the database
res = db.saveRecord(&project);

// obtain an instance of project from the database with the id=1
project = db.findRecord<Project>(1);
```

## Requirements

 - QtSql 5+
 - fmtlib (you need to provide the fmt target in you CMake project yourself)

## ABI Stability

**Not existent.** Highly recommended to bundle with your application and link it
statically. This library makes extreme usage of `typeid()` and other very
compiler-dependant language features. It is advised to compile this library and
your application with the same compiler and linker.

## CMake

```cmake
add_subdirectory(path/to/awesomedb)
target_link_libraries(yourapp PRIVATE awesomedb)
```
