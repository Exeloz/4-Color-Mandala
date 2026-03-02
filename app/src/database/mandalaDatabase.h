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
    };

    MandalaDatabase();

    void loadMandala(int id);
    const std::vector<MandalaListItem>& getMandalaListItems() const;
    const std::vector<std::shared_ptr<Mandala>>& getAllMandala() const;
    std::shared_ptr<Mandala> getMandalaById(int id) const;

private:
    struct MandalaAssetDescriptor {
        int id;
        std::string name;
        std::string regionsPath;
        std::string adjacencyPath;
    };

    std::vector<std::shared_ptr<Mandala>> mandalaList;
    std::vector<MandalaListItem> mandalaListItems;
    std::vector<MandalaAssetDescriptor> mandalaDescriptors;

    void createSampleMandala();
    void createHexagonMandala();

    bool loadManifest();
    bool loadMandalaFromAssets(const MandalaAssetDescriptor& descriptor);
    bool hasMandala(int id) const;
};
