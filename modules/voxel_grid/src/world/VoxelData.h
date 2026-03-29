#ifndef VOXEL_DATA_H
#define VOXEL_DATA_H

#include <array>
#include <bitset>

#include "core/math/math_defs.h"
#include "modules/noise/noise.h"

#include "../global.h"

namespace VG {

class VoxelData;
/**
 * @brief Voxel... ist mir egal was hier steht.
 *
 * 8-bits: HP (256)
 *
 * 4-bits: Farbe (16)
 *
 * 2-bits: Farbhelligkeit (4)
 *
 * 5-bits: Rotation (24 (+8))
 *
 * 10-bits: Blocktyp (1024) [2-bits: AMOR (0-empfindlich, 1-normal, 2-stabil, 3-gepanzert)]
 *
 * ?-bits: Texture (0-alle faces gleich, 1-alle unterschiedlich, 2-??)
 *
 * 2-bits Agregat (0 Weltr.-Nebel/Vak., 1 Atmo, 2 Wasser, 3 Solid)
 *
 * 1-bit Transparent (0-trasnparent, 1-opak)
 */
union Voxel {
    friend class VG::VoxelData;
    //TODO: 16-bit könnten ausreichend sein...
    using T = uint32_t;

    enum AGGREGATE {
        VAC = 0,
        GAS,
        LIQ,
        SOL
    };
    enum COLOR {
        MEOW
    };
    enum COLOR_BRIGHTNESS {
        NYA
    };
    enum ROT {
        BAU_BAU
    };
    struct {
        BLOCK_ID_TYPE id : 10;
        T hp : 8;
        COLOR color : 4;
        COLOR_BRIGHTNESS color_brightness : 2;
        ROT rotation : 5;
        AGGREGATE aggregate : 2;
        bool transparent : 1;
    };

    Voxel() : _bits(0) {}
    // inline explicit Voxel(T bits) : _bits(bits) {}
    // inline explicit Voxel(BLOCK_ID_TYPE id) : id(id), hp(100), aggregate(SOL) {}
    inline void operator=(T bits) {
        _bits = bits;
    }
    inline void operator=(BLOCK_ID_TYPE block) = delete;
    inline bool operator==(BLOCK_ID_TYPE block) const {
        return _bits == block;
    }
    inline bool is_invisible() const {
        return id == BLOCK_ID::NOTHING;
    }
    inline bool is_solid() const {
        return aggregate == SOL;
    }
    inline operator std::string() const {
        std::stringstream stream;
        stream << "VOXEL: 0x" << std::uppercase << std::hex << _bits << "\n";
        stream << "Id         : 0x" << std::uppercase << std::hex << id << "\n";
        stream << "Color      : " << color << "\n";
        stream << "Brightness : " << color_brightness << "\n";
        stream << "Hitpoitns  : " << std::dec << hp << "\n";
        stream << "Aggregate  : " << std::bitset<2>(aggregate) << "\n";
        stream << "Transparent: " << (transparent ? "True" : "False");
        return stream.str();
    }
    Voxel(BLOCK_ID ID, AGGREGATE aggregate, bool transparent,
        uint8_t HP = 100, ROT rotation = BAU_BAU,
        COLOR_BRIGHTNESS color_brightness = NYA, COLOR color = MEOW)
    : id(ID), hp(HP), color(color)
    , color_brightness(color_brightness)
    ,rotation(rotation),aggregate(aggregate)
    ,transparent(transparent) {}

private:
    T _bits;
};

inline static std::array<ushort, 2> get_texture_coords(const Voxel voxel) {
    // TODO:
    return { 0, 0 };
}




class VoxelData {
    std::array<std::array<std::array<Voxel, CHUNK_SIZE>, CHUNK_SIZE>, CHUNK_SIZE> _container;

public:
    inline Voxel& get(const ushort x, const ushort y, const ushort z) {
        return _container[x][y][z];
    }
    inline const Voxel& get(const ushort x, const ushort y, const ushort z) const {
        return _container[x][y][z];
    }

    // Wenn ein VoxelData nicht valide ist, kann ich andere Daten in ihm speichern.
    // Momentan für das Laden von Voxeln gebraucht, da ich so den Speicher vorab erstellen kann,
    // auch wenn die Voxel nicht auf der Festplatte existierten. //TODO: wahrscheinlich geht das besser... ist momentan nur ein workaround
    bool is_valid() {
        return _container[0][0][0]._bits != 0xFF'FF'FF'FF;
    }
};


} //namespace VG

#endif // VOXEL_DATA_H