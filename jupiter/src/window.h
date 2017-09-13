/*
Copyright (C) 2012-2017 tim cotter. All rights reserved.
*/

#pragma once

/**
spinning world example.
**/

class JupiterWindow {
protected:
    JupiterWindow() noexcept;
public:
    JupiterWindow(const JupiterWindow &) = delete;
    static JupiterWindow *create() noexcept;
    virtual ~JupiterWindow() noexcept;

    virtual void init() noexcept = 0;
    virtual void exit() noexcept = 0;
    virtual void run() noexcept = 0;
};
