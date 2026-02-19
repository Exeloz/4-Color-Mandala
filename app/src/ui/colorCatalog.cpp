#include "colorCatalog.h"
#include "colors.h"

const std::vector<Color>& ColorCatalog::getAvailableColors() {
    static const std::vector<Color> colors = {
        Colors::Red, Colors::DarkRed, Colors::Crimson, Colors::FireBrick, Colors::Coral, Colors::Salmon, Colors::LightCoral, Colors::IndianRed,
        Colors::Orange, Colors::DarkOrange, Colors::OrangeRed, Colors::Tomato, Colors::Tangerine, Colors::PeachPuff,
        Colors::Yellow, Colors::Gold, Colors::LightYellow, Colors::LemonChiffon, Colors::Khaki, Colors::DarkKhaki,
        Colors::Green, Colors::DarkGreen, Colors::ForestGreen, Colors::LimeGreen, Colors::Lime, Colors::SpringGreen, Colors::SeaGreen, Colors::MediumSeaGreen,
        Colors::LightGreen, Colors::PaleGreen, Colors::Chartreuse, Colors::YellowGreen, Colors::OliveDrab, Colors::Olive,
        Colors::Cyan, Colors::Aqua, Colors::DarkCyan, Colors::LightCyan, Colors::Turquoise, Colors::MediumTurquoise, Colors::DarkTurquoise, Colors::Aquamarine, Colors::PaleTurquoise,
        Colors::Blue, Colors::DarkBlue, Colors::MediumBlue, Colors::Navy, Colors::RoyalBlue, Colors::SteelBlue, Colors::DodgerBlue, Colors::DeepSkyBlue,
        Colors::SkyBlue, Colors::LightSkyBlue, Colors::LightBlue, Colors::PowderBlue, Colors::CornflowerBlue, Colors::CadetBlue,
        Colors::Purple, Colors::DarkMagenta, Colors::DarkViolet, Colors::DarkOrchid, Colors::Indigo, Colors::BlueViolet, Colors::MediumPurple, Colors::MediumOrchid,
        Colors::Orchid, Colors::Violet, Colors::Plum, Colors::Thistle, Colors::Lavender,
        Colors::Magenta, Colors::Fuchsia, Colors::DeepPink, Colors::HotPink, Colors::Pink, Colors::LightPink, Colors::PaleVioletRed, Colors::MediumVioletRed,
        Colors::Brown, Colors::SaddleBrown, Colors::Sienna, Colors::Chocolate, Colors::Peru, Colors::SandyBrown, Colors::BurlyWood, Colors::Tan, Colors::RosyBrown, Colors::Wheat, Colors::Beige,
        Colors::White, Colors::Snow, Colors::Ivory, Colors::Honeydew, Colors::MintCream, Colors::Azure, Colors::AliceBlue, Colors::GhostWhite, Colors::WhiteSmoke,
        Colors::Seashell, Colors::OldLace, Colors::FloralWhite, Colors::AntiqueWhite, Colors::Linen, Colors::LavenderBlush, Colors::MistyRose,
        Colors::Gray, Colors::DarkGray, Colors::Silver, Colors::LightGray, Colors::Gainsboro, Colors::DimGray, Colors::SlateGray, Colors::LightSlateGray, Colors::DarkSlateGray,
        Colors::Black
    };

    return colors;
}
