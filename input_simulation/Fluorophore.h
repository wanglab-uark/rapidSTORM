#ifndef LOCPREC_FLUOROPHORE_H
#define LOCPREC_FLUOROPHORE_H

#include <Eigen/StdVector>
#include <simparm/Eigen_decl.hh>
#include <simparm/BoostUnits.hh>
#include <dStorm/Localization.h>
#include <dStorm/engine/Image.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_matrix.h>
#include <simparm/Set.hh>
#include <simparm/Entry.hh>
#include <simparm/Entry_Impl.hh>
#include <simparm/Structure.hh>
#include <vector>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <iostream>
#include <boost/units/systems/si/activity.hpp>
#include <boost/units/systems/si/time.hpp>
#include <boost/units/power10.hpp>
#include <dStorm/traits/optics.h>
#include <dStorm/traits/Projection.h>
#include <dStorm/UnitEntries/Nanometre.h>

namespace input_simulation {

using namespace boost::units;

class _FluorophoreConfig : public simparm::Set {
  protected:
    void registerNamedEntries();
  public:
    _FluorophoreConfig();
    ~_FluorophoreConfig() {}

    simparm::Entry<unsigned long> countsPerPhoton;
    simparm::Entry<double> averageActivationTime, averageDeactivationTime;
    simparm::Entry<double> photonEmittanceRate;
    simparm::Entry<double> numerical_aperture, refractive_index;
    dStorm::FloatNanometreEntry wavelength;
};

typedef simparm::Structure<_FluorophoreConfig> FluorophoreConfig;

class Fluorophore {
  public:
    typedef dStorm::samplepos Position;
  private:
    Position pos;
    bool isOn;
    std::vector<int> history;
    quantity<si::time> restTime;
    quantity<si::time> av_t_on, av_t_off;
    quantity<si::activity> photonRate;
    int photonWeight;

    quantity<si::time> remaining_time_in_current_state(gsl_rng *rng);

    void initTimes( const FluorophoreConfig& c );
    
    typedef quantity< camera::length, int > PixelIndex;

    struct Plane {
        Eigen::MatrixXd densities;
        dStorm::traits::Projection::ImagePosition pixel, range;
        EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    };
    std::vector<Plane> planes;
  public:
    Fluorophore(const Position& pos, int noImages,
                const FluorophoreConfig&, const dStorm::input::Traits< dStorm::engine::ImageStack >& optics,
                const int fluorophore_index);
    Fluorophore(std::istream&, const FluorophoreConfig&);
    ~Fluorophore();

    /** @return Total number of photons emitted in image. */
    int glareInImage(gsl_rng *, dStorm::engine::ImageStack &targetImage, 
                     int imNum, quantity<si::time> integrationTime);
    /** @return Total number of photons emitted in image. */
    int wasOnInImage(int index) const { return history[index]; }

    Position::Scalar x() const { return pos[0]; }
    Position::Scalar y() const { return pos[1]; }
    const Position& position() const { return pos; }

    void recenter( Position new_position, const dStorm::traits::Projection& optics );

    friend std::ostream& operator<<(std::ostream&, const Fluorophore&);

  public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

}

#endif
