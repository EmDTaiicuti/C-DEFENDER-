#pragma once
#include <raylib.h>

const int TILE_SIZE = 40;
const int GRID_WIDTH = 20;   
const int GRID_HEIGHT = 15;  

enum TileType {
    EMPTY,       
    PATH,        
    OBSTACLE,    
    SPAWN,       
    BASE         
};

struct Point {
    int x, y;
    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
};
