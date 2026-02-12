# Building for Android

## Prerequisites

1. **Android NDK** (r21 or later)
   - Download: https://developer.android.com/ndk/downloads
   - Extract to: `/usr/lib/android/ndk` (Linux) or `C:/android-ndk` (Windows)

2. **Android SDK** with Build Tools 29.0.3+
   - Download: https://developer.android.com/studio
   - Set path: `C:/android-sdk` (Windows) or install via package manager (Linux)

3. **Java JDK** (OpenJDK 8 or later)
   - Linux: `sudo apt install openjdk-11-jdk`
   - Windows: Download from https://adoptium.net/

## Environment Setup (Linux)

```bash
export ANDROID_NDK=/usr/lib/android/ndk
export ANDROID_HOME=$HOME/Android/Sdk
export JAVA_HOME=/usr/lib/jvm/java-11-openjdk-amd64
```

Add to your `~/.bashrc` for persistence.

## Build Commands

### Build APK (ARM64 - recommended for modern devices)
```bash
make PLATFORM=PLATFORM_ANDROID ANDROID_ARCH=ARM64
```

### Build for specific architectures
```bash
# ARM 32-bit (older devices)
make PLATFORM=PLATFORM_ANDROID ANDROID_ARCH=ARM

# x86_64 (emulators)
make PLATFORM=PLATFORM_ANDROID ANDROID_ARCH=x86_64
```

### Install to connected device
```bash
make PLATFORM=PLATFORM_ANDROID ANDROID_ARCH=ARM64
adb install -r android.color_mandala/bin/ColorMandala.apk
```

## Output Files

- APK: `android.color_mandala/bin/ColorMandala.apk`
- Native library: `android.color_mandala/lib/arm64-v8a/libmain.so`

## Input Handling

The app uses `Input::IsPointerPressed()` and `Input::GetPointerPosition()` which automatically work with:
- **Desktop**: Mouse input
- **Android**: Touch input via gesture detection

No code changes needed between platforms!

## Troubleshooting

**"NDK not found"**: Set `ANDROID_NDK` environment variable
**"Build tools not found"**: Install Android SDK Build Tools 29.0.3
**"Java not found"**: Set `JAVA_HOME` to your JDK installation

## Screen Orientation

Default: **Landscape** mode
Change in `Makefile.Android` line 86: `APP_SCREEN_ORIENTATION ?= portrait`
