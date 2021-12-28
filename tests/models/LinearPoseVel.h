#pragma once

#include <cnkalman/model.h>

struct LinearPoseVel : public cnkalman::KalmanModel {
    LinearPoseVel() : cnkalman::KalmanModel("LinearToy", 4){
        FLT _H[12] = {
                -1, 0, 0, 0,
                0, 1, 0, 0,
                -.5, .5, 0, 0
        };
        CnMat H = cnMat(3, 4, _H);

        measurementModels.emplace_back(std::make_unique<cnkalman::KalmanLinearMeasurementModel>(this, "meas", H));
        cnkalman_state_reset(&kalman_state);
    }

    FLT _F[16] = {
            1, 0, 1, 0,
            0, 1, 0, 1,
            0, 0, 1, 0,
            0, 0, 0, 1,
    };
    CnMat F = cnMat(4, 4, _F);
    void state_transition(FLT dt, CnMat& cF, const CnMat& x) override {
        cnCopy(&F, &cF, 0);
    }

    void process_noise(double dt, const CnMat &x, CnMat &Q_out) override {
        cn_set_zero(&Q_out);
        cnMatrixSet(&Q_out, 2, 2, .01);
        cnMatrixSet(&Q_out, 3, 3, .01);
    }

    void sample_state(double dt, const CnMat &x0, CnMat &x1) override {
        KalmanModel::sample_state(dt, x0, x1);
        FLT wall = 10;
        if(x1.data[0] > +wall && x1.data[2] > 0) x1.data[2] *= .9;
        if(x1.data[0] < -wall && x1.data[2] < 0) x1.data[2] *= .9;
        if(x1.data[1] > +wall && x1.data[3] > 0) x1.data[3] *= .9;
        if(x1.data[1] < -wall && x1.data[3] < 0) x1.data[3] *= .9;
    }
};