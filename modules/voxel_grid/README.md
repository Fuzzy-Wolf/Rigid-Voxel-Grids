## VVG steht für "Versatile Voxel-Grid"

### Fahrplan:

- Reine Funktionalität (so einfach wie möglich)
    - nur Mesh (keine Kollision oder sonst was)
    - erstellung mit einem Block
- erster Stress-Test (wie viel bringt der ganze Quatsch)
- Klären: Was wird in C++ und was in GDscript gehandelt?
- weitere Features:
    - Staticbody
        - übernehme Mesh als ColliderShape [nicht geplant]
    - Chunk-Konstruktor macht keine Meshs oder Shapes -> eigene Funktionen dafür!
    - raycast collision
    - Blöcke setzen/entfernen
    - Rigidbody!
        - nutze viele BoxCollider (müssen positioniert und scaliert werden)
    (- löschen der Grids, wenn letzter Block entfernt) eher nicht...

    - Chunk MESH und COLLSHAPE deaktivieren, wenn außer Reichweite und bei laden (wenn bereits geladen) MESH und COLL wieder aktivieren
    - mehrere (2) Meshinstancen pro Chunk (eine weitere für animierte Texturen)
    - Unterscheidung zwischen "akive" und "kaum aktive" Chunks
        - "kaum aktive" Chunks werden erkannt, aber nicht berechnet (damit kein Fehler entsteht wenn der Rand verändert wird...) vieleicht unnötig... [nicht geplant]
    - LODs nutzen 6 Texturen (wohl in einem Bild gespeichert) und berechen die UVs aus den Koordinaten ihrer Vertices (+greedy-face)
    - Spieler Position auf 0,0,0 zurücksetzen und universum-kontent bewegen
    - ...
- Optimierungen:
    - wird ein alter Chunk in der ChunkMap reaktiviert, sollte auch das Mesh reaktiviert werden und nicht neu erzeugt.
    - RID durch tatsächliche "JoltObject-Pointer"/"RenderServerObjekt-Pointer" ersetzen und eigene Funktionen in den Servern implementieren
    - Load- und Delete-Thread hat einen ganz eigenen unter Thread, weil er so viel tuen muss (push- und remove-chunk passieren alle auf dem selben Thread) (remove muss glaube ich auf dem MainThread passieren....) - hier sind noch einige Dinge unklar!
    - PhysicsGrid-Punkte werden erst nach längerem Bestehen zusammen gefasst (nicht bei Erzeugung).
    - neben RADIUS auch min_aktivation_radius für jeden Thread 
    - wenn Bits in der VOXEL::ID übrig bleiben speichern, wie viele der nächsten (z-Richtung) Blöcke keine Coll haben
    - chunk-wände können bedeckt sein (Nachbarchunks müssen im Meshing berücksichtigt werden)
    - Chunk-laden auf es müssen mindestens 2 Chunks Differenz sein, um schnelles hin-und-her über chunk-grenze zu verbessern
    - (Vertice-Reduktion (ist hiermit greedy-meshing gemeint?))
    - cache/drawcall-Optimierung
    - [?] in der Block-ID speichern, ob der Block sichtbar ist (alle 6 nachbarn sichtbar) (ist wahrscheinlich nicht so sinnvoll...)
    - VOXEL_SIZE: 1 -> 0.66 (besser alles andere einfach 1.3-mal größer/schneller machen)
    - Godot hat interen workerthreads:
```C++
#include <godot_cpp/classes/thread_pool.hpp>

ThreadPool* pool = Engine::get_singleton()->get_thread_pool();

// Eine Aufgabe hinzufügen
pool->push([]() {
    // Hier Heavy Work
});
```
- 
    - [?] seltener Randfall: zu viele Collisionboxes, dann einfach über Lücken drüber
    - [?]* Nicht alle Grids in die selbe HashMap. Für kleine Grids eigene Speicher...
- Stress-Test (erkennt man die Verbesserungen?)
- check custom compilerflags (https://docs.godotengine.org/en/4.5/engine_details/architecture/custom_modules_in_cpp.html)
- finaler Stress-Test




VoxelTool -> liefert Chunk-Meshs (Mesh,Position)
Chunks (Daten) sind in VoxelTool gespeichert
VoxelTool -> liefert Chunk-CollisionShapes (Shapes,Positionen)
    -> static CollisionShape
    -> rigid CollisionShapes
CollisionShapes -> wissen zu welchen Chunk sie gehören (Speicher-Addresse des Chunks)
    -> Mesh anpassen
    -> CollisionShape anpassen (static, rigid)

2 Arten wie das VoxelTool funktioniert:
    - 1. erzeugen von Objekten -> Rückgabe: MeshInstance3D + Array[CollisionShape3D]
    - 2. aktualisieren von Chunks -> Rückgabe: Mesh + Array[CollisionShape3D]


(für Y:
- Packet_TYPE_Array gibt es in GDscript)



## Erledigte Sachen:
    - Eigene HashMap für die Speicherung der aktiven Chunks:
        - Feste Größe, bei Kollision checken ob Chunk alt ist
        - Kollisionsstrategie...
    - CHUNK_SIZE: 16³ -> 32³
    - Voxeldaten in Chunk separat (per Pointer) Speichern, neben Pointer auch LOD. Dann können bei weit entfernt nur LOD geladen werden.
    - RenderingServer::get_singleton()->instance_create(); sollte vorgehalten werden. Also einmal am Anfang für alle Chunks erstellen und dann niemals löschen und nur mit "shape" arbeiten
    - multi-Threading



*[?] bedeutet : Idee nicht ausgereift


Stellas ingressi sumus
Inter astra iter facimus
Inter astra vivimus



### zum Compilieren:
```bash
scons platform=linuxbsd use_llvm=yes linker=mold
```
Mit aufspüren von Memoryleaks und Nullpointer-Zugriffe (Hilfreich, wenn Godot abstürzt):
```bash
scons platform=linuxbsd use_llvm=yes linker=mold use_asan=yes use_ubsan=yes target=editor
```
