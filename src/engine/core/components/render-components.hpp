#pragma once

#include <memory>

#include "rendering/model.hpp"

namespace niji
{

struct MeshComponent
{
    std::shared_ptr<Model> Model = nullptr;
    uint16_t MeshID = -1;
    //uint16_t MaterialID = -1;
};

} // namespace niji