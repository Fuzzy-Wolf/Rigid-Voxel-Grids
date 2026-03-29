#include <cstdint>
#include <vector>

#include "core/error/error_macros.h"
#include "core/math/vector3.h"
#include "core/math/vector3i.h"
#include "core/variant/array.h"
#include "scene/resources/mesh.h"

#include "../utils.h"
#include "../global.h"
#include "chunk.h"
#include "chunk_builder.h"


ChunkBuilder*const ChunkBuilder::singleton = new ChunkBuilder();


Array VG::ChunkBuilder::generate_mesh_data(const VoxelData *voxels) {
    ERR_FAIL_NULL_V_MSG(voxels, Array(), "Warnung: Der übergebene Chunk hatte keine Voxel.");
    _vertice_count = 0;
    _vertices.clear();
    _indices.clear();
    _uvs.clear();
    _normals.clear();
    /* ------------------------------- XY -------------------------------------- */
    // if (NEIGHBOR.north) {
    //     ushort z = 0;
    //     for (ushort x = 0; x < CHUNK_SIZE; ++x) {
    //         for (ushort y = 0; y < CHUNK_SIZE; ++y) {
    //             bool south_invisible = is_invisible(data[x][y][z]);
    //             bool north_invisible = is_invisible(NEIGHBOR.north->data[x][y][CHUNK_SIZE-1]);

    //             if (!south_invisible && north_invisible) {
    //                 _create_face_xy(data[x][y][z], x, y, z, false);
    //             }
    //         }
    //     }
    // } else
    {
        ushort z = 0;
        for (ushort x = 0; x < CHUNK_SIZE; ++x) {
            for (ushort y = 0; y < CHUNK_SIZE; ++y) {
                const Voxel& voxel = voxels->get(x,y,z);
                if (!voxel.is_invisible()) {
                    _create_face_xy(voxel, x, y, z, false);
                }
            }
        }
    }
    for (ushort z = 1; z < CHUNK_SIZE; ++z) { // 1 bis CHUNK_SIZE
        for (ushort x = 0; x < CHUNK_SIZE; ++x) {
            for (ushort y = 0; y < CHUNK_SIZE; ++y) {
                const Voxel &voxel_front = voxels->get(x, y, z);
                const Voxel &voxel_back = voxels->get(x, y, z - 1);
                
                bool south_invisible = voxel_front.is_invisible();
                bool north_invisible = voxel_back.is_invisible();

                if (!south_invisible && north_invisible) {
                    _create_face_xy(voxel_front, x, y, z, false);
                } else if (!north_invisible && south_invisible) {
                    _create_face_xy(voxel_back, x, y, z, true);
                }
            }
        }
    }
    // if (NEIGHBOR.south) {
    //     ushort z = CHUNK_SIZE-1;
    //     for (ushort x = 0; x < CHUNK_SIZE; ++x) {
    //         for (ushort y = 0; y < CHUNK_SIZE; ++y) {
    //             bool south_invisible = !is_invisible(NEIGHBOR.south->data[x][y][0]);
    //             bool north_invisible = is_invisible(data[x][y][z]);

    //             if ((!north_invisible && south_invisible)) {
    //                 _create_face_xy(data[x][y][z], x, y, z+1, true);
    //             }
    //         }
    //     }
    // } else
    {
        ushort z = CHUNK_SIZE - 1;
        for (ushort x = 0; x < CHUNK_SIZE; ++x) {
            for (ushort y = 0; y < CHUNK_SIZE; ++y) {
                const Voxel& voxel = voxels->get(x,y,z);
                if (!voxel.is_invisible()) {
                    _create_face_xy(voxel, x, y, z+1, true);
                }
            }
        }
    }
    /* ------------------------------- YZ -------------------------------------- */
    // if (NEIGHBOR.west) {
    //     ushort x = 0;
    //     for (ushort y = 0; y < CHUNK_SIZE; ++y) {
    //         for (ushort z = 0; z < CHUNK_SIZE; ++z) {
                
    //             bool south_invisible = is_invisible(data[x][y][z]);
    //             bool north_invisible = is_invisible(NEIGHBOR.west->data[CHUNK_SIZE-1][y][z]);

    //             if ((!south_invisible && north_invisible)) {
    //                 _create_face_yz(data[x][y][z], y, z, x, false);
    //             }
    //         }
    //     }
    // } else
    {
        ushort x = 0;
        for (ushort y = 0; y < CHUNK_SIZE; ++y) {
            for (ushort z = 0; z < CHUNK_SIZE; ++z) {
                const Voxel& voxel = voxels->get(x,y,z);
                
                if (!voxel.is_invisible()) {
                    _create_face_yz(voxel, y, z, x, false);
                }
            }
        }
    }
    for (ushort x = 1; x < CHUNK_SIZE; ++x) { // 1 bis CHUNK_SIZE
        for (ushort y = 0; y < CHUNK_SIZE; ++y) {
            for (ushort z = 0; z < CHUNK_SIZE; ++z) {
                const Voxel &voxel_front = voxels->get(x, y, z);
                const Voxel &voxel_back = voxels->get(x - 1, y, z);

                bool south_invisible = voxel_front.is_invisible();
                bool north_invisible = voxel_back.is_invisible();
                
                if (!south_invisible && north_invisible) {
                    _create_face_yz(voxel_front, y, z, x, false);
                } else if (!north_invisible && south_invisible) {
                    _create_face_yz(voxel_back, y, z, x, true);
                }
            }
        }
    }
    // if (NEIGHBOR.east) {
    //     ushort x = CHUNK_SIZE - 1;
    //     for (ushort y = 0; y < CHUNK_SIZE; ++y) {
    //         for (ushort z = 0; z < CHUNK_SIZE; ++z) {
                
    //             bool north_invisible = !is_invisible(id);
    //             bool south_invisible = is_invisible(NEIGHBOR.east->data[0][y][z]);

    //             if ((!north_invisible && south_invisible)) {
    //                 _create_face_yz(id, y, z, x+1, true);
    //             }
    //         }
    //     }
    // } else
    {
        ushort x = CHUNK_SIZE - 1;
        for (ushort y = 0; y < CHUNK_SIZE; ++y) {
            for (ushort z = 0; z < CHUNK_SIZE; ++z) {
                const Voxel& voxel = voxels->get(x,y,z);
                
                if (!voxel.is_invisible()) {
                    _create_face_yz(voxel, y, z, x+1, true);
                }
            }
        }
    }
    /* ------------------------------- XZ -------------------------------------- */
    // if (NEIGHBOR.bottom) {
    //     ushort y = 0;
    //     for (ushort x = 0; x < CHUNK_SIZE; ++x) {
    //         for (ushort z = 0; z < CHUNK_SIZE; ++z) {
    //             bool south_invisible = is_invisible(id);
    //             bool north_invisible = is_invisible(NEIGHBOR.bottom->data[x][CHUNK_SIZE-1][z]);

    //             if ((!south_invisible && north_invisible)) {
    //                 _create_face_xz(id, x, z, y, true);
    //             }
    //         }
    //     }
    // } else
    {
        ushort y = 0;
        for (ushort x = 0; x < CHUNK_SIZE; ++x) {
            for (ushort z = 0; z < CHUNK_SIZE; ++z) {
                const Voxel& voxel = voxels->get(x,y,z);
    
                if (!voxel.is_invisible()) {
                    _create_face_xz(voxel, x, z, y, true);
                }
            }
        }
    }
    for (ushort y = 1; y < CHUNK_SIZE; ++y) { // 1 bis CHUNK_SIZE
        for (ushort x = 0; x < CHUNK_SIZE; ++x) {
            for (ushort z = 0; z < CHUNK_SIZE; ++z) {
                const Voxel &voxel_front = voxels->get(x, y, z);
                const Voxel& voxel_back = voxels->get(x, y - 1, z);
                
                bool south_invisible = voxel_front.is_invisible();
                bool north_invisible = voxel_back.is_invisible();

                if (!south_invisible && north_invisible) {
                    _create_face_xz(voxel_front, x, z, y, true);
                } else if (!north_invisible && south_invisible) {
                    _create_face_xz(voxel_back, x, z, y, false);
                }
            }
        }
    }
    // if (NEIGHBOR.top) {
    //     ushort y = CHUNK_SIZE - 1;
    //     for (ushort x = 0; x < CHUNK_SIZE; ++x) {
    //         for (ushort z = 0; z < CHUNK_SIZE; ++z) {
                
    //             bool north_invisible = !is_invisible(id);
    //             bool south_invisible = is_invisible(NEIGHBOR.top->data[x][0][z]);

    //             if ((!north_invisible && south_invisible)) {
    //                 _create_face_xz(id, x, z, y+1, false);
    //             }
    //         }
    //     }
    // } else
    {
        ushort y = CHUNK_SIZE - 1;
        for (ushort x = 0; x < CHUNK_SIZE; ++x) {
            for (ushort z = 0; z < CHUNK_SIZE; ++z) {
                const Voxel& voxel = voxels->get(x,y,z);
                
                if (!voxel.is_invisible()) {
                    _create_face_xz(voxel, x, z, y+1, false);
                }
            }
        }
    }

    Array mesh_data;
    if (_vertices.size() == 0) {
        return mesh_data; // Chunks können komplett leer sein
    }
    mesh_data.resize(Mesh::ARRAY_MAX);
    // TODO: wahrscheinlich verflucht ineffizient, weil Kopie
     mesh_data[Mesh::ARRAY_VERTEX] = _vertices;
     mesh_data[Mesh::ARRAY_INDEX] = _indices;
     mesh_data[Mesh::ARRAY_TEX_UV] = _uvs;
     mesh_data[Mesh::ARRAY_NORMAL] = _normals;
    return mesh_data;
}

void ChunkBuilder::_add_polygons(bool invert_normals) noexcept {
    if (invert_normals) {
        // Polygon 1
        _indices.append(1 + _vertice_count);
        _indices.append(0 + _vertice_count);
        _indices.append(2 + _vertice_count);
        // Polygon 2
        _indices.append(2 + _vertice_count);
        _indices.append(0 + _vertice_count);
        _indices.append(3 + _vertice_count);
    } else {
        // Polygon 1
        _indices.append(0 + _vertice_count);
        _indices.append(1 + _vertice_count);
        _indices.append(2 + _vertice_count);
        // Polygon 2
        _indices.append(0 + _vertice_count);
        _indices.append(2 + _vertice_count);
        _indices.append(3 + _vertice_count);
    }
    _vertice_count += 4;
}
void VG::ChunkBuilder::_create_face_xy(const Voxel block_id, const ushort x, const ushort y, const ushort z_offset, bool invert_normals) {
    
    _vertices.append(Vector3(x  , y+1 , z_offset)); // 0     0       1
    _vertices.append(Vector3(x  , y   , z_offset));// 1
    _vertices.append(Vector3(x+1, y   , z_offset));// 2
    _vertices.append(Vector3(x+1, y+1 , z_offset)); // 3     3       2

    Vector3 normal_vec(0,0,2*invert_normals - 1);
    _normals.append(normal_vec);
    _normals.append(normal_vec);
    _normals.append(normal_vec);
    _normals.append(normal_vec);

    const auto [ uv_x, uv_y ] = get_texture_coords(block_id);

    _uvs.append(Vector2(SCALE* uv_x   , SCALE* uv_y   ));
    _uvs.append(Vector2(SCALE*(uv_x+1), SCALE* uv_y   ));
    _uvs.append(Vector2(SCALE*(uv_x+1), SCALE*(uv_y+1)));
    _uvs.append(Vector2(SCALE* uv_x   , SCALE*(uv_y+1)));

    _add_polygons(invert_normals);
}
void VG::ChunkBuilder::_create_face_yz(const Voxel block_id, const ushort x, const ushort y, const ushort x_offset, bool invert_normals) {
    _vertices.append(Vector3(x_offset, x  , y+1 ));// 0     0       1
    _vertices.append(Vector3(x_offset, x  , y   ));// 1
    _vertices.append(Vector3(x_offset, x+1, y   ));// 2
    _vertices.append(Vector3(x_offset, x+1, y+1 ));// 3     3       2

    Vector3 normal_vec(2*invert_normals - 1,0,0);
    _normals.append(normal_vec);
    _normals.append(normal_vec);
    _normals.append(normal_vec);
    _normals.append(normal_vec);
    
    const auto [ uv_x, uv_y ] = get_texture_coords(block_id);
    
    _uvs.append(Vector2(SCALE* uv_x   , SCALE* uv_y   ));
    _uvs.append(Vector2(SCALE*(uv_x+1), SCALE* uv_y   ));
    _uvs.append(Vector2(SCALE*(uv_x+1), SCALE*(uv_y+1)));
    _uvs.append(Vector2(SCALE* uv_x   , SCALE*(uv_y+1)));

    _add_polygons(invert_normals);
}
void VG::ChunkBuilder::_create_face_xz(const Voxel block_id, const ushort x, const ushort y, const ushort y_offset, bool invert_normals) {
    _vertices.append(Vector3(x  , y_offset, y+1 ));// 0     0       1
    _vertices.append(Vector3(x  , y_offset, y   ));// 1
    _vertices.append(Vector3(x+1, y_offset, y   ));// 2
    _vertices.append(Vector3(x+1, y_offset, y+1 ));// 3     3       2

    Vector3 normal_vec(0,1 - 2*invert_normals, 0);
    _normals.append(normal_vec);
    _normals.append(normal_vec);
    _normals.append(normal_vec);
    _normals.append(normal_vec);
    
    const auto [ uv_x, uv_y ] = get_texture_coords(block_id);

    _uvs.append(Vector2(SCALE* uv_x   , SCALE* uv_y   ));
    _uvs.append(Vector2(SCALE*(uv_x+1), SCALE* uv_y   ));
    _uvs.append(Vector2(SCALE*(uv_x+1), SCALE*(uv_y+1)));
    _uvs.append(Vector2(SCALE* uv_x   , SCALE*(uv_y+1)));

    _add_polygons(invert_normals);
}

