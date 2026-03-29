#ifndef TYPE_DEFS_H
#define TYPE_DEFS_H

#include <chrono>
#include <cstdint>
#include <plf_colony.h>
template<typename T>
using FastContainer = plf::colony<T>;

#include "core/math/math_defs.h"

#include "utils.h"


namespace VG {
/************************************************ Einheiten ************************************************/
using ushort = unsigned short;
// using VoxelKoords = SVec3<uint8_t,VoxelKoords>; // [Voxel] in Chunk
// using ChunkKoords = SVec3<int16_t>; // [Chunk] in Grid
// using    PGKoords = SVec3<int16_t>; // [Voxel] in Grid (PG = PhyicsGrid) sollte immer ein Vielfaches von PG_SIZE sein! PGKoords sind VKoords, aber nicht andersherum
// using     VKoords = SVec3<int16_t>; // [Voxel] in Grid

// [Voxel] in Chunk
NEW_VECTOR_TYPE(VoxelKoords, uint8_t)
// // [Chunk] in Grid
NEW_VECTOR_TYPE(ChunkKoords, int16_t)
// // [Voxel] in Grid (PG = PhyicsGrid) sollte immer ein Vielfaches von PG_SIZE sein! PGKoords sind VKoords, aber nicht andersherum
NEW_VECTOR_TYPE(PGKoords, int16_t)
// // [Voxel] in Grid
NEW_VECTOR_TYPE_2(VKoords, int16_t, PGKoords)

// Helfer-struct
struct BoxShapeData {
    VoxelKoords size_times_2; // doppelte größe des BoxShapes (da ich mit halben Koordinaten arbeiten muss)
    VoxelKoords koords_times_2; // Koordinaten des BoxShapes im PG
};
using BoxShapeContainer = std::vector<VG::BoxShapeData>;


// static constexpr PGKoords INVALID_PGKoords = PGKoords{INT16_MAX};
// static constexpr ChunkKoords INVALID_ChunkKoords = ChunkKoords{INT16_MAX};
// static constexpr VoxelKoords INVALID_VoxelKoords = VoxelKoords{INT8_MAX};

using BLOCK_ID_TYPE = uint16_t; // 10-bits (momentan zu mindest...)
enum BLOCK_ID : BLOCK_ID_TYPE {
    // --- Blocks: ---
    NOTHING = 0x0000,
    _MASS_0_ = NOTHING,
    // GRAS, STEIN, GENERATOR_01, etc...

    DEBUG = 0x03FF,
    _MASS_1000000_ = DEBUG
};


/************************************************ Konstanten ************************************************/

// Shading/Meshing:
static constexpr ushort FACE_TEXTURE_SIZE = 64; // Pixelgröße der Flächen der Würfel Texturen
static constexpr ushort WHOLE_TEXTURE_SIZE = 1024; // Pixelgröße der gesamt Textur
static constexpr real_t SCALE = (4.0 * FACE_TEXTURE_SIZE) / WHOLE_TEXTURE_SIZE; //TODO: ??

//Phyiscs/ShapeHolder
static constexpr ushort MAX_DELETED_SHAPE_HOLDER_COUNT = 32; // sind mehr ShapeHolder in der "gelöscht"-Liste wird diese geleert und die PhysicsShapes fachgemäß entsorgt.

// Fundamentale Größen:
static constexpr float VOXEL_SIZE = 1; // [Godot-units] Kantenlänge von jeden Würfel/Voxel
static constexpr ushort PG_SIZE = 8; // [Voxel] Größe einer PhysicsGrid-(PG)-Einheit
static constexpr ushort MAX_SHAPE_EXTENT = 8; // [Voxel] Maximale Größe der CollisionShapes ([= vielfaches von 8!] größer -> aufwändiger, aber weniger shapes)
static constexpr ushort CHUNK_SIZE = 32; // [Voxel]
static constexpr ushort MAX_GRID_SIZE = 512; // [Chunks] Maximale Größe eines VoxelGrids
static constexpr ushort MAX_CHARACTER_COUNT = 512; // Maximale Anzahl an physikbasierten Characters (siehe ShapeHolder für Begründung: passt in 8 Bit)
enum RADIUS /* [Chunks] */ : ushort  { 
    COLLISION_NEAR = 1,
    COLLISION_MID = 4,
    COLLISION_FAR = 16, // evtl. überflüssig

    DRAW_FULL = 2,//4
    DRAW_LOD1 = 8,
    DRAW_LOD2 = 16,
    DRAW_LOD3 = 32,

    //TODO: das hier sollte wohl später entfernt werden:
    LOAD = DRAW_FULL + 1,
};
enum TaskID {
    LOADING = 0,
    MESHING,
    PHYSICS
};
struct PersistentThreadData {
    std::tuple <
        /*LOADING*/ ChunkKoords,
        /*MESHING*/ ChunkKoords
        /*PHYSICS*/ /* CharaktersPhysicsTask::ThreadData */> data;
};

// Speichermanagement (Lesen, Schreiben, Datenhaltung)
//TODO: Damit 'CHUNK_MEMORY_UNIT_SIZE' größer als 1 sein kann muss das TODO in 'generate_voxels' abgearbeitet werden!!
static constexpr size_t CHUNK_MEMORY_UNIT_SIZE = 1; // [1³] sollte eine Qubik-Zahl sein
static constexpr ushort AKTIVE_CHUNKS_STORAGE_SIZE = next_prim(8192); // größe des Chunkspeichers, hier müssen alle (großen?) VoxelGrid-Chunks hinein passen
static constexpr size_t SHAPEHOLDER_STORAGE_SIZE = 128; // initiale Anzahl an haltbaren 'ShapeHolder' pro VoxelGrid

// MulitThreading Konstanten
static constexpr std::chrono::milliseconds WORKER_THREAD_WAIT_TIME{5}; // 5 Milliseconds


enum DIRECTION {
    TOP,
    BOTTOM,
    EAST,
    WEST,
    SOUTH,
    NORTH,
};

//TODO: !!!!!!!! GLOABEL FLAGS nutzen um zu erkennen wenn neue Charactere/VoxelGrids/ oder sonst was hinzu oder weg kommen. -> viele Optimierungen möglich!!!!!
// könnte über statische semaphores realisiert werden! vieleicht eher globales Array mit allen semaphores statt static...

}; //namespace VVG

#endif // TYPE_DEFS_H