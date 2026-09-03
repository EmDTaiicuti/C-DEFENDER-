#include "raylib.h"
#include "raymath.h"
#include <algorithm>
#include <functional>
#include <memory>
#include <vector>

constexpr int WIDTH = 1200;
constexpr int HEIGHT = 760;
constexpr int MAP_X = 250;
constexpr int MAP_Y = 90;
constexpr int MAP_W = 915;
constexpr int MAP_H = 610;
constexpr int TOWER_COST = 80;

enum class EnemyState { Alive, Dead, ReachedEnd };
enum class EffectKind { Single, Area, Slow };
class GameManager;

struct Enemy {
    int id;
    Vector2 position;
    float hp;
    float maxHp;
    float speed;
    int waypoint = 1;
    float slowTimer = 0.0f;
    EnemyState state = EnemyState::Alive;

    Enemy(int enemyId, Vector2 start, float health, float moveSpeed)
        : id(enemyId), position(start), hp(health), maxHp(health), speed(moveSpeed) {}

    void update(float dt, const std::vector<Vector2>& path) {
        if (state != EnemyState::Alive) return;
        slowTimer = std::max(0.0f, slowTimer - dt);
        if (waypoint >= static_cast<int>(path.size())) { state = EnemyState::ReachedEnd; return; }
        Vector2 delta = Vector2Subtract(path[waypoint], position);
        float distance = Vector2Length(delta);
        float step = speed * (slowTimer > 0.0f ? 0.45f : 1.0f) * dt;
        if (distance <= step) {
            position = path[waypoint++];
            if (waypoint >= static_cast<int>(path.size())) state = EnemyState::ReachedEnd;
        } else position = Vector2Add(position, Vector2Scale(Vector2Normalize(delta), step));
    }

    void takeDamage(float amount) {
        if (state != EnemyState::Alive) return;
        hp -= amount;
        if (hp <= 0.0f) { hp = 0.0f; state = EnemyState::Dead; }
    }
};

using TargetStrategy = std::function<int(Vector2, const std::vector<std::unique_ptr<Enemy>>&)>;

class IDamageEffect {
public:
    virtual ~IDamageEffect() = default;
    virtual void apply(GameManager&, Enemy&, float) const = 0;
    virtual EffectKind kind() const = 0;
};

class SingleDamage final : public IDamageEffect {
public:
    void apply(GameManager&, Enemy& enemy, float damage) const override { enemy.takeDamage(damage); }
    EffectKind kind() const override { return EffectKind::Single; }
};

class AreaDamage final : public IDamageEffect {
    float radius;
    float falloff;
public:
    AreaDamage(float areaRadius, float areaFalloff) : radius(areaRadius), falloff(areaFalloff) {}
    void apply(GameManager&, Enemy&, float) const override;
    EffectKind kind() const override { return EffectKind::Area; }
};

class SlowDamage final : public IDamageEffect {
    float duration;
public:
    explicit SlowDamage(float seconds) : duration(seconds) {}
    void apply(GameManager&, Enemy& enemy, float damage) const override {
        enemy.takeDamage(damage);
        if (enemy.state == EnemyState::Alive) enemy.slowTimer = duration;
    }
    EffectKind kind() const override { return EffectKind::Slow; }
};

class Bullet {
public:
    bool active = false;
    Vector2 position{};
    int targetId = -1;
    float damage = 0.0f;
    float speed = 430.0f;
    std::shared_ptr<const IDamageEffect> effect;

    void launch(Vector2 start, int target, float amount, std::shared_ptr<const IDamageEffect> hitEffect) {
        active = true; position = start; targetId = target; damage = amount; effect = std::move(hitEffect);
    }
    void reset() { active = false; targetId = -1; effect.reset(); }
    void update(float dt, GameManager& game);
    void render() const { if (active) DrawCircleV(position, 5.0f, GOLD); }
};

class BulletPool {
    std::vector<Bullet> bullets;
public:
    explicit BulletPool(size_t capacity) : bullets(capacity) {}
    Bullet* acquire() {
        for (Bullet& bullet : bullets) if (!bullet.active) return &bullet;
        return nullptr;
    }
    void update(float dt, GameManager& game) { for (Bullet& bullet : bullets) bullet.update(dt, game); }
    void render() const { for (const Bullet& bullet : bullets) bullet.render(); }
    int activeCount() const {
        return static_cast<int>(std::count_if(bullets.begin(), bullets.end(), [](const Bullet& b) { return b.active; }));
    }
};

class Tower {
public:
    Vector2 position;
    int id;
    float range = 150.0f;
    float cooldown = 0.0f;
    float fireRate = 0.62f;
    float damage = 25.0f;
    TargetStrategy strategy;
    std::shared_ptr<const IDamageEffect> effect;

    Tower(int towerId, Vector2 location, TargetStrategy target, std::shared_ptr<const IDamageEffect> hitEffect)
        : position(location), id(towerId), strategy(std::move(target)), effect(std::move(hitEffect)) {}
    void update(float dt, GameManager& game);
    void render() const {
        DrawCircleV(position, range, Color{70, 170, 160, 25});
        DrawCircleV(position, 22.0f, Color{62, 170, 125, 255});
        DrawCircleLines(static_cast<int>(position.x), static_cast<int>(position.y), 22.0f, RAYWHITE);
        DrawCircleV(position, 8.0f, Color{245, 218, 95, 255});
    }
};

class GameManager {
    std::vector<std::unique_ptr<Enemy>> enemies;
    std::vector<std::unique_ptr<Tower>> towers;
    BulletPool bulletPool{180};
    std::vector<Vector2> path{{280, 180}, {500, 180}, {500, 390}, {760, 390}, {760, 180}, {1080, 180}};
    int nextEnemyId = 1;
    int nextTowerId = 1;
    int wave = 0;
    int pendingSpawns = 0;
    int hp = 20;
    int gold = 260;
    float spawnTimer = 0.0f;
    float nextWaveTimer = 1.0f;
    bool gameOver = false;
    bool victory = false;
    TargetStrategy selectedStrategy;
    int targetMode = 0;
    EffectKind selectedEffect = EffectKind::Single;

public:
    GameManager() { reset(); }
    void reset() {
        enemies.clear(); towers.clear(); nextEnemyId = nextTowerId = 1;
        wave = 0; pendingSpawns = 0; hp = 20; gold = 260;
        spawnTimer = 0.0f; nextWaveTimer = 1.0f; gameOver = victory = false;
        selectedStrategy = first; targetMode = 0; selectedEffect = EffectKind::Single;
    }

    static int first(Vector2 origin, const std::vector<std::unique_ptr<Enemy>>& list) {
        int result = -1; int furthest = -1;
        for (const auto& enemy : list) if (enemy->state == EnemyState::Alive && Vector2Distance(origin, enemy->position) <= 150.0f && enemy->waypoint > furthest) {
            result = enemy->id; furthest = enemy->waypoint;
        }
        return result;
    }
    static int lowestHp(Vector2 origin, const std::vector<std::unique_ptr<Enemy>>& list) {
        int result = -1; float lowest = 1e30f;
        for (const auto& enemy : list) if (enemy->state == EnemyState::Alive && Vector2Distance(origin, enemy->position) <= 150.0f && enemy->hp < lowest) {
            result = enemy->id; lowest = enemy->hp;
        }
        return result;
    }
    static int closest(Vector2 origin, const std::vector<std::unique_ptr<Enemy>>& list) {
        int result = -1; float nearest = 1e30f;
        for (const auto& enemy : list) {
            float distance = Vector2Distance(origin, enemy->position);
            if (enemy->state == EnemyState::Alive && distance <= 150.0f && distance < nearest) { result = enemy->id; nearest = distance; }
        }
        return result;
    }

    Enemy* findEnemy(int id) {
        for (const auto& enemy : enemies) if (enemy->id == id) return enemy.get();
        return nullptr;
    }
    const std::vector<std::unique_ptr<Enemy>>& getEnemies() const { return enemies; }
    BulletPool& getBullets() { return bulletPool; }
    int getHp() const { return hp; }
    int getGold() const { return gold; }
    int getWave() const { return wave; }
    int getEnemyCount() const { return static_cast<int>(enemies.size()); }
    bool ended() const { return gameOver || victory; }
    void setStrategy(TargetStrategy value, int mode) { selectedStrategy = std::move(value); targetMode = mode; }
    void setEffect(EffectKind value) { selectedEffect = value; }

    void placeTower(Vector2 location) {
        if (ended() || gold < TOWER_COST || location.x < MAP_X || location.x > MAP_X + MAP_W || location.y < MAP_Y || location.y > MAP_Y + MAP_H) return;
        for (const auto& tower : towers) if (Vector2Distance(location, tower->position) < 54.0f) return;
        std::shared_ptr<const IDamageEffect> effect = std::make_shared<SingleDamage>();
        if (selectedEffect == EffectKind::Area) effect = std::make_shared<AreaDamage>(70.0f, 0.45f);
        if (selectedEffect == EffectKind::Slow) effect = std::make_shared<SlowDamage>(2.0f);
        towers.push_back(std::make_unique<Tower>(nextTowerId++, location, selectedStrategy, effect));
        gold -= TOWER_COST;
    }

    void update(float dt) {
        if (ended()) return;
        if (pendingSpawns == 0 && enemies.empty()) {
            nextWaveTimer -= dt;
            if (nextWaveTimer <= 0.0f) {
                ++wave;
                if (wave > 10) { victory = true; return; }
                pendingSpawns = 4 + wave * 2; spawnTimer = 0.0f;
            }
        }
        if (pendingSpawns > 0) {
            spawnTimer -= dt;
            if (spawnTimer <= 0.0f) {
                float health = 75.0f + wave * 22.0f;
                enemies.push_back(std::make_unique<Enemy>(nextEnemyId++, path.front(), health, 42.0f + wave * 2.0f));
                --pendingSpawns; spawnTimer = 0.72f;
            }
        }
        for (auto& enemy : enemies) enemy->update(dt, path);
        for (auto& tower : towers) tower->update(dt, *this);
        bulletPool.update(dt, *this);
        for (const auto& enemy : enemies) {
            if (enemy->state == EnemyState::Dead) gold += 18 + wave;
            else if (enemy->state == EnemyState::ReachedEnd) --hp;
        }
        enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [](const std::unique_ptr<Enemy>& enemy) { return enemy->state != EnemyState::Alive; }), enemies.end());
        if (hp <= 0) gameOver = true;
        if (wave == 10 && pendingSpawns == 0 && enemies.empty()) victory = true;
        if (pendingSpawns == 0 && enemies.empty()) nextWaveTimer = 2.0f;
    }

    void render() const {
        DrawRectangle(0, 0, WIDTH, HEIGHT, Color{21, 27, 37, 255});
        DrawRectangle(0, 0, MAP_X - 1, HEIGHT, Color{29, 39, 52, 255});
        DrawRectangle(MAP_X, MAP_Y, MAP_W, MAP_H, Color{42, 61, 62, 255});
        DrawRectangleLines(MAP_X, MAP_Y, MAP_W, MAP_H, Color{100, 148, 125, 255});
        for (size_t i = 1; i < path.size(); ++i) DrawLineEx(path[i - 1], path[i], 36.0f, Color{88, 78, 65, 255});
        for (const auto& tower : towers) tower->render();
        for (const auto& enemy : enemies) {
            DrawCircleV(enemy->position, 16.0f, Color{211, 76, 72, 255});
            DrawRectangle(static_cast<int>(enemy->position.x - 20), static_cast<int>(enemy->position.y - 28), 40, 5, DARKGRAY);
            DrawRectangle(static_cast<int>(enemy->position.x - 20), static_cast<int>(enemy->position.y - 28), static_cast<int>(40.0f * enemy->hp / enemy->maxHp), 5, LIME);
        }
        bulletPool.render();
        DrawText("TOWER DEFENSE", 24, 24, 28, RAYWHITE);
        DrawText(TextFormat("HP: %d   GOLD: %d   WAVE: %d/10", hp, gold, wave), 24, 68, 20, GOLD);
        DrawText("Click map: build tower (80 gold)", 24, 130, 17, LIGHTGRAY);
        DrawText("[1] First  [2] Lowest HP  [3] Closest", 24, 166, 16, LIGHTGRAY);
        DrawText("[Q] Single  [W] Area  [E] Slow", 24, 195, 16, LIGHTGRAY);
        DrawText(TextFormat("Target: %s", targetMode == 0 ? "First" : targetMode == 1 ? "Lowest HP" : "Closest"), 24, 250, 18, SKYBLUE);
        DrawText(TextFormat("Effect: %s", selectedEffect == EffectKind::Area ? "Area" : selectedEffect == EffectKind::Slow ? "Slow" : "Single"), 24, 278, 18, SKYBLUE);
        DrawText(TextFormat("Enemies: %d  Bullets: %d", getEnemyCount(), bulletPool.activeCount()), 24, 320, 17, LIGHTGRAY);
        if (nextWaveTimer > 0.0f && pendingSpawns == 0 && enemies.empty()) DrawText(TextFormat("Next wave in %.1f", nextWaveTimer), 24, 355, 18, LIME);
        if (ended()) {
            DrawRectangle(0, 0, WIDTH, HEIGHT, Color{0, 0, 0, 150});
            const char* title = victory ? "VICTORY!" : "GAME OVER";
            DrawText(title, WIDTH / 2 - MeasureText(title, 54) / 2, 280, 54, victory ? LIME : RED);
            DrawText("Press R to play again", WIDTH / 2 - 115, 350, 22, RAYWHITE);
        }
    }
};

void AreaDamage::apply(GameManager& game, Enemy& target, float damage) const {
    for (const auto& enemy : game.getEnemies()) if (Vector2Distance(target.position, enemy->position) <= radius)
        enemy->takeDamage(&target == enemy.get() ? damage : damage * falloff);
}

void Bullet::update(float dt, GameManager& game) {
    if (!active) return;
    Enemy* target = game.findEnemy(targetId);
    if (!target || target->state != EnemyState::Alive) { reset(); return; }
    Vector2 direction = Vector2Subtract(target->position, position);
    float distance = Vector2Length(direction);
    if (distance <= speed * dt || CheckCollisionCircles(position, 5.0f, target->position, 16.0f)) {
        effect->apply(game, *target, damage); reset(); return;
    }
    position = Vector2Add(position, Vector2Scale(Vector2Normalize(direction), speed * dt));
}

void Tower::update(float dt, GameManager& game) {
    cooldown -= dt;
    if (cooldown > 0.0f) return;
    int targetId = strategy(position, game.getEnemies());
    if (targetId < 0) return;
    Bullet* bullet = game.getBullets().acquire();
    if (!bullet) return;
    bullet->launch(position, targetId, damage, effect);
    cooldown = fireRate;
}

int main() {
    InitWindow(WIDTH, HEIGHT, "Tower Defense - Raylib C++");
    SetTargetFPS(60);
    GameManager game;
    while (!WindowShouldClose()) {
        const float deltaTime = GetFrameTime();
        if (IsKeyPressed(KEY_R)) game.reset();
        if (IsKeyPressed(KEY_ONE)) game.setStrategy(GameManager::first, 0);
        if (IsKeyPressed(KEY_TWO)) game.setStrategy(GameManager::lowestHp, 1);
        if (IsKeyPressed(KEY_THREE)) game.setStrategy(GameManager::closest, 2);
        if (IsKeyPressed(KEY_Q)) game.setEffect(EffectKind::Single);
        if (IsKeyPressed(KEY_W)) game.setEffect(EffectKind::Area);
        if (IsKeyPressed(KEY_E)) game.setEffect(EffectKind::Slow);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) game.placeTower(GetMousePosition());
        game.update(deltaTime);
        BeginDrawing();
        game.render();
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
