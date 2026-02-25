#include "test_framework.h"
#include "../app/src/ui/button.h"

TEST_CASE(button_bounds_update_with_position_and_size) {
    Button button(10.0f, 20.0f, 100.0f, 40.0f, "Play");

    Rectangle initial = button.getBounds();
    EXPECT_NEAR(initial.x, 10.0f, 1e-4f);
    EXPECT_NEAR(initial.y, 20.0f, 1e-4f);
    EXPECT_NEAR(initial.width, 100.0f, 1e-4f);
    EXPECT_NEAR(initial.height, 40.0f, 1e-4f);

    button.setPosition(50.0f, 60.0f);
    button.setSize(120.0f, 55.0f);

    Rectangle updated = button.getBounds();
    EXPECT_NEAR(updated.x, 50.0f, 1e-4f);
    EXPECT_NEAR(updated.y, 60.0f, 1e-4f);
    EXPECT_NEAR(updated.width, 120.0f, 1e-4f);
    EXPECT_NEAR(updated.height, 55.0f, 1e-4f);
}

TEST_CASE(button_is_not_clicked_by_default) {
    Button button(0.0f, 0.0f, 80.0f, 30.0f, "X");
    EXPECT_FALSE(button.isClicked());
}

TEST_CASE(button_text_scale_accepts_small_values_without_crash) {
    Button button(0.0f, 0.0f, 80.0f, 30.0f, "Scale");
    button.setTextScale(0.1f);
    button.setTextScale(1.5f);
    EXPECT_TRUE(true);
}
