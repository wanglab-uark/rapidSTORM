#ifndef DSTORM_LOCALIZATIONS_H
#define DSTORM_LOCALIZATIONS_H

#include <memory>
#include <stdexcept>
#include <iostream>
#include <dStorm/data-c++/VectorList.h>
#include <dStorm/Localization.h>
#include <dStorm/input/LocalizationTraits.h>

namespace dStorm {
namespace output {
   /** The Localizations class represents a number of localizations
    *  with common width and height. */
   class Localizations : public data_cpp::VectorList<Localization> {
      private:
        input::Traits<Localization> t;

      public:
        Localizations() {}
        Localizations(const input::Traits<Localization>& t) : t(t) {}
        virtual ~Localizations();

        pixel_count dimx() const { return t.size.x(); }
        pixel_count dimy() const { return t.size.y(); }
        void setDim(const input::Traits<Localization>& t) 
            { this->t = t; }
   };
}
}

#endif
