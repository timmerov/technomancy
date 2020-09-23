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

Ballots get_voting_data() noexcept;

Candidates get_candidates() noexcept;
