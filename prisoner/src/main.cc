/*
Copyright (C) 2012-2020 tim cotter. All rights reserved.
*/

/**
prisoner's dilemma.
**/

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>

namespace {

class Prisoners1 {
public:
	Prisoners1() = default;
	~Prisoners1() = default;

	void run() noexcept {
		/**
		the prisoner's dilemma.
		there's a payoff matrix.
		two prisoners can cooperate (with each other) or defect.
		their best combined strategy is to cooperate.
		but any greedy strategy leads to both defect.
		**/
		/** from memory **/
		/*int payoff_both_cooperate = 10;
		int payoff_cooperate_defect = 0;
		int payoff_defect_cooperate = 15;
		int payoff_both_defect = 1;*/
		/** from wikipedia **/
		int payoff_both_cooperate = -1;
		int payoff_cooperate_defect = -3;
		int payoff_defect_cooperate = 0;
		int payoff_both_defect = -2;

		int cooperators = 50;
		int defectors;
		double cooperator_expectation;
		double defector_expectation;

		for (int i = 0; i < 100; ++i) {
			defectors = 100 - cooperators;
			cooperator_expectation = (cooperators * payoff_both_cooperate + defectors * payoff_cooperate_defect) / 100.0;
			defector_expectation = (cooperators * payoff_defect_cooperate + defectors * payoff_both_defect) / 100.0;

			if (cooperators == 0 || cooperators == 100) {
				break;
			}

			if (cooperator_expectation > defector_expectation) {
				++cooperators;
			} else {
				--cooperators;
			}
		}

		LOG("Prisoner's Dilemma version 1: greedy");
		LOG("counts: cooperators="<<cooperators<<" defectors="<<defectors<<
			" expectations: cooperator="<<cooperator_expectation<<" defector="<<defector_expectation);
	}
};


class Prisoners2 {
public:
	Prisoners2() = default;
	~Prisoners2() = default;

	void run() noexcept {
		/**
		the prisoner's dilemma.
		there's a payoff matrix.
		two prisoners can cooperate (with each other) or defect.
		their best combined strategy is to cooperate.
		the homogeneous strategy:
		assume the other guy will do the same as i do.
		so there are only two results: both cooperate or both defect.
		**/
		/** from memory **/
		int payoff_both_cooperate = 10;
		//int payoff_cooperate_defect = 0;
		//int payoff_defect_cooperate = 15;
		int payoff_both_defect = 1;
		/** from wikipedia **/
		/*int payoff_both_cooperate = -1;
		//int payoff_cooperate_defect = -3;
		//int payoff_defect_cooperate = 0;
		int payoff_both_defect = -2;*/

		double cooperator_expectation = payoff_both_cooperate;
		double defector_expectation = payoff_both_defect;

		LOG("Prisoner's Dilemma version 2: homogenous");
		LOG("expectations: cooperator="<<cooperator_expectation<<" defector="<<defector_expectation);
	}
};

}

int main(
    int argc, char *argv[]
) noexcept {
    (void) argc;
    (void) argv;

    agm::log::init(AGM_TARGET_NAME ".log");

	Prisoners1().run();
	Prisoners2().run();

    return 0;
}
