/*
Copyright (C) 2012-2016 tim cotter. All rights reserved.
*/

#pragma once

/**
spinning world example.
**/

class SimpleWindow {
protected:
    SimpleWindow() throw();
public:
    SimpleWindow(const SimpleWindow &) = delete;
    static SimpleWindow *create() throw();
    virtual ~SimpleWindow() throw();

    virtual void init() throw() = 0;
    virtual void exit() throw() = 0;
    virtual void run() throw() = 0;
};
