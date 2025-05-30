/*
Copyright (C) 2012-2020 tim cotter. All rights reserved.
*/

/**
analyze guthrie voting.
**/

#pragma once

class GuthrieVoting {
public:
    GuthrieVoting() noexcept;
    ~GuthrieVoting() noexcept;

    void run() noexcept;

private:
    void *impl_ = nullptr;
};
