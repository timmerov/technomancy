/*
Copyright (C) 2012-2017 tim cotter. All rights reserved.
*/

/**
pluto simulation.
**/

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>
#include <aggiornamento/master.h>

#include <cmath>


namespace {
/**
numbers from wikipedia
**/
	const double kPlutoRadiusKm = 1188.3;
	const double kPlutoMassKg = 1.303e22;
	const double kCharonRadiusKm = 606.0;
	const double kCharonMassKg =1.586e21;
	const double kSeparationKm = 19571;
	const double kCharonBarycenterKm = 17536;
	const double kPeriodDays = 6.3872304;

    class Pluto {
    public:
        Pluto() = default;
        Pluto(const Pluto &) = default;
        ~Pluto() = default;

        void run() noexcept {
			std::cout << "Hello, World!" << std::endl;

			double pi = acos(-1.0);

			double p_volume = 4.0/3.0*pi*kPlutoRadiusKm*kPlutoRadiusKm*kPlutoRadiusKm;
			double c_volume = 4.0/3.0*pi*kCharonRadiusKm*kCharonRadiusKm*kCharonRadiusKm;
			double p_density = kPlutoMassKg / p_volume;
			double c_density = kCharonMassKg / c_volume;
			double avg_density = (p_density + c_density) / 2.0;

			double pc_radius = kPlutoRadiusKm / kCharonRadiusKm;
			double pc_mass = kPlutoMassKg / kCharonMassKg;

			double pc_sep_p_radius = kSeparationKm / kPlutoRadiusKm;
			double pc_sep_c_radius = kSeparationKm / kCharonRadiusKm;

			double period_hours = kPeriodDays * 24.0;
			double period_minutes = period_hours * 60.0;
			double period_seconds = period_minutes * 60.0;

			std::cout << "Pluto   Radius : " << kPlutoRadiusKm << " km" << std::endl;
			std::cout << "Pluto   Mass   : " << kPlutoMassKg << " kg" << std::endl;
			std::cout << "Pluto   Volume : " << p_volume << " km^3" << std::endl;
			std::cout << "Pluto   Density: " << p_density << " kg/km^3" << std::endl;
			std::cout << "Charon  Radius : " << kCharonRadiusKm << " km" << std::endl;
			std::cout << "Charon  Mass   : " << kCharonMassKg << " kg" << std::endl;
			std::cout << "Charon  Volume : " << c_volume << " km^3" << std::endl;
			std::cout << "Charon  Density: " << c_density << " kg/km^3" << std::endl;
			std::cout << "P/C     Radius : " << pc_radius << std::endl;
			std::cout << "P/C     Mass   : " << pc_mass << std::endl;
			std::cout << "Average Density: " << avg_density << " kg/km^3" << std::endl;
			std::cout << "Separation Pluto  Radii: " << pc_sep_p_radius << std::endl;
			std::cout << "Separation Charon Radii: " << pc_sep_c_radius << std::endl;
			std::cout << "Period: " << kPeriodDays << " days" << std::endl;
			std::cout << "Period: " << period_hours << " hours" << std::endl;
			std::cout << "Period: " << period_minutes << " minutes" << std::endl;
			std::cout << "Period: " << period_seconds << " seconds" << std::endl;

			std::cout << "Goodbye, World!" << std::endl;
        }
    };
}

int main(
    int argc, char *argv[]
) noexcept {
    (void) argc;
    (void) argv;

    agm::log::init(AGM_TARGET_NAME ".log");

    Pluto pluto;
    pluto.run();

    return 0;
}
