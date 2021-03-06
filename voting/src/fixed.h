/*
Copyright (C) 2012-2020 tim cotter. All rights reserved.
*/

/**
reverse rank order voting.

use a selection of fixed data sets.
**/

class FixedDataVoting {
public:
    FixedDataVoting() noexcept;
    ~FixedDataVoting() noexcept;

    void run() noexcept;

private:
    void *impl_ = nullptr;
};
