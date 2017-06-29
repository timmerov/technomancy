/*
Copyright (C) 2012-2016 tim cotter. All rights reserved.
*/

#pragma once

/**
spinning world example.
**/

class WorldWindow {
protected:
    WorldWindow() noexcept;
public:
    WorldWindow(const WorldWindow &) = delete;
    static WorldWindow *create() noexcept;
    virtual ~WorldWindow() noexcept;

    virtual void init() noexcept = 0;
    virtual void exit() noexcept = 0;
    virtual void run() noexcept = 0;
};
