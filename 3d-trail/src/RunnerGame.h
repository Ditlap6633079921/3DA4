#pragma once

#include <vector>

struct Obstacle
{
    int lane = 0; // -1, 0, 1
    float z = 0.0f; // approaches 0 where player stands
    bool tall = true; // if true, must jump; if false, must switch lane (or jump)
};

class RunnerGame
{
public:
    void Reset();
    void Update(float dt);

    bool IsGameOver() const { return gameOver_; }
    bool IsPlaying() const { return playing_; }
    void StartRun();

    int Lane() const { return lane_; }
    float PlayerX() const { return playerX_; }
    float PlayerY() const { return playerY_; }
    float Speed() const { return speed_; }
    float Score() const { return score_; }
    float AnimTime() const { return animTime_; }
    float FailAnimTime() const { return failAnimTime_; }
    /// Visual-only distance for scrolling ground / parallax (meters along the run axis).
    float ScrollDistance() const { return scrollDist_; }

    void OnLaneLeft();
    void OnLaneRight();
    void OnJump();

    const std::vector<Obstacle>& Obstacles() const { return obstacles_; }

private:
    void SpawnObstacle();
    void CheckCollisions();

    int lane_ = 0;
    float playerX_ = 0.0f;
    float playerY_ = 0.0f;
    float velY_ = 0.0f;

    float speed_ = 14.0f;
    float score_ = 0.0f;
    float spawnZ_ = 80.0f;
    float nextSpawnDist_ = 0.0f;

    float animTime_ = 0.0f;
    float failAnimTime_ = 0.0f;
    float scrollDist_ = 0.0f;

    bool playing_ = false;
    bool gameOver_ = false;

    std::vector<Obstacle> obstacles_;

    static constexpr float kLaneW = 2.1f;
    static constexpr float kGravity = 38.0f;
    static constexpr float kJumpVel = 12.0f;
};
