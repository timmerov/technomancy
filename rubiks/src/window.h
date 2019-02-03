/*
Copyright (C) 2012-2019 tim cotter. All rights reserved.
*/

#pragma once

/**
rubiks cube example.
**/

class RubiksWindow {
protected:
    RubiksWindow() noexcept;
public:
    RubiksWindow(const RubiksWindow &) = delete;
    static RubiksWindow *create() noexcept;
    virtual ~RubiksWindow() noexcept;

    virtual void init() noexcept = 0;
    virtual void exit() noexcept = 0;
    virtual void run() noexcept = 0;
};
