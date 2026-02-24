#ifndef COLORS_H
#define COLORS_H

#include "raylib.h"

namespace Colors {
    constexpr Color None = {200, 200, 200, 255};
    constexpr Color Transparent = {0, 0, 0, 0};
    
    constexpr Color Red = {255, 0, 0, 255};
    constexpr Color DarkRed = {139, 0, 0, 255};
    constexpr Color Crimson = {220, 20, 60, 255};
    constexpr Color FireBrick = {178, 34, 34, 255};
    constexpr Color Coral = {255, 127, 80, 255};
    constexpr Color Salmon = {250, 128, 114, 255};
    constexpr Color LightCoral = {240, 128, 128, 255};
    constexpr Color IndianRed = {205, 92, 92, 255};
    
    constexpr Color Orange = {255, 165, 0, 255};
    constexpr Color DarkOrange = {255, 140, 0, 255};
    constexpr Color OrangeRed = {255, 69, 0, 255};
    constexpr Color Tomato = {255, 99, 71, 255};
    constexpr Color Tangerine = {242, 133, 0, 255};
    constexpr Color PeachPuff = {255, 218, 185, 255};
    
    constexpr Color Yellow = {255, 255, 0, 255};
    constexpr Color Gold = {255, 215, 0, 255};
    constexpr Color LightYellow = {255, 255, 224, 255};
    constexpr Color LemonChiffon = {255, 250, 205, 255};
    constexpr Color Khaki = {240, 230, 140, 255};
    constexpr Color DarkKhaki = {189, 183, 107, 255};
    
    constexpr Color Green = {0, 255, 0, 255};
    constexpr Color DarkGreen = {0, 100, 0, 255};
    constexpr Color ForestGreen = {34, 139, 34, 255};
    constexpr Color LimeGreen = {50, 205, 50, 255};
    constexpr Color Lime = {0, 255, 0, 255};
    constexpr Color SpringGreen = {0, 255, 127, 255};
    constexpr Color SeaGreen = {46, 139, 87, 255};
    constexpr Color MediumSeaGreen = {60, 179, 113, 255};
    constexpr Color LightGreen = {144, 238, 144, 255};
    constexpr Color PaleGreen = {152, 251, 152, 255};
    constexpr Color Chartreuse = {127, 255, 0, 255};
    constexpr Color YellowGreen = {154, 205, 50, 255};
    constexpr Color OliveDrab = {107, 142, 35, 255};
    constexpr Color Olive = {128, 128, 0, 255};
    
    constexpr Color Cyan = {0, 255, 255, 255};
    constexpr Color Aqua = {0, 255, 255, 255};
    constexpr Color DarkCyan = {0, 139, 139, 255};
    constexpr Color LightCyan = {224, 255, 255, 255};
    constexpr Color Turquoise = {64, 224, 208, 255};
    constexpr Color MediumTurquoise = {72, 209, 204, 255};
    constexpr Color DarkTurquoise = {0, 206, 209, 255};
    constexpr Color Aquamarine = {127, 255, 212, 255};
    constexpr Color PaleTurquoise = {175, 238, 238, 255};
    
    constexpr Color Blue = {0, 0, 255, 255};
    constexpr Color DarkBlue = {0, 0, 139, 255};
    constexpr Color MediumBlue = {0, 0, 205, 255};
    constexpr Color Navy = {0, 0, 128, 255};
    constexpr Color RoyalBlue = {65, 105, 225, 255};
    constexpr Color SteelBlue = {70, 130, 180, 255};
    constexpr Color DodgerBlue = {30, 144, 255, 255};
    constexpr Color DeepSkyBlue = {0, 191, 255, 255};
    constexpr Color SkyBlue = {135, 206, 235, 255};
    constexpr Color LightSkyBlue = {135, 206, 250, 255};
    constexpr Color LightBlue = {173, 216, 230, 255};
    constexpr Color PowderBlue = {176, 224, 230, 255};
    constexpr Color CornflowerBlue = {100, 149, 237, 255};
    constexpr Color CadetBlue = {95, 158, 160, 255};
    
