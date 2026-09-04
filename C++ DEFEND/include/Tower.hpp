#pragma once
#include "Common.hpp"
#include "Enemy.hpp"
#include <vector>

class Tower {
protected:
    Vector2 position;
    float range;
    float fireRate;
    float timer;
    int cost;
    int level;

public:
    Tower(Vector2 pos, float r, float rate, int c) 
        : position(pos), range(r), fireRate(rate), timer(0.0f), cost(c), level(1) {}
    virtual ~Tower() {}

    virtual void Update(float dt, std::vector<Enemy*>& enemies) = 0;
    virtual void Draw() = 0;                                        
    virtual void Upgrade() { level++; range *= 1.15f; }
    
    Vector2 GetPosition() const { return position; }
    int GetCost() const { return cost; }
};

class ArcherTower : public Tower {
public:
    ArcherTower(Vector2 pos) : Tower(pos, 120.0f, 0.8f, 100) {}
    void Update(float dt, std::vector<Enemy*>& enemies) override;
    void Draw() override;
};