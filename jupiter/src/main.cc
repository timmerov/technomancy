/*
Copyright (C) 2012-2016 tim cotter. All rights reserved.
*/

/**
spinning jupiter example.
**/

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>

#include "window.h"


namespace {
	const auto kFrameTimeMS = 1000/60;

    bool g_stop_flag = false;

    class Jupiter {
    public:
        Jupiter() = default;
        Jupiter(const Jupiter &) = default;
        ~Jupiter() = default;

        bool stop_flag_ = false;
        JupiterWindow *window_ = nullptr;

        void run() throw() {
            window_ = JupiterWindow::create();
            window_->init();
            run_loop();
            window_->exit();
            delete window_;
        }

        void run_loop() throw() {
            while (g_stop_flag == false) {
                //agm::sleep::milliseconds(kFrameTimeMS);
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

    Jupiter jupiter;
    jupiter.run();

    return 0;
}
