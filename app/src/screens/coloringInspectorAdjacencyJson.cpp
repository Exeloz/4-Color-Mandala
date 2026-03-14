#include "coloringInspector.h"

#include <fstream>
#include <regex>
#include <set>
#include <sstream>

bool ColoringInspector::applyAdjacencyJsonEdit(const Mandala& mandala,
                                               bool shouldExist,
                                               int regionA,
                                               int regionB) const {
    const std::string& adjacencyPath = mandala.getAdjacencySourcePath();
    if (adjacencyPath.empty()) {
        return false;
    }

    std::string content;
    if (!readFileText(adjacencyPath, content)) {
        return false;
    }

    std::set<std::pair<int, int>> pairs;
    const std::regex pairRegex(R"(\[\s*(-?\d+)\s*,\s*(-?\d+)\s*\])");
    for (std::sregex_iterator it(content.begin(), content.end(), pairRegex), end; it != end; ++it) {
        const int first = std::stoi((*it)[1].str());
        const int second = std::stoi((*it)[2].str());
        if (first == second) {
            continue;
        }
        pairs.emplace(std::min(first, second), std::max(first, second));
    }

    const std::pair<int, int> targetPair{std::min(regionA, regionB), std::max(regionA, regionB)};
    if (shouldExist) {
        pairs.insert(targetPair);
    } else {
        pairs.erase(targetPair);
    }

    std::ostringstream output;
    output << "{\n";
    output << "  \"mandala_id\": " << mandala.getId() << ",\n";
    output << "  \"pairs\": [\n";

    bool first = true;
    for (const auto& pair : pairs) {
        if (!first) {
            output << ",\n";
        }
        output << "    [" << pair.first << ", " << pair.second << "]";
        first = false;
    }

    output << "\n  ]\n";
    output << "}\n";

    return writeFileText(adjacencyPath, output.str());
}

bool ColoringInspector::applyAdjacencyJsonRemoveAllForRegion(const Mandala& mandala, int regionId) const {
    const std::string& adjacencyPath = mandala.getAdjacencySourcePath();
    if (adjacencyPath.empty()) {
        return false;
    }

    std::string content;
    if (!readFileText(adjacencyPath, content)) {
        return false;
    }

    std::set<std::pair<int, int>> pairs;
    const std::regex pairRegex(R"(\[\s*(-?\d+)\s*,\s*(-?\d+)\s*\])");
    for (std::sregex_iterator it(content.begin(), content.end(), pairRegex), end; it != end; ++it) {
        const int first = std::stoi((*it)[1].str());
        const int second = std::stoi((*it)[2].str());
        if (first == second) {
            continue;
        }
        if (first == regionId || second == regionId) {
            continue;
        }
        pairs.emplace(std::min(first, second), std::max(first, second));
    }

    std::ostringstream output;
    output << "{\n";
    output << "  \"mandala_id\": " << mandala.getId() << ",\n";
    output << "  \"pairs\": [\n";

    bool first = true;
    for (const auto& pair : pairs) {
        if (!first) {
            output << ",\n";
        }
        output << "    [" << pair.first << ", " << pair.second << "]";
        first = false;
    }

    output << "\n  ]\n";
    output << "}\n";

    return writeFileText(adjacencyPath, output.str());
}

bool ColoringInspector::readFileText(const std::string& path, std::string& content) {
    std::ifstream stream(path);
    if (!stream.is_open()) {
        return false;
    }

    std::ostringstream buffer;
    buffer << stream.rdbuf();
    content = buffer.str();
    return true;
}

bool ColoringInspector::writeFileText(const std::string& path, const std::string& content) {
    std::ofstream stream(path, std::ios::trunc);
    if (!stream.is_open()) {
        return false;
    }

    stream << content;
    return stream.good();
}
