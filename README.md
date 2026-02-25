# 4-Color Mandala

Application de coloriage de mandalas en C++ avec raylib.

Note: `$HOME/cegep` correspond a `/mnt/c/Users/Admin/OneDrive - Cégep Garneau`.

## Compilation sur ordinateur (Linux/WSL)

```bash
# Init/update submodules (raylib + libtess2)
cd $HOME/cegep/Documents/Dev/Mobile/Raylib/4-Color_Mandala
git submodule update --init --recursive

# Raylib desktop (shared)
cd external/raylib/src
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
export LD_LIBRARY_PATH=$HOME/cegep/Documents/Dev/Mobile/Raylib/4-Color_Mandala/external/raylib/src:$LD_LIBRARY_PATH
./main
```

Execution (release):

```bash
./main
```

## Tests unitaires (code C++)

Depuis la racine du projet:

```bash
make test
```

Ce target compile un binaire `tests_runner` avec les modules coeur (mandala, base de donnees, palette/couleurs, actions de coloriage) et execute toute la suite.

## Debug (VS Code)

- Ouvrir `main.code-workspace`
- Appuyer sur `F5`

Le task `build debug` compile automatiquement avant le lancement.

## Compilation Android (APK) - recommande (`gradlew`)

Installation rapide (apres pairing/connexion ADB):

```bash
cd $HOME/cegep/Documents/Dev/Mobile/Raylib/4-Color_Mandala
./gradlew assembleDebug
$HOME/Android/Sdk/platform-tools/adb install -r app/build/outputs/apk/debug/app-debug.apk
```

Prerequis:
- NDK: `$HOME/android-ndk-r26d`
- SDK: `$HOME/Android/Sdk`
- Java: `/usr/lib/jvm/java-17-openjdk-amd64`
- CMake: `cmake;3.30.3` (via `sdkmanager`)

Configurer `local.properties` (a la racine du projet, adaptez le chemin utilisateur):

```properties
sdk.dir=/home/ccoulombe/Android/Sdk
ndk.dir=/home/ccoulombe/android-ndk-r26d
```

Commandes:

```bash
cd $HOME/cegep/Documents/Dev/Mobile/Raylib/4-Color_Mandala

# Installer CMake si absent
$HOME/Android/Sdk/cmdline-tools/latest/bin/sdkmanager "cmake;3.30.3"

# Build debug / release
./gradlew assembleDebug
./gradlew assembleRelease
```

APK generes:

```
app/build/outputs/apk/debug/app-debug.apk
app/build/outputs/apk/release/app-release-unsigned.apk
```

Signer l'APK release avant installation (obligatoire):

```bash
APKSIGNER=$(ls -1 $HOME/Android/Sdk/build-tools/*/apksigner | sort -V | tail -n 1)

# Keystore debug local (si absent)
[ -f "$HOME/.android/debug.keystore" ] || keytool -genkeypair -v -storetype PKCS12 \
  -keystore "$HOME/.android/debug.keystore" -storepass android -keypass android \
  -alias androiddebugkey -dname "CN=Android Debug,O=Android,C=US" \
  -keyalg RSA -keysize 2048 -validity 10000

cp app/build/outputs/apk/release/app-release-unsigned.apk \
   app/build/outputs/apk/release/app-release-signed.apk

$APKSIGNER sign --ks "$HOME/.android/debug.keystore" --ks-pass pass:android \
  --key-pass pass:android --ks-key-alias androiddebugkey \
  app/build/outputs/apk/release/app-release-signed.apk
```

Installation debug sur appareil:

```bash
# Generer d'abord l'APK debug si le dossier n'existe pas:
# ./gradlew assembleDebug
$HOME/Android/Sdk/platform-tools/adb install -r app/build/outputs/apk/debug/app-debug.apk

# Installation release (APK non signe):
# ./gradlew assembleRelease
$ADB -r app/build/outputs/apk/release/app-release-signed.apk
```

### Installer sur le telephone

# 0) Pair / connexion ADB (Wi-Fi)
ADB=$HOME/Android/Sdk/platform-tools/adb
$ADB pair <IP_TEL>:<PORT_PAIR>        # entrer le code affiché sur le téléphone
$ADB connect <IP_TEL>:<PORT_CONNECT>
$ADB devices

```bash
# 1) Verifier que le telephone est visible
$HOME/Android/Sdk/platform-tools/adb devices

# 2) Installer (ou mettre a jour) l'APK debug
$ADB install -r app/build/outputs/apk/debug/app-debug.apk

#    Installer (ou mettre a jour) l'APK release (signe)
$ADB install -r app/build/outputs/apk/release/app-release-signed.apk

# 3) (Optionnel) Lancer l'app depuis le PC
$HOME/Android/Sdk/platform-tools/adb shell monkey -p com.CegepGarneau.colormandala -c android.intent.category.LAUNCHER 1
```

Notes utiles:
- Si `INSTALL_FAILED_UPDATE_INCOMPATIBLE`, desinstaller puis reinstaller:
  - `$HOME/Android/Sdk/platform-tools/adb uninstall com.CegepGarneau.colormandala`
  - relancer la commande `adb install -r ...`
- Un APK release non signe (`app-release-unsigned.apk`) ne peut pas etre installe.

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

### Generer un modele MiniZinc (nombre minimal de couleurs)

Le script `generate_minizinc.py` lit:
- `app/src/database/<id>/<id>_regions.cpp`
- `app/src/database/<id>/<id>_adjacency.cpp`

et cree un fichier `.mzn` a cote des assets, par exemple:
- `resources/assets/1/mandala_1.mzn`

```bash
# Depuis le dossier tools/
python generate_minizinc.py --mandala-id 1

# Generer pour tous les IDs detectes
python generate_minizinc.py
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
