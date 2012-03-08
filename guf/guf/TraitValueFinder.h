#ifndef DSTORM_FITTER_GUF_TRAIT_VALUE_FINDER_H
#define DSTORM_FITTER_GUF_TRAIT_VALUE_FINDER_H

#include <dStorm/engine/JobInfo.h>
#include "FitAnalysis.h"
#include "Config.h"
#include <dStorm/engine/Input.h>
#include <dStorm/traits/optics.h>
#include <dStorm/engine/InputTraits.h>
#include <boost/variant/get.hpp>
#include "guf/psf/parameters.h"
#include <limits>

namespace dStorm {
namespace guf {

struct TraitValueFinder {
    const dStorm::engine::JobInfo& info;
    const dStorm::traits::Optics& plane;
    const boost::optional<traits::Optics::PSF> psf;
    const bool is_3d;

  public:
    typedef void result_type;
    TraitValueFinder( 
        const dStorm::engine::JobInfo& info, const dStorm::traits::Optics& plane );

    template <int Dim, typename Structure>
    void operator()( PSF::BestSigma<Dim> p, Structure& m ) const 
        { m(p) = (*psf)[Dim]; }
    template <int Dim, typename Structure, int Term>
    void operator()( PSF::DeltaSigma<Dim,Term> p, Structure& m ) const {
        if ( is_3d )
            m(p) = boost::get<traits::Polynomial3D>(*info.traits.depth_info)
                .get_slope( static_cast<dStorm::Direction>(Dim), Term );
    }

    template <int Dim, typename Structure>
    void operator()( PSF::ZPosition<Dim> p, Structure& m ) const { 
        if ( is_3d )
            m( p ) = (*plane.z_position)[Dim];
    }

    template <typename Structure>
    void operator()( PSF::Prefactor  p, Structure& m ) const {
        m(p) = plane.transmission_coefficient(info.fluorophore); 
    }
};

}
}

#endif
