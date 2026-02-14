#include "mandalaDatabase.h"
#include <cmath>

namespace {
    constexpr float SCREEN_CENTER_X = 400.0f;
    constexpr float SCREEN_CENTER_Y = 300.0f;
    constexpr float DEGREES_TO_RADIANS = 3.14159f / 180.0f;
    
    constexpr float HEXAGON_RADIUS = 100.0f;
    constexpr float HEXAGON_INNER_RATIO = 0.5f;
    constexpr int HEXAGON_SEGMENTS = 6;
    constexpr float HEXAGON_DEGREES_PER_SEGMENT = 360.0f / HEXAGON_SEGMENTS;
    
    constexpr float SQUARE_SIZE = 80.0f;
    constexpr int SQUARE_GRID_DIMENSION = 2;
}

MandalaDatabase::MandalaDatabase() {
    createSampleMandala();
}

void MandalaDatabase::createSampleMandala() {
    createHexagonMandala();
    createSquaresMandala();
    createRealMandala();
}

void MandalaDatabase::loadMandala(int id) {
}

const std::vector<std::shared_ptr<Mandala>>& MandalaDatabase::getAllMandala() const {
    return mandalaList;
}

std::shared_ptr<Mandala> MandalaDatabase::getMandalaById(int id) const {
    for (const auto& mandala : mandalaList) {
        if (mandala->getId() == id) {
            return mandala;
        }
    }
    return nullptr;
}

void MandalaDatabase::createHexagonMandala() {
    std::vector<Region> regions;
    AdjacencyGraph adjacencyGraph(HEXAGON_SEGMENTS);
    
    Vector2 center = {SCREEN_CENTER_X, SCREEN_CENTER_Y};

    for (int i = 0; i < HEXAGON_SEGMENTS; i++) {
        float angle1 = (i * HEXAGON_DEGREES_PER_SEGMENT) * DEGREES_TO_RADIANS;
        float angle2 = ((i + 1) * HEXAGON_DEGREES_PER_SEGMENT) * DEGREES_TO_RADIANS;

        std::vector<Vector2> vertices = {
            {center.x + HEXAGON_RADIUS * std::cos(angle1), center.y + HEXAGON_RADIUS * std::sin(angle1)},
            {center.x + HEXAGON_RADIUS * HEXAGON_INNER_RATIO * std::cos(angle1), center.y + HEXAGON_RADIUS * HEXAGON_INNER_RATIO * std::sin(angle1)},
            {center.x, center.y},
            {center.x + HEXAGON_RADIUS * HEXAGON_INNER_RATIO * std::cos(angle2), center.y + HEXAGON_RADIUS * HEXAGON_INNER_RATIO * std::sin(angle2)},
            {center.x + HEXAGON_RADIUS * std::cos(angle2), center.y + HEXAGON_RADIUS * std::sin(angle2)},
        };

        regions.emplace_back(i, vertices);
    }

    for (int i = 0; i < HEXAGON_SEGMENTS; i++) {
        adjacencyGraph.addAdjacency(i, (i + 1) % HEXAGON_SEGMENTS);
    }

    auto mandala = std::make_shared<Mandala>(1, "Basic Hexagon", regions, adjacencyGraph);
    mandalaList.push_back(mandala);
}

void MandalaDatabase::createSquaresMandala() {
    std::vector<Region> regions;
    AdjacencyGraph adjacencyGraph(SQUARE_GRID_DIMENSION * SQUARE_GRID_DIMENSION);
    
    Vector2 center = {SCREEN_CENTER_X, SCREEN_CENTER_Y};

    regions.emplace_back(0, std::vector<Vector2>{
        {center.x - SQUARE_SIZE, center.y - SQUARE_SIZE},
        {center.x - SQUARE_SIZE, center.y},
        {center.x, center.y},
        {center.x, center.y - SQUARE_SIZE}
    });

    regions.emplace_back(1, std::vector<Vector2>{
        {center.x, center.y - SQUARE_SIZE},
        {center.x, center.y},
        {center.x + SQUARE_SIZE, center.y},
        {center.x + SQUARE_SIZE, center.y - SQUARE_SIZE}
    });

    regions.emplace_back(2, std::vector<Vector2>{
        {center.x, center.y},
        {center.x, center.y + SQUARE_SIZE},
        {center.x + SQUARE_SIZE, center.y + SQUARE_SIZE},
        {center.x + SQUARE_SIZE, center.y}
    });

    regions.emplace_back(3, std::vector<Vector2>{
        {center.x - SQUARE_SIZE, center.y},
        {center.x - SQUARE_SIZE, center.y + SQUARE_SIZE},
        {center.x, center.y + SQUARE_SIZE},
        {center.x, center.y}
    });

    adjacencyGraph.addAdjacency(0, 1);
    adjacencyGraph.addAdjacency(1, 2);
    adjacencyGraph.addAdjacency(2, 3);
    adjacencyGraph.addAdjacency(3, 0);

    auto mandala = std::make_shared<Mandala>(2, "Four Squares", regions, adjacencyGraph);
    mandalaList.push_back(mandala);
}
