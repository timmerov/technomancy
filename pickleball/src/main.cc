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

for data collected 2023-08-24 of me serving - movie 5173 frame 0816.
unfortunately the lower left marker is blocked by a bucket.
ah well.
here are the x,y coordinates for the rest:

                pixels          feet
left top        280,542         -22,6
center          968,637         0,3
center bottom   966,732         0,0
right top       1652,545        22,6
right bottom    1653,733        22,0

the markers are on the wall.
the wall is 6'5" from the first court.
the first court is 20' wide.
the second court is 11'6" from the first court.
the second court is 20' wide.
the camera is on the net line on the far side of the second court.
the camera is 3' above the playing surface.
the camera is 20+11.5+20=51.5 feet from the plane the ball travels in.
the camera is 51.5+6.5=58 feet from the wall.

serves were made from the far corner of the first court.
serves were hit more/less directly down the first court sideline.
we can assume the serve is parallel to the wall.

pixel coordinates of the ball are extracted from the video frames.
these are converted by code to feet.
which is then scaled by the distance from the camera to the sideline (51.5 feet)
divided by the distance from the camera to the wall (58 feet).

it is assumed the camera is running at 30 fps.
this needs to be confirmed.
so every frame is 33.3 ms.

the vector of unknowns is defined as follows:
0: initial ball velocity - 60 fps
1: initial angle - 10 degrees
2: height of the ball at contact - 2 feet
3: t0 - time zero - the time the ball was hit - 0 seconds.
4: drag coefficient 0.40
5: effective lift coefficient (lift times spin, we don't know spin) - 0.75 (600 rpm / 60 min/sec * 0.075)

the data was collected on 2023-06-24 movie 5173 frames 806 through 829.
we have N data points consisting of x,y,t.
where t is the frame number (starting at 1) times .0333 seconds per frame.
the x,y points are extracted from the video.
the shutter speed is pretty long.
so the ball is a long streak in each image.
the center of the streak is used.

frame   x       y
1       246     662
2       320     646
3       392     633
4       464     622
5       532     612
6       no data
7       no data
8       730     593
9       793     590
10      855     589
11      915     588
12      973     590
13      1030    592
14      1086    596
15      1141    602
16      1194    609
17      1246    616
18      1298    626
19      1347    636
20      1395    647
21      1443    660
22      1488    673
23      1534    689
24      1577    704
25      no data

frames 5,6 are corrupted by the light pole.
maybe put something dark on it.

to do: automate the process by subtracting the reference frame and finding the centroid of the differences.

we don't have a way to measure spin.
but...
i have video of serves.
some of which catch the paddle just as it hits the ball.
one can extract the angle between the trajectory of the ball and the tangent to the motion of the paddle
at the point of contact.
it's about 14 degrees.
we will know the speed of the pickleball at contact.
we can assume this is the speed of the paddle normal to the face.
the angle then gives us the tangential speed of the paddle.
which we can assume is imparted as spin.
the tangential speed of the paddle divided by the circumference of the pickleball is the spin rate.
**/

#include <math.h>

#include <sstream>
#include <vector>

#include <eigen3/Eigen/Dense>

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>

class LevenbergMarquardt {
public:
    LevenbergMarquardt() = default;
    ~LevenbergMarquardt() = default;

    /** must set these. **/
    int ndata_points_ = 0;
    int nparams_ = 0;

    /** set this to the initial guess. **/
    Eigen::VectorXd solution_;

    /** set this to the target values. **/
    Eigen::VectorXd targets_;

    /** verbosity level **/
    enum class Verbosity {
        kQuiet,
        kResultsOnly,
        kDetailedResults,
        kIterations,
        kDebug
    };
    Verbosity verbosity_ = Verbosity::kResultsOnly;

    /** tweaking these are optional. **/
    int max_error_iters_ = 100;
    int max_lambda_iters_ = 100;
    double init_lambda_ = 1.0;
    double epsilon_ = 0.0001;
    double lambda_inc_ = 2.0;
    double lambda_dec_ = 0.5;
    double good_error_ = 0.01;
    double min_error_change_ = 0.0001;

    /** must implement this. **/
    virtual void make_prediction(
        const Eigen::VectorXd &solution,
        Eigen::VectorXd &predicted
    ) noexcept = 0;

    /** sove the problem. **/
    void solve() noexcept {
        Eigen::VectorXd predicted(ndata_points_);
        make_prediction(solution_, predicted);
        if (verbosity_ >= Verbosity::kIterations) {
            LOG("predicted = "<<predicted.transpose());
        }

        double error = calculate_error(predicted);
        if (verbosity_ >= Verbosity::kIterations) {
            LOG("error = "<<error);
        }

        double lambda = init_lambda_;
        Eigen::MatrixXd jacobian(ndata_points_, nparams_);
        Eigen::MatrixXd jacobian_transpose(nparams_, ndata_points_);
        Eigen::MatrixXd jacobian_squared(nparams_, nparams_);
        Eigen::MatrixXd diagonal(nparams_, nparams_);
        Eigen::MatrixXd inverse(nparams_, nparams_);
        Eigen::VectorXd residuals(ndata_points_);
        Eigen::VectorXd shift(nparams_);
        Eigen::VectorXd new_solution(nparams_);
        Eigen::VectorXd new_predicted(ndata_points_);
        bool done = false;

        for (int err_iter = 0; err_iter < max_error_iters_; ++err_iter) {
            if (done) {
                break;
            }
            if (error < good_error_) {
                break;
            }
            if (verbosity_ >= Verbosity::kIterations) {
                LOG("error iter = "<<err_iter);
            }

            calculate_jacobian(jacobian, epsilon_);
            if (verbosity_ >= Verbosity::kDebug) {
                LOG("jacobian = "<<jacobian);
            }

            jacobian_transpose = jacobian.transpose();
            if (verbosity_ >= Verbosity::kDebug) {
                LOG("jacobian_transpose = "<<jacobian_transpose);
            }

            jacobian_squared = jacobian_transpose * jacobian;
            if (verbosity_ >= Verbosity::kDebug) {
                LOG("jacobian_squared = "<<jacobian_squared);
            }

            diagonal = jacobian_squared;

            for (int lambda_iter = 0; lambda_iter < max_lambda_iters_; ++lambda_iter) {
                if (verbosity_ >= Verbosity::kIterations) {
                    LOG("lambda iter = "<<lambda_iter<<" lambda = "<<lambda);
                }

                for (int i = 0; i < nparams_; ++i) {
                    diagonal(i, i) = jacobian_squared(i,i) + lambda;
                }
                if (verbosity_ >= Verbosity::kDebug) {
                    LOG("diagonal = "<<diagonal);
                }

                inverse = diagonal.inverse();
                if (verbosity_ >= Verbosity::kDebug) {
                    LOG("inverse = "<<inverse);
                }

                residuals = targets_ - predicted;
                if (verbosity_ >= Verbosity::kDebug) {
                    LOG("residuals = "<<residuals.transpose());
                }

                shift = inverse * jacobian_transpose * residuals;
                if (verbosity_ >= Verbosity::kDebug) {
                    LOG("shift = "<<shift.transpose());
                }

                new_solution = solution_ + shift;
                if (verbosity_ >= Verbosity::kIterations) {
                    LOG("new_solution = "<<new_solution.transpose());
                }

                make_prediction(new_solution, new_predicted);

                double new_error = calculate_error(new_predicted);
                if (verbosity_ >= Verbosity::kIterations) {
                    LOG("new_error = "<<new_error);
                }

                if (new_error >= error) {
                    lambda *= lambda_inc_;
                    continue;
                }

                double change_error = error - new_error;
                if (change_error < min_error_change_) {
                    done = true;
                }

                lambda *= lambda_dec_;
                std::swap(solution_, new_solution);
                std::swap(predicted, new_predicted);
                error = new_error;
                break;
            }
        }

        /** results **/
        if (verbosity_ >= Verbosity::kResultsOnly
        &&  verbosity_ <= Verbosity::kDetailedResults) {
            LOG("error = "<<error);
            LOG("solution = "<<solution_.transpose());
        }

        /** brag **/
        if (verbosity_ >= Verbosity::kDetailedResults) {
            make_prediction(solution_, predicted);
            for (int i = 0; i < ndata_points_; ++i) {
                double p = predicted[i];
                double t = targets_[i];
                double d = p - t;
                LOG(i<<": predicted: "<<p<<" target: "<<t<<" diff: "<<d);
            }
        }
    }

    double calculate_error(
        const Eigen::VectorXd &predicted
    ) const noexcept {
        auto residuals = targets_ - predicted;
        double error = residuals.dot(residuals);
        return error;
    }

    void calculate_jacobian(
        Eigen::MatrixXd &jacobian,
        double epsilon
    ) noexcept {
        Eigen::VectorXd current_predicted(ndata_points_);
        make_prediction(solution_, current_predicted);

        auto solution = solution_;
        Eigen::VectorXd predicted(ndata_points_);
        for (int i = 0; i < nparams_; ++i) {
            double save = solution(i);
            solution(i) = save + epsilon;
            make_prediction(solution, predicted);
            solution(i) = save;

            jacobian.col(i) = (predicted - current_predicted) / epsilon;
        }
    }
};

/**
divine a transformation matrix M and translation vector T to convert pixel coordinates P
to real world x,y coordinates in feet W.
    W = M * P + T
the data set is markers (duct tape) at known positions on the wall
and their corresponding pixel locations in the images.
the transformation matrix M is:
    { a  b }
    { c  d }
the translation vector T is:
    { e }
    { f }
the solution_ input/output field of the LevenbergMarquardt class is:
    { a b c d e f }
**/
class TransformCoordinates : LevenbergMarquardt {
public:
    TransformCoordinates() = default;
    ~TransformCoordinates() = default;

    static constexpr int kNMarkers = 5;
    static constexpr int kNDataPoints = 2*kNMarkers;
    static constexpr int kNParams = 2*2 + 2;
    static constexpr double kEpsilon = 0.00001;
    static constexpr double kMinErrorChange = 0.00001;

    /** pixel coordinates of the markers. **/
    Eigen::VectorXd pixels_;

    /** outputs and used internally by the solver. **/
    Eigen::MatrixXd xform_;
    Eigen::VectorXd xlate_;

    void run() noexcept {
        init();
        solve();
        cleanup();
    }

    void init() noexcept {
        /** mandatory **/
        ndata_points_ = kNDataPoints;
        nparams_ = kNParams;
        /** configuration **/
        verbosity_ = Verbosity::kDetailedResults;
        epsilon_ = kEpsilon;
        min_error_change_ = kMinErrorChange;

        /** set the initial horrible guess: identity transform and no translation. **/
        solution_.resize(kNParams);
        solution_ << 1.0, 0.0, 0.0, 1.0, 0.0, 0.0;

        /** set the source and target values. **/
        pixels_.resize(kNDataPoints);
        targets_.resize(kNDataPoints);
        #include "data/2023-06-24-5173-0816.hpp"

        /** size the outputs. **/
        xform_.resize(2, 2);
        xlate_.resize(2);
    }

    /** transform all pixels to x,y using the solution. **/
    virtual void make_prediction(
        const Eigen::VectorXd &solution,
        Eigen::VectorXd &predicted
    ) noexcept {
        to_xform_xlate(solution);
        Eigen::VectorXd pixel(2);
        Eigen::VectorXd pred(2);
        for (int i = 0; i < kNDataPoints; i += 2) {
            pixel[0] = pixels_[i];
            pixel[1] = pixels_[i+1];
            pred = xform_ * pixel + xlate_;
            predicted[i] = pred[0];
            predicted[i+1] = pred[1];
        }
    }

    void to_xform_xlate(
        const Eigen::VectorXd &solution
    ) noexcept {
        xform_(0, 0) = solution[0];
        xform_(0, 1) = solution[1];
        xform_(1, 0) = solution[2];
        xform_(1, 1) = solution[3];
        xlate_(0) = solution[4];
        xlate_(1) = solution[5];
    }

    void cleanup() noexcept {
        /** release memory **/
        pixels_.resize(0);

        /** copy the solution to a well-defined place. **/
        to_xform_xlate(solution_);
    }
};

/**
divine a set of parameters for the physics model that match the observed positions.

params(6):
0: time at x=-22 ft = 0 seconds.
1: height at x=-22 ft = 2'
2: velocity at x=-22 ft = 52.0 mph.
3: angle at x=-22 ft = 10 degrees.
4: drag coefficient = 0.40.
5: effective lift = spin rate * lift coefficient = 500 rpm * 0.075.

we have data for a tracked pickleball: t, x, y
we assume t is measured perfectly.
and we predict x,y for every t.
otherwise it's kinda hard to calculate the error.
cause the error is dt^2 + dx^2 + dy^2.
and we kinda have a units problem.
we could scale dt by the velocity.
but which velocity: the initial guess, the instantaneous?
but that's probably complexity we don't need.
ergo, the targets are:
0: x
1: y
and we keep t in seconds separately.
assume the t's increase monotonically.

the transformed x,y positions put the ball on the wall.
but the ball is 6.5' closer to the camera.
the camera is 58' from the wall.
it's easy to write down the forward transformation:
observed = (actual -3') * 58'/51.5' + 3'
inverting that gives the tranformation we want:
actual = (observed - 3') * 51.5'/58' + 3'
**/
class PickleballServe : LevenbergMarquardt {
public:
    PickleballServe() = default;
    ~PickleballServe() = default;

    /** configuration of the solver. **/
    static constexpr int kNParams = 6;
    static constexpr double kEpsilon = 0.00001;
    static constexpr double kMinErrorChange = 0.00001;
    static constexpr double kFPS = 30;
    static constexpr double kFrameTime = 1.0/kFPS;

    /** transform from wall coordinates to sideline coordinates. **/
    static constexpr double kScalingFactor = 51.5 / 58.0;
    static constexpr double kCameraHeight = 3.0;

    /** pickleball physics. **/
    static constexpr double kX0 = -22.0; // ft
    static constexpr double kAirDensity = 0.075; /// lb/ft^3
    static constexpr double kDiameter = 2.9 / 12.0; /// ft
    static constexpr double kGravity = -32.17; /// ft/s^3 down
    static constexpr double kWeight = 0.0535; /// pounds
    static constexpr double kRadius = kDiameter / 2.0;
    static constexpr double kArea = M_PI * kRadius * kRadius;
    static constexpr double kVolume = 4.0 / 3.0 * kArea * kRadius;
    /**
    Fdrag = _(1/2 * p * A)_ * Cd * v^2
    Flift = _((4/3 * pi * r^3) * 4 * pi * p)_ * s * Cl * v
    **/
    static constexpr double kDragFactor = 0.5 * kAirDensity * kArea / kWeight;
    static constexpr double kLiftFactor = kVolume * 4.0 * M_PI * kAirDensity / kWeight;

    /** translate pixels to x,y coordinates **/
    Eigen::MatrixXd xform_;
    Eigen::VectorXd xlate_;

    class PositionData {
    public:
        double t_;
        double x_;
        double y_;
    };

    /** times in seconds. **/
    Eigen::VectorXd times_;

    /** used to advance the model one step. **/
    double t_ = 0.0;
    double x_ = 0.0;
    double y_ = 0.0;
    double vx_ = 0.0;
    double vy_ = 0.0;
    double drag_ = 0.0;
    double lift_ = 0.0;
    int end_pt_ = 0;
    double dt_ = 0.0;

    void run() noexcept {
        init();
        LOG("initial solution: "<<solution_.transpose());
        solve();
        LOG("final solution: "<<solution_.transpose());

        double y0 = solution_[1];
        double v0 = solution_[2];
        double angle = solution_[3];
        double cd = solution_[4];
        double effective_spin = solution_[5];

        /** convert units **/
        v0 = v0 / 5280 /*ft/mi*/ * 60 /*s/m*/ * 60 /*m/h*/;
        angle = angle * 180.0 / M_PI;
        double spin = effective_spin / 0.075 * 60 /*s/m*/;

        LOG("initial height  : "<<y0<<" ft");
        LOG("velocity        : "<<v0<<" mph");
        LOG("angle           : "<<angle<<" degrees");
        LOG("drag coefficient: "<<cd);
        LOG("spin            : "<<spin<<" rpm");
    }

    void init() noexcept {
        /** frame, x pixels, y pixels **/
        PositionData positions[] = {
            #include "data/2023-06-24-5173-0564-0588.hpp"
            //#include "data/2023-06-24-5173-0806-0829.hpp"
        };
        int npts = sizeof(positions) / sizeof(PositionData);
        //LOG("npts="<<npts);

        /** mandatory **/
        ndata_points_ = 2 * npts;
        nparams_ = kNParams;

        /** configuration **/
        verbosity_ = Verbosity::kIterations;
        epsilon_ = kEpsilon;
        min_error_change_ = kMinErrorChange;

        /** set the initial horrible guess: identity transform and no translation. **/
        solution_.resize(kNParams);
        solution_ <<
            0.0, // starting time
            2.0, // starting height
             // starting velocity
            52.0 /*mph*/ * 5280 /*ft/mi*/ / 60 /*m/h*/ / 60 /*s/m*/,
            // starting angle
            10.0 * M_PI / 180.0,
            0.40, // drag coefficient
            // effect lift = spin rate * lift coefficient
            500 /*rpm*/ / 60 /*m/s*/ * 0.075;

        /** set the source and target values. **/
        targets_.resize(ndata_points_);
        times_.resize(npts);
        Eigen::VectorXd pixel(2);
        Eigen::VectorXd xy(2);
        for ( int i = 0; i < npts; ++i) {
            auto& pos = positions[i];

            /** scale frame number to seconds. **/
            times_[i] = pos.t_ * kFrameTime;

            /** transform pixels to feet. **/
            pixel[0] = pos.x_;
            pixel[1] = pos.y_;
            xy = xform_ * pixel + xlate_;

            /** move the x,y positions from the wall to the sideline. **/
            int k = 2*i;
            targets_[k+0] = kScalingFactor * xy[0];
            targets_[k+1] = kScalingFactor * (xy[1] - kCameraHeight) + kCameraHeight;

            //LOG("wall x,y="<<xy[0]<<","<<xy[1]<<" scaled x,y="<<targets_[k+0]<<","<<targets_[k+1]);
        }
    }

    /**
    pickleball physics.
    the time step should move the ball about one diameter.
    divide the time interval to the next data point into a good number of steps.
    advance the state one step at a time.
    record the x,y positions.
    **/
    virtual void make_prediction(
        const Eigen::VectorXd &solution,
        Eigen::VectorXd &predicted
    ) noexcept {
        t_ = solution[0];
        x_ = kX0;
        y_ = solution[1];
        double v0 = solution[2];
        double theta = solution[3];
        vx_ = v0 * std::cos(theta);
        vy_ = v0 * std::sin(theta);
        /**
        Fdrag = (1/2 * p * A) * _Cd_ * v^2
        Flift = ((4/3 * pi * r^3) * 4 * pi * p) * _(s * Cl)_ * v
        **/
        drag_ = kDragFactor * solution[4];
        lift_ = kLiftFactor * solution[5];
        //LOG("drag_="<<drag_<<" lift_="<<lift_);

        //LOG("t="<<t_<<" x="<<x_<<" y="<<y_<<" vx="<<vx_<<" vy="<<vy_);

        int npts = times_.size();
        for (end_pt_ = 0; end_pt_ < npts; ++end_pt_) {
            interval();

            /** save the position. **/
            int k = 2 * end_pt_;
            predicted[k+0] = x_;
            predicted[k+1] = y_;
        }
    }

    /** advance the model to the time of the next data point. **/
    void interval() noexcept {
        double t1 = times_[end_pt_];
        double delta_t = t1 - t_;
        double delta_x = delta_t * vx_;
        int steps = (int) std::round(delta_x / kDiameter);
        if (steps < 1) {
            steps = 1;
        }
        dt_ = delta_t / steps;
        //LOG("end_pt_="<<end_pt_<<" t0="<<t_<<" t1="<<t1<<" steps="<<steps<<" dt="<<dt_);

        for (int i = 0; i < steps; ++i) {
            advance();
        }
    }

    /** advance the model one step. **/
    void advance() noexcept {

        /** acceleration due to drag and lift **/
        double v2 = vx_*vx_ + vy_*vy_;
        double v = std::sqrt(v2);
        /**
        Fdrag = (1/2 * p * A) * Cd * _(v^2)_
        Flift = ((4/3 * pi * r^3) * 4 * pi * p) * (s * Cl) * _v_

        drag is drag factor * v^2
        dragx is drag * vx / v
        cancel some v's.
        **/
        double drag_v = drag_ * v;
        /** drag is opposite to velocity. **/
        double dragx = - drag_v * vx_;
        double dragy = - drag_v * vy_;
        /** if vy > 0 then liftx > 0 **/
        /** if vy < 0 then liftx < 0 **/
        double liftx = + lift_ * vy_;
        /** if vx > 0 then lifty < 0 **/
        double lifty = - lift_ * vx_;
        //LOG("dragx="<<dragx<<" dragy="<<dragy<<" liftx="<<liftx<<" lifty="<<lifty);

        /**
        acceleration
        gravity is down = -32 ft/s^2.
        **/
        double ax = dragx + liftx;
        double ay = dragy + lifty + kGravity;
        //LOG("ax="<<ax<<" ay="<<ay);

        /** update variables **/
        t_ += dt_;
        double dt2 = dt_ * dt_;
        x_ += vx_ * dt_ + 0.5 * ax * dt2;
        y_ += vy_ * dt_ + 0.5 * ay * dt2;
        vx_ += ax * dt_;
        vy_ += ay * dt_;

        //LOG("t="<<t_<<" x="<<x_<<" y="<<y_<<" vx="<<vx_<<" vy="<<vy_);
    }
};

int main(
    int argc, char *argv[]
) noexcept {
    (void) argc;
    (void) argv;

    agm::log::init(AGM_TARGET_NAME ".log");

    TransformCoordinates tc;
    tc.run();

    PickleballServe pbs;
    pbs.xform_ = tc.xform_;
    pbs.xlate_ = tc.xlate_;
    pbs.run();

    return 0;
}
