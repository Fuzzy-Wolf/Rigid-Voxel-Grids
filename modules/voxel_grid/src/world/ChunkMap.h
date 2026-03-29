/**
 * @file ChunkMap.h
 * @author your name (you@domain.com)
 * @brief kann nicht von folgenden Files genutzt werden:
    - chunks.h
 * @version 0.1
 * @date 2026-03-03
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#ifndef CHUNKMAP_H
#define CHUNKMAP_H

#include <atomic>
#include <cstddef>

#include "core/error/error_macros.h"
#include "core/typedefs.h"

#include "chunk.h"
#include "../global.h"

namespace VG {


template <size_t SIZE>
class ChunkMapV2 {
    static_assert(MAX_GRID_SIZE*2 < SIZE); // siehe 'next_idx' (? veraltet?)
    friend struct UnLoadData;

    static constexpr ushort SEARCH_DEPTH = 16;
    static constexpr ushort TRYS = 20;
    size_t _occupancy = 0;

    std::array<ChunkV2, SIZE> chunks;
    std::array<ChunkContent, SIZE> contents;
    

    inline static size_t hash(const ChunkKey &key) {
        size_t id_hash = std::hash<size_t>{}(key.grid_id);
        size_t pre_hash = static_cast<size_t>((key.koords.as_int() * 4)  ^ id_hash);
        // size_t pre_hash = static_cast<size_t>(
        //       key.koords.x * 17 // rein zufällige kleine Zahl, damit sich Chunks nicht durch nahe x-Werte Knubbeln
        //     + key.koords.y * MAX_GRID_SIZE
        //     + key.koords.z * MAX_GRID_SIZE * MAX_GRID_SIZE)
        //     ^ id_hash;
        return (pre_hash + (pre_hash==0)) % SIZE; // TODO(erledigt! ich lass es trotzdem mal noch stehen...): "Remainder by zero is undefined" vieleicht muss ich das verbessern
    }

    // TODO: Was ist besser:
    //      - kein Clustering
    //      - Chash-optimiert, aber mit möglicher Clusterbildung
    // TODO: Messe Kollisionen -> ERR_PRINT_ED wenn Kollision oder Counter
    // ich könnte Bsp.weise (da Chunks 32 Byte groß sind) immer 2 Chunks neben einander prüfen, dann einen weiteren Sprung machen u.s.w.
    inline static void next_idx(ushort &count, size_t &idx, const ChunkKey &key) {
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
        ERR_PRINT("Oh Nyo! Es hat nicht geklappt OwO'");
        override_next_free_rekursiv(chunks[move_idx], number_of_try);
        chunks[move_idx].override_with(chunk);
    }

public:

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

    ChunkContent* push(const ChunkKey &key) {
        // TODO: dieser Teil erstmal ein workaround und zum sicher gehen:
        ERR_FAIL_COND_V_MSG((SIZE * 0.8) < _occupancy, nullptr, "WARNUNG!!!! ChunkMap ist voll!!!!!!!");
        
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
        ERR_PRINT("Oh Nyo! Es hat SCHON WIEDER nicht geklappt OwO'");
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





}; //namespace VVG

#endif // CHUNKMAP_H