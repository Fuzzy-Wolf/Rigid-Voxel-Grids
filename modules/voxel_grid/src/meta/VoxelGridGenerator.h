#ifndef VOXEL_GRID_GENERATOR_H
#define VOXEL_GRID_GENERATOR_H

#include "modules/noise/fastnoise_lite.h"

#include "../world/chunk.h"
#include "modules/voxel_grid/src/global.h"

namespace VG {
struct GridData;

class VoxelGridGenerator {
public:
    virtual ~VoxelGridGenerator() {
        delete noise; // new_delete noise
    }
    VoxelGridGenerator() : noise(new FastNoiseLite()) /* new_delete noise */ {}
   
    FastNoiseLite * const noise;

    virtual void generate_voxels(ChunkContent *, ChunkKoords, const GridData&) const;
    virtual ChunkContent * generate_chunk_with_voxels(const ChunkKey&, const GridData&) const;
    virtual void generate_chunk_without_voxels(const ChunkKey &, const GridData &) const;

    void fill_voxels_random(VoxelData*, ChunkKoords) const;
};

class PlanetGenerator : public VoxelGridGenerator {
    //TODO:...
    ushort target_radius; // [Chunks]
public:
    virtual ChunkContent* generate_chunk_with_voxels(const ChunkKey &, const GridData &) const override;
    PlanetGenerator() = delete;
    virtual ~PlanetGenerator() = default;
    PlanetGenerator(ushort radius) : target_radius(radius) {}
};

}; //namespace VG

#endif // VOXEL_GRID_GENERATOR_H