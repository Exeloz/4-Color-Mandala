#pragma once
#include "../mandala/mandala.h"
#include <vector>
#include <memory>
#include <string>

class MandalaDatabase {
public:
    struct MandalaListItem {
        int id;
        std::string name;
        bool hasHardMode;
    };

    MandalaDatabase();

    void loadMandala(int id, bool hardMode = false);
    const std::vector<MandalaListItem>& getMandalaListItems() const;
    std::vector<std::shared_ptr<Mandala>> getAllMandala() const;
    std::shared_ptr<Mandala> getMandalaById(int id, bool hardMode = false) const;
    bool isHardModeEnabled() const;

private:
    struct MandalaAssetDescriptor {
        int id;
        std::string name;
        std::string regionsPath;
        std::string adjacencyPath;
        std::string hardAdjacencyPath;
    };

    struct LoadedMandalaEntry {
        int id;
        bool hardMode;
        std::shared_ptr<Mandala> mandala;
    };

    std::vector<LoadedMandalaEntry> loadedMandalas;
    std::vector<MandalaListItem> mandalaListItems;
    std::vector<MandalaAssetDescriptor> mandalaDescriptors;
    bool hardModeEnabled;

    void createSampleMandala();
    void createHexagonMandala();

    bool loadManifest();
    bool loadMandalaFromAssets(const MandalaAssetDescriptor& descriptor, bool hardMode);
    bool hasMandala(int id, bool hardMode) const;
};
