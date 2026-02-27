# 4-Color Mandala

Application de coloriage de mandalas en C++ avec raylib.

Note: `$HOME/cegep` correspond a `/mnt/c/Users/Admin/OneDrive - Cégep Garneau`.

## Quickstart (Linux/WSL)

```bash
cd $HOME/cegep/Documents/Dev/Mobile/Raylib/4-Color_Mandala
git submodule update --init --recursive

# Build raylib (shared)
cd external/raylib/src
make RAYLIB_LIBTYPE=SHARED

# Build app (debug)
cd $HOME/cegep/Documents/Dev/Mobile/Raylib/4-Color_Mandala
make PROJECT_NAME=main RAYLIB_LIBTYPE=SHARED BUILD_MODE=DEBUG

# Run
export LD_LIBRARY_PATH=$PWD/external/raylib/src:$LD_LIBRARY_PATH
./main
```

## Tests

```bash
cd $HOME/cegep/Documents/Dev/Mobile/Raylib/4-Color_Mandala
make test
```

## Android (debug APK)

Prerequis principaux:
- SDK: `$HOME/Android/Sdk`
- NDK: `$HOME/android-ndk-r26d`
- Java 17

`local.properties` (a la racine):

```properties
sdk.dir=/home/<user>/Android/Sdk
ndk.dir=/home/<user>/android-ndk-r26d
```

Build + install:

```bash
cd $HOME/cegep/Documents/Dev/Mobile/Raylib/4-Color_Mandala
./gradlew assembleDebug

# ADB Wi-Fi (optionnel, si pas en USB)
ADB=$HOME/Android/Sdk/platform-tools/adb
$ADB pair <IP_TEL>:<PORT_PAIR>
$ADB connect <IP_TEL>:<PORT_CONNECT>

$HOME/Android/Sdk/platform-tools/adb install -r app/build/outputs/apk/debug/app-debug.apk
```

## Outils mandala (JSON runtime)

Pipeline recommande:
1. `svg_to_polygons.js` -> polygones JSON
2. `json_to_mandala_code.py` -> `*_regions.json`
3. `generate_adjacency.py` -> `*_adjacency.json`
4. (optionnel) `generate_minizinc.py` -> `.mzn`

### Setup rapide (copier-coller)

```bash
cd $HOME/cegep/Documents/Dev/Mobile/Raylib/4-Color_Mandala/tools

MID=3
SVG=../resources/assets/$MID/$MID.svg

# 1) SVG -> polygones
node svg_to_polygons.js "$SVG" ../resources/assets/$MID/mandala_

# 2) Polygones -> regions runtime
python3 json_to_mandala_code.py ../resources/assets/$MID/mandala_1.json \
  --name "Mandala $MID" --id $MID \
  -o ../resources/assets/$MID/mandala_${MID}_regions.json

# 3) Regions -> adjacency
python3 generate_adjacency.py --json ../resources/assets/$MID/mandala_${MID}_regions.json \
  --mandala-id $MID \
  -o ../resources/assets/$MID/mandala_${MID}_adjacency.json

# 4) (Optionnel) MiniZinc
python3 generate_minizinc.py --mandala-id $MID
```

Winding (`json_to_mandala_code.py`):
- défaut: clockwise
- `--counter-clockwise`: force anti-horaire
- `--no-normalize`: garde l'ordre source

## Manifest runtime

Ajouter/mettre a jour `resources/assets/mandalas_manifest.json`:

```json
[
  {
    "id": 3,
    "name": "Real",
    "regions": "3/mandala_3_regions.json",
    "adjacency": "3/mandala_3_adjacency.json"
  }
]
```

Le jeu charge ce manifest au runtime (desktop + Android assets).

## Debug adjacency en jeu

En mode debug adjacency (`F3`):
- `A`: ajoute une adjacency entre region inspectee et region survolee
- `R`: retire une adjacency

Les changements sont maintenant ecrits automatiquement dans le fichier `*_adjacency.json` du mandala actif.
