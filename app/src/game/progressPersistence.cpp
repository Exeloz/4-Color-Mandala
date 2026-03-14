#include "progressPersistence.h"

namespace {
constexpr const char* SAVE_FILE_NAME = "mandala_progress.dat";
}

ProgressPersistence::ProgressPersistence()
    : saveFilePath(SAVE_FILE_NAME),
      savedPalette(),
      paletteAvailable(false),
      mandalaStates() {}

std::string ProgressPersistence::makeMandalaKey(int mandalaId, bool hardMode) {
    return hardMode ? (std::to_string(mandalaId) + "H") : std::to_string(mandalaId);
}

void ProgressPersistence::setPalette(const std::vector<Color>& palette) {
    setPalette(palette, false);
}

void ProgressPersistence::setPalette(const std::vector<Color>& palette, bool hardMode) {
    (void)hardMode;
    if (palette.empty()) {
        return;
    }

    savedPalette = palette;
    paletteAvailable = true;
}

const std::vector<Color>& ProgressPersistence::getPalette() const {
    return getPalette(false);
}

const std::vector<Color>& ProgressPersistence::getPalette(bool hardMode) const {
    (void)hardMode;
    return savedPalette;
}

bool ProgressPersistence::hasPalette() const {
    return hasPalette(false);
}

bool ProgressPersistence::hasPalette(bool hardMode) const {
    (void)hardMode;
    return paletteAvailable && !savedPalette.empty();
}
