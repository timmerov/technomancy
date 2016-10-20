/*
Copyright (C) 2012-2016 tim cotter. All rights reserved.
*/

/**
spinning world example.
**/

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>

#include "window.h"


namespace {
	const auto kFrameTimeMS = 1000/60;

    bool g_stop_flag = false;

    class World {
    public:
        World() = default;
        World(const World &) = default;
        ~World() = default;

        bool stop_flag_ = false;
        SimpleWindow *window_ = nullptr;

        void run() throw() {
            window_ = SimpleWindow::create();
            window_->init();
            run_loop();
            window_->exit();
            delete window_;
        }

        void run_loop() throw() {
            while (g_stop_flag == false) {
                agm::sleep::milliseconds(kFrameTimeMS);
                window_->run();
            }
        }
    };
}

void main_stop() {
    g_stop_flag = true;
}

int main(
    int argc, char *argv[]
) throw() {
    (void) argc;
    (void) argv;

    agm::log::init(AGM_TARGET_NAME ".log");

    World world;
    world.run();

    return 0;
}
