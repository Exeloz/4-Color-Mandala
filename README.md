# 4-Color Mandala

Application de coloriage de mandalas en C++ avec raylib.

Note: `$HOME/cegep` correspond a `/mnt/c/Users/Admin/OneDrive - Cégep Garneau`.

## Compilation sur ordinateur (Linux/WSL)

```bash
# Raylib desktop (shared)
cd $HOME/cegep/Documents/Dev/Mobile/Raylib/raylib/src
make clean
make RAYLIB_LIBTYPE=SHARED

# Application (debug)
cd $HOME/cegep/Documents/Dev/Mobile/Raylib/4-Color_Mandala
make clean PROJECT_NAME=main
make PROJECT_NAME=main RAYLIB_LIBTYPE=SHARED BUILD_MODE=DEBUG

# Application (release)
make clean PROJECT_NAME=main
make PROJECT_NAME=main RAYLIB_LIBTYPE=SHARED
```

Execution:

```bash
export LD_LIBRARY_PATH=$HOME/cegep/Documents/Dev/Mobile/Raylib/raylib/src:$LD_LIBRARY_PATH
./main
```

Execution (release):

```bash
./main
```

## Debug (VS Code)

- Ouvrir `main.code-workspace`
- Appuyer sur `F5`

Le task `build debug` compile automatiquement avant le lancement.

## Compilation Android (APK)

Prerequis:
- NDK: `$HOME/android-ndk-r26d`
- SDK: `$HOME/Android/Sdk`
- Java: `/usr/lib/jvm/java-17-openjdk-amd64`
- CMake: `cmake;3.30.3` (via `sdkmanager`)

Commandes:

```bash
# RayMob (Gradle) - recommande
# Installer CMake si absent
$HOME/Android/Sdk/cmdline-tools/latest/bin/sdkmanager "cmake;3.30.3"

# Build debug (RayMob)
cd $HOME/cegep/Documents/Dev/Mobile/Raylib/raymob
./gradlew assembleDebug

# Build release (APK non signe par defaut)
./gradlew assembleRelease
```

Execution (debug) sur appareil:

```bash
adb install -r ../raymob/app/build/outputs/apk/debug/app-debug.apk
```

Note: si `adb` n'est pas trouve, utiliser `$HOME/Android/Sdk/platform-tools/adb`.

Installation via ADB (USB ou Wi-Fi):

```bash
# Verifier la connexion
$HOME/Android/Sdk/platform-tools/adb devices

# Installer l'APK (debug)
$HOME/Android/Sdk/platform-tools/adb install -r ../raymob/app/build/outputs/apk/debug/app-debug.apk
```

ADB Wi-Fi (Android 11+):

```bash
# 1) Activer "Wireless debugging" et lancer "Pair device with pairing code"
$HOME/Android/Sdk/platform-tools/adb pair <IP:PAIR_PORT>

# 2) Se connecter a l'IP:PORT affiche sous "Wireless debugging"
$HOME/Android/Sdk/platform-tools/adb connect <IP:CONNECT_PORT>
$HOME/Android/Sdk/platform-tools/adb devices
```

APK genere:

```
../raymob/app/build/outputs/apk/debug/app-debug.apk
../raymob/app/build/outputs/apk/release/app-release-unsigned.apk
```

## Compilation Android (Makefile.Android - legacy)

Commandes:

```bash
# Raylib Android (static)
cd $HOME/cegep/Documents/Dev/Mobile/Raylib/raylib/src
make clean
make PLATFORM=PLATFORM_ANDROID ANDROID_ARCH=arm64 ANDROID_NDK=$HOME/android-ndk-r26d RAYLIB_LIBTYPE=STATIC

# Application Android (debug)
cd $HOME/cegep/Documents/Dev/Mobile/Raylib/4-Color_Mandala
make PLATFORM=PLATFORM_ANDROID ANDROID_ARCH=arm64 ANDROID_NDK=$HOME/android-ndk-r26d \
  JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64
```

APK genere:

```
game.apk
```

## Outils de création de mandalas

Les outils dans le dossier `tools/` permettent de convertir des SVG en code C++:

```bash
# 1. SVG paths → JSON polygones
npm run svg-to-polygons -- ../resources/assets/3/3.svg ../resources/assets/3/mandala_

# 2. JSON → C++ code avec normalisation automatique du winding
python json_to_mandala_code.py ../resources/assets/3/mandala_1.json \
  --name "Real" --id 3 \
  -o ../src/database/3/3_regions.cpp

# 3. JSON/C++ regions → C++ adjacency graph (tolérant aux petits écarts)
python generate_adjacency.py --json ../resources/assets/3/mandala_1.json \
  -o ../src/database/3/3_adjacency.cpp
```

Le script normalise automatiquement l'ordre des vertices (counter-clockwise par défaut) pour assurer une tessellation correcte. Options disponibles:
- `--clockwise`: normaliser en sens horaire
- `--no-normalize`: préserver l'ordre original (non recommandé)
- `--no-summary`: masquer le résumé des polygones

Le script d'adjacence accepte aussi une source C++ existante:

```bash
python generate_adjacency.py --regions-cpp ../src/database/3/3_regions.cpp \
  -o ../src/database/3/3_adjacency.cpp
```

### Exemples rapides (adjacency)

```bash
# 1) Generation standard (recommandee pour commencer)
python generate_adjacency.py --json ../resources/assets/3/mandala_1.json \
  -o ../src/database/3/3_adjacency.cpp

# 2) Voir le resultat sans ecrire de fichier
python generate_adjacency.py --json ../resources/assets/3/mandala_1.json --stdout

# 3) Tolerance plus grande pour des bordures plus espacees
python generate_adjacency.py --json ../resources/assets/3/mandala_1.json \
  --eps-edge 95 \
  --min-overlap 5 \
  --min-shared-len 28 \
  -o ../src/database/3/3_adjacency.cpp

# 4) Exclure certaines regions (ex: ignorer la region 0)
python generate_adjacency.py --json ../resources/assets/3/mandala_1.json \
  --exclude-regions 0 \
  -o ../src/database/3/3_adjacency.cpp
```

### Comment augmenter la tolerance (distance entre bordures)

Le parametre principal est `--eps-edge`:
- plus grand `--eps-edge` => plus de regions considerees adjacentes meme avec un petit espace
- trop grand `--eps-edge` => risque de faux positifs (regions proches mais pas vraiment voisines)

Parametres utiles a ajuster ensemble:
- `--eps-edge`: distance max entre segments pour etre candidats (principal)
- `--min-overlap`: longueur minimale de recouvrement projete
- `--min-shared-len`: longueur cumulee minimale pour accepter une adjacency

Regle pratique:
- si des voisins manquent: augmenter `--eps-edge` (ex: 70 -> 85 -> 100)
- si trop de voisins apparaissent: augmenter `--min-overlap` et/ou `--min-shared-len`

Exemple "tolerance large" pour debug initial:

```bash
python generate_adjacency.py --json ../resources/assets/3/mandala_1.json \
  --eps-edge 40 \
  --min-overlap 0 \
  --min-shared-len 0 \
  --exclude-regions 0 \
  -o ../src/database/3/3_adjacency.cpp
```

```
