#pragma once
#include "../mandala/mandala.h"
#include <vector>
#include <memory>

class MandalaDatabase {
public:
    MandalaDatabase();

    void loadMandala(int id);
    const std::vector<std::shared_ptr<Mandala>>& getAllMandala() const;
    std::shared_ptr<Mandala> getMandalaById(int id) const;

private:
    std::vector<std::shared_ptr<Mandala>> mandalaList;

    void createSampleMandala();
    void createHexagonMandala();
    void createSquaresMandala();
    void createRealMandala();
};
