/**
voting data.
**/

#pragma once

#include <string>
#include <vector>

using Candidates = std::vector<std::string>;

class Ballot {
public:
    int choice_[6];
};
using Ballots = std::vector<Ballot>;

namespace burlington {
Ballots get_voting_data() noexcept;
Candidates get_candidates() noexcept;
}

namespace minneapolis_park_1 {
Ballots get_voting_data() noexcept;
Candidates get_candidates() noexcept;
}

namespace minneapolis_park_3 {
Ballots get_voting_data() noexcept;
Candidates get_candidates() noexcept;
}

namespace minneapolis_park_5 {
Ballots get_voting_data() noexcept;
Candidates get_candidates() noexcept;
}

namespace minneapolis_park_6 {
Ballots get_voting_data() noexcept;
Candidates get_candidates() noexcept;
}

namespace minneapolis_ward_1 {
Ballots get_voting_data() noexcept;
Candidates get_candidates() noexcept;
}

namespace minneapolis_ward_3 {
Ballots get_voting_data() noexcept;
Candidates get_candidates() noexcept;
}

namespace minneapolis_ward_4 {
Ballots get_voting_data() noexcept;
Candidates get_candidates() noexcept;
}

namespace minneapolis_ward_5 {
Ballots get_voting_data() noexcept;
Candidates get_candidates() noexcept;
}

namespace minneapolis_ward_6 {
Ballots get_voting_data() noexcept;
Candidates get_candidates() noexcept;
}

namespace minneapolis_ward_7 {
Ballots get_voting_data() noexcept;
Candidates get_candidates() noexcept;
}

namespace minneapolis_ward_8 {
Ballots get_voting_data() noexcept;
Candidates get_candidates() noexcept;
}

namespace minneapolis_ward_9 {
Ballots get_voting_data() noexcept;
Candidates get_candidates() noexcept;
}

namespace minneapolis_ward_10 {
Ballots get_voting_data() noexcept;
Candidates get_candidates() noexcept;
}

namespace minneapolis_ward_11 {
Ballots get_voting_data() noexcept;
Candidates get_candidates() noexcept;
}

namespace minneapolis_ward_12 {
Ballots get_voting_data() noexcept;
Candidates get_candidates() noexcept;
}
