#ifndef VOXEL_UNIVERSE_H
#define VOXEL_UNIVERSE_H

#include "core/math/vector3.h"
#include "core/math/vector3i.h"
#include <cassert>

#include "scene/3d/node_3d.h"

namespace VG {
class Universe : public Node3D {
    GDCLASS(Universe, Node3D)
    
protected:
    static void _bind_methods();

public:
    
    //TODO: Planet-Builder-Pattern
    void add_planet(const Vector3& pos, ushort size);

};
}; //namespace VVG

using VG::Universe;

#endif // VOXEL_UNIVERSE_H