/*
Copyright (C) 2012-2016 tim cotter. All rights reserved.
*/

#pragma once

/**
spinning world example.
**/

class WorldWindow {
protected:
    WorldWindow() throw();
public:
    WorldWindow(const WorldWindow &) = delete;
    static WorldWindow *create() throw();
    virtual ~WorldWindow() throw();

    virtual void init() throw() = 0;
    virtual void exit() throw() = 0;
    virtual void run() throw() = 0;
};
