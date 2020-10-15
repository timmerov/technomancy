/*
Copyright (C) 2012-2020 tim cotter. All rights reserved.
*/

/**
reverse rank order voting.

use a selection of fixed data sets.
**/

class Voting {
public:
    Voting() noexcept;
    ~Voting() noexcept;

    void run() noexcept;

private:
    void *impl_ = nullptr;
};
