#ifndef DSTORM_OUTPUT_BINNING_BINNING_H
#define DSTORM_OUTPUT_BINNING_BINNING_H

#include "binning_decl.h"

#include "../Output.h"
#include "../../ImageTraits.h"
#include <boost/units/quantity.hpp>
#include <boost/units/systems/camera/length.hpp>
#include "../../helpers/DisplayDataSource_decl.h"

namespace dStorm {
namespace output {
namespace binning {

struct Unscaled {
    virtual ~Unscaled();
    virtual Unscaled* clone() const = 0;
    virtual void announce(const Output::Announcement& a) = 0;
    virtual traits::ImageResolution resolution() const = 0;
    virtual void bin_points( const output::LocalizedImage&, float* target, int stride ) const = 0;
    virtual float bin_point( const Localization& ) const = 0;

    virtual int field_number() const = 0;
};

struct Scaled
: public Unscaled
{
    virtual ~Scaled() {}
    virtual Scaled* clone() const = 0;
    virtual float get_size() const = 0;
    virtual std::pair< float, float > get_minmax() const = 0;
    virtual double reverse_mapping( float ) const = 0;
};

struct UserScaled
: public Scaled
{
    virtual ~UserScaled() {}
    virtual UserScaled* clone() const = 0;
    virtual void set_user_limit( bool lower_limit, const std::string& s ) = 0;
    virtual bool is_bounded() const = 0;
    virtual Display::KeyDeclaration key_declaration() const = 0;
};

}
}
}

#endif