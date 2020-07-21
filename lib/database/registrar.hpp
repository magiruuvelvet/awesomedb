#pragma once

#include "model.hpp"

#include <memory>
#include <typeindex>
#include <unordered_map>

namespace DatabaseRegistrar {

template<class T, class F>
static inline std::pair<const std::type_index, std::function<std::shared_ptr<Model>(const Model::Query*, const Database*)>>
    to_model_constructor(F const &f)
{
    return {
        std::type_index(typeid(T)),
        [g = f](const Model::Query *query, const Database *db)
        {
            return g(query, db);
        }
    };
}

extern std::unordered_map<
    std::type_index, std::function<std::shared_ptr<Model>(const Model::Query*, const Database*)>>
    model_registrar;

template<class T, class F>
inline void register_model(F const& f)
{
    model_registrar.insert(to_model_constructor<T>(f));
}

}
