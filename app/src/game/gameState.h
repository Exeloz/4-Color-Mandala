#pragma once

class GameState {
public:
    virtual ~GameState() = default;

    virtual void update(float deltaTime) = 0;
    virtual void draw() = 0;
};
