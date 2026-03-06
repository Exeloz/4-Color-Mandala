#include "test_framework.h"
#include "test_helpers.h"
#include "../app/src/screens/coloringActionManager.h"

TEST_CASE(coloring_action_manager_applies_and_undoes_changes) {
    Mandala mandala = makeTwoRegionMandala();
    ColoringActionManager manager;

    EXPECT_TRUE(manager.applyColorChange(mandala, 0, 2));
    EXPECT_EQ(mandala.getRegionById(0)->getColor(), 2);
    EXPECT_TRUE(manager.canUndo());

    EXPECT_TRUE(manager.undoLast(mandala));
    EXPECT_EQ(mandala.getRegionById(0)->getColor(), -1);
    EXPECT_FALSE(manager.canUndo());
}

TEST_CASE(coloring_action_manager_rejects_noop_and_invalid_updates) {
    Mandala mandala = makeTwoRegionMandala();
    ColoringActionManager manager;

    EXPECT_FALSE(manager.applyColorChange(mandala, 999, 1));

    mandala.getRegionById(0)->setColor(3);
    EXPECT_FALSE(manager.applyColorChange(mandala, 0, 3));

    mandala.getRegionById(1)->setColorable(false);
    EXPECT_FALSE(manager.applyColorChange(mandala, 1, 2));
    EXPECT_FALSE(manager.canUndo());
}

TEST_CASE(coloring_action_manager_clear_resets_history) {
    Mandala mandala = makeTwoRegionMandala();
    ColoringActionManager manager;

    EXPECT_TRUE(manager.applyColorChange(mandala, 0, 1));
    EXPECT_TRUE(manager.canUndo());

    manager.clear();
    EXPECT_FALSE(manager.canUndo());
    EXPECT_FALSE(manager.undoLast(mandala));
}

TEST_CASE(coloring_action_manager_treats_none_as_uncolored) {
    Mandala mandala = makeTwoRegionMandala();
    ColoringActionManager manager;

    EXPECT_FALSE(manager.applyColorChange(mandala, 0, 0));
    EXPECT_EQ(mandala.getRegionById(0)->getColor(), -1);

    EXPECT_TRUE(manager.applyColorChange(mandala, 0, 2));
    EXPECT_TRUE(manager.applyColorChange(mandala, 0, 0));
    EXPECT_EQ(mandala.getRegionById(0)->getColor(), -1);
}
