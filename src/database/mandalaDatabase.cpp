#include "mandalaDatabase.h"
#include <cmath>

MandalaDatabase::MandalaDatabase() {
    createSampleMandala();
}

void MandalaDatabase::createSampleMandala() {
    std::vector<Region> regions;
    AdjacencyGraph adjacencyGraph(6);

    Vector2 center = {400, 300};
    float radius = 100;

    for (int i = 0; i < 6; i++) {
        float angle1 = (i * 60.0f) * 3.14159f / 180.0f;
        float angle2 = ((i + 1) * 60.0f) * 3.14159f / 180.0f;

        std::vector<Vector2> vertices = {
            {center.x + radius * static_cast<float>(cos(angle1)), center.y + radius * static_cast<float>(sin(angle1))},
            {center.x + radius * 0.5f * static_cast<float>(cos(angle1)), center.y + radius * 0.5f * static_cast<float>(sin(angle1))},
            {center.x, center.y},
            {center.x + radius * 0.5f * static_cast<float>(cos(angle2)), center.y + radius * 0.5f * static_cast<float>(sin(angle2))},
            {center.x + radius * static_cast<float>(cos(angle2)), center.y + radius * static_cast<float>(sin(angle2))},
        };

        regions.emplace_back(i, vertices);
    }

    for (int i = 0; i < 6; i++) {
        adjacencyGraph.addAdjacency(i, (i + 1) % 6);
    }

    auto mandala = std::make_shared<Mandala>(1, "Basic Hexagon", regions, adjacencyGraph);
    mandalaList.push_back(mandala);

    std::vector<Region> regions2;
    AdjacencyGraph adjacencyGraph2(4);
    Vector2 center2 = {400, 300};
    float size = 80;

    regions2.emplace_back(0, std::vector<Vector2>{
        {center2.x - size, center2.y - size},
        {center2.x, center2.y - size},
        {center2.x, center2.y},
        {center2.x - size, center2.y}
    });

    regions2.emplace_back(1, std::vector<Vector2>{
        {center2.x, center2.y - size},
        {center2.x + size, center2.y - size},
        {center2.x + size, center2.y},
        {center2.x, center2.y}
    });

    regions2.emplace_back(2, std::vector<Vector2>{
        {center2.x, center2.y},
        {center2.x + size, center2.y},
        {center2.x + size, center2.y + size},
        {center2.x, center2.y + size}
    });

    regions2.emplace_back(3, std::vector<Vector2>{
        {center2.x - size, center2.y},
        {center2.x, center2.y},
        {center2.x, center2.y + size},
        {center2.x - size, center2.y + size}
    });

    adjacencyGraph2.addAdjacency(0, 1);
    adjacencyGraph2.addAdjacency(1, 2);
    adjacencyGraph2.addAdjacency(2, 3);
    adjacencyGraph2.addAdjacency(3, 0);

    auto mandala2 = std::make_shared<Mandala>(2, "Four Squares", regions2, adjacencyGraph2);
    mandalaList.push_back(mandala2);
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
