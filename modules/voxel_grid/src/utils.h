#ifndef UTILS_H
#define UTILS_H

#include "core/error/error_macros.h"
#include "core/math/math_defs.h"
#include "core/math/vector3.h"
#include "core/math/vector3i.h"
#include "core/typedefs.h"
#include <sys/types.h>
#include <bit>
#include <bitset>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <array>
#include <string>
#include <type_traits>
#include <vector>

namespace VG {

static consteval size_t next_prim(size_t number) {
    if (number % 2 == 0) {
        number += 1;
    }
    bool found = false;
    while (!found) {
        found = true;
        number += 2;
        for (size_t divisor = 3; divisor * divisor < number; divisor += 2) {
            if (number % divisor == 0) {
                found = false;
                break;
            }
        }
    }
    return number;
}

template <typename T>
concept IntLike = std::integral<T> && (sizeof(T) < 4);// || std::floating_point<T>;

template <typename T>
concept FloatLike = std::floating_point<T>;

// ermöglicht kleinere Vectoren zu nutzen, die in ein DOUBLE-WORD (8 byte) passen.
template <IntLike T, typename TYP>
struct SVec3 {
    using IntegerRepresentationType = std::conditional_t<sizeof(T) < 2, uint32_t, uint64_t>;
    union {
        struct {
            T x;
            T y;
            T z;
        };
        T coord[3] = { 0 };
    };

    constexpr SVec3() = default;
    constexpr SVec3(const SVec3 &) = default;
    constexpr SVec3& operator=(const SVec3 &) = default;
    constexpr explicit SVec3(T x, T y, T z) : x(x), y(y), z(z) {}
    constexpr SVec3(T val) : x(val), y(val), z(val) {}
    
    constexpr operator Vector3i() const {
        return Vector3i(static_cast<int32_t>(x),static_cast<int32_t>(y),static_cast<int32_t>(z));
    }
    constexpr operator Vector3() const {
        return Vector3(static_cast<real_t>(x),static_cast<real_t>(y),static_cast<real_t>(z));
    }
    constexpr SVec3 operator*(int p_scalar) const {
        return SVec3(static_cast<T>(x * p_scalar), static_cast<T>(y * p_scalar), static_cast<T>(z * p_scalar));
    }
    constexpr Vector3 operator*(real_t p_scalar) const {
        return Vector3(static_cast<real_t>(x) * p_scalar, static_cast<real_t>(y) * p_scalar, static_cast<real_t>(z) * p_scalar);
    }
    constexpr Vector3 operator/(real_t p_scalar) const {
        return Vector3(x / p_scalar, y / p_scalar, z / p_scalar);
    }
    template<IntLike I>
    constexpr SVec3 operator/(I p_scalar) const {
        return SVec3(static_cast<T>(x / p_scalar), static_cast<T>(y / p_scalar), static_cast<T>(z / p_scalar));
    }
    constexpr SVec3 operator+(const SVec3& vec) const {
        return SVec3(static_cast<T>(x + vec.x), static_cast<T>(y + vec.y), static_cast<T>(z + vec.z));
    }
    //[[deprecated("Bei PGKoords ist darauf zu achten, dass nur vielfache von PG_SIZE addiert werden!")]]
    constexpr SVec3 operator+(T p_scalar) const {
        return SVec3(x + p_scalar, y + p_scalar, z + p_scalar);
    }
    constexpr void operator+=(const SVec3& vec) {
        x += vec.x, y += vec.y, z += vec.z;
    }
    //[[deprecated("Bei PGKoords ist darauf zu achten, dass nur vielfache von PG_SIZE addiert werden!")]]
    constexpr void operator+=(T p_scalar) {
        x += p_scalar, y += p_scalar, z += p_scalar;
    }
    constexpr void operator-=(const SVec3& vec) {
        x -= vec.x, y -= vec.y, z -= vec.z;
    }
    //[[deprecated("Bei PGKoords ist darauf zu achten, dass nur vielfache von PG_SIZE addiert werden!")]]
    constexpr void operator-=(T p_scalar) {
        x -= p_scalar, y -= p_scalar, z -= p_scalar;
    }
    //TODO: vieleicht etwas gefährlich, da für T=unsigned keine negativen Werte ausgegeben werden.
    constexpr SVec3 operator-(const SVec3& vec) const {
        return SVec3(static_cast<T>(x - vec.x), static_cast<T>(y - vec.y), static_cast<T>(z - vec.z));
    }
    constexpr const T& operator[](size_t index) const {
        return coord[index];
    }
    constexpr T& operator[](size_t index) {
        return coord[index];
    }
    constexpr std::string to_str() const {
        return '(' + std::to_string(x) + ',' + std::to_string(y) + ',' + std::to_string(z) + ')';
    }
    constexpr bool operator==(const SVec3& vec) const {
        //TODO: Assembler angucken, ob hier optimal gearbeitet wird.
        return (x == vec.x) && (y == vec.y) && (z == vec.z);
    }
    constexpr SVec3 operator-() const {
        return SVec3(static_cast<T>(-x),static_cast<T>(-y),static_cast<T>(-z));
    }
    constexpr SVec3<bool, TYP> operator<(const SVec3& vec) const {
        return SVec3<bool, TYP>(x < vec.x, y < vec.y, z < vec.z);
    }
    constexpr SVec3 operator&(const SVec3<bool, TYP>& vec) const {
        return SVec3(static_cast<T>(x * vec.x), static_cast<T>(y * vec.y), static_cast<T>(z * vec.z));
    }
    constexpr bool is_between_incl(const SVec3& l, const SVec3& r) const {
        return l.x <= x && x <= r.x &&
               l.y <= y && y <= r.y &&
               l.z <= z && z <= r.z;
    }
    constexpr bool is_between_excl(const SVec3& l, const SVec3& r) const {
        return l.x < x && x < r.x &&
               l.y < y && y < r.y &&
               l.z < z && z < r.z;
    }

    constexpr inline T max() const {
        return std::max(std::max(x,y),z);
    }
    constexpr inline SVec3 abs() const {
        return SVec3{static_cast<T>(std::abs(x)), static_cast<T>(std::abs(y)), static_cast<T>(std::abs(z))};
    }
    constexpr inline T squared() const {
        return x*x + y*y + z*z;
    }
    constexpr inline bool is_neighbor(const SVec3& vec) const {
        return (std::abs(x-vec.x) < 2) && (std::abs(y-vec.y) < 2) && (std::abs(z-vec.z) < 2);
    }
    constexpr inline T max_of_abs() const {
        return std::abs(x) < std::abs(y) ? (std::abs(y) < std::abs(z) ? z : y) : (std::abs(x) < std::abs(z) ? z : x);
    }
    
    template<IntLike U, typename UTYP>
    static constexpr SVec3 from(const SVec3<U,UTYP> &vec) {
        return SVec3(static_cast<T>(vec.x),static_cast<T>(vec.y),static_cast<T>(vec.z));
    }
    static constexpr SVec3 from(const Vector3 &vec) {
        return SVec3(static_cast<T>(vec.x),static_cast<T>(vec.y),static_cast<T>(vec.z));
    }
    static constexpr SVec3 from(const Vector3i &vec) {
        return SVec3(static_cast<T>(vec.x),static_cast<T>(vec.y),static_cast<T>(vec.z));
    }
    static constexpr IntegerRepresentationType as_int(const SVec3 &vec) {
        //auto b = *((IntegerType *)&vec);
        IntegerRepresentationType int_rep = *(reinterpret_cast<const IntegerRepresentationType *>(&vec));
        if constexpr (sizeof(IntegerRepresentationType) == 4) {
            int_rep &= 0x00'FF'FF'FF;
        } else {
            int_rep &= 0x0000'FFFF'FFFF'FFFF;
        }
        return int_rep;
    }
    constexpr IntegerRepresentationType as_int() const {
        IntegerRepresentationType int_rep = *reinterpret_cast<const IntegerRepresentationType *>(coord);
        if constexpr (sizeof(IntegerRepresentationType) == 4) {
            int_rep &= 0x00'FF'FF'FF;
        } else {
            int_rep &= 0x0000'FFFF'FFFF'FFFF;
        }
        return int_rep;
    }
};

template <IntLike T, typename TYP>
inline SVec3<T, TYP> min(const SVec3<T, TYP>& a, const SVec3<T, TYP>& b) {
    return SVec3<T, TYP>(std::min(a.x, b.x),std::min(a.y, b.y),std::min(a.z, b.z));
}

template <IntLike T, typename TYP>
inline SVec3<T, TYP> max(const SVec3<T, TYP>& a, const SVec3<T, TYP>& b) {
    return SVec3<T, TYP>(std::max(a.x, b.x),std::max(a.y, b.y),std::max(a.z, b.z));
}

inline real_t max_of_abs(const Vector3& vec) {
    return std::abs(vec.x) < std::abs(vec.y) ? (std::abs(vec.y) < std::abs(vec.z) ? vec.z : vec.y) : (std::abs(vec.x) < std::abs(vec.z) ? vec.z : vec.x);
}


constexpr inline std::string to_str(const Vector3& vec) {
    return '(' + std::to_string(vec.x) + ',' + std::to_string(vec.y) + ',' + std::to_string(vec.z) + ')';
}


class Bit8Array {
private:
    uint8_t _bits = 0x00;

public:
    Bit8Array() = default;
    Bit8Array(uint8_t bits) : _bits(bits) {}
    
