/*
Copyright (C) 2012-2020 tim cotter. All rights reserved.
*/

/**
reverse rank order voting.

generate data sets.
**/

class GeneratedDataVoting {
public:
    GeneratedDataVoting() noexcept;
    ~GeneratedDataVoting() noexcept;

    void run() noexcept;

private:
    void *impl_ = nullptr;
};
