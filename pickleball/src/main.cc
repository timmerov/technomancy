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

w = weight pickleball = 0.0536 lb

http://www.physics.usyd.edu.au/~cross/TRAJECTORIES/42.%20Ball%20Trajectories.pdf

Cl = 1 / (2 + v/vs)

vs = velocity of surface of the pickleball

https://twu.tennis-warehouse.com/learning_center/pickleball/pickleballspin.php

the ball leaves the paddle at 10 degrees.
the sweet spot to shoulder angle is about 35 degrees.
so the paddle is travelling about 25 degrees from the direction the ball is launched.
the above websit says typical spin rates top out at 1000 rpm.
and obviously depend on contact angle.
for 25 degress the spin rate is theoretically and observed to be 450 to 500 depending on paddle.
interesting.
so maybe 300 rpms isn't too crazy.

so we have a model that determines how a pickleball moves through space over time.
the function depends on a number of parameters.
including: v0, theta, effective spin, drag coefficient.
these are the parameters pk.
we have some data points xj yj that we pull from the video.
these are the data points di
the model produces a vector of estimated data points mi.
we have an error function which is the sum of the squares of the errors of the differences between
the data points and the predictions from the model.
s = sumi( (di - mi)^2 )
the sum is a minimum when the gradients are zero.
2 sumi( (di - m)*dmi/dpk ) = 0
to solve we need the jacobian J.
and its transpose Jt.
Jik = delta(mi) / delta(pk)
in other words...
we're going to modify each pk by a small amount...
run the model...
and see how the data points change.
then from levenberg-marquardt, we have to solve this set of equations:
(Jt*J + lambda*I) * dp = Jt * (d - m)
where lambda is a small factor that we tweak each iteration.
I is the identity matrix.
dp is the "shift vector" that we're going to add to p to hopefully converge on a solution.
d is the vector of the di.
m is the vector of the mi.
p is the vector of the pk.
**/

#include <math.h>

#include <sstream>

#include <eigen3/Eigen/Dense>

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>

class MathTest {
public:
    MathTest() = default;
    ~MathTest() = default;

    const int kNDataPoints = 3;
    const int kNParams = 2;

    Eigen::VectorXd data_x_;
    Eigen::VectorXd data_y_;

    void run() noexcept {
        /**
        let's solve y = mx + b with two x,y points chosen at random.
        **/
        data_x_ = Eigen::VectorXd::Random(kNDataPoints);
        data_y_ = Eigen::VectorXd::Random(kNDataPoints);
        LOG("data_x = "<<data_x_.transpose());
        LOG("data_y = "<<data_y_.transpose());

        Eigen::VectorXd current_guess = Eigen::VectorXd::Random(kNParams);
        LOG("current_guess = "<<current_guess.transpose());

        Eigen::VectorXd predicted(kNDataPoints);
        make_prediction(current_guess, predicted);

        double error = calculate_error(predicted);
        LOG("error = "<<error);

        /**
        repeat the following
        for (int j = 0; j < kMaxErrorIterations; ++j) { //=100?
            if (error < kGoodEnough) {
                break;
            }
        ***/
            Eigen::MatrixXd jacobian(kNDataPoints, kNParams);
            calculate_jacobian(current_guess, jacobian, 0.001);
            LOG("jacobian = "<<jacobian);

            Eigen::MatrixXd jacobian_transpose = jacobian.transpose();
            LOG("jacobian_transpose = "<<jacobian_transpose);

            Eigen::MatrixXd jacobian_squared = jacobian_transpose * jacobian;
            LOG("jacobian_squared = "<<jacobian_squared);

            /**
            repeat the following
            for (int k = 0; k < kMaxLambdaIterations; ++k) { //=100?
            **/
                double lambda = 1.0;
                Eigen::MatrixXd diagonal = jacobian_squared;
                for (int i = 0; i < kNParams; ++i) {
                    diagonal(i, i) += lambda;
                }
                LOG("diagonal = "<<diagonal);

                Eigen::MatrixXd inverse = diagonal.inverse();
                LOG("inverse = "<<inverse);

                Eigen::VectorXd residuals = data_y_- predicted;
                LOG("residuals = "<<residuals.transpose());

                Eigen::VectorXd shift = inverse * jacobian_transpose * residuals;
                LOG("shift = "<<shift.transpose());

                Eigen::VectorXd new_guess = current_guess + shift;
                LOG("new_guess = "<<new_guess.transpose());

                Eigen::VectorXd new_predicted(kNDataPoints);
                make_prediction(new_guess, new_predicted);

                double new_error = calculate_error(new_predicted);
                LOG("new_error = "<<new_error);

                /**
                if (new_error > error) {
                    lambda *= kIncreaseLambda; //=2?
                    continue;
                }
                else {
                    lambda *= kDecreaseLambda; //=0.5?
                    current_guess = new_guess;
                    predicted = new_predicted;
                    error = new_error;
                    break;
                }
                **/
    }