    inline bool get(const uint8_t index) const {
        return (_bits >> index) & 0x01;
    }
    inline void set(const uint8_t index) {
        _bits |= (1 << index);
    }
    inline void unset(const uint8_t index) {
        _bits &= ~(1 << index);
    }
    inline uint8_t first_zero() const {
        return static_cast<uint8_t>(std::countr_one(_bits));
    }
    inline uint8_t first_zero_from(const uint8_t index) const {
        return static_cast<uint8_t>(std::countr_one(static_cast<uint8_t>(_bits >> index)));
    }
    inline void operator|=(const uint8_t byte) {
        _bits |= byte;
    }
    inline void operator&=(const uint8_t byte) {
        _bits &= byte;
    }
    inline bool operator==(const Bit8Array arr) {
        return _bits == arr._bits;
    }
    inline Bit8Array operator~() const {
        return static_cast<uint8_t>(~_bits);
    }
    constexpr std::bitset<8> get_bits() {
        return std::bitset<8>(_bits);
    }

        // ERR_PRINT("---------------------A-------------------");
        // Bit8Array arr;
        // std::stringstream strstr;
        // strstr << "arr: " << arr.get_bits() << "\n";
        // Bit8Array arr2{0xFF};
        // strstr << "arr2: " << arr2.get_bits() << "\n";
        // arr2.unset(3);
        // strstr << "unset(3) -> arr2: " << arr2.get_bits() << "\n";
        // strstr << "arr2.first_zero(): " << arr2.first_zero() << "\n";
        // arr.set(5);
        // strstr << "set(5) -> arr: " << arr.get_bits() << "\n";
        
