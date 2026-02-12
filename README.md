# 4-Color Mandala

Application de coloriage de mandalas en C++ avec raylib.

## Compilation sur ordinateur (Linux/WSL)

```bash
# Raylib desktop (shared)
cd $HOME/cegep/Documents/Dev/Mobile/Raylib/raylib/src
make clean
make RAYLIB_LIBTYPE=SHARED

# Application
cd /mnt/c/Users/Admin/OneDrive\ -\ Cégep\ Garneau/Documents/Dev/Mobile/Raylib/4-Color_Mandala
make clean PROJECT_NAME=main
make PROJECT_NAME=main RAYLIB_LIBTYPE=SHARED BUILD_MODE=DEBUG
```

Execution:

```bash
export LD_LIBRARY_PATH=$HOME/cegep/Documents/Dev/Mobile/Raylib/raylib/src:$LD_LIBRARY_PATH
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

Commandes:

```bash
# Raylib Android (static)
cd $HOME/cegep/Documents/Dev/Mobile/Raylib/raylib/src
make clean
make PLATFORM=PLATFORM_ANDROID ANDROID_ARCH=arm64 ANDROID_NDK=$HOME/android-ndk-r26d RAYLIB_LIBTYPE=STATIC

# Application Android
cd /mnt/c/Users/Admin/OneDrive\ -\ Cégep\ Garneau/Documents/Dev/Mobile/Raylib/4-Color_Mandala
make PLATFORM=PLATFORM_ANDROID ANDROID_ARCH=arm64 ANDROID_NDK=$HOME/android-ndk-r26d \
  JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64
```

APK genere:

```
game.apk
```
