#include "coloringInspector.h"

#include "../ui/colors.h"
#include "../ui/input.h"

namespace {
Color oppositeColor(Color color) {
    return {
        static_cast<unsigned char>(255 - color.r),
        static_cast<unsigned char>(255 - color.g),
        static_cast<unsigned char>(255 - color.b),
        color.a
    };
}

FillPattern makeLargeDottedStyle(Color fillColor) {
    FillPattern style;
    style.type = FillPatternType::Dotted;
    style.size = 7.5f;
    style.useAccentColor = true;
    style.accentColor = oppositeColor(fillColor);
    return style;
}

FillPattern makeLargeStripedStyle(Color fillColor) {
    FillPattern style;
    style.type = FillPatternType::Striped;
    style.size = 5.f;
    style.useAccentColor = true;
    style.accentColor = oppositeColor(fillColor);
    return style;
}

bool isNativeMobilePlatform() {
#if defined(PLATFORM_ANDROID) || defined(PLATFORM_IOS)
    return true;
#else
    return false;
#endif
}

Color resolveRegionPaletteColor(const Region& region, const std::vector<Color>& colorPalette, Color fallbackColor) {
    int colorIndex = region.getColor();
    if (region.hasColor() && colorIndex >= 0 && colorIndex < static_cast<int>(colorPalette.size())) {
        return colorPalette[colorIndex];
    }

    return fallbackColor;
}
}

void ColoringInspector::enterAnalysisMode() {
    analysisMode = true;
    clearAnalysisSelection();
}

void ColoringInspector::exitAnalysisMode() {
    analysisMode = false;
    clearAnalysisSelection();
}

void ColoringInspector::clearAnalysisSelection() {
    analysisInspectRegionId = -1;
    analysisHoverRegionId = -1;
}

bool ColoringInspector::isAnalysisMode() const {
    return analysisMode;
}

void ColoringInspector::updateAnalysis(const Mandala& mandala,
                                      const Camera2D& camera,
                                      bool pointerOverUi,
                                      bool isDraggingCamera) {
    if (!analysisMode) {
        return;
    }

    if (isNativeMobilePlatform()) {
        analysisHoverRegionId = -1;

        if (pointerOverUi) {
            return;
        }

        if (!isDraggingCamera && Input::IsPointerPressed()) {
            Vector2 worldPos = GetScreenToWorld2D(Input::GetPointerPosition(), camera);
            int tappedRegionId = getRegionIdAtWorldPosition(mandala, worldPos);
            if (tappedRegionId >= 0) {
                analysisInspectRegionId = tappedRegionId;
            }
        }
        return;
    }

    if (pointerOverUi) {
        analysisHoverRegionId = -1;
        return;
    }

    Vector2 worldPos = GetScreenToWorld2D(Input::GetPointerPosition(), camera);
    analysisHoverRegionId = getRegionIdAtWorldPosition(mandala, worldPos);

    if (!isDraggingCamera && Input::IsPointerPressed() && analysisHoverRegionId >= 0) {
        analysisInspectRegionId = analysisHoverRegionId;
    }
}

void ColoringInspector::drawAnalysisOverlay(const Mandala& mandala, const std::vector<Color>& colorPalette) const {
    if (!analysisMode) {
        return;
    }

    if (analysisInspectRegionId >= 0) {
        const Region* selectedRegion = mandala.getRegionById(analysisInspectRegionId);
        if (selectedRegion != nullptr) {
            Color selectedFill = resolveRegionPaletteColor(*selectedRegion, colorPalette, selectedRegion->getDefaultColor());
            selectedRegion->drawWithColor(selectedFill,
                                          Colors::DarkCyan,
                                          6.0f,
                                          makeLargeStripedStyle(selectedFill));

            const auto& neighbors = mandala.getAdjacencyGraph().getAdjacentRegions(analysisInspectRegionId);
            for (int neighborId : neighbors) {
                const Region* neighborRegion = mandala.getRegionById(neighborId);
                if (neighborRegion != nullptr) {
                    Color neighborFill = resolveRegionPaletteColor(*neighborRegion,
                                                                   colorPalette,
                                                                   neighborRegion->getDefaultColor());
                    neighborRegion->drawWithColor(neighborFill,
                                                  Colors::Blue,
                                                  3.0f,
                                                  makeLargeDottedStyle(neighborFill));
                }
            }
        }
    }

    if (analysisHoverRegionId >= 0 && analysisHoverRegionId != analysisInspectRegionId) {
        const Region* hoverRegion = mandala.getRegionById(analysisHoverRegionId);
        if (hoverRegion != nullptr) {
            Color hoverFill = Fade(Colors::LightSkyBlue, 0.35f);
            hoverRegion->drawWithColor(hoverFill,
                                       Colors::DodgerBlue,
                                       2.0f,
                                       makeLargeStripedStyle(hoverFill));
        }
    }
}
