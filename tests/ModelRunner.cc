#include <vector>
#include <cmath>

#include "cnkalman/kalman.h"
#include "cnkalman/model.h"
#include "ModelRunner.h"


#include <iostream>
#include <cnmatrix/cn_matrix.h>
#include <fstream>

#if HAS_SCIPLOT
#include <sciplot/sciplot.hpp>

namespace sciplot::internal {
    template<>
    auto escapeIfNeeded<std::tuple<double, double>>(const std::tuple<double, double> &val) {
        return internal::str(std::get<0>(val)) + " " + internal::str(std::get<1>(val));
    }
}

#endif

static inline FLT linmath_enforce_range(FLT v, FLT mn, FLT mx) {
    if (v < mn)
        return mn;
    if (v > mx)
        return mx;
    return v;
}

static inline uint32_t create_rgb(const FLT* rgb) {
    FLT _r = rgb[0], _g = rgb[1], _b = rgb[2];
    uint8_t r = linmath_enforce_range(_r * 127 + 127, 0, 255);
    uint8_t g = linmath_enforce_range(_g * 127 + 127, 0, 255);
    uint8_t b = linmath_enforce_range(_b * 127 + 127, 0, 255);
    return 0xff << 24 | b << 16 | g << 8 | r;
}

static inline std::ostream& write_matrix(std::ostream& os, const char* name, const std::vector<std::tuple<FLT, FLT>>& m) {
    os << "\"" << name << "\": [" << std::endl;
    bool needsCommaA = false;
    for(auto& row : m) {
        if(needsCommaA) os << ",";
        needsCommaA = true;

        os << "\t[" << std::get<0>(row) << ", " << std::get<1>(row) << "]" << std::endl;
    }
    os << "]" << std::endl;
    return os;
}
static inline std::ostream& write_matrix(std::ostream& os, const std::vector<std::vector<FLT>>& m) {
    os << "[" << std::endl;
    bool needsCommaA = false;
    for(auto& row : m) {
        if(needsCommaA) os << ",";
        needsCommaA = true;

        bool needsCommaB = false;
        os << "\t[ ";
        for(auto& col : row) {
            if(needsCommaB) os << ",";
            needsCommaB = true;

            os << col;
        }
        os << "]" << std::endl;
    }
    os << "]" << std::endl;
    return os;
}
static inline std::ostream& write_matrix(std::ostream& os, const char* name, const std::vector<std::vector<FLT>>& m) {
    os << "\"" << name << "\":";
    return write_matrix(os, m);
}
static inline std::ostream& write_matrix(std::ostream& os, const char* name, const std::vector<std::vector<std::vector<FLT>>>& m) {
    os << "\"" << name << "\": [" << std::endl;
    bool needsCommaA = false;
    for(auto& matrix : m) {
        if(needsCommaA) os << ",";
        needsCommaA = true;
        write_matrix(os, matrix);
    }
    os << "]" << std::endl;
    return os;
}
static inline std::ostream& write_matrix(std::ostream& os, const CnMat& m) {
    os << "[" << std::endl;
    for(int i = 0;i < m.rows;i++) {
        if(i != 0) os << ", ";
        os << "\t[";
        for(int j = 0;j < m.cols;j++) {
            if(j != 0) os << ", ";
            os << m(i, j) << " ";
        }
        os << "]" << std::endl;
    }
    os << "]";
    return os;
}
static inline std::ostream& write_matrix(std::ostream& os, const char* name, const std::vector<CnMat>& m) {
    os << "\"" << name << "\": [" << std::endl;
    bool needsCommaA = false;
    for(auto& matrix : m) {
        if(needsCommaA) os << ",";
        needsCommaA = true;

        write_matrix(os, matrix);
    }
    os << "]" << std::endl;
    return os;
}
static inline std::ostream& write_matrix(std::ostream & os, const char* name, const CnMat* m) {
    os << "\"" << name << "\":" << std::endl;
    write_matrix(os, *m);
    return os;
}