    void make_prediction(
        const Eigen::VectorXd &guess,
        Eigen::VectorXd &predicted
    ) const noexcept {
        double m = guess(0);
        double b = guess(1);
        for (int i = 0; i < kNDataPoints; ++i) {
            double x = data_x_(i);
            predicted(i) = m*x + b;
        }
    }

    double calculate_error(
        const Eigen::VectorXd &predicted
    ) const noexcept {
        double error = 0.0;
        for (int i = 0; i < kNDataPoints; ++i) {
            double r = predicted(i) - data_y_(i);
            error += r*r;
        }
        return error;
    }

    void calculate_jacobian(
        const Eigen::VectorXd &current_guess,
        Eigen::MatrixXd &jacobian,
        double epsilon
    ) const noexcept {
        Eigen::VectorXd current_predicted(kNDataPoints);
        make_prediction(current_guess, current_predicted);

        auto guess = current_guess;
        Eigen::VectorXd predicted(kNDataPoints);
        for (int i = 0; i < kNParams; ++i) {
            guess(i) += epsilon;
            make_prediction(guess, predicted);
            guess(i) -= epsilon;

            jacobian.col(i) = (predicted - current_predicted) / epsilon;
        }
    }
};

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
        compute_drag_coefficient();

        /*headers();
        for (int i = 0; i <= partitions_; ++i) {
            interval();
        }*/
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

    void compute_drag_coefficient() noexcept {
        /**
        for this experiment we recorded dropping a pickleball from a height of 57".
        repeated 12 times.
        average time to fall was 0.5736 seconds = 17.03 frames.

        the frame when the fingers first started to move counted as an entire frame.
        the frame when the ball lands was counted as an entire frame.
        they should really be counted as half a frame each.
        so the time to fall should be 0.5403 seconds.
        which is less than the 0.5434 seconds it takes the ball to fall neglecting air resistance.
        0.5434 seconds is 16.3 frames.
        ergo...
        our measurements are not sufficiently accurate to measure the drag coefficient of a pickleball.
        **/
        double height = 57.0 / 12.0; /// ft
        double drag_coeff = 0.0; /// 0.40; /// 2.16;
        double dt = 0.00001; /// s
        double drag_a2 = 0.5 * drag_coeff * air_density_ * area_ / weight_;
        LOG("drag_a2="<<drag_a2);

        double v = 0.0;
        double d = height;
        for (int i = 0; i < 100000; ++i) {
            double a = gravity_ - drag_a2 * v * v;
            double dd = v * dt + 0.5 * a * dt * dt;
            v += a * dt;
            d -= dd;
            double t = dt * double(i);
            LOG("t="<<t<<"s  d="<<d<<"ft  v="<<v<<"ft/s  a="<<a<<"ft/s^2");
            if (d < 0) {
                break;
            }
        }
    }
};

int main(
    int argc, char *argv[]
) noexcept {
    (void) argc;
    (void) argv;

    agm::log::init(AGM_TARGET_NAME ".log");

    MathTest mt;
    mt.run();

    /*Pickleball pb;
    pb.run();*/

    return 0;
}
