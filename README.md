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

Version Release:
```bash
./gradlew assembleRelease
BT_VER=$(ls -1 $HOME/Android/Sdk/build-tools | sort -V | tail -n1)
APKSIGNER=$HOME/Android/Sdk/build-tools/$BT_VER/apksigner

$APKSIGNER sign \
  --ks $HOME/.android/debug.keystore \
  --ks-key-alias androiddebugkey \
  --ks-pass pass:android \
  --key-pass pass:android \
  --out app/build/outputs/apk/release/app-release-signed.apk \
  app/build/outputs/apk/release/app-release-unsigned.apk

$HOME/Android/Sdk/platform-tools/adb install -r app/build/outputs/apk/release/app-release-signed.apk
```

Note: `app-release-unsigned.apk` ne peut pas etre installe tel quel. Pour distribution, utiliser un keystore dedie (pas `debug.keystore`).

## Outils mandala (JSON runtime)

Pipeline recommande:
1. `svg_to_polygons.js` -> polygones JSON
2. `json_to_mandala_code.py` -> `*_regions.json`
3. `generate_adjacency.py` -> `*_adjacency.json`
4. (optionnel) `generate_minizinc.py` -> `.mzn`

### Setup rapide (copier-coller)

```bash
cd $HOME/cegep/Documents/Dev/Mobile/Raylib/4-Color_Mandala/tools

MID=1
SVG=../resources/assets/$MID/$MID.svg

# 1) SVG -> polygones
node svg_to_polygons.js "$SVG" ../resources/assets/$MID/mandala_

# 1b) (Optionnel) Preview SVG des polygones générés
python3 polygons_to_svg.py ../resources/assets/$MID/mandala_1.json \
  ../resources/assets/$MID/mandala_polygons.svg

# 2) Polygones -> regions runtime
python3 json_to_mandala_code.py ../resources/assets/$MID/mandala_1.json \
  --name "Mandala $MID" --id $MID \
  -o ../resources/assets/$MID/mandala_regions.json

# 3) Regions -> adjacency
python3 generate_adjacency.py --json ../resources/assets/$MID/mandala_regions.json \
  --mandala-id $MID --min-overlap 2 --min-shared-len 2 --eps-edge 40 \
  -o ../resources/assets/$MID/mandala_adjacency.json

# 4) (Optionnel) MiniZinc
python3 generate_minizinc.py --mandala-id $MID --include-hard
```

### Banque de solutions Daily (optionnel)

Prerequis: MiniZinc installe et disponible (`minizinc --version`).

```bash
cd $HOME/cegep/Documents/Dev/Mobile/Raylib/4-Color_Mandala/tools

# Genere/re-genere le modele .mzn
python3 generate_minizinc.py --mandala-id 1

# Genere 100 solutions uniques (fichier: resources/assets/1/daily_solutions.txt)
python3 generate_solution_bank.py ../resources/assets/1/mandala_1_regions_satisfy.mzn --count 1000 --solver gecode
```

Si `minizinc` n'est pas dans le PATH:

```bash
python3 generate_solution_bank.py ../resources/assets/1/mandala_1_regions_satisfy.mzn \
  --count 1000 --solver gecode --minizinc-bin /full/path/to/minizinc
```

Winding (`json_to_mandala_code.py`):
- défaut: clockwise
- `--counter-clockwise`: force anti-horaire
- `--no-normalize`: garde l'ordre source

Preview SVG (`polygons_to_svg.py`):
- accepte `mandala_1.json` (polygones bruts) et `mandala_<id>_regions.json` (runtime)
- génère un SVG de contrôle visuel (tracé uniquement, sans remplissage)

Scale (`json_to_mandala_code.py`):
- défaut: largeur cible `10000` (pour garder une épaisseur de contour visuellement cohérente entre mandalas)
- `--target-width <valeur>`: changer la largeur cible
- `--target-width 0`: désactiver le scale automatique

## Manifest runtime

Ajouter/mettre a jour `resources/assets/mandalas_manifest.json`:

```json
[
  {
    "id": 1,
    "name": "Real",
    "regions": "1/mandala_1_regions.json",
    "adjacency": "1/mandala_1_adjacency.json"
  }
]
```

Le jeu charge ce manifest au runtime (desktop + Android assets).

## Debug adjacency en jeu

En mode debug adjacency (`F3`):
- `A`: ajoute une adjacency entre region inspectee et region survolee
- `R`: retire une adjacency

Les changements sont maintenant ecrits automatiquement dans le fichier `*_adjacency.json` du mandala actif.
