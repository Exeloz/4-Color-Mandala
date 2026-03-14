#include "mandalaDatabase.h"

#include <algorithm>

MandalaDatabase::MandalaDatabase()
    : hardModeEnabled(false) {
    loadManifest();
}

void MandalaDatabase::loadMandala(int id, bool hardMode) {
    auto loadedIterator = std::find_if(
        loadedMandalas.begin(),
        loadedMandalas.end(),
        [id, hardMode](const LoadedMandalaEntry& entry) {
            return entry.id == id && entry.hardMode == hardMode;
        }
    );
    if (loadedIterator != loadedMandalas.end()) {
        return;
    }

    const auto descriptorIterator = std::find_if(
        mandalaDescriptors.begin(),
        mandalaDescriptors.end(),
        [id](const MandalaAssetDescriptor& descriptor) {
            return descriptor.id == id;
        }
    );
    if (descriptorIterator == mandalaDescriptors.end()) {
        return;
    }

    const bool effectiveHardMode = hardMode && !descriptorIterator->hardAdjacencyPath.empty();
    loadedIterator = std::find_if(
        loadedMandalas.begin(),
        loadedMandalas.end(),
        [id, effectiveHardMode](const LoadedMandalaEntry& entry) {
            return entry.id == id && entry.hardMode == effectiveHardMode;
        }
    );
    if (loadedIterator != loadedMandalas.end()) {
        return;
    }

    if (!loadMandalaFromAssets(*descriptorIterator, effectiveHardMode)) {
        return;
    }
}

std::vector<std::shared_ptr<Mandala>> MandalaDatabase::getAllMandala() const {
    MandalaDatabase* mutableThis = const_cast<MandalaDatabase*>(this);
    std::vector<std::shared_ptr<Mandala>> result;
    result.reserve(mandalaDescriptors.size());

    for (const MandalaAssetDescriptor& descriptor : mandalaDescriptors) {
        mutableThis->loadMandala(descriptor.id, false);
    }

    for (const LoadedMandalaEntry& entry : loadedMandalas) {
        if (!entry.hardMode && entry.mandala != nullptr) {
            result.push_back(entry.mandala);
        }
    }

    return result;
}

std::shared_ptr<Mandala> MandalaDatabase::getMandalaById(int id, bool hardMode) const {
    MandalaDatabase* mutableThis = const_cast<MandalaDatabase*>(this);
    mutableThis->loadMandala(id, hardMode);

    bool effectiveHardMode = hardMode;
    auto descriptorIterator = std::find_if(
        mandalaDescriptors.begin(),
        mandalaDescriptors.end(),
        [id](const MandalaAssetDescriptor& descriptor) {
            return descriptor.id == id;
        }
    );
    if (descriptorIterator != mandalaDescriptors.end()) {
        effectiveHardMode = hardMode && !descriptorIterator->hardAdjacencyPath.empty();
    }

    auto loadedIterator = std::find_if(
        loadedMandalas.begin(),
        loadedMandalas.end(),
        [id, effectiveHardMode](const LoadedMandalaEntry& entry) {
            return entry.id == id && entry.hardMode == effectiveHardMode;
        }
    );
    if (loadedIterator != loadedMandalas.end()) {
        return loadedIterator->mandala;
    }

    return nullptr;
}

bool MandalaDatabase::isHardModeEnabled() const {
    return hardModeEnabled;
}

const std::vector<MandalaDatabase::MandalaListItem>& MandalaDatabase::getMandalaListItems() const {
    return mandalaListItems;
}

bool MandalaDatabase::hasMandala(int id, bool hardMode) const {
    auto descriptorIterator = std::find_if(
        mandalaDescriptors.begin(),
        mandalaDescriptors.end(),
        [id](const MandalaAssetDescriptor& descriptor) {
            return descriptor.id == id;
        }
    );
    if (descriptorIterator == mandalaDescriptors.end()) {
        return false;
    }

    if (!hardMode) {
        return true;
    }

    return !descriptorIterator->hardAdjacencyPath.empty();
}