        // ERR_PRINT((strstr.str()).c_str());
        // ERR_PRINT("---------------------V-------------------");

};


class Bit8x8x8Array {
    std::array<std::array<Bit8Array, 8>, 8> _arr = {{{0}}};

public:
    inline bool get_bit(uint8_t x, uint8_t y, uint8_t z) const {
        return _arr[x][y].get(z);
    }
    // gibt die Distanz in z-Richtung zur nächsten 0 aus.
    inline uint8_t distance_to_next_zero(uint8_t x, uint8_t y, uint8_t z) const {
        return _arr[x][y].first_zero_from(z);
    }
    inline void set_bit(uint8_t x, uint8_t y, uint8_t z) {
        _arr[x][y].set(z);
    }
    inline void set_multiple(uint8_t x, uint8_t y, uint8_t mask) {
        _arr[x][y] |= mask;
    }
    inline bool any(uint8_t x, uint8_t y) {
        return _arr[x][y] != 0;
    }
    inline bool none(uint8_t x, uint8_t y) {
        return _arr[x][y] == 0;
    }
    inline bool all(uint8_t x, uint8_t y) {
        return _arr[x][y] == UINT8_MAX;
    }
    inline void reset() {
        memset(&_arr[0][0], 0, sizeof(_arr));
    }
};


class Bit4x4x4Array {
    uint64_t _val = 0;

public:
    inline bool get_bit(uint8_t x, uint8_t y, uint8_t z) const {
        return _val & (1ul << (x + 4*y + 16*z));
    }
    inline void set_bit(uint8_t x, uint8_t y, uint8_t z) {
        _val |= (1ul << (x + 4*y + 16*z));
    }
    inline void reset_bit(uint8_t x, uint8_t y, uint8_t z) {
        _val &= ~(1ul << (x + 4*y + 16*z));
    }
    inline bool any() {
        return _val != 0;
    }
    inline bool none() {
        return _val == 0;
    }
    inline bool all() {
        return _val == UINT64_MAX;
    }
    inline void reset() {
        _val = 0;
    }
};



// TODO: besserer Name hier für (gibt eine Bitmaske aus, die ab 'shift' mit 'count' vielen 1en gefüllt ist)
template <typename T>
constexpr T get_mask(T count, T shift) {
    return static_cast<T>(((1 << count) - 1) << shift);
}
template <typename T>
constexpr T get_mask(int count, T shift) {
    return static_cast<T>(((1 << count) - 1) << shift);
}









/*
template <size_t SIZE, typename KEY, typename HASH>
class HashMap {
    static constexpr ushort SEARCH_DEPTH = 16;
    static constexpr ushort TRYS = 20;
    static constexpr ushort X_SHIFT = 17; // rein zufällige kleine Zahl, damit sich Chunks nicht durch nahe x-Werte Knubbeln
    static constexpr ushort Y_SHIFT = std::pow(SIZE, 1./3.);
    static constexpr ushort Z_SHIFT = std::pow(SIZE, 2./3.) * std::pow(SIZE, 2./3.);
    size_t _occupancy = 0;

    std::array<KEY, SIZE> _elements;
    

    inline static size_t hash(const KEY &key) {
        size_t id_hash = std::hash<size_t>{}(key.grid_id);
        size_t pre_hash = static_cast<size_t>(
              key.koords.x * X_SHIFT 
            + key.koords.y * Y_SHIFT
            + key.koords.z * Z_SHIFT)
            ^ id_hash;
        return (pre_hash + (pre_hash==0)) % SIZE;
    }

    inline static void next_idx(ushort &count, size_t &idx, const KEY &key) {
        count += 1;
        idx += static_cast<size_t>(count * 2 + key.koords.y);
        idx %= SIZE;
    }

    void override_next_free_rekursiv(ChunkV2 &chunk, ushort number_of_try) {
        ERR_FAIL_COND_MSG(SEARCH_DEPTH <= number_of_try, "ERROR: Es wurde einfach kein Freier Platz für den Chunk gefunden...");

        ChunkKey key = chunk.get_key();
        size_t idx = hash(key);

        // merke hier nicht mehr den ersten index sondern "Anzahl an Versuchen" weiter, damit kein unendliches hin-und-her tauschen von 2 Elemetnen entsteht.
        size_t move_idx = idx;
        for (int i = 0; i <= number_of_try; ++i) {
            next_idx(number_of_try, move_idx, key);
        }

        ushort count = 0;
        while (count < SEARCH_DEPTH) {
            if (chunks[idx].is_free()) {
                chunks[idx].override_with(chunk);
                _occupancy += 1;
                return;
            }
            next_idx(count,idx,key);
        }

        // Wenn nach 'SEARCH_DEPTH' Kollisionen kein Platz gefunden wurde: Chunk austauschen und neu Versuchen
        msg += "Oh Nyo! Es hat nicht geklappt OwO'\n";
        override_next_free_rekursiv(chunks[move_idx], number_of_try);
        chunks[move_idx].override_with(chunk);
    }

public:
    //TODO: entferne diese debug msg!
    String msg = "";

    ChunkMapV2() {
        for (uint i = 0; i < SIZE; ++i) {
            chunks[i].initialize_with(&contents[i]);
        }
    }

    inline double get_occupancy_rate() {
        return static_cast<double>(_occupancy) / static_cast<double>(SIZE);
    }

    ChunkContent* get(const ChunkKey &key) {
        ushort count = 0;
        size_t idx = hash(key);
        while (count < SEARCH_DEPTH) {
            if (key == chunks[idx] && !chunks[idx].is_free()) {
                return chunks[idx].get_content();
            }
            next_idx(count, idx, key);
        }
        return nullptr;
    }

    bool get_has_changed(const ChunkKey &key) {
        ushort count = 0;
        size_t idx = hash(key);
        while (count < SEARCH_DEPTH) {
            if (key == chunks[idx] && !chunks[idx].is_free()) {
                return chunks[idx].has_changed_since_load.load(std::memory_order_acquire);
            }
            next_idx(count, idx, key);
        }
        return false;
    }

    void remove(const ChunkKey &key) {
        ushort count = 0;
        size_t idx = hash(key);
        while (count < SEARCH_DEPTH) {
            if (key == chunks[idx] && !chunks[idx].is_free()) {
                _occupancy -= 1;
                chunks[idx].free();
                return;
            }
            next_idx(count, idx, key);
        }
        ERR_FAIL_MSG("Warnung: Zu löschender Chunk existiert nicht!");
    }

    ChunkContent* push(const ChunkKey &&key) {
        msg = "";
        // TODO: dieser Teil erstmal ein workaround und zum sicher gehen:
        if (unlikely(SIZE * 0.8 < _occupancy)) {
            msg += "WARNUNG!!!! ChunkMap ist voll!!!!!!!\n";
            return nullptr;
        }
        
        // key zu index auflösen
        size_t idx = hash(key);
        // merke den ersten index, beim Scheitern wird diese Var. genutzt um das zuverschiebene Element zu bestimmen
        size_t move_idx = idx;
        // nutze loop um 'SEARCH_DEPTH' viele Kollisionen aufzulösen
        ushort count = 0;
        while (count < SEARCH_DEPTH) {
            ChunkV2::PUSH_STATE state = chunks[idx].try_store_successful(key);
            switch (state) {
                case ChunkV2::NOT_POSSIBLE: 
                    next_idx(count, idx, key);
                    break;
                case ChunkV2::WAS_FREE: 
                    _occupancy += 1;
                case ChunkV2::ALREADY_THERE: 
                    return chunks[idx].get_content();
            }
        }
        // Wenn nach 'SEARCH_DEPTH' Kollisionen kein Platz gefunden wurde: Chunk austauschen und neu Versuchen
        msg += "Oh Nyo! Es hat nicht geklappt OwO'\n";
        // move_idx gibt an, welcher chunk ausgetauscht werden soll...
        // ...finde hierfür den nächsten freien Platz durch rekursive Anwendung des selben Algorithmus wie 'push'
        // Hinweis: das Tauschen ist etwas komplizierter, da der Member 'content' nie gelöscht, oder erzeugt werden darf...
        // ... daher swapt override den content. "override" heißt eigentlich "push_and_swap_content".
        // Im Rekursieven Aufruf wandert der gefundene freie 'content' wie bei "Bubble-sort" swap-für-swap nach "oben" und landet zum Schluss hier in 'chunks[move_idx]'
        override_next_free_rekursiv(chunks[move_idx], 0);

        chunks[move_idx].force_store_on_non_free(key);
        return chunks[move_idx].get_content();
    }

};
*/


template <typename TYPE, uint INITIAL_SIZE = 8>
class RingContainer {
    std::vector<TYPE> _vec;
    uint16_t _next = static_cast<uint16_t>(-1);

public:
    RingContainer(const TYPE& val) {
        _vec.resize(INITIAL_SIZE);
        for (auto& ref : _vec) {
            ref = val;
        }
    }
    void add(const TYPE&& type) {
        _next = (_next < _vec.size()) * (_next + 1);
        _vec[_next] = std::move(type);
    }
    void add(const TYPE& type) {
        _next = (_next < _vec.size()) * (_next + 1);
        _vec[_next] = type;
    }
    bool has(const TYPE &type) {
        //ERR_PRINT_ONCE("alle bekannten leeren Chunks:");
        for (auto& ref : _vec) {
            //ERR_PRINT(ref.to_str().c_str());
            if (type == ref) {
                //ERR_PRINT("FOUND!");
                return true;
            }
        }
        return false;
    }
    operator std::vector<TYPE>&() {
        return _vec;
    }
    std::vector<TYPE>::iterator begin() { return _vec.begin(); }
    std::vector<TYPE>::iterator end() { return _vec.end(); }
};





// Behilfs-Makro für ChunkBuilder
#define COLLISIONDATA(STATE)                                                                                                                  \
private:                                                                                                                                      \
    std::bitset<MAX_SHAPE_EXTENT * MAX_SHAPE_EXTENT * MAX_SHAPE_EXTENT> _##STATE;                                                             \
                                                                                                                                              \
    inline std::bitset<MAX_SHAPE_EXTENT * MAX_SHAPE_EXTENT * MAX_SHAPE_EXTENT> &set_##STATE(uint16_t x, uint16_t y, uint16_t z, bool state) { \
        return _##STATE.set(static_cast<size_t>(z + MAX_SHAPE_EXTENT * y + MAX_SHAPE_EXTENT * MAX_SHAPE_EXTENT * x), state);                  \
    }                                                                                                                                         \
                                                                                                                                              \
public:                                                                                                                                       \
    inline bool all_##STATE() const {                                                                                                         \
        return _##STATE.all();                                                                                                                \
    }                                                                                                                                         \
    inline bool any_##STATE() const {                                                                                                         \
        return _##STATE.any();                                                                                                                \
    }                                                                                                                                         \
    inline bool none_##STATE() const {                                                                                                        \
        return _##STATE.none();                                                                                                               \
    }                                                                                                                                         \
    inline bool get_##STATE(uint16_t x, uint16_t y, uint16_t z) const {                                                                       \
        return _##STATE.test(static_cast<size_t>(z + MAX_SHAPE_EXTENT * y + MAX_SHAPE_EXTENT * MAX_SHAPE_EXTENT * x));                        \
    }                                                                                                                                         \
    inline uint8_t STATE##_row(uint16_t x, uint16_t y) const {                                                                                \
        return reinterpret_cast<const uint8_t *>(& _##STATE)[MAX_SHAPE_EXTENT * y + MAX_SHAPE_EXTENT * MAX_SHAPE_EXTENT * x];                           \
    }




#define NEW_VECTOR_TYPE(NAME, UNIT)                                              \
    struct NAME : public SVec3<UNIT, NAME> {                                     \
        constexpr NAME(const SVec3<UNIT, NAME> &vec) : SVec3<UNIT, NAME>(vec) {} \
        constexpr NAME(UNIT x, UNIT y, UNIT z) : SVec3<UNIT, NAME>(x, y, z) {}   \
        constexpr NAME(UNIT val) : SVec3<UNIT, NAME>(val, val, val) {}           \
        constexpr NAME() = default;                                              \
    };

#define NEW_VECTOR_TYPE_2(NAME, UNIT, CONVERTABLE)                                               \
    struct NAME : public SVec3<UNIT, NAME> {                                                     \
        constexpr NAME(const SVec3<UNIT, NAME> &vec) : SVec3<UNIT, NAME>(vec) {}                 \
        constexpr NAME(UNIT x, UNIT y, UNIT z) : SVec3<UNIT, NAME>(x, y, z) {}                   \
        constexpr NAME(UNIT val) : SVec3<UNIT, NAME>(val, val, val) {}                           \
        constexpr NAME() = default;                                                              \
        constexpr NAME(const CONVERTABLE &vec) : SVec3<int16_t, VKoords>(vec.x, vec.y, vec.z) {} \
    };


}; //namespace VVG

#endif // UTILS_H