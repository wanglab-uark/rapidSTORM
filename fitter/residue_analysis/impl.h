#ifndef FITTER_RESIDUEANALYSIS_IMPL_H
#define FITTER_RESIDUEANALYSIS_IMPL_H
#include "main.h"
#include <fit++/FitFunction_impl.hh>
#include <dStorm/engine/Image.h>
#include <dStorm/engine/Config.h>
#include <dStorm/engine/JobInfo.h>
#include <dStorm/engine/Spot.h>
#include <dStorm/Localization.h>
#include <dStorm/engine/Input.h>
#include "Config.h"

#include <boost/units/io.hpp>

template <typename Ty>
inline Ty sq(const Ty& a) { return a*a; }

namespace dStorm {
namespace fitter {
namespace residue_analysis {

template <class BaseInvariants>
CommonInfo<BaseInvariants>::CommonInfo(
    const Config& config, 
    const engine::JobInfo& info
) 
: BaseInvariants(config, info),
  asymmetry_threshold( config.asymmetry_threshold() ),
  required_peak_distance_sq( 
    sq( (config.required_peak_distance() * 1E-9 * boost::units::si::meter)
        * (info.traits.resolution[0]->in_dpm() + info.traits.resolution[1]->in_dpm()) * 0.5 / camera::pixel ) )
{
}

template <class BaseInvariants>
void
CommonInfo<BaseInvariants>::set_two_kernel_improvement( 
    Localization& l,  float value )
{
    l.two_kernel_improvement() = value;
}

template <class BaseInvariants>
bool
CommonInfo<BaseInvariants>::peak_distance_small( 
    typename BaseInvariants::DoubleFit *variables
) {
    return BaseInvariants::sq_peak_distance(variables) < required_peak_distance_sq;
}

}
}
}

#endif