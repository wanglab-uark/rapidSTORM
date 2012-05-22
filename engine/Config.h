#ifndef DSTORM_CONFIG_H
#define DSTORM_CONFIG_H

#include <simparm/Eigen_decl.hh>
#include <simparm/BoostUnits.hh>
#include <simparm/BoostOptional.hh>
#include <simparm/Eigen.hh>
#include <simparm/Set.hh>
#include <simparm/Entry.hh>
#include <simparm/NodeHandle.hh>
#include <dStorm/UnitEntries.h>
#include <simparm/Entry.hh>
#include <simparm/ChoiceEntry.hh>
#include <simparm/ChoiceEntry_Impl.hh>
#include <simparm/ManagedChoiceEntry.hh>
#include <boost/units/cmath.hpp>
#include <dStorm/output/Basename_decl.h>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <dStorm/Engine.h>
#include <dStorm/engine/Image_decl.h>
#include <dStorm/output/LocalizedImage_decl.h>
#include <dStorm/input/Source.h>
#include <dStorm/engine/SpotFinder.h>
#include <dStorm/engine/SpotFitterFactory.h>
#include <dStorm/units/nanolength.h>
#include <boost/units/systems/camera/length.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

namespace dStorm {
namespace engine {
   using namespace simparm;
   using boost::units::quantity;
   namespace si = boost::units::si;
   namespace camera = boost::units::camera;

   /** Config entry collection class for the dSTORM engine. */
   class Config
   {
        simparm::Set name_object;
      public:
        Config();
        ~Config();

        typedef Eigen::Matrix< quantity<camera::length,int>, 2, 1, Eigen::DontAlign > 
            PixelVector2D;
        Entry< PixelVector2D > nms;

        /** The method to use for spot detection. */
        simparm::ManagedChoiceEntry<spot_finder::Factory> spotFindingMethod;

        simparm::Set weights;
        boost::ptr_vector< simparm::Entry<float> > spot_finder_weights;

        /** The method to use for spot fitting. */
        simparm::ManagedChoiceEntry< spot_fitter::Factory > spotFittingMethod;
        
        /** Continue fitting until this number of bad fits occured. */
        Entry<unsigned long> motivation;
        /** Amplitude threshold to judge localizations by. */
        simparm::Entry< bool > guess_threshold;
        simparm::Entry< float > threshold_height_factor;
        simparm::Entry< boost::units::quantity<
            camera::intensity, float> > amplitude_threshold;

        void addSpotFinder( std::auto_ptr<spot_finder::Factory> factory );
        void addSpotFinder( spot_finder::Factory* factory ) 
            { addSpotFinder(std::auto_ptr<spot_finder::Factory>(factory)); }
        void addSpotFitter( std::auto_ptr<spot_fitter::Factory> factory );
        void addSpotFitter( spot_fitter::Factory* factory ) 
            { addSpotFitter(std::auto_ptr<spot_fitter::Factory>(factory)); }

        void set_variables( output::Basename& ) const;
        void attach_ui( simparm::Node& );

        simparm::NodeHandle weights_insertion_point;
   };

}
}

#endif
