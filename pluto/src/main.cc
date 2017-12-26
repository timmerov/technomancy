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
double is 64 bits (8 bytes)
long double is 128 bits (16 bytes)
long double is much slower and no more accurate.
weird.
**/
//typedef long double fp;
typedef double fp;

/**
configurations
**/
	const int kYears = 1;
	const int kTimeIntervals = 10*1000*1000;

/**
numbers from wikipedia
**/
	const fp kPlutoRadiusKm = 1188.3;
	const fp kPlutoMassKg = 1.303e22;
	const fp kCharonRadiusKm = 606.0;
	const fp kCharonMassKg =1.586e21;
	const fp kSeparationKm = 19571;
	const fp kCharonBarycenterKm = 17536;
	const fp kPeriodDays = 6.3872304;
	const fp kBigGM3pKgpS = 6.67408e-11;

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

update formulas:
	: position: x += v*dt + 0.5*a*dt^2
	: velocity: v += a*dt
**/

	class Vector2 {
	public:
		fp x_;
		fp y_;
	};

	class Object {
	public:
		Vector2 pos_;
		Vector2 vel_;
		Vector2 acc_;
		fp gm_;
	};

    class Pluto {
    public:
        Pluto() = default;
        Pluto(const Pluto &) = default;
        ~Pluto() = default;

		fp pi_ = 3.14;
		Object pluto_;
		Object charon_;
		fp periodS_ = 0.0;

        void run() noexcept {
			std::cout << "Hello, World!" << std::endl;

			pi_ = std::acos(-1.0);

			setup();
			process();

			std::cout << "Goodbye, World!" << std::endl;
        }

        void setup() noexcept {
			fp p_volume = 4.0/3.0*pi_*kPlutoRadiusKm*kPlutoRadiusKm*kPlutoRadiusKm;
			fp c_volume = 4.0/3.0*pi_*kCharonRadiusKm*kCharonRadiusKm*kCharonRadiusKm;
			fp p_density = kPlutoMassKg / p_volume;
			fp c_density = kCharonMassKg / c_volume;
			fp avg_density = (p_density + c_density) / 2.0;

			fp pc_radius = kPlutoRadiusKm / kCharonRadiusKm;
			fp pc_mass = kPlutoMassKg / kCharonMassKg;

			fp pc_sep_p_radius = kSeparationKm / kPlutoRadiusKm;
			fp pc_sep_c_radius = kSeparationKm / kCharonRadiusKm;

			fp period_hours = kPeriodDays * 24.0;
			fp period_minutes = period_hours * 60.0;
			fp period_seconds = period_minutes * 60.0;

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
			std::cout << "Separation             : " << kSeparationKm << " km" << std::endl;
			std::cout << "Separation Pluto  Radii: " << pc_sep_p_radius << std::endl;
			std::cout << "Separation Charon Radii: " << pc_sep_c_radius << std::endl;
			std::cout << "Period: " << kPeriodDays << " days" << std::endl;
			std::cout << "Period: " << period_hours << " hours" << std::endl;
			std::cout << "Period: " << period_minutes << " minutes" << std::endl;
			std::cout << "Period: " << period_seconds << " seconds" << std::endl;

			init_objects();

			fp r1 = pluto_.pos_.y_;
			fp r2 = - charon_.pos_.y_;
			fp v1 = - pluto_.vel_.x_;
			fp v2 = charon_.vel_.x_;
			fp T1 = 2 * pi_ * r1 / v1;
			fp T2 = 2 * pi_ * r2 / v2;

			std::cout << "r1: " << r1 << " km" << std::endl;
			std::cout << "r2: " << r2 << " km" << std::endl;
			std::cout << "v1: " << v1 << " km/s" << std::endl;
			std::cout << "v2: " << v2 << " km/s" << std::endl;
			std::cout << "T1: " << T1 << " s" << std::endl;
			std::cout << "T2: " << T2 << " s" << std::endl;

			periodS_ = T1;
        }

        void init_objects() noexcept {
			fp m1 = kPlutoMassKg;
			fp m2 = kCharonMassKg;
			fp m = m1 + m2;
			fp r = kSeparationKm;
			fp r1 = r * m2 / m;
			fp r2 = r * m1 / m;
			fp G = kBigGM3pKgpS / 1e9; /// units
			fp gm1 = G * m2;
			fp gm2 = G * m1;
			fp g1 = gm1 / (r * r);
			fp g2 = gm2 / (r * r);
			fp v1 = std::sqrt(g1 * r1);
			fp v2 = std::sqrt(g2 * r2);

			pluto_.pos_.x_ = 0.0;
			pluto_.pos_.y_ = r1;
			pluto_.vel_.x_ = - v1;
			pluto_.vel_.y_ = 0.0;
			pluto_.acc_.x_ = 0.0;
			pluto_.acc_.y_ = - g1;
			pluto_.gm_ = gm1;

			charon_.pos_.x_ = 0.0;
			charon_.pos_.y_ = - r2;
			charon_.vel_.x_ = v2;
			charon_.vel_.y_ = 0.0;
			charon_.acc_.x_ = 0.0;
			charon_.acc_.y_ = g2;
			charon_.gm_ = gm2;
        }

        void process() noexcept {
			fp dt = periodS_ / fp(kTimeIntervals);

			print_objects();
			fp e0 = print_energy();

			for (int k = 0; k < kYears; ++k) {
				for (int i = 0; i < kTimeIntervals; ++i) {
					update_object(pluto_, dt);
					update_object(charon_, dt);
					update_gravity();
				}

				print_objects();
				fp e1 = print_energy();
				fp err = (e1 - e0) / e0;
				std::cout << "Energy Drift: " << err << std::endl;
			}
        }

        void print_objects() noexcept {
			std::cout << "Pluto .pos.x=" << pluto_.pos_.x_ << " km" << std::endl;
			std::cout << "Pluto .pos.y=" << pluto_.pos_.y_ << " km" << std::endl;
			std::cout << "Pluto .vel.x=" << pluto_.vel_.x_ << " km/s" << std::endl;
			std::cout << "Pluto .vel.y=" << pluto_.vel_.y_ << " km/s" << std::endl;
			std::cout << "Pluto .acc.x=" << pluto_.acc_.x_ << " km/s^2" << std::endl;
			std::cout << "Pluto .acc.y=" << pluto_.acc_.y_ << " km/s^2" << std::endl;
			std::cout << "Charon.pos.x=" << charon_.pos_.x_ << " km" << std::endl;
			std::cout << "Charon.pos.y=" << charon_.pos_.y_ << " km" << std::endl;
			std::cout << "Charon.vel.x=" << charon_.vel_.x_ << " km/s" << std::endl;
			std::cout << "Charon.vel.y=" << charon_.vel_.y_ << " km/s" << std::endl;
			std::cout << "Charon.acc.x=" << charon_.acc_.x_ << " km/s^2" << std::endl;
			std::cout << "Charon.acc.y=" << charon_.acc_.y_ << " km/s^2" << std::endl;
        }

        fp print_energy() noexcept {
			fp m1 = kPlutoMassKg;
			fp m2 = kCharonMassKg;
			fp v1_2 = pluto_.vel_.x_*pluto_.vel_.x_ + pluto_.vel_.y_*pluto_.vel_.y_;
			fp v2_2 = charon_.vel_.x_*charon_.vel_.x_ + charon_.vel_.y_*charon_.vel_.y_;
            fp ke1 = 0.5 * m1 * v1_2;
            fp ke2 = 0.5 * m2 * v2_2;
			fp G = kBigGM3pKgpS / 1e9; /// units
			fp dx = pluto_.pos_.x_ - charon_.pos_.x_;
			fp dy = pluto_.pos_.y_ - charon_.pos_.y_;
            fp r2 = dx*dx + dy*dy;
			fp r = std::sqrt(r2);
            fp pe = - G * m1 * m2 / r;
            fp energy = ke1 + ke2 + pe;
            std::cout << "Pluto  Kinetic Energy: " << ke1 << std::endl;
            std::cout << "Charon Kinetic Energy: " << ke2 << std::endl;
            std::cout << "Potential Energy     : " << pe << std::endl;
            std::cout << "Total Energy         : " << energy << std::endl;
            return energy;
        }

        void update_object(
			Object& obj,
			fp dt
		) noexcept {
			fp dvx = obj.acc_.x_*dt;
			fp dvy = obj.acc_.y_*dt;
            obj.pos_.x_ += (obj.vel_.x_ + 0.5*dvx) * dt;
            obj.pos_.y_ += (obj.vel_.y_ + 0.5*dvy) * dt;
            obj.vel_.x_ += dvx;
            obj.vel_.y_ += dvy;
		}

		void update_gravity() noexcept {
            fp dx = pluto_.pos_.x_ - charon_.pos_.x_;
            fp dy = pluto_.pos_.y_ - charon_.pos_.y_;
            fp r2 = dx*dx + dy*dy;
			fp r = std::sqrt(r2);
            fp g1 = pluto_.gm_ / r2;
            fp g2 = charon_.gm_ / r2;
            fp dxr = dx / r;
            fp dyr = dy / r;
            pluto_.acc_.x_ = - g1 * dxr;
            pluto_.acc_.y_ = - g1 * dyr;
            charon_.acc_.x_ = g2 * dxr;
            charon_.acc_.y_ = g2 * dyr;
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
