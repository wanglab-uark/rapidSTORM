#include "Basename.h"
#include <dStorm/input/Source_impl.h>
#include <dStorm/input/InputMutex.h>
#include <dStorm/input/BasenameChange.h>

namespace dStorm {
namespace input {
namespace Basename {

Config::Config()
: simparm::Object("OutputBasename", "Set output basename"),
  output("Basename", "Output file basename", "")
{
    output.helpID = "OutputBasename";
}

void Config::registerNamedEntries() {
    push_back( output );
}

ChainLink::ChainLink() 
: input::chain::Forwarder(),
  simparm::Listener( simparm::Event::ValueChanged ),
  default_output_basename(""),
  user_changed_output(false)
{
}

void ChainLink::traits_changed( TraitsRef traits, Link *l )
{
    Link::traits_changed(traits,l); 

    if ( traits.get() == NULL )  {
        default_output_basename = "";
        this->traits.reset();
    } else {
        this->traits.reset( traits->clone() );
        if ( traits->suggested_output_basename.unformatted()() != "" )
            default_output_basename = traits->suggested_output_basename.unformatted()();
    }

    if ( user_changed_output ) {
        this->traits->suggested_output_basename.unformatted() = output();
        traits->get_signal< BasenameChange >()( this->traits->suggested_output_basename );
    } else {
        output = default_output_basename;
    }
    /* Check that no recursive call triggered by a signal happened */
    if ( upstream_traits() == traits ) 
        update_current_meta_info(this->traits);
}

void ChainLink::operator()(const simparm::Event&)
{
    ost::MutexLock lock( global_mutex() );
    if ( output() == "" ) output = default_output_basename;
    user_changed_output = ( output() != "" && output() != default_output_basename );
    if ( traits.get() ) {
        traits->suggested_output_basename.unformatted() = output();
        traits->get_signal< BasenameChange >()( traits->suggested_output_basename );
        update_current_meta_info( this->traits );
    }
}

std::auto_ptr<chain::Link> makeLink() {
    return std::auto_ptr<chain::Link>( new ChainLink() );
}

}
}
}
