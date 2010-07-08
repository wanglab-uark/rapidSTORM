#define DSTORM_SIGMAGUESSER_CPP
#include "debug.h"
#include "engine/SigmaGuesser.h"
#include <dStorm/engine/Input.h>
#include <dStorm/engine/Image.h>
#include <fit++/Exponential2D.hh>
#include <limits>
#include <boost/units/io.hpp>

#include <cassert>
#include <math.h>

#include "studentPinv.h"
#include "engine/SigmaFitter.h"

#include <sstream>

using namespace std;
using namespace fitpp;

namespace dStorm {
namespace engine {

void (*SigmaGuesser_fitCallback)(double , double, double, int , bool) = NULL;

SigmaGuesserMean::SigmaGuesserMean(Config &c)
: OutputObject("SigmaGuesser", "Standard deviation estimator"),
  config(c), fitter(new SigmaFitter(c)),
  status("Status", "Std. dev. estimation")
{
    nextCheck = 23;
    deleteAllResults();

    stringstream ss;
    ss << "Trying " << c.sigma_x() << ", " << c.sigma_y() << 
          " with correlation " << c.sigma_xy();
    status = ss.str();
    status.editable = false;

    push_back( status );
}
SigmaGuesserMean::~SigmaGuesserMean() {}

Output::Result
SigmaGuesserMean::receiveLocalizations(const EngineResult& er)

{
    if (defined_result != KeepRunning) return defined_result;
    ost::MutexLock lock(mutex);
    if (defined_result != KeepRunning) return defined_result;
    DEBUG("Adding fits");

    for (int i = 0; i < er.number; i++)
        meanAmplitude.addValue(er.first[i].getStrength());

    DEBUG("Updated mean amplitude");
    int old_n = n;
    double sigmas[4];
    for (int i = 0; i < er.number; i++) {
        if (meanAmplitude.mean() < er.first[i].getStrength()) {
            DEBUG("Fitting " << i);
            bool good = fitter->fit(*er.source, er.first[i], sigmas);
            DEBUG("Fitted " << i);
            if (good) {
                for (int i = 0; i < 3; i++) {
                    DEBUG("Guess for " << i << " is " << sigmas[i+((i==2)?1:0)]);
                    data[i].addValue(sigmas[i+((i==2)?1:0)]);
                }
                n++;
            } 
        }
    }
    discarded += n - old_n;
    DEBUG("Fitted data");

    DEBUG("Have " << n << " of " << nextCheck);
    if (n >= nextCheck) {
        Output::Result r = check();
        nextCheck = 3*nextCheck/2;
#ifndef NDEBUG
        if (SigmaGuesser_fitCallback) {
            if (r == RestartEngine || r == RemoveThisOutput)
                SigmaGuesser_fitCallback(
                    config.sigma_x() / cs_units::camera::pixel,
                    config.sigma_y() / cs_units::camera::pixel,
                    config.sigma_xy(), discarded, 
                    ( r == RemoveThisOutput) );
        }
#endif
        return r;
    } else
        return KeepRunning;
}

Output::Result
SigmaGuesserMean::check() {
    DEBUG("Checking result");
    int converged = 0, failed = 0;
    double t_term = studentPinv95(n-1);
    for (int i = 0; i < 3; i++) {
        double confidence = t_term * sqrt( data[i].variance() / n );
        double confInterval[2];
        double curVal = data[i].mean();
        confInterval[0] = curVal - confidence, confInterval[1] = curVal + confidence;

        if ( (confInterval[0] >= accept[i][0] &&
              confInterval[1] <= accept[i][1]) ) 
        {
            DEBUG("Sigma " << i << " converged at "
                     << (accept[i][0] + accept[i][1])/2);
            converged++;
        } else if ( 
             confInterval[0] > decline[i][1] ||
             confInterval[1] < decline[i][0] )
        {
            double newValue = curVal;

            DEBUG("Sigma " << i << " changed to " << newValue);
            if ( i == 0 )
                config.sigma_x = float(newValue) * cs_units::camera::pixel;
            else if ( i == 1 )
                config.sigma_y = float(newValue) * cs_units::camera::pixel;
            else
                config.sigma_xy = newValue;
                    
            failed++;
        } else {
            DEBUG("Sigma " << i << " undecided at " << curVal);
        }
    }
    DEBUG("Checked result");
    if (failed) {
        stringstream ss;
        ss << "Trying " << config.sigma_x() << ", " << config.sigma_y() << 
            " with correlation " << config.sigma_xy();
        status = ss.str();

        defined_result = RestartEngine;
        nextCheck = n+1;
        DEBUG("Significant difference");
    } else if (converged == 3) {
        nextCheck = numeric_limits<int>::max();
        DEBUG("Insignificant difference");
        defined_result = RemoveThisOutput;
    } else {
        nextCheck = (3*n)/2;
        DEBUG("No significance. Next check at " << nextCheck);
    }

    return defined_result;
}

void SigmaGuesserMean::deleteAllResults() {
    ost::MutexLock lock(mutex);
    DEBUG("Resetting entries");
    double sigmas[3] = {
        config.sigma_x() / cs_units::camera::pixel,
        config.sigma_y() / cs_units::camera::pixel,
        config.sigma_xy() };
    for (int i = 0; i < 3; i++) {
        double ds = config.delta_sigma() / ((i == 2) ? 2 : 1);
        data[i].reset();
        accept[i][0] = sigmas[i] - ds;
        accept[i][1] = sigmas[i] + ds;
        decline[i][0] = sigmas[i] - 0.5 * ds;
        decline[i][1] = sigmas[i] + 0.5 * ds;
    }
    n = discarded = 0;
    defined_result = KeepRunning;
    meanAmplitude.reset();

    DEBUG("Resetting fitter");
    fitter->useConfig(config);
    DEBUG("Deleted all results");
}

}
}
