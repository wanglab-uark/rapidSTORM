#include "engine/SigmaFitter.h"
#include <CImg.h>
#include <fit++/FitFunction_impl.hh>
#include <fit++/Exponential2D_impl.hh>
#include <fit++/Exponential2D_Correlated_Derivatives.hh>

namespace Eigen {
    template <>
    class NumTraits<unsigned short>
        : public NumTraits<int> {};
}

using namespace std;
using namespace fitpp;

namespace dStorm {
namespace engine {

SigmaFitter::SigmaFitter(Config &config)
: msx(-1), msy(-1)
{
    fitter.reset( new Fitting::FitObject<StormPixel>()  );
    fitter->set_absolute_epsilon<Exponential2D::SigmaX>
        ( config.delta_sigma()/5 );
    fitter->set_absolute_epsilon<Exponential2D::SigmaY>
        ( config.delta_sigma()/5 );
    fitter->set_absolute_epsilon<Exponential2D::SigmaXY>
        ( config.delta_sigma()/5 );

    useConfig(config);
}

SigmaFitter::~SigmaFitter() {}

void SigmaFitter::useConfig(Config &config) {
    initial_sigmas[0] = config.sigma_x();
    initial_sigmas[1] = config.sigma_y();
    initial_sigmas[2] = config.sigma_xy();

    prefac = 2 * M_PI * config.sigma_x() * config.sigma_y() *
                  sqrt( 1 - config.sigma_xy() * config.sigma_xy() );
    /* This parameter is set independently of the fit mask size because
     * small fit masks have a very detrimental effect on free-form fitting. */
    int nmsx = int((3*config.sigma_x()));
    int nmsy = int((3*config.sigma_y()));
    if (fitter.get() == NULL || nmsx != msx || nmsy != msy) {
        msx = nmsx; msy = nmsy;
        fitter->setSize( 2*msx+1, 2*msy+1 );
    }
}

static const double sigmaTol = 2;

bool SigmaFitter::fit(const cimg_library::CImg<StormPixel> &i,
    const Localization &f, double dev[4]) 

{
    double cx = f.getPreciseX(), cy = f.getPreciseY();
    int cxr = round(cx), cyr = round(cy);
    /* Reject localizations too close to image border. */
    if ( cxr < msx || cyr < msy 
         || cxr >= int(i.width-msx) || cyr >= int(i.height-msy) )
        return false;

    fitter->setData(i.ptr(), i.width, i.height);
    fitter->setUpperLeftCorner( cxr-msx, cyr-msy );
    
    fitter->setMeanX<0>(cx);
    fitter->setMeanY<0>(cy);
    fitter->setSigmaX<0>(initial_sigmas[0]);
    fitter->setSigmaY<0>(initial_sigmas[1]);
    fitter->setSigmaXY<0>(initial_sigmas[2]);
    fitter->setShift( fitter->getCorner(-1, -1) );
    fitter->setAmplitude<0>( f.getStrength() );
    fitter->fit();

    double amp = abs(fitter->getAmplitude<0>()),
           nsigmaX = abs(fitter->getSigmaX<0>()),
           nsigmaY = abs(fitter->getSigmaY<0>());
    dev[0] = nsigmaX;
    dev[1] = nsigmaY;
    dev[2] = amp;
    dev[3] = fitter->getSigmaXY<0>();
    if (amp < f.getStrength() / 10 || amp > f.getStrength() * 10
        || (nsigmaX < initial_sigmas[0] / sigmaTol) 
        || (nsigmaX > initial_sigmas[0] * sigmaTol) 
        || (nsigmaY < initial_sigmas[1] / sigmaTol) 
        || (nsigmaY > initial_sigmas[1] * sigmaTol)
        || (fitter->getSigmaXY<0>() > 1) 
        || (fitter->getSigmaXY<0>() < -1) )
        return false;
    else {
        return true;
    }
}

}
}
