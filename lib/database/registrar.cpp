#include "registrar.hpp"

namespace DatabaseRegistrar {

std::unordered_map<
    std::type_index, std::function<std::shared_ptr<Model>(const Model::Query*, const Database*)>>
    model_registrar;

}
