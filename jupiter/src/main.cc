/*
Copyright (C) 2012-2020 tim cotter. All rights reserved.
*/

/**
spinning jupiter example.
**/

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>
#include <aggiornamento/master.h>

#include "window.h"


namespace {
	const auto kFrameTimeMS = 1000/60;

    class Jupiter {
    public:
        Jupiter() = default;
        Jupiter(const Jupiter &) = default;
        ~Jupiter() = default;

        JupiterWindow *window_ = nullptr;

        void run() noexcept {
            window_ = JupiterWindow::create();
            window_->init();
            runLoop();
            window_->exit();
            delete window_;
        }

        void runLoop() noexcept {
            for(;;) {
                auto is_done = agm::master::isDone();
                if (is_done) {
                    break;
                }
                //agm::sleep::milliseconds(kFrameTimeMS);
                window_->run();
            }
        }
    };
}

int main(
    int argc, char *argv[]
) noexcept {
    (void) argc;
    (void) argv;

    agm::log::init(AGM_TARGET_NAME ".log");

    Jupiter jupiter;
    jupiter.run();

    return 0;
}