void ModelRunner::Run(cnkalman::KalmanModel &model, bool show, const CnMat* iX, const CnMat *Q, std::vector<CnMat> mRs) {
    ModelPlot plotter(model.name);
    plotter.show = show;
    Run(plotter, model, iX, Q, mRs);
}
void ModelRunner::Run(ModelPlot& plotter, cnkalman::KalmanModel &model, const CnMat* iX, const CnMat *Q, std::vector<CnMat> mRs) {
    model.reset();

    std::vector<std::vector<std::vector<FLT>>> observations;
    std::vector<std::vector<FLT>> Xs;
    std::vector<std::vector<FLT>> Xfs;
    std::vector<std::tuple<double, double>> pts_GT;
    std::vector<std::tuple<double, double>> pts_Filtered;

    std::vector<double> orignorms, bestnorms, error;

    FLT deviations = 2;

    std::vector<CnMat> Rs;
    for (auto &measModel : model.measurementModels) {
        auto &R = Rs.emplace_back(measModel->default_R());
        measModel->meas_mdl.term_criteria.max_iterations = max_iterations;
        //measModel->meas_mdl.meas_jacobian_mode = cnkalman_jacobian_mode_debug;
    }

    //model.kalman_state.debug_transition_jacobian = 1;

    std::vector<std::vector<std::vector<FLT>>> covs;
    std::vector<double> norm;

    CN_CREATE_STACK_VEC(X, model.state_cnt);
    cnCopy(model.stateM, &X, 0);

    if(iX) {
        cnCopy(iX, model.stateM, 0);
    }

    FLT t = 1 - dt;
    plotter.include_point_in_range(model.state);
    plotter.include_point_in_range(_X);

    covs.emplace_back(cnMatToVectorVector(model.kalman_state.P));
    pts_Filtered.emplace_back(model.state[0], model.state[1]);
    pts_GT.emplace_back(X.data[0], X.data[1]);
    Xfs.push_back(cnMatToVector(*model.stateM));

    for (int i = 0; i <= iterations; i++) {
        t += dt;

        model.sample_state(dt, X, X, Q);
        plotter.include_point_in_range(X.data);

        Xs.push_back(cnMatToVector(X));
        pts_GT.emplace_back(X.data[0], X.data[1]);

        if (i % run_every != 0)
            continue;

        model.update(t);

        if (i % ellipse_step == 0) {
            //plot_cov(map, model, deviations, "blue");
        }

        FLT norm = 0, onorm = 0;

        std::vector<std::vector<FLT>> obs;
        std::vector<CnMat> Zs;
        for (int j = 0; j < (int) model.measurementModels.size(); j++) {
            auto &measModel = model.measurementModels[j];
            auto Z = cnMatCalloc(measModel->meas_cnt, 1);
            measModel->sample_measurement(X, Z, mRs.size() ? mRs[j] : Rs[j]);
            Zs.push_back(Z);
            obs.push_back(cnMatToVector(Z));
        }

        if (bulk_update) {
            model.bulk_update(t, Zs, Rs);
        } else {
            for (int j = 0; j < (int) model.measurementModels.size(); j++) {
                auto &measModel = model.measurementModels[j];
                auto Z = Zs[j];

                auto stats = measModel->update(t, Z, Rs[j]);
                norm += stats.bestnorm;
                onorm += stats.orignorm;
            }
        }

        for (int j = 0; j < (int) model.measurementModels.size(); j++) {
            auto &Z = Zs[j];
            free(Z.data);
        }

        observations.push_back(obs);
        plotter.include_point_in_range(model.state);

        Xfs.push_back(cnMatToVector(*model.stateM));
        pts_Filtered.emplace_back(model.state[0], model.state[1]);

        bestnorms.push_back(norm);
        orignorms.push_back(onorm);

        error.emplace_back(cnDistance(&X, model.stateM));

        if (i % ellipse_step == 0) {
            plotter.plot_cov(model, deviations);
        }
        covs.emplace_back(cnMatToVectorVector(model.kalman_state.P));
    }

    std::string suffix = "";
    if(!settingsName.empty()) suffix += "." + settingsName;
    std::ofstream f(model.name + suffix + ".kf");
    f << "{" << std::endl;
    model.write(f);
    f << ", " << std::endl;
    write_matrix(f, "Z", observations) << ", ";
    write_matrix(f, "X", Xs) << ", ";

    if(Q) {
        write_matrix(f, "Q", Q) << ", ";
    }

    write_matrix(f, "Xf", Xfs) << ", ";
    write_matrix(f, "Ps", covs) << ", ";
    write_matrix(f, "Rs", Rs) << ", ";
    f << "\"run_every\":" << run_every << ", " << std::endl;
    f << "\"dt\":" << dt << ", " << std::endl;
    f << "\"ellipse_step\":" << ellipse_step << "" << std::endl;
    f << "}";


#ifdef HAS_SCIPLOT

    //plotter.plot.drawWithVecs("lines", orignorms).label(settingsName + " Orig");
    plotter.plot.drawWithVecs("lines", bestnorms).label(settingsName + " Best");
    plotter.plot.drawWithVecs("lines", error).label(settingsName + " GT Error");

/*
    if (!model.measurementModels.empty() && model.measurementModels[0]->meas_cnt >= 3) {
        FILE *f = fopen("map.rgb", "w");
        for (int j = 0; j < 250; j++) {
            for (int i = 0; i < 250; i++) {
                FLT sx = i / 250. * w + x - w / 2;
                FLT sy = j / 250. * w + y - w / 2;
                X.data[0] = sx;
                X.data[1] = sy;

                auto &meas = model.measurementModels[0];
                if (meas->meas_cnt < 3) break;

                CN_CREATE_STACK_VEC(Z, MAX(meas->meas_cnt, 3));
                meas->predict_measurement(X, &Z, 0);

                uint32_t rgb32 = create_rgb(Z.data);
                fwrite(&rgb32, 1, 4, f);
            }
        }
        fclose(f);
        std::stringstream ss;
        ss << "'map.rgb' binary array=(250,250) center=(" << x << ", " << y << ") dx=" << (w / 250.)
           << " format='%uchar'";
    }
*/
    plotter.map.drawWithVecs("lines", pts_GT).label(settingsName + " GT");
    plotter.map.drawWithVecs("lines", pts_Filtered).label(settingsName + " Filter");
    model.draw(plotter.map);

#endif

    for (auto &R : Rs)
        free(R.data);

}

ModelRunner::ModelRunner(const std::string &settingsName, double dt, int iterations, int max_iterations, int runEvery, int ellipseStep,
                         bool bulkUpdate) : settingsName(settingsName), dt(dt), iterations(iterations), max_iterations(max_iterations),
                                            run_every(runEvery), ellipse_step(ellipseStep), bulk_update(bulkUpdate) {}

ModelPlot::ModelPlot(const std::string& name) : name(name){
#ifdef HAS_SCIPLOT
    plot.gnuplot("set title \"" + name + "\"");
    map.gnuplot("set title \"" + name + "\"");
    map.palette("jet");

    map.size(1600, 1600);
    map.gnuplot("set size square");
    map.border().none();

    plot.size(1600, 1200);
#endif
}

void ModelPlot::include_point_in_range(const double *X) {
    for(int x = 0;x < 2;x++) {
        range[x*2] = std::min(range[x*2], X[x]);
        range[x*2+1] = std::max(range[x*2+1], X[x]);
    }
}

void ModelPlot::include_point_in_range(FLT x, FLT y) {
    FLT Xs[] = { x, y};
    include_point_in_range(Xs);
}

ModelPlot::~ModelPlot() {

    FLT dx = fmax(1, range[1] - range[0]);
    FLT dy = fmax(1, range[3] - range[2]);
    range[0] -= .1 * dx;
    range[1] += .1 * dx;
    range[2] -= .1 * dy;
    range[3] += .1 * dy;
    FLT w = range[1] - range[0], h = range[3] - range[2];
    FLT x = range[1] - w / 2, y = range[3] - h / 2;
    w = fmax(w, h);
    range[0] = x - w / 2;
    range[1] = x + w / 2.;
    range[2] = y - w / 2;
    range[3] = y + w / 2.;
#ifdef HAS_SCIPLOT
    map.xrange(range[0], range[1]);
    map.yrange(range[2], range[3]);

    if (show) {
        plot.show();
        map.show();
    }
    map.save(name + ".png");
#endif
}

void ModelPlot::plot_cov(const cnkalman::KalmanModel &model, double deviations, const std::string &color) {
#ifdef HAS_SCIPLOT
        CN_CREATE_STACK_MAT(Pp, 2, 2);
        CN_CREATE_STACK_MAT(Evec, 2, 2);
        CN_CREATE_STACK_VEC(Eval, 2);
        static int idx = 1;

        cnCopy(&model.kalman_state.P, &Pp, 0);
        cnSVD(&Pp, &Eval, &Evec, 0, (enum cnSVDFlags)0);
        FLT angle = atan2(_Evec[2], _Evec[0]) *57.2958;
        FLT v1 = deviations*2*sqrt(_Eval[0]), v2 = deviations*2*sqrt(_Eval[1]);
        std::stringstream ss;
        ss << "set obj " << idx++ << " ellipse fc rgb \"" << color << "\" fs transparent solid .1 center "
           << (model.state[0]) << "," <<
           (model.state[1]) << " size "
           << v1 << "," << v2 <<
           " angle " << angle << " front\n";
        map.gnuplot(ss.str());
#endif
}
