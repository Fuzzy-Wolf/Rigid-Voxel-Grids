#ifndef CHARACTER_H
#define CHARACTER_H

#include <vector>

#include "core/error/error_macros.h"
#include "core/math/vector3.h"
#include "core/object/object_id.h"
#include "scene/3d/node_3d.h"

#include "../global.h"


namespace VG {

class Character {
public:
    Node3D *node;

    inline const Vector3 get_position() const {
        return node->get_position();
    }
    
    struct THREAD_SAVE {
        // struct CharacterInfo {
        //     // class CloseGrids {
        //     //     struct GridInfo {
        //     //         PGKoords koords;
        //     //         const ObjectID grid_id;
        //     //     };
        //     //     //TODO: besseren Container-typ finden und nutzen
        //     //     FastContainer<GridInfo> _close_voxel_grid_infos;
        //     // public:
        //     //     bool are_koords_still_the_same(const ObjectID vg_id, PGKoords koords) {
        //     //         ERR_PRINT("HEYHOOO!! _close_voxel_grid_infos:");
        //     //         ERR_PRINT(std::to_string(_close_voxel_grid_infos.size()).c_str());
        //     //         for (GridInfo& info : _close_voxel_grid_infos) {
        //     //             if (info.grid_id == vg_id) {
        //     //                 bool is_same = info.koords == koords;
        //     //                 info.koords = koords;
        //     //                 return is_same;
        //     //             }
        //     //         }
        //     //         ERR_PRINT("SOLLTE NICHT SICHTBAR SEIN!");
        //     //         _close_voxel_grid_infos.emplace(GridInfo{koords, vg_id});
        //     //         return false;
        //     //     }
        //     //     void try_remove_voxel_grid(const ObjectID vg_id) {
        //     //         for (auto it = _close_voxel_grid_infos.begin(); it != _close_voxel_grid_infos.end(); ++it) {
        //     //             if (it->grid_id == vg_id) {
        //     //                 _close_voxel_grid_infos.erase(it);
        //     //                 break;
        //     //             }
        //     //         }
        //     //     }
        //     // } close_grids;
        //     PGKoords koords;
        //     CharacterInfo() = delete;
        //     CharacterInfo(const PGKoords& koor) : koords(koor) {}
        // };

    };

};

}; //namespace VG

#endif // CHARACTER_H