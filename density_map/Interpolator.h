#ifndef DSTORM_DENSITY_MAP_INTERPOLATOR_H
#define DSTORM_DENSITY_MAP_INTERPOLATOR_H

#include <dStorm/Image.h>
#include <memory>
#include <vector>
#include <Eigen/Core>

namespace dStorm {

class Localization;

namespace density_map {

template <int Dim>
struct Interpolator {
    typedef typename dStorm::ImageTypes<Dim>::Position Position;
    typedef std::auto_ptr< Interpolator > Ptr;
    struct ResultPoint {
        Position position;
        float relative_value;
        ResultPoint( Position p, float v ) : position(p), relative_value(v) {}
    };

    virtual ~Interpolator() {}
    void interpolate( 
        const dStorm::Localization& l, const Eigen::Array<float,Dim,1>& f,
        std::vector<ResultPoint>& target ) const 
        { interpolate_( l, f, target ); }
    Ptr clone() const { return Ptr( clone_() ); }
private:
    virtual void interpolate_( 
        const dStorm::Localization&, const Eigen::Array<float,Dim,1>&,
        std::vector<ResultPoint>& target ) const = 0;
    virtual Interpolator* clone_() const = 0;
};

}
}

#endif
