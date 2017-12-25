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
	const double kBigGM3pKgpS = 6.67408e-11;

/**
definitions:
	barycenter = center of mass
	m1,m2 = mass of objects
	m = mass of all objects
	r1,r2 = object to center of mass
	r = separation of objects
	v1,v2 = velocities of objects in non-rotating reference frame of the barycenter

formulas:
	:                       total mass: m = m1 + m2
	:                       separation: r = r1 + r2
	:                   center of mass: r * m = r1 * m1 + r2 * m2
	:   gravitational potential energy:  - G * m1 * m2 / r
	: gravitational acceleration of m1:    G * m2 / r^2
	: gravitational acceleration of m2:    G * m1 / r^2
	:             kinetic energy of m1:  1/2 * m1 * v1^2
	:             kinetic energy of m2:  1/2 * m2 * v2^2
	:   centripetal acceleration of m1:  v1^2 / r1
	:   centripetal acceleration of m2:  v2^2 / r2
	:                   velocity of m1:  (G * m2 / r^2 * r2)^0.5
	:                   velocity of m2:  (G * m1 / r^2 * r1)^0.5
**/

    class Pluto {
    public:
        Pluto() = default;
        Pluto(const Pluto &) = default;
        ~Pluto() = default;

        void run() noexcept {
			std::cout << "Hello, World!" << std::endl;

			double pi = std::acos(-1.0);

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

			double m1 = kPlutoMassKg;
			double m2 = kCharonMassKg;
			double m = m1 + m2;
			double r = kSeparationKm;
			double r1 = r * m2 / m;
			double r2 = r * m1 / m;
			double G = kBigGM3pKgpS / 1e9; /// units
			double g1 = G * m2 / (r * r);
			double g2 = G * m1 / (r * r);
			double v1 = std::sqrt(g1 * r1);
			double v2 = std::sqrt(g2 * r2);
			double T1 = 2 * pi * r1 / v1;
			double T2 = 2 * pi * r2 / v2;

			std::cout << "r1: " << r1 << " km" << std::endl;
			std::cout << "r2: " << r2 << " km" << std::endl;
			std::cout << "v1: " << v1 << " km/s" << std::endl;
			std::cout << "v2: " << v2 << " km/s" << std::endl;
			std::cout << "T1: " << T1 << " s" << std::endl;
			std::cout << "T2: " << T2 << " s" << std::endl;

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
