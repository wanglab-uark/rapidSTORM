#include "Factory.h"
#include "no_analysis/main.h"
#include "residue_analysis/main.h"
#include "fitter/residue_analysis/main.h"
#include <dStorm/output/Traits.h>
#include "fitter/SizeSpecializing_impl.h"
#include "fitter/MarquardtConfig_impl.h"
#include "fitter/residue_analysis/Config_impl.h"

namespace dStorm {
namespace gauss_2d_fitter {

using engine::SpotFitter;
using fitter::SizeSpecializing;

Factory::Factory() 
: simparm::Structure<Config>(),
  SpotFitterFactory( static_cast<Config&>(*this) )
{
}

Factory::Factory(const Factory& c)
: simparm::Structure<Config>(c), 
  SpotFitterFactory( static_cast<Config&>(*this) )
{
}

Factory::~Factory() {
}

template <int FitFlags,bool HonorCorrelation>
std::auto_ptr<SpotFitter>
instantiate(const Config& c, const engine::JobInfo& i)
{
    return fitter::residue_analysis::Fitter<
            residue_analysis::Fitter<FitFlags,HonorCorrelation> >
        ::select_fitter(c,i);
};

using namespace fitpp::Exponential2D;

std::auto_ptr<SpotFitter> 
Factory::make (const engine::JobInfo &i)
{
    bool correlation_negligible = sigma_xy_negligible_limit() 
        >= abs( i.config.sigma_xy() );

    if ( freeSigmaFitting() )
        if ( fixCorrelationTerm() )
            if ( correlation_negligible )
                return instantiate<FreeForm_NoCorrelation,false>(*this,i);
            else
                return instantiate<FreeForm_NoCorrelation,true>(*this, i);
        else 
            return instantiate<FreeForm,true>( *this, i );
    else if ( correlation_negligible )
        return instantiate<FixedForm,false>( *this, i );
    else
        return instantiate<FixedForm,true>( *this, i );
}

void Factory::set_requirements( input::Traits<engine::Image>& t ) {
    t.photon_response.require( deferred::JobTraits );
    t.background_stddev.require( deferred::JobTraits );
}

void Factory::set_traits( output::Traits& rv, const engine::JobInfo& info ) {
    fitter::residue_analysis::Config::set_traits(rv);
    rv.covariance_matrix_is_set = freeSigmaFitting();
    rv.z_coordinate_is_set = false;

    if ( info.traits.photon_response.is_promised(deferred::JobTraits)
         && info.traits.background_stddev.is_promised(deferred::JobTraits) ) 
    {
        rv.uncertainty_is_set = true;
    }
}

}
}

