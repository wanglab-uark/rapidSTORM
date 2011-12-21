#include "debug.h"

#include "Config.h"
#include <simparm/ChoiceEntry_Iterator.hh>
#include <algorithm>
#include <functional>
#include "chain/Link.h"
#include "join.h"
#include "InputMethods.h"
#include <dStorm/input/chain/Forwarder.h>

namespace dStorm {
namespace input {

struct Config::InputChainBase 
: public chain::Forwarder
{
    simparm::Set input_config;
    InputChainBase() : input_config("Input", "Input options") {}

    InputChainBase* clone() const { return new InputChainBase(); }
    AtEnd traits_changed( TraitsRef t, Link* l ) 
        { chain::Link::traits_changed(t,l);
          return notify_of_trait_change(t); }
    BaseSource* makeSource() { return Forwarder::makeSource(); }
    std::string name() const { return "InputChainBase"; }
    std::string description() const { return "InputChainBase"; }
    void registerNamedEntries( simparm::Node& node ) {
        chain::Forwarder::registerNamedEntries( input_config );
        node.push_back( input_config );
    }
};

Config::Config()
: method( join::create_link( std::auto_ptr<chain::Link>( new InputMethods() ) ) ),
  terminal_node( new InputChainBase() )
{
    terminal_node->set_more_specialized_link_element( method.get() );
}

Config::Config(const Config& c)
: method( c.method->clone() ),
  terminal_node( new InputChainBase(*c.terminal_node) )
{
    terminal_node->set_more_specialized_link_element( method.get() );

    for ( boost::ptr_list<chain::Link>::const_iterator 
          i = c.forwards.begin(); i != c.forwards.end(); ++i )
    {
        add_filter( std::auto_ptr<chain::Link>( i->clone() ) );
    }
}

Config::~Config() {
    terminal_node->set_more_specialized_link_element( NULL );
    dynamic_cast< dStorm::input::chain::Forwarder& >(forwards.back()).set_more_specialized_link_element( NULL );
}

void Config::add_method( std::auto_ptr<chain::Link> method, chain::Link::Place p )
{
    this->method->insert_new_node( method, p );
}

void Config::add_filter( std::auto_ptr<chain::Link> forwarder, bool front )
{
    if ( front && ! forwards.empty() ) {
        dynamic_cast< chain::Forwarder& >(forwards.front()).set_more_specialized_link_element(NULL);
    } else {
        terminal_node->set_more_specialized_link_element(NULL);
    }
    if ( forwards.empty() || front ) {
        dynamic_cast< chain::Forwarder& >(*forwarder).set_more_specialized_link_element( method.get() );
    } else {
        dynamic_cast< chain::Forwarder& >(*forwarder).set_more_specialized_link_element( &forwards.back() );
    }
    if ( front && ! forwards.empty() ) {
        dynamic_cast< chain::Forwarder& >(forwards.front()).set_more_specialized_link_element( forwarder.get() );
    } else
        terminal_node->set_more_specialized_link_element( forwarder.get() );


    if ( front ) {
        forwards.push_front( forwarder );
    } else {
        forwards.push_back( forwarder );
    }
}

chain::Link& Config::get_link_element() {
    return *terminal_node;
}
const chain::Link& Config::get_link_element() const {
    return *terminal_node;
}

}
}
