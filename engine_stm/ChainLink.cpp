#include "debug.h"

#include "ChainLink.h"
#include "LocalizationBuncher.h"
#include <dStorm/input/chain/MetaInfo.h>
#include <dStorm/input/chain/Filter_impl.h>
#include <dStorm/input/chain/EngineHelpers.h>
#include <dStorm/output/LocalizedImage_traits.h>
#include <dStorm/ImageTraits.h>

#include <boost/mpl/vector.hpp>
#include <dStorm/output/LocalizedImage_decl.h>
#include <dStorm/localization/record.h>
#include <dStorm/Localization_decl.h>

namespace dStorm {

namespace engine_stm {

class ChainLink::Visitor {
  public:
    typedef boost::mpl::vector<output::LocalizedImage,localization::Record,dStorm::Localization>
        SupportedTypes;

    const Config& config;
    typedef std::auto_ptr< input::Source<output::LocalizedImage> > SourceResult;
    boost::shared_ptr<input::BaseTraits> new_traits;
    SourceResult new_source;

    Visitor(const Config& config ) : config(config) {}

    template <typename Type>
    bool operator()( input::Traits<Type>& source_traits ) { return true; }
    template <typename Type>
    bool operator()( const input::Traits<Type>& source_traits )
        { new_traits.reset( new input::Traits<output::LocalizedImage>(source_traits, "STM", "Localizations") ); 
          return true; }
    template <typename Type>
    inline bool operator()( std::auto_ptr< input::Source< Type > > p );

    bool operator()( input::chain::MetaInfo& ) { return true; }

    bool unknown_trait(std::string trait_desc) const {
        return false;
    }
    void unknown_base_source() const {
        throw std::runtime_error("Localization replay engine cannot process input of the given type");
    }
};

template <>
bool ChainLink::Visitor::operator()( std::auto_ptr< input::Source< output::LocalizedImage > > p )
{
    DEBUG("Passing through source");
    new_source = p;
    return true;
}

template <typename Type>
bool ChainLink::Visitor::operator()( std::auto_ptr< input::Source< Type > > input )
{
    DEBUG("Adapting source");
    new_source.reset( new Source<Type>( config, input ) );
    return true;
}

input::BaseSource* ChainLink::makeSource() 
{
    return input::chain::DelegateToVisitor::makeSource(*this);
}

ChainLink::AtEnd
ChainLink::traits_changed(TraitsRef r, Link* l)
{
    return input::chain::DelegateToVisitor::traits_changed(*this, r, l);
}

std::auto_ptr<input::chain::Link>
make_STM_engine_link()
{
    return std::auto_ptr<input::chain::Link>( new ChainLink( ) );
}

}
}
