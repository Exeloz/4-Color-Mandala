#include "dailyRuleset.h"

#include <algorithm>
#include <cctype>
#include <numeric>
#include <raylib.h>
#include <string>

namespace {
uint64_t mix64(uint64_t value) {
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31);
}

std::vector<std::string> makeCandidateAssetPaths(const std::string& relativePath) {
    std::vector<std::string> candidates;
    candidates.emplace_back(relativePath);
    candidates.emplace_back("resources/assets/" + relativePath);
    candidates.emplace_back("assets/" + relativePath);
    return candidates;
}

bool tryLoadTextFile(const std::vector<std::string>& candidatePaths, std::string& outputText) {
    for (const std::string& path : candidatePaths) {
        char* rawData = LoadFileText(path.c_str());
        if (rawData == nullptr) {
            continue;
        }

        outputText = rawData;
        UnloadFileText(rawData);
        return true;
    }

    return false;
}

std::vector<int> parseIntsFromLine(const std::string& line) {
    std::vector<int> values;
    int sign = 1;
    int value = 0;
    bool inNumber = false;

    for (char ch : line) {
        if (ch == '-') {
            if (inNumber) {
                values.push_back(sign * value);
            }
            sign = -1;
            value = 0;
            inNumber = false;
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(ch)) != 0) {
            value = inNumber ? (value * 10 + (ch - '0')) : (ch - '0');
            inNumber = true;
            continue;
        }

        if (inNumber) {
            values.push_back(sign * value);
            sign = 1;
            value = 0;
            inNumber = false;
        }
    }

    if (inNumber) {
        values.push_back(sign * value);
    }

    return values;
}

std::vector<std::vector<int>> parseSolutionBank(const std::string& content) {
    std::vector<std::vector<int>> solutions;

    size_t lineStart = 0;
    while (lineStart <= content.size()) {
        const size_t lineEnd = content.find('\n', lineStart);
        const std::string line = content.substr(lineStart,
                                                lineEnd == std::string::npos ? std::string::npos
                                                                             : lineEnd - lineStart);

        // Accept either raw arrays ([1,2,3]) or MiniZinc output lines (colors=[1,2,3]).
        if (line.find("colors=") != std::string::npos || line.find('[') != std::string::npos) {
            std::vector<int> values = parseIntsFromLine(line);
            if (!values.empty()) {
                solutions.push_back(std::move(values));
            }
        }

        if (lineEnd == std::string::npos) {
            break;
        }
        lineStart = lineEnd + 1;
    }

    return solutions;
}

std::vector<std::vector<int>> loadSolutions(int mandalaId, bool hardMode) {
    const std::string basePath = std::to_string(mandalaId) + "/";
    const std::vector<std::string> candidates = hardMode
        ? makeCandidateAssetPaths(basePath + "daily_solutions_hard.txt")
        : makeCandidateAssetPaths(basePath + "daily_solutions.txt");

    std::string content;
    if (!tryLoadTextFile(candidates, content)) {
        return {};
    }

    return parseSolutionBank(content);
}

uint64_t buildDailyRulesetSeed(const DailyRuleContext& context, uint64_t salt) {
    uint64_t seed = context.daySeed;
    seed ^= static_cast<uint64_t>(context.mandalaId) * 0x9e3779b185ebca87ULL;
    seed ^= context.hardMode ? 0xa0761d6478bd642fULL : 0xe7037ed1a0b428dbULL;
    seed ^= salt;
    return mix64(seed);
}
}

const std::vector<std::shared_ptr<DailyRuleset>>& getAllDailyRulesets() {
    static const std::vector<std::shared_ptr<DailyRuleset>> rulesets = {
        std::make_shared<SolutionFreezeDailyRuleset>(),
        std::make_shared<LastColorHintDailyRuleset>(),
    };
    return rulesets;
}

const DailyRuleset& getDailyRulesetById(int id) {
    for (const auto& ruleset : getAllDailyRulesets()) {
        if (ruleset->getId() == id) {
            return *ruleset;
        }
    }
    // Fallback to Default if the id is not registered.
    return *getAllDailyRulesets().front();
}

void SolutionFreezeDailyRuleset::applyToMandala(Mandala& mandala, const DailyRuleContext& context) const {
    mandala.clearLastColorHintData();

    std::vector<std::vector<int>> solutions = loadSolutions(context.mandalaId, context.hardMode);
    if (solutions.empty()) {
        return;
    }

    std::vector<int> colorableRegionIds;
    int maxRegionId = -1;
    colorableRegionIds.reserve(mandala.getRegions().size());
    for (const Region& region : mandala.getRegions()) {
        maxRegionId = std::max(maxRegionId, region.getId());
        if (region.isColorable()) {
            colorableRegionIds.push_back(region.getId());
        }
    }

    if (colorableRegionIds.empty()) {
        return;
    }

    std::sort(colorableRegionIds.begin(), colorableRegionIds.end());

    // Keep solutions that can be mapped to colorable regions either:
    // 1) by absolute region id (solution indexed by region id), or
    // 2) by compact colorable ordering (legacy compact banks).
    std::vector<const std::vector<int>*> compatibleSolutions;
    compatibleSolutions.reserve(solutions.size());
    for (const auto& solution : solutions) {
        const bool hasRegionIdIndexing = maxRegionId >= 0 && solution.size() > static_cast<size_t>(maxRegionId);
        const bool hasCompactColorableIndexing = solution.size() == colorableRegionIds.size();
        if (hasRegionIdIndexing || hasCompactColorableIndexing) {
            compatibleSolutions.push_back(&solution);
        }
    }

    if (compatibleSolutions.empty()) {
        return;
    }

    // Dedicated deterministic hash for solution pick.
    const uint64_t solutionSeed = buildDailyRulesetSeed(context, hashContribution() ^ 0x2d358dccaa6c78a5ULL);

    const size_t solutionIndex = static_cast<size_t>(solutionSeed % static_cast<uint64_t>(compatibleSolutions.size()));
    const std::vector<int>& pickedSolution = *compatibleSolutions[solutionIndex];

    // Dedicated deterministic hash for which regions get frozen.
    const uint64_t freezeSeed = buildDailyRulesetSeed(context, hashContribution() ^ 0x8bb84b93962eacc9ULL);

    std::vector<size_t> order(colorableRegionIds.size());
    std::iota(order.begin(), order.end(), static_cast<size_t>(0));
    std::sort(order.begin(), order.end(), [&](size_t a, size_t b) {
        const uint64_t scoreA = mix64(freezeSeed ^ static_cast<uint64_t>(a) * 0xbf58476d1ce4e5b9ULL);
        const uint64_t scoreB = mix64(freezeSeed ^ static_cast<uint64_t>(b) * 0xbf58476d1ce4e5b9ULL);
        return scoreA > scoreB;
    });

    const int frozenCount = std::min(defaultFrozenRegionCount(), static_cast<int>(order.size()));
    for (int i = 0; i < frozenCount; ++i) {
        const size_t idx = order[static_cast<size_t>(i)];
        const int regionId = colorableRegionIds[idx];
        int colorIndex = -1;
        if (pickedSolution.size() > static_cast<size_t>(regionId)) {
            colorIndex = pickedSolution[static_cast<size_t>(regionId)];
        } else if (pickedSolution.size() == colorableRegionIds.size()) {
            colorIndex = pickedSolution[idx];
        }

        Region* region = mandala.getRegionById(regionId);
        if (region == nullptr) {
            continue;
        }

        if (colorIndex <= 0) {
            continue;
        }

        region->setColor(colorIndex);
        region->setColorable(false);
    }
}

void LastColorHintDailyRuleset::applyToMandala(Mandala& mandala, const DailyRuleContext& context) const {
    mandala.clearLastColorHintData();

    std::vector<std::vector<int>> solutions = loadSolutions(context.mandalaId, context.hardMode);
    if (solutions.empty()) {
        return;
    }

    std::vector<int> colorableRegionIds;
    int maxRegionId = -1;
    colorableRegionIds.reserve(mandala.getRegions().size());
    for (const Region& region : mandala.getRegions()) {
        maxRegionId = std::max(maxRegionId, region.getId());
        if (region.isColorable()) {
            colorableRegionIds.push_back(region.getId());
        }
    }

    if (colorableRegionIds.empty() || maxRegionId < 0) {
        return;
    }

    std::sort(colorableRegionIds.begin(), colorableRegionIds.end());

    std::vector<const std::vector<int>*> compatibleSolutions;
    compatibleSolutions.reserve(solutions.size());
    for (const auto& solution : solutions) {
        const bool hasRegionIdIndexing = solution.size() > static_cast<size_t>(maxRegionId);
        const bool hasCompactColorableIndexing = solution.size() == colorableRegionIds.size();
        if (hasRegionIdIndexing || hasCompactColorableIndexing) {
            compatibleSolutions.push_back(&solution);
        }
    }

    if (compatibleSolutions.empty()) {
        return;
    }

    const uint64_t solutionSeed = buildDailyRulesetSeed(context, hashContribution() ^ 0xf6c8e9a36a6cf1d5ULL);
    const size_t solutionIndex = static_cast<size_t>(solutionSeed % static_cast<uint64_t>(compatibleSolutions.size()));
    const std::vector<int>& pickedSolution = *compatibleSolutions[solutionIndex];

    std::vector<int> solutionByRegionId(static_cast<size_t>(maxRegionId) + 1, -1);
    if (pickedSolution.size() > static_cast<size_t>(maxRegionId)) {
        for (int regionId : colorableRegionIds) {
            solutionByRegionId[static_cast<size_t>(regionId)] = pickedSolution[static_cast<size_t>(regionId)];
        }
    } else if (pickedSolution.size() == colorableRegionIds.size()) {
        for (size_t i = 0; i < colorableRegionIds.size(); ++i) {
            solutionByRegionId[static_cast<size_t>(colorableRegionIds[i])] = pickedSolution[i];
        }
    } else {
        return;
    }

    int targetColor = -1;
    for (int regionId : colorableRegionIds) {
        const int color = solutionByRegionId[static_cast<size_t>(regionId)];
        if (color > targetColor) {
            targetColor = color;
        }
    }

    if (targetColor <= 0) {
        return;
    }

    std::vector<int> initialCountByRegionId(solutionByRegionId.size(), -1);
    const AdjacencyGraph& graph = mandala.getAdjacencyGraph();
    for (const Region& region : mandala.getRegions()) {
        if (!region.isColorable()) {
            continue;
        }

        const int regionId = region.getId();
        if (regionId < 0 || static_cast<size_t>(regionId) >= initialCountByRegionId.size()) {
            continue;
        }

        int initialCount = 0;
        const std::set<int>& adjacent = graph.getAdjacentRegions(regionId);
        for (int adjacentId : adjacent) {
            if (adjacentId < 0 || static_cast<size_t>(adjacentId) >= solutionByRegionId.size()) {
                continue;
            }
            if (solutionByRegionId[static_cast<size_t>(adjacentId)] == targetColor) {
                initialCount++;
            }
        }

        initialCountByRegionId[static_cast<size_t>(regionId)] = initialCount;
    }

    mandala.setLastColorHintData(targetColor, solutionByRegionId, initialCountByRegionId);
}
