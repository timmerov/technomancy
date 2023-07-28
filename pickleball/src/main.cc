/*
Copyright (C) 2012-2023 tim cotter. All rights reserved.
*/

/**
flight of the pickleball math.

credits and sources:
https://pickleballscience.org/pickleball-aerodynamic-drag/
https://pickleballscience.org/pickleball-topspin-aerodynamics/

Fdrag = 1/2 * Cd * p * A * v^2

Cd = coefficient of drag = 0.40
p = density of air = 0.075 lb / ft^3
A = cross sectional area of pickleball
d = diameter of pickleball = 2.9 in
g = gravity = 32.17 ft/s^3

Flift = Cl * 4/3 * 4 * pi^2 * r^3 * s * p * V

Cl = coefficient of lift = 0.075
s = spin rate of pickleball = 1200 - 1500 RPM average paddle average player
V = volume of pickleball

w = weight pickleball
**/

#include <math.h>

#include <sstream>

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>


class Pickleball {
public:
    Pickleball() = default;
    ~Pickleball() = default;

    double x0_ = -22.0; /// ft
    double x1_ = +22.0; /// ft
    double y0_ = +2.0; /// ft
    double drag_coeff_ = 0.40;
    double air_density_ = 0.075; /// lb/ft^3
    double diameter_ = 2.9 / 12.0; /// ft
    double gravity_ = 32.17; /// ft/s^3
    double lift_coeff_ = 0.075;
    double spin_ = 300.0 / 60.0; /// revolutions per second
    double weight_ = 0.0535; /// pounds
    int partitions_ = 200;  /// partition the distance into steps
    double radius_;
    double area_;
    double volume_;
    double drag_a2_;
    double lift_a1_;
    double step_;
    double v0_ = 52.0 * 5280 / 60 / 60;
    double theta_ = 10.0 * M_PI / 180.0;
    double vx_;
    double vy_;
    double x_;
    double y_;
    double elapsed_t_;

    void run() noexcept {
        init();

        headers();
        for (int i = 0; i <= partitions_; ++i) {
            interval();
        }
    }

    void init() noexcept {
        radius_ = diameter_ / 2.0;
        area_ = M_PI * radius_ * radius_;
        volume_ = 4.0 / 3.0 * area_ * radius_;
        drag_a2_ = 0.5 * drag_coeff_ * air_density_ * area_ / weight_;
        lift_a1_ = lift_coeff_ * volume_ * 4.0 * M_PI * spin_ * air_density_ / weight_;
        step_ = (x1_ - x0_) / double(partitions_);

        vx_ = v0_ * std::cos(theta_);
        vy_ = v0_ * std::sin(theta_);

        x_ = x0_;
        y_ = y0_;
        elapsed_t_ = 0;

        LOG("parameters:");
        LOG("x0:          "<<x0_<<" ft");
        LOG("x1:          "<<x1_<<" ft");
        LOG("y0:          "<<y0_<<" ft");
        LOG("drag_coeff:  "<<drag_coeff_);
        LOG("air_density: "<<air_density_<<" lbs/ft^3");
        LOG("diameter:    "<<diameter_<<" ft");
        LOG("gravity:     "<<gravity_<<" ft/s^2");
        LOG("lift_coeff:  "<<lift_coeff_);
        LOG("spin:        "<<spin_<<" rps");
        LOG("weight:      "<<weight_<<" lbs");
        LOG("radius:      "<<radius_<<" ft");
        LOG("area:        "<<area_<<" ft^2");
        LOG("volume:      "<<volume_<<" ft^3");
        LOG("");
        LOG("starting values:");
        LOG("v0:    "<<v0_<<" ft/s");
        LOG("theta: "<<theta_<<" radians");
        LOG("vx:    "<<vx_<<" ft/s");
        LOG("vy:    "<<vy_<<" ft/s");
        LOG("x:     "<<x_<<" ft");
        LOG("y:     "<<y_<<" ft");
    }

    void headers() noexcept {
        LOG("");
        LOG("intervals:");
        LOG("\tx\ty\tt\tvx\tvy\tdrag\tdragx\tdragy\tdt\tx\ty\tt\tvx\tvy");
    }

    void interval() noexcept {

        std::stringstream ss;

        /** deceleration due to drag **/
        double v2 = vx_*vx_ + vy_*vy_;
        double v = std::sqrt(v2);
        double drag = drag_a2_ * v2;
        double drag_v = drag / v;
        double dragx = drag_v * vx_;
        double dragy = drag_v * vy_;
        double liftx = - lift_a1_ * vy_;
        double lifty = - lift_a1_ * vx_;

        ss<<"\t"<<x_;
        ss<<"\t"<<y_;
        ss<<"\t"<<elapsed_t_;
        ss<<"\t"<<vx_;
        ss<<"\t"<<vy_;
        ss<<"\t"<<drag;
        ss<<"\t"<<dragx;
        ss<<"\t"<<dragy;

        /**
        time for pickleball to travel one step distance.
        - 1/2 a t^2 + v t - s = 0
        a = acceleration = drag_a2
        v = velocity = vx
        s = distance = step
        t = ( - v +/- sqrt(v^2 - 2 a s) ) / a
        t = ( vx - sqrt(vx^2 - 2 drag_a2 step) ) / drag_a2
        **/
        double disc = std::sqrt(vx_*vx_ - 2.0 * drag_a2_ * step_);
        double dt = (vx_ - disc) / drag_a2_;
        ss<<"\t"<<dt;

        /** acceleration **/
        double ax = - dragx + liftx;
        double ay = - gravity_ + lifty;
        if (vy_ > 0.0) {
            ay -= dragy;
        } else {
            ay += dragy;
        }

        /** update variables **/
        x_ += step_;
        y_ += vy_ * dt + 0.5 * ay * dt * dt;
        elapsed_t_ += dt;
        vx_ += ax * dt;
        vy_ += ay * dt;

        ss<<"\t"<<x_;
        ss<<"\t"<<y_;
        ss<<"\t"<<elapsed_t_;
        ss<<"\t"<<vx_;
        ss<<"\t"<<vy_;
        /** print a row of data **/
        LOG(ss.str());
    }
};

int main(
    int argc, char *argv[]
) noexcept {
    (void) argc;
    (void) argv;

    agm::log::init(AGM_TARGET_NAME ".log");

    Pickleball pb;
    pb.run();
    return 0;
}
