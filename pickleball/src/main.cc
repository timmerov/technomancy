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

for data collected 2023-08-24 of me serving.
unfortunately the lower left marker is blocked by a bucket.
ah well.
here are the x,y coordinates for the rest:

                pixels          feet
left top        432,996         -22,6
center          1501,1144       0,3
center bottom   1499,1292       0,0
right top       2566,1001       22,6
right bottom    2568,1293       22,0

the markers are on the wall.
the wall is 6'5" from the first court.
the first court is 20' wide.
the second court is 11'6" from the first court.
the second court is 20' wide.
the camera is on the net line on the far side of the second court.
the camera is 6.5+20+11.5+20=58 feet from the wall.

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

class PickleballServe {
public:
    PickleballServe() = default;
    ~PickleballServe() = default;

    class PositionData {
    public:
        double t_;
        double x_;
        double y_;
    };

    std::vector<PositionData> positions_;

    void run() noexcept {
        init();
    }

    void init() noexcept {
        /** frame, x pixels, y pixels **/
        PositionData positions[] = {
            { 1, 246, 662 },
            { 2, 320, 646 },
            { 3, 392, 633 },
            { 4, 464, 622 },
            { 5, 532, 612 },
            { 8, 730, 593 },
            { 9, 793, 590 },
            { 10, 855, 589 },
            { 11, 915, 588 },
            { 12, 973, 590 },
            { 13, 1030, 592 },
            { 14, 1086, 596 },
            { 15, 1141, 602 },
            { 16, 1194, 609 },
            { 17, 1246, 616 },
            { 18, 1298, 626 },
            { 19, 1347, 636 },
            { 20, 1395, 647 },
            { 21, 1443, 660 },
            { 22, 1488, 673 },
            { 23, 1534, 689 },
            { 24, 1577, 704 }
        };

        for (auto pos : positions) {
            positions_.push_back(pos);
        }
        int sz = positions_.size();
        LOG("positions.size="<<sz);
    }
};

class TransformCoordinates {
public:
    TransformCoordinates() = default;
    ~TransformCoordinates() = default;

    static constexpr int kNMarkers = 5;
    static constexpr int kNDataPoints = 2*kNMarkers;
    static constexpr int kNParams = 2*2 + 2;
    static constexpr int kMaxErrorIters = 100;
    static constexpr int kMaxLambdaIters = 100;
    static constexpr double kInitLambda = 1.0;
    static constexpr double kEpsilon = 0.00001;
    static constexpr double kLambdaInc = 2.0;
    static constexpr double kLambdaDec = 0.5;
    static constexpr double kGoodError = 0.01;
    static constexpr double kMinErrorChange = 0.00001;

    Eigen::VectorXd pixels_;
    Eigen::VectorXd markers_;
    Eigen::VectorXd xform_;  /** 0..3 are matrix 4..5 are translation **/

    void run() noexcept {
        init();

        /** start with a horrible guess. **/
        Eigen::VectorXd current_guess(kNParams);
        current_guess << 1.0, 0.0, 0.0, 1.0, 0.0, 0.0;
        LOG("current_guess = "<<current_guess.transpose());

        Eigen::VectorXd predicted(kNDataPoints);
        make_prediction(current_guess, predicted);
        LOG("predicted = "<<predicted.transpose());

        double error = calculate_error(predicted);
        LOG("error = "<<error);

        double lambda = kInitLambda;
        Eigen::MatrixXd jacobian(kNDataPoints, kNParams);
        Eigen::MatrixXd jacobian_transpose(kNParams, kNDataPoints);
        Eigen::MatrixXd jacobian_squared(kNParams, kNParams);
        Eigen::MatrixXd diagonal(kNParams, kNParams);
        Eigen::MatrixXd inverse(kNParams, kNParams);
        Eigen::VectorXd residuals(kNDataPoints);
        Eigen::VectorXd shift(kNParams);
        Eigen::VectorXd new_guess(kNParams);
        Eigen::VectorXd new_predicted(kNDataPoints);
        bool done = false;

        for (int err_iter = 0; err_iter < kMaxErrorIters; ++err_iter) {
            if (done) {
                break;
            }
            if (error < kGoodError) {
                break;
            }
            LOG("error iter = "<<err_iter);

            calculate_jacobian(current_guess, jacobian, kEpsilon);
            //LOG("jacobian = "<<jacobian);

            jacobian_transpose = jacobian.transpose();
            //LOG("jacobian_transpose = "<<jacobian_transpose);

            jacobian_squared = jacobian_transpose * jacobian;
            //LOG("jacobian_squared = "<<jacobian_squared);

            diagonal = jacobian_squared;

            for (int lambda_iter = 0; lambda_iter < kMaxLambdaIters; ++lambda_iter) {
                LOG("lambda iter = "<<lambda_iter<<" lambda = "<<lambda);

                for (int i = 0; i < kNParams; ++i) {
                    diagonal(i, i) = jacobian_squared(i,i) + lambda;
                }
                //LOG("diagonal = "<<diagonal);

                inverse = diagonal.inverse();
                //LOG("inverse = "<<inverse);

                residuals = markers_ - predicted;
                //LOG("residuals = "<<residuals.transpose());

                shift = inverse * jacobian_transpose * residuals;
                //LOG("shift = "<<shift.transpose());

                new_guess = current_guess + shift;
                LOG("new_guess = "<<new_guess.transpose());

                make_prediction(new_guess, new_predicted);

                double new_error = calculate_error(new_predicted);
                LOG("new_error = "<<new_error);

                if (new_error >= error) {
                    lambda *= kLambdaInc;
                    continue;
                }

                double change_error = error - new_error;
                if (change_error < kMinErrorChange) {
                    done = true;
                }

                lambda *= kLambdaDec;
                std::swap(current_guess, new_guess);
                std::swap(predicted, new_predicted);
                error = new_error;
                break;
            }
        }

        /** brag **/
        make_prediction(current_guess, predicted);
        for (int i = 0; i < kNDataPoints; ++i) {
            LOG(i<<": predicted: "<<predicted[i]<<" actual: "<<markers_[i]);
        }

        /** store the transformation matrix where the creator can access it. **/
        std::swap(xform_, current_guess);

        exit();
    }

    void init() noexcept {
        pixels_.resize(kNDataPoints);
        pixels_ <<
             432.0,  996.0,
            1501.0, 1144.0,
            1499.0, 1292.0,
            2566.0, 1001.0,
            2568.0, 1293.0;
        markers_.resize(kNDataPoints);
        markers_ <<
            -22.0, 6.0,
              0.0, 3.0,
              0.0, 0.0,
             22.0, 6.0,
             22.0, 0.0;
    }

    void make_prediction(
        const Eigen::VectorXd &guess,
        Eigen::VectorXd &predicted
    ) const noexcept {
        Eigen::MatrixXd xform(2, 2);
        xform(0, 0) = guess[0];
        xform(0, 1) = guess[1];
        xform(1, 0) = guess[2];
        xform(1, 1) = guess[3];
        Eigen::VectorXd xlate(2);
        xlate(0) = guess[4];
        xlate(1) = guess[5];
        Eigen::VectorXd pixel(2);
        Eigen::VectorXd pred(2);
        for (int i = 0; i < kNDataPoints; i += 2) {
            pixel[0] = pixels_[i];
            pixel[1] = pixels_[i+1];
            pred = xform * pixel + xlate;
            predicted[i] = pred[0];
            predicted[i+1] = pred[1];
        }
    }

    double calculate_error(
        const Eigen::VectorXd &predicted
    ) const noexcept {
        auto residuals = markers_ - predicted;
        double error = residuals.dot(residuals);
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

    void exit() noexcept {
        /** release memory **/
        pixels_.resize(0);
        markers_.resize(0);
    }
};

class MathTest {
public:
    MathTest() = default;
    ~MathTest() = default;

    const int kNDataPoints = 3;
    const int kNParams = 2;
    const int kMaxErrorIters = 100;
    const int kMaxLambdaIters = 100;
    const double kInitLambda = 1.0;
    const double kLambdaInc = 2.0;
    const double kLambdaDec = 0.5;
    const double kGoodError = 0.01;
    const double kMinErrorChange = 0.00001;

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

        double lambda = kInitLambda;
        Eigen::MatrixXd jacobian(kNDataPoints, kNParams);
        Eigen::MatrixXd jacobian_transpose(kNParams, kNDataPoints);
        Eigen::MatrixXd jacobian_squared(kNParams, kNParams);
        Eigen::MatrixXd diagonal(kNParams, kNParams);
        Eigen::MatrixXd inverse(kNParams, kNParams);
        Eigen::VectorXd residuals(kNDataPoints);
        Eigen::VectorXd shift(kNParams);
        Eigen::VectorXd new_guess(kNParams);
        Eigen::VectorXd new_predicted(kNDataPoints);
        bool done = false;

        for (int err_iter = 0; err_iter < kMaxErrorIters; ++err_iter) {
            if (done) {
                break;
            }
            if (error < kGoodError) {
                break;
            }
            LOG("error iter = "<<err_iter);

            calculate_jacobian(current_guess, jacobian, 0.001);
            //LOG("jacobian = "<<jacobian);

            jacobian_transpose = jacobian.transpose();
            //LOG("jacobian_transpose = "<<jacobian_transpose);

            jacobian_squared = jacobian_transpose * jacobian;
            //LOG("jacobian_squared = "<<jacobian_squared);

            diagonal = jacobian_squared;

            for (int lambda_iter = 0; lambda_iter < kMaxLambdaIters; ++lambda_iter) {
                LOG("lambda iter = "<<lambda_iter<<" lambda = "<<lambda);

                for (int i = 0; i < kNParams; ++i) {
                    diagonal(i, i) = jacobian_squared(i,i) + lambda;
                }
                //LOG("diagonal = "<<diagonal);

                inverse = diagonal.inverse();
                //LOG("inverse = "<<inverse);

                residuals = data_y_- predicted;
                //LOG("residuals = "<<residuals.transpose());

                shift = inverse * jacobian_transpose * residuals;
                //LOG("shift = "<<shift.transpose());

                new_guess = current_guess + shift;
                //LOG("new_guess = "<<new_guess.transpose());

                make_prediction(new_guess, new_predicted);

                double new_error = calculate_error(new_predicted);
                LOG("new_error = "<<new_error);

                if (new_error >= error) {
                    lambda *= kLambdaInc;
                    continue;
                }

                double change_error = error - new_error;
                if (change_error < kMinErrorChange) {
                    done = true;
                }

                lambda *= kLambdaDec;
                std::swap(current_guess, new_guess);
                std::swap(predicted, new_predicted);
                error = new_error;
                break;
            }
        }
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

    /*MathTest mt;
    mt.run();*/

    /*Pickleball pb;
    pb.run();*/

    /*PickleballServe pbs;
    pbs.run();*/

    TransformCoordinates tc;
    tc.run();

    return 0;
}
