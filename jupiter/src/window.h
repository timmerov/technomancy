/*
Copyright (C) 2012-2016 tim cotter. All rights reserved.
*/

#pragma once

/**
spinning world example.
**/

class JupiterWindow {
protected:
    JupiterWindow() throw();
public:
    JupiterWindow(const JupiterWindow &) = delete;
    static JupiterWindow *create() throw();
    virtual ~JupiterWindow() throw();

    virtual void init() throw() = 0;
    virtual void exit() throw() = 0;
    virtual void run() throw() = 0;
};