// alt:
/*
void ChunkBuilder::greedy_box(const Vector3i &begin, Vector3i &end, const VoxelData* data) {
    // Ob Wachstum in den entspr. Richtungen funktioniert:
    bool z_works = true;
    bool y_works = true;
    bool x_works = true;
    while (true) {
        // überprüfe Chunkgrenzen:
        z_works &= end.z < CHUNK_SIZE;
        y_works &= end.y < CHUNK_SIZE;
        x_works &= end.x < CHUNK_SIZE;
        
        if (z_works) { // hat es irgendwo nicht geklappt, brauch' ich gar nicht mehr zu prüfen
            for (uint x = begin.x; x < end.x; ++x) {
                for (uint y = begin.y; y < end.y; ++y) {
                    z_works &= data->get(x, y, end.z).is_solid() && colbox_jump_value[x][y][end.z]==0; // Die z-Dim. kann als einzige in andere Kollboxen wachsen
                }
                if (!z_works) {
                    break;
                }
            }
        }
        if (y_works) {
            for (uint x = begin.x; x < end.x; ++x) {
                for (uint z = begin.z; z < end.z; ++z) {
                    y_works &= data->get(x,end.y,z).is_solid();
                }
                if (!y_works) {
                    break;
                }
            }
        }
        if (x_works) {
            for (uint y = begin.y; y < end.y; ++y) {
                for (uint z = begin.z; z < end.z; ++z) {
                    x_works &= data->get(end.x,y,z).is_solid();
                }
                if (!x_works) {
                    break;
                }
            }
        }
        bool xy_works = x_works && y_works;
        if (xy_works) {
            // überprüfe diagonal-Block
            for (uint z = begin.z; z < end.z; ++z) {
                xy_works &= data->get(end.x,end.y,z).is_solid();
            }
        }
        bool yz_works = y_works && z_works;
        if (yz_works) {
            for (uint x = begin.x; x < end.x; ++x) {
                yz_works &= data->get(x,end.y,end.z).is_solid();
            }
        }
        bool xz_works = x_works && z_works;
        if (xz_works) {
            for (uint y = begin.y; y < end.y; ++y) {
                xz_works &= data->get(end.x,y,end.z).is_solid();
            }
        }
        bool xyz_works = xy_works && yz_works && xz_works && data->get(end.x,end.y,end.z).is_solid();

        if (xyz_works) {
            end.x += 1; end.y += 1; end.z += 1;
        } else if (xy_works) {
            end.x += 1; end.y += 1;
        } else if (yz_works) {
            end.y += 1; end.z += 1;
        } else if (xz_works) {
            end.x += 1; end.z += 1;
        } else if (x_works) {
            end.x += 1;
        } else if (y_works) {
            end.y += 1;
        } else if (z_works) {
            end.z += 1;
        } else {
            // kein Wachstum mehr möglich
            // Speicher in box-Rand wohin übersprungen werden kann
            for (uint x = begin.x; x < end.x; ++x) {
                for (uint y = begin.y; y < end.y; ++y) {
                    uint z = begin.z;
                    colbox_jump_value[x][y][z] = end.z;
                }
            }
            return;
        }
    }
}


void VG::ChunkBuilder::generate_collision(std::vector<BoxShapeData> &box_data, short chunk_x, short chunk_y, short chunk_z, const VoxelData* data) {
    return;
    const Vector3 chunk_pos = Vector3(chunk_x, chunk_y, chunk_z);
    for (uint x = 0; x < CHUNK_SIZE; ++x) {
        for (uint y = 0; y < CHUNK_SIZE; ++y) {
            for (uint z = 0; z < CHUNK_SIZE; ++z) {
                // Springe bis zum Rand der ColBox, wenn diese breits da sein sollte UND überprüfe ob ich auf dem Rand bin...
                while (z != CHUNK_SIZE && colbox_jump_value[x][y][z] != 0) {
                    z = colbox_jump_value[x][y][z];
                }
                //... wenn auf Rand, dann nächste Zeile
                if (z == CHUNK_SIZE) {
                    break;
                }
                // Springe bis zum Rand der ColBox, wenn diese breits da sein sollte UND überprüfe ob ich auf dem Rand bin...
                //                 while (colbox_jump_value[x][y][z] != 0) {
                //                     z = colbox_jump_value[x][y][z];
                //                     //... wenn auf Rand, dann nächste Zeile
                //                     if (z == end_z) {
                // goto _break_z_;
                //                     }
                //                 }



                // setze Dimension für 1x1x1 Box:
                Vector3i begin_idx{ static_cast<int32_t>(x), static_cast<int32_t>(y), static_cast<int32_t>(z) };
                Vector3i end_idx{ begin_idx.x + 1, begin_idx.y + 1, begin_idx.z + 1 };

                // Kollision für feste Blöcke:
                if (data->get(begin_idx.x,begin_idx.y,begin_idx.z).is_solid()) {
                    // finde möglichst große Box:
                    greedy_box(begin_idx, end_idx, data);
                    // überspringe die nächsten Blöcke, die in der Box sind
                    z = end_idx.z - 1;
                    const Vector3 box_dim = end_idx - begin_idx;
                    const Vector3 pos = (box_dim/2. + begin_idx + chunk_pos) * VOXEL_SIZE;
                    // TODO: 'transform' korrekt berechnen!
                    const Transform3D transform = Transform3D(Basis(), pos);
                    const Vector3 size = (box_dim * VOXEL_SIZE) / 2.;
//                    box_data.emplace_back(ShapeData{size,transform});
                }
            }
        }
    }

    // Setze die Jump-Werte zurück
    memset(&colbox_jump_value[0][0][0], 0, sizeof(colbox_jump_value));
}
*/


VG::VoxelKoords ChunkBuilder::greedy_box_v2(const VoxelKoords begin, const CollisionData data) {
    // Ob Wachstum in den entspr. Richtungen funktioniert:
    bool z_works = true;
    bool y_works = true;
    bool x_works = true;
    VoxelKoords end = begin + 1;
    while (true) {
        // überprüfe ob maximale Shape-Ausdehnung erreicht:
        z_works &= end.z < MAX_SHAPE_EXTENT;
        y_works &= end.y < MAX_SHAPE_EXTENT;
        x_works &= end.x < MAX_SHAPE_EXTENT;

        if (z_works) { // hat es irgendwo nicht geklappt, brauch' ich gar nicht mehr zu prüfen
            for (uint8_t x = begin.x; x < end.x; ++x) {
                for (uint8_t y = begin.y; y < end.y; ++y) {
                    z_works &= data.get_solid(x, y, end.z) && !shape_indicator.get_bit(x, y, end.z);// Die z-Dim. kann als einzige in andere Kollboxen wachsen
                }
                if (!z_works) {
                    break;
                }
            }
        }
        if (y_works) {
            for (uint8_t x = begin.x; x < end.x; ++x) { //TODO: X(und Y)-Richtung braucht -dank bitset- keine Loop mehr
                for (uint8_t z = begin.z; z < end.z; ++z) {
                    y_works &= data.get_solid(x,end.y,z);
                }
                if (!y_works) {
                    break;
                }
            }
        }
        if (x_works) {
            for (uint8_t y = begin.y; y < end.y; ++y) {
                for (uint8_t z = begin.z; z < end.z; ++z) {
                    x_works &= data.get_solid(end.x,y,z);
                }
                if (!x_works) {
                    break;
                }
            }
        }
        bool xy_works = x_works && y_works;
        if (xy_works) {
            // überprüfe diagonal-Block
            for (uint8_t z = begin.z; z < end.z; ++z) {
                xy_works &= data.get_solid(end.x,end.y,z);
            }
        }
        bool yz_works = y_works && z_works;
        if (yz_works) {
            for (uint8_t x = begin.x; x < end.x; ++x) {
                yz_works &= data.get_solid(x,end.y,end.z);
            }
        }
        bool xz_works = x_works && z_works;
        if (xz_works) {
            for (uint8_t y = begin.y; y < end.y; ++y) {
                xz_works &= data.get_solid(end.x,y,end.z);
            }
        }
        bool xyz_works = xy_works && yz_works && xz_works && data.get_solid(end.x,end.y,end.z);

        if (xyz_works) {
            end.x += 1; end.y += 1; end.z += 1;
        } else if (xy_works) {
            end.x += 1; end.y += 1;
        } else if (yz_works) {
            end.y += 1; end.z += 1;
        } else if (xz_works) {
            end.x += 1; end.z += 1;
        } else if (x_works) {
            end.x += 1;
        } else if (y_works) {
            end.y += 1;
        } else if (z_works) {
            end.z += 1;
        } else {
            // kein Wachstum mehr möglich
            // Speicher in 'shape_indicator' welche Voxel Shapes haben
            uint8_t mask = get_mask(end.z - begin.z, begin.z);
            for (uint8_t x = begin.x; x < end.x; ++x) {
                for (uint8_t y = begin.y; y < end.y; ++y) {
                    shape_indicator.set_multiple(x, y, mask);
                }
            }
            return end;
        }
    }
}

