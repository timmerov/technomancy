/*
Copyright (C) 2012-2016 tim cotter. All rights reserved.
*/

#pragma once

/**
spinning world example.
**/


class Render {
protected:
    Render() throw();
public:
    Render(const Render &) = default;
    static Render *create() throw();
    virtual ~Render() throw();

    virtual void init(int width, int height) throw() = 0;
    virtual void exit() throw() = 0;
    virtual void draw() throw() = 0;
    virtual void resize(int width, int height) throw() = 0;
};
