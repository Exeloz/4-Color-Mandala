#include "test_framework.h"
#include "../app/src/game/progressPersistence.h"
#include "../app/src/ui/colors.h"

#include <cstdio>
#include <fstream>

namespace {
constexpr const char* kSaveFilePath = "mandala_progress.dat";
constexpr const char* kBackupFilePath = "mandala_progress.dat.test_backup";

bool fileExists(const char* path) {
    std::ifstream stream(path);
    return stream.good();
}

struct SaveFileScope {
    bool hadOriginal = false;

    SaveFileScope() {
        std::remove(kBackupFilePath);
        hadOriginal = fileExists(kSaveFilePath);
        if (hadOriginal) {
            std::rename(kSaveFilePath, kBackupFilePath);
        }
        std::remove(kSaveFilePath);
    }

    ~SaveFileScope() {
        std::remove(kSaveFilePath);
        if (hadOriginal) {
            std::rename(kBackupFilePath, kSaveFilePath);
        } else {
            std::remove(kBackupFilePath);
        }
    }
};

std::shared_ptr<Mandala> makePersistenceMandala(int id) {
    std::vector<Region> regions;
    regions.emplace_back(0, std::vector<Vector2>{{0.0f, 0.0f}, {8.0f, 0.0f}, {0.0f, 8.0f}});
    regions.emplace_back(1, std::vector<Vector2>{{20.0f, 0.0f}, {28.0f, 0.0f}, {20.0f, 8.0f}});

    AdjacencyGraph adjacency(2);
    return std::make_shared<Mandala>(id, "Persisted", regions, adjacency);
}
}

TEST_CASE(progress_persistence_load_returns_false_when_missing) {
    SaveFileScope saveFileScope;

    ProgressPersistence persistence;
    EXPECT_FALSE(persistence.load());
}

TEST_CASE(progress_persistence_palette_roundtrip) {
    SaveFileScope saveFileScope;

    ProgressPersistence writer;
    std::vector<Color> palette = {
        Colors::None,
        Colors::DarkBlue,
        Colors::Gold,
        Colors::PaleTurquoise,
    };
    writer.setPalette(palette);
    EXPECT_TRUE(writer.save());

    ProgressPersistence reader;
    EXPECT_TRUE(reader.load());
    EXPECT_TRUE(reader.hasPalette());
    EXPECT_EQ(reader.getPalette().size(), palette.size());
    EXPECT_EQ(reader.getPalette()[2].r, palette[2].r);
    EXPECT_EQ(reader.getPalette()[2].g, palette[2].g);
    EXPECT_EQ(reader.getPalette()[2].b, palette[2].b);
    EXPECT_EQ(reader.getPalette()[2].a, palette[2].a);
}

TEST_CASE(progress_persistence_region_colors_roundtrip) {
    SaveFileScope saveFileScope;

    auto sourceMandala = makePersistenceMandala(321);
    sourceMandala->getRegionById(0)->setColor(1);
    sourceMandala->getRegionById(1)->setColor(3);

    ProgressPersistence writer;
    writer.captureMandalaState(*sourceMandala);
    EXPECT_TRUE(writer.save());

    auto targetMandala = makePersistenceMandala(321);
    EXPECT_EQ(targetMandala->getRegionById(0)->getColor(), -1);
    EXPECT_EQ(targetMandala->getRegionById(1)->getColor(), -1);

    ProgressPersistence reader;
    EXPECT_TRUE(reader.load());
    reader.applyToMandalas({targetMandala});

    EXPECT_EQ(targetMandala->getRegionById(0)->getColor(), 1);
    EXPECT_EQ(targetMandala->getRegionById(1)->getColor(), 3);
}

TEST_CASE(progress_persistence_completed_flag_stays_set_after_subsequent_saves) {
    SaveFileScope saveFileScope;

    auto mandala = makePersistenceMandala(777);
    mandala->getRegionById(0)->setColor(1);
    mandala->getRegionById(1)->setColor(2);

    ProgressPersistence writer;
    writer.captureMandalaState(*mandala);

    mandala->getRegionById(1)->setColor(-1);
    writer.captureMandalaState(*mandala);
    EXPECT_TRUE(writer.save());

    ProgressPersistence reader;
    EXPECT_TRUE(reader.load());
    EXPECT_TRUE(reader.isMandalaCompleted(777));

    std::unordered_set<int> completed = reader.getCompletedMandalaIds();
    EXPECT_TRUE(completed.count(777) > 0);
}

TEST_CASE(progress_persistence_clear_mandala_state_removes_saved_colors_and_completion) {
    SaveFileScope saveFileScope;

    auto mandala = makePersistenceMandala(909);
    mandala->getRegionById(0)->setColor(1);
    mandala->getRegionById(1)->setColor(2);

    ProgressPersistence writer;
    writer.captureMandalaState(*mandala);
    writer.clearMandalaState(909);
    EXPECT_TRUE(writer.save());

    auto target = makePersistenceMandala(909);
    ProgressPersistence reader;
    EXPECT_TRUE(reader.load());
    reader.applyToMandalas({target});

    EXPECT_EQ(target->getRegionById(0)->getColor(), -1);
    EXPECT_EQ(target->getRegionById(1)->getColor(), -1);
    EXPECT_FALSE(reader.isMandalaCompleted(909));
}