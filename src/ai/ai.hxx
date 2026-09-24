#pragma once

#include "../world/world.hxx"

#include <cstdlib>
#include <iostream>

direction get_direct_path(const coordinate& from, const coordinate& to)
{
    auto delta_x = from.x - to.x;
    auto delta_y = from.y - to.y;

    if ((delta_x == 0) && (delta_y == 0))
        return STATIC;
    
    if (std::abs(delta_x) > std::abs(delta_y))
    {
        if (delta_x > 0)
            return WEST;
        else
            return EAST;
    }
    else
    {
        if (delta_y > 0)
            return NORTH;
        else
            return SOUTH;
    }
}

void hunt(world_object& hunter, const world_object& target)
{
    auto* movable = dynamic_cast<i_movable*>(&hunter);

    if (!movable) return;

    movable->set_move_intent(get_direct_path(hunter.position_, target.position_));
}


void block(world_object& protector, const world_object& aggressor, const world_object& target)
{
    auto* movable = dynamic_cast<i_movable*>(&protector);

    if (!movable) return;

    auto mid_x = (aggressor.position_.x + target.position_.x) / 2;
    auto mid_y = (aggressor.position_.y + target.position_.y) / 2;

    movable->set_move_intent(get_direct_path(protector.position_, coordinate{mid_x, mid_y}));
}


void erratic(i_movable& obj)
{
    auto x = rand() % 8;

    switch (x)
    {
    case 0:
        obj.set_move_intent(NORTH);
    break;
    case 1:
        obj.set_move_intent(SOUTH);
    break;
    case 2:
        obj.set_move_intent(EAST);
    break;
    case 3:
        obj.set_move_intent(WEST);
    break;
    case 4:
    case 5:
    case 6:
    case 7:
        obj.set_move_intent(STATIC);
    break;
    }
}

struct hunter : public world_object, i_movable, i_drawable
{
    hunter(coordinate position, std::function<void(void)> logic) 
    : world_object('~', position, logic ) 
    {}

    char get_representation() override { return animation[animation_index++ % 8]; }
    
    void set_move_intent(direction dir) override { move_intent = dir; }
    direction get_move_intent(bool reset = true) 
    { 
        using namespace std::literals::chrono_literals;
        
        if (std::chrono::system_clock::now() < next_move)
            return STATIC;

        next_move = std::chrono::system_clock::now() + 250ms;
            
        if (reset) 
        {
            auto tmp = move_intent; 
            move_intent = STATIC; 
            return tmp;
        } 
        else
            return move_intent; 
    }

    std::chrono::time_point<std::chrono::system_clock> next_move;

    char animation[8] { '~', '/', '|', '\\', '~', '/', '|', '\\'};
    std::size_t animation_index { rand() };

    direction move_intent { STATIC };
};

struct bounty : public world_object, i_movable, i_drawable
{
    bounty(coordinate position) 
    : world_object('$', position, [this]() {  erratic(*this); escape_jump(); })
    , next_jump(std::chrono::system_clock::now())
    , next_move(std::chrono::system_clock::now()) 
    {}

    char get_representation() override { return symbol_; }
    void set_move_intent(direction dir) override { move_intent = dir; }

    direction get_move_intent(bool reset = true) 
    { 
        using namespace std::literals::chrono_literals;
        
        if (std::chrono::system_clock::now() < next_move)
            return STATIC;

        next_move = std::chrono::system_clock::now() + 125ms;
            
        if (reset) 
        {
            auto tmp = move_intent; 
            move_intent = STATIC; 
            return tmp;
        } 
        else
            return move_intent; 
    }

    void escape_jump()
    {
        using namespace std::literals::chrono_literals;
        
        if (std::chrono::system_clock::now() < next_jump)
            return;

        next_jump = std::chrono::system_clock::now() + 5000ms;
        std::lock_guard<std::mutex> world_guard(world_mutex);
        position_ = get_random_coord();
    }

    void on_capture()
    {
        next_jump = std::chrono::system_clock::now();
        escape_jump();
    }

    std::chrono::time_point<std::chrono::system_clock> next_jump, next_move;
    direction move_intent { STATIC };
};
