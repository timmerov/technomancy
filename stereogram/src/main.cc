/*
Copyright (C) 2012-2020 tim cotter. All rights reserved.
*/

/**
stereogram example.
**/

#include "window.h"

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>
#include <aggiornamento/master.h>


namespace {
	const auto kFrameTimeMS = 1000/60;

    class Stereo {
    public:
        Stereo() = default;
        Stereo(const Stereo &) = default;
        ~Stereo() = default;

        StereoWindow *window_ = nullptr;

        void run() noexcept {
            window_ = StereoWindow::create();
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
                agm::sleep::milliseconds(kFrameTimeMS);
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

    Stereo stereo;
    stereo.run();

    return 0;
}
