#include "ai/ai.hxx"
#include "input/input.hxx"
#include "terminal/terminal.hxx"
#include "world/world.hxx"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

//world w;
std::atomic<bool> is_running { true };
std::atomic<bool> is_paused { false };

std::mutex pause_mutex;
std::condition_variable pause_cv;

void stop()
{
    is_running = false;
}

void render_loop(terminal_screen& term)
{
    using namespace std::literals::chrono_literals;
    while (is_running)
    {
        term.draw_world();
        term.draw_ui(is_paused);
        std::this_thread::sleep_for(50ms);
    }
}

void logic_loop(world_p& w)
{
    using namespace std::literals::chrono_literals;
    player* p;
    bounty* b;
    std::vector<hunter*> hunters;

    for (auto* o : w)
    {
        if (auto* p_ = dynamic_cast<player*>(o))
        {
            p = p_;
            continue;
        }
        else if (auto* h = dynamic_cast<hunter*>(o))
        {
            hunters.push_back(h);
            continue;
        }
        else if (auto* b_ = dynamic_cast<bounty*>(o))
        {
            b = b_;
            continue;
        }
    }

    while (is_running)
    {
        std::unique_lock pause_lock(pause_mutex);
        pause_cv.wait(pause_lock, [](){ return !is_paused; });
        
        if (is_collision(*p, *b))
        {
            b->on_capture();
            p->on_capture();
        }

        for (auto* h : hunters)
        {
            if (is_collision(*p, *h))
            {
                stop();
                std::cout << "you are dead\n";
            }
        }

        for (auto* h : hunters)
        {
            for (auto* hh : hunters)
            {
                if (h == hh)
                    continue;

                if (is_collision(*h, *hh))
                {
                    std::lock_guard<std::mutex> world_guard(world_mutex);
                    h->position_ = get_random_coord();
                    hh->position_ = get_random_coord();
                }
            }
        }

        for (auto* o : w)
        {        
            if (auto* movable = dynamic_cast<i_movable*>(o))
            {
                auto dir = movable->get_move_intent();
                move_object(*o, dir);
            }
            o->update();
        }
        
        std::this_thread::sleep_for(50ms);
    }
}

int main()
{
    std::srand(std::time({}));

    player p { get_random_coord() };
    bounty b { get_random_coord() };
    hunter h1 { get_random_coord(), [&h1, &p](){ hunt(h1, p); } };
    hunter h2 { get_random_coord(), [&h2, &p](){ hunt(h2, p); } };
    hunter h3 { get_random_coord(), [&h3, &p, &b](){ block(h3, p, b); } };
    

/*    world w;
    w.emplace_back('@', get_random_coord(), [](){});
    w.emplace_back('$', get_random_coord(), [](){});
    w.emplace_back('#', get_random_coord(), [&w](){ hunt(w[2], w[0]); });*/

    world_p w;
    w.push_back(&p);
    w.push_back(&b);
    w.push_back(&h1);
    w.push_back(&h2);    
    w.push_back(&h3);

    //w[2].logic_ = [&w](){ hunt(w[2], w[0]); };
    //w.push_back(std::move(hunter));

    terminal_screen term(w, p);
    input_listener input(term.fd());
    input.add_callback('f', [](){ std::cout << "respect\n"; });
    input.add_callback('q', [](){ is_paused = false; pause_cv.notify_all(); stop(); });

    input.add_callback('w', [&p](){
        p.set_move_intent(NORTH);
    });
    input.add_callback('a', [&p](){
        p.set_move_intent(WEST);
    });
    input.add_callback('s', [&p](){
        p.set_move_intent(SOUTH);
    });
    input.add_callback('d', [&p](){
        p.set_move_intent(EAST);
    });
    input.add_callback(' ', [&p]() { p.set_move_intent(STATIC); } );

    // todo: wall clock != in-game time
    input.add_callback('p', [](){ is_paused = !is_paused; if (!is_paused) pause_cv.notify_all(); } ) ;

    std::thread render_thread { render_loop, std::ref(term) };
    std::thread logic_thread { logic_loop, std::ref(w) };

    render_thread.join();
    logic_thread.join();
    
    return 0;
}
