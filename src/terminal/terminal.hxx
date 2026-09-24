#pragma once

#include "../world/world.hxx"

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <format>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <fcntl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>


struct terminal_screen 
{
    using buffer = std::vector<std::string>;

    explicit terminal_screen(world_p& w, const player& p);
    virtual ~terminal_screen();

    void draw_world();
    void draw_ui(bool is_paused);
    inline int fd() { return tty_fd; }

private:
    terminal_screen::buffer get_renderbuffer();

    world_p& world_;
    const player& player_;
    struct termios original_terminal_settings;

    int tty_fd { -1 };
    std::size_t width  { 80 };
    std::size_t height { 24 };
    
};

terminal_screen::buffer terminal_screen::get_renderbuffer()
{
    buffer tmp;

//    std::cout << "setup buffer...\n";
    for (auto i = 0; i < height; ++i)
    {
        tmp.push_back(std::string(width, ' '));
    }
//    std::cout << "done.\n";

    return tmp;
}

terminal_screen::terminal_screen(world_p& w, const player& p)
 : world_(w), player_(p)
{
    std::cout << __PRETTY_FUNCTION__ << "\n";

    tty_fd = open("/dev/tty", O_RDWR);
    std::cout << "tty_fd: " << tty_fd << "\n";
    
    if (tcgetattr(tty_fd, &original_terminal_settings))
    {
        perror("get");
        std::cout << "error getattr\n";
    }
    struct termios settings = original_terminal_settings;
    settings.c_lflag &= ~(ICANON | ECHO);

    if (tcsetattr(tty_fd, TCSANOW, &settings))
    {
        perror("set");
        std::cout << "error setattr\n";
    }
}

terminal_screen::~terminal_screen()
{
    tcsetattr(tty_fd, TCSAFLUSH, &original_terminal_settings);
    close(tty_fd);
}

void terminal_screen::draw_ui(bool is_paused)
{
    std::string mt { "" };
    std::string paused { "PAUSED" };
    std::string instructions { "WASD to move, SPACEBAR to stop moving, P to pause/unpause, Q to quit" };
    std::string score { "SCORE: "};
    
    mt.resize(80);
    paused.resize(80);
    instructions.resize(80);
    score += std::to_string(player_.score);

    std::cout << (is_paused ? paused : mt) << "\n" << instructions << "\n" << score << "\n";
}

void terminal_screen::draw_world()
{
//    std::cout << __PRETTY_FUNCTION__ << "\n";
    auto buf = get_renderbuffer();

    std::lock_guard<std::mutex> world_guard(world_mutex);
    for (auto* obj : world_)
    {
        if (auto* drawable = dynamic_cast<i_drawable*>(obj))
            buf[obj->position_.y][obj->position_.x] = drawable->get_representation();
    }
    std::cout << "\033[H";
    for (const auto& line : buf)
        std::cout << line << "\n";
}