//TODO: Funktion die 8 'BoxShapeContainer' ausgibt, damit unteranderem nicht evtl. 8-mal der selbe Chunk geholt wird
// gibt CollisionShapes aus, welche die übergebene PG-position ausfüllen.
VG::BoxShapeContainer VG::ChunkBuilder::generate_collision_shapes(const CollisionData& data) {
    std::vector<BoxShapeData> box_data{};
    box_data.reserve(8); // TODO: ich weiß noch nicht, was hier sinnvoll ist...
    
    
    /************* Behandlung häufiger sonder Fälle: *************/
    // TODO: sind sie wirklich häufig? (nach Häufigkeit sortieren!)
    // - 1. Fall: der GESAMTE PG ist leer:
    if (data.none_solid()) {
        // TODO: andere Aggregatzustände überprüfen!
        //ERR_PRINT("'data.none_solid()' ist eingetreten!");
        return box_data;
    }
    // - 2. Fall: der GESAMTE PG ist massiv:
    if (data.all_solid()) {
        //ERR_PRINT("'data.all_solid()' ist eingetreten!");
        box_data.emplace_back(BoxShapeData{MAX_SHAPE_EXTENT, VoxelKoords{MAX_SHAPE_EXTENT} });
        return box_data;
    }
    
    // Itteriere durch CollisionData der Voxels:
    for (uint8_t x = 0; x < MAX_SHAPE_EXTENT; ++x) {
        for (uint8_t y = 0; y < MAX_SHAPE_EXTENT; ++y) {
            if (shape_indicator.all(x, y)) { // ALLES (in Z-Richtung) ist bereits "geshaped", was will ich dann hier noch tun!
                continue;
            }
            for (uint8_t z = 0; z < MAX_SHAPE_EXTENT; ++z) {
                // springe (in Z-Richtung) bis zum ersten nicht geShapedten Voxel:
                z += shape_indicator.distance_to_next_zero(x, y, z);
                if (z >= 8) {
                    break;
                }
                // Kollision für feste Blöcke:
                if (data.get_solid(x, y, z)) {
                    const VoxelKoords begin{x,y,z};
                    // finde möglichst große Box:
                    const VoxelKoords end = greedy_box_v2(begin, data);

                    const VoxelKoords box_dim = end - begin;
                    const VoxelKoords pos_times_2 = box_dim + begin * 2;
                    box_data.emplace_back(BoxShapeData{box_dim, pos_times_2 });
                }
            }
        }
    }
    // Setze die Jump-Werte zurück
    shape_indicator.reset();

    return box_data;
}



/*
void VVG::Chunk::greedy_face_xy(const Vector2i &begin, Vector2i &end, ushort z) {
    while (true) {
        bool x_works = end.x < CHUNK_SIZE;
        if (x_works) {
            end.x += 1;
            for (uint y = begin.y; y < end.y; ++y) {
                x_works &= is_solid(data[end.x-1][y][z]) & !is_in_face(begin,end);
            }
            end.x -= 1;
        }
        bool y_works = end.y < CHUNK_SIZE;
        if (y_works) {
            end.y += 1;
            for (int32_t x = begin.x; x < end.x; ++x) {
                y_works &= is_solid(data[x][end.y-1][z]) & !is_in_face(begin,end);
            }
            end.y -= 1;
        }
        end.x += 1;
        end.y += 1;
        bool xy_works = x_works & y_works && (is_solid(data[end.x-1][end.y-1][z]) & !is_in_face(begin,end));
        end.x -= 1;
        end.y -= 1;

        if (xy_works) {
            end.x += 1; end.y += 1;
        } else if (x_works) {
            end.x += 1;
        } else if (y_works) {
            end.y += 1;
        } else {
            return; // kein Wachstum mehr möglich
        }
    }
}

void VVG::Chunk::greedy_face_yz(const Vector2i &begin, Vector2i &end, ushort x) {
    while (true) {
        bool y_works = end[0] < CHUNK_SIZE;
        if (y_works) {
            end[0] += 1;
            for (int32_t z = begin[1]; z < end[1]; ++z) {
                y_works &= is_solid(data[x][end[0]-1][z]) & !is_in_face(begin,end);
            }
            end[0] -= 1;
        }
        bool z_works = end[1] < CHUNK_SIZE;
        if (z_works) {
            end[1] += 1;
            for (int32_t y = begin[0]; y < end[0]; ++y) {
                z_works &= is_solid(data[x][y][end[1]-1]) & !is_in_face(begin,end);
            }
            end[1] -= 1;
        }
        end[0] += 1;
        end[1] += 1;
        bool yz_works = y_works & z_works && (is_solid(data[x][end[0]-1][end[1]-1]) & !is_in_face(begin,end));
        end[0] -= 1;
        end[1] -= 1;

        if (yz_works) {
            end[0] += 1; end[1] += 1;
        } else if (y_works) {
            end[0] += 1;
        } else if (z_works) {
            end[1] += 1;
        } else {
            return; // kein Wachstum mehr möglich
        }
    }
}

void VVG::Chunk::greedy_face_xz(const Vector2i &begin, Vector2i &end, ushort y) {
    while (true) {
        bool x_works = end[0] < CHUNK_SIZE;
        if (x_works) {
            end[0] += 1;
            for (int32_t z = begin[1]; z < end[1]; ++z) {
                x_works &= is_solid(data[end[0]-1][y][z]) & !is_in_face(begin,end);
            }
            end[0] -= 1;
        }
        bool z_works = end[1] < CHUNK_SIZE;
        if (z_works) {
            end[1] += 1;
            for (int32_t x = begin[0]; x < end[0]; ++x) {
                z_works &= is_solid(data[x][y][end[1]-1]) & !is_in_face(begin,end);
            }
            end[1] -= 1;
        }
        end[0] += 1;
        end[1] += 1;
        bool xz_works = x_works & z_works && (is_solid(data[end[0]-1][y][end[1]-1]) & !is_in_face(begin,end));
        end[0] -= 1;
        end[1] -= 1;

        if (xz_works) {
            end[0] += 1; end[1] += 1;
        } else if (x_works) {
            end[0] += 1;
        } else if (z_works) {
            end[1] += 1;
        } else {
            return; // kein Wachstum mehr möglich
        }
    }
}
*/
