#pragma once

#include <cstdlib>
#include <ctime>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <unistd.h>

std::mutex world_mutex;

enum direction
{
    NORTH, EAST, SOUTH, WEST, STATIC
};

struct coordinate
{
    int x, y;
};

coordinate get_random_coord()
{
    auto x = rand() % 79;
    auto y = rand() % 23;

    return {x, y};
}

struct world_object
{
    world_object(coordinate position, std::function<void(void)> logic) : position_(position), logic_(logic) {}
    world_object(char symbol, coordinate position, std::function<void(void)> logic) : symbol_(symbol), position_(position), logic_(logic) {}
    virtual ~world_object() = default;
    world_object(const world_object&) = default;
    world_object(world_object&&) = default;

    virtual void update() { logic_(); };

    char symbol_ { '?' };
    coordinate position_;
    std::function<void(void)> logic_;
};

struct i_movable
{
    virtual void set_move_intent(direction dir) = 0;
    virtual direction get_move_intent(bool reset=false) = 0;
};

struct i_timed_movement
{
    virtual std::chrono::time_point<std::chrono::system_clock> next_move() = 0;
};

struct i_drawable
{
    virtual char get_representation() = 0;
};

struct i_collidable
{
    virtual void on_collision() = 0;
};

struct player : public world_object, i_movable, i_drawable
{
    player(coordinate position) : world_object('.', position, [](){}) {}
    char get_representation() override 
    {
        static auto animation_index = 0; 
        return animation[animation_index++ % 14]; 
    }

    void set_move_intent(direction dir) override { move_intent = dir; }
    direction get_move_intent(bool reset = false) override 
    { 
        if (reset) 
        { 
            auto tmp = move_intent; 
            move_intent = STATIC; 
            return tmp;
        } 
    else 
        return move_intent; 
    }

    direction move_intent { STATIC };

    void on_capture()
    {
        score += 1;
    }

    char animation[14] { '.', '.', 'o', 'o', '8', '8', 'O', 'O', '8', '8', 'o', 'o', '.', '.'};
    int score = 0;
};


using world = std::vector<world_object>;
using world_p = std::vector<world_object*>;

bool is_collision(const world_object& a, const world_object& b)
{
    std::lock_guard<std::mutex> world_guard(world_mutex);
    return a.position_.x == b.position_.x && a.position_.y == b.position_.y;
}

//world_object& collided_with()

void move_object(world_object& object, direction dir)
{
    std::lock_guard<std::mutex> world_guard(world_mutex);
    switch(dir)
    {
    case NORTH:
        object.position_.y = std::clamp( object.position_.y -= 1, 0, 23); //w N
        break;
    case WEST:
        object.position_.x = std::clamp( object.position_.x -= 1, 0, 79); //a W
        break;
    case SOUTH:
        object.position_.y = std::clamp( object.position_.y += 1, 0, 23); //s S
        break;
    case EAST:
        object.position_.x = std::clamp( object.position_.x += 1, 0, 79); //d E
        break;
    }
}
