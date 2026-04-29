#include "RunnerGame.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

void RunnerGame::Reset()
{
    lane_ = 0;
    playerX_ = 0.0f;
    playerY_ = 0.0f;
    velY_ = 0.0f;
    speed_ = 16.0f;
    score_ = 0.0f;
    spawnZ_ = 80.0f;
    nextSpawnDist_ = 18.0f;
    animTime_ = 0.0f;
    failAnimTime_ = 0.0f;
    scrollDist_ = 0.0f;
    playing_ = false;
    gameOver_ = false;
    obstacles_.clear();
}

void RunnerGame::StartRun()
{
    if (playing_ && !gameOver_)
        return;
    Reset();
    playing_ = true;
    gameOver_ = false;
}

void RunnerGame::OnLaneLeft()
{
    if (!playing_ || gameOver_)
        return;
    lane_ = std::max(-1, lane_ - 1);
}

void RunnerGame::OnLaneRight()
{
    if (!playing_ || gameOver_)
        return;
    lane_ = std::min(1, lane_ + 1);
}

void RunnerGame::OnJump()
{
    if (!playing_ || gameOver_)
        return;
    if (playerY_ <= 0.01f)
        velY_ = kJumpVel;
}

void RunnerGame::SpawnObstacle()
{
    Obstacle o;
    o.lane = (std::rand() % 3) - 1;
    o.z = spawnZ_;
    o.tall = (std::rand() % 100) < 55;
    obstacles_.push_back(o);
}

void RunnerGame::CheckCollisions()
{
    constexpr float hitZ = 1.2f;

    for (const Obstacle& o : obstacles_)
    {
        if (o.lane != lane_)
            continue;
        if (std::fabs(o.z) > hitZ)
            continue;

        if (o.tall)
        {
            if (playerY_ < 0.72f)
            {
                gameOver_ = true;
                return;
            }
        }
        else
        {
            if (playerY_ < 0.2f && std::fabs(playerX_ - static_cast<float>(o.lane) * kLaneW) < kLaneW * 0.45f)
            {
                gameOver_ = true;
                return;
            }
        }
    }
}

void RunnerGame::Update(float dt)
{
    if (gameOver_)
    {
        failAnimTime_ += dt;
        return;
    }

    animTime_ += dt;

    if (!playing_)
        return;

    scrollDist_ += speed_ * dt;

    const float targetX = static_cast<float>(lane_) * kLaneW;
    playerX_ += (targetX - playerX_) * std::min(1.0f, dt * 11.0f);

    velY_ -= kGravity * dt;
    playerY_ += velY_ * dt;
    if (playerY_ < 0.0f)
    {
        playerY_ = 0.0f;
        velY_ = 0.0f;
    }

    speed_ += dt * 0.35f;
    score_ += speed_ * dt * 0.2f;

    for (Obstacle& o : obstacles_)
        o.z -= speed_ * dt;

    obstacles_.erase(std::remove_if(obstacles_.begin(),
                                    obstacles_.end(),
                                    [](const Obstacle& o) { return o.z < -12.0f; }),
                     obstacles_.end());

    nextSpawnDist_ -= speed_ * dt;
    if (nextSpawnDist_ <= 0.0f)
    {
        SpawnObstacle();
        nextSpawnDist_ = 7.0f + static_cast<float>(std::rand() % 16);
    }

    CheckCollisions();
}