    constexpr Color Purple = {128, 0, 128, 255};
    constexpr Color DarkMagenta = {139, 0, 139, 255};
    constexpr Color DarkViolet = {148, 0, 211, 255};
    constexpr Color DarkOrchid = {153, 50, 204, 255};
    constexpr Color Indigo = {75, 0, 130, 255};
    constexpr Color BlueViolet = {138, 43, 226, 255};
    constexpr Color MediumPurple = {147, 112, 219, 255};
    constexpr Color MediumOrchid = {186, 85, 211, 255};
    constexpr Color Orchid = {218, 112, 214, 255};
    constexpr Color Violet = {238, 130, 238, 255};
    constexpr Color Plum = {221, 160, 221, 255};
    constexpr Color Thistle = {216, 191, 216, 255};
    constexpr Color Lavender = {230, 230, 250, 255};
    
    constexpr Color Magenta = {255, 0, 255, 255};
    constexpr Color Fuchsia = {255, 0, 255, 255};
    constexpr Color DeepPink = {255, 20, 147, 255};
    constexpr Color HotPink = {255, 105, 180, 255};
    constexpr Color Pink = {255, 192, 203, 255};
    constexpr Color LightPink = {255, 182, 193, 255};
    constexpr Color PaleVioletRed = {219, 112, 147, 255};
    constexpr Color MediumVioletRed = {199, 21, 133, 255};
    
    constexpr Color Brown = {165, 42, 42, 255};
    constexpr Color SaddleBrown = {139, 69, 19, 255};
    constexpr Color Sienna = {160, 82, 45, 255};
    constexpr Color Chocolate = {210, 105, 30, 255};
    constexpr Color Peru = {205, 133, 63, 255};
    constexpr Color SandyBrown = {244, 164, 96, 255};
    constexpr Color BurlyWood = {222, 184, 135, 255};
    constexpr Color Tan = {210, 180, 140, 255};
    constexpr Color RosyBrown = {188, 143, 143, 255};
    constexpr Color Wheat = {245, 222, 179, 255};
    constexpr Color Beige = {245, 245, 220, 255};
    
    constexpr Color White = {255, 255, 255, 255};
    constexpr Color Snow = {255, 250, 250, 255};
    constexpr Color Ivory = {255, 255, 240, 255};
    constexpr Color Honeydew = {240, 255, 240, 255};
    constexpr Color MintCream = {245, 255, 250, 255};
    constexpr Color Azure = {240, 255, 255, 255};
    constexpr Color AliceBlue = {240, 248, 255, 255};
    constexpr Color GhostWhite = {248, 248, 255, 255};
    constexpr Color WhiteSmoke = {245, 245, 245, 255};
    constexpr Color Seashell = {255, 245, 238, 255};
    constexpr Color OldLace = {253, 245, 230, 255};
    constexpr Color FloralWhite = {255, 250, 240, 255};
    constexpr Color AntiqueWhite = {250, 235, 215, 255};
    constexpr Color Linen = {250, 240, 230, 255};
    constexpr Color LavenderBlush = {255, 240, 245, 255};
    constexpr Color MistyRose = {255, 228, 225, 255};
    
    constexpr Color Gray = {128, 128, 128, 255};
    constexpr Color DarkGray = {169, 169, 169, 255};
    constexpr Color Silver = {192, 192, 192, 255};
    constexpr Color LightGray = {211, 211, 211, 255};
    constexpr Color Gainsboro = {220, 220, 220, 255};
    constexpr Color DimGray = {105, 105, 105, 255};
    constexpr Color SlateGray = {112, 128, 144, 255};
    constexpr Color LightSlateGray = {119, 136, 153, 255};
    constexpr Color DarkSlateGray = {47, 79, 79, 255};
    
    constexpr Color Black = {0, 0, 0, 255};
}

#endif
