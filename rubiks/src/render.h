/*
Copyright (C) 2012-2020 tim cotter. All rights reserved.
*/

#pragma once

/**
rubiks cube example.
**/


class Render {
protected:
    Render() noexcept;
public:
    Render(const Render &) = default;
    static Render *create() noexcept;
    virtual ~Render() noexcept;

    virtual void init(int width, int height) noexcept = 0;
    virtual void exit() noexcept = 0;
    virtual void draw() noexcept = 0;
    virtual void resize(int width, int height) noexcept = 0;
    virtual void keyPressed(int symbol) noexcept = 0;
};
