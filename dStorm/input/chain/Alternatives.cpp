#include "../debug.h"

#include "Alternatives.h"
#include "Forwarder.h"
#include <simparm/ChoiceEntry_Impl.hh>
#include <simparm/Message.hh>
#include <map>
#include "Forwarder.h"
#include "../InputMutex.h"
#include <dStorm/input/chain/MetaInfo.h>

namespace dStorm {
namespace input {
namespace chain {

class Alternatives::UpstreamCollector 
: public Forwarder, boost::noncopyable
{
    Alternatives& papa;

  public:
    UpstreamCollector(Alternatives& papa) : papa(papa) {}
    UpstreamCollector(Alternatives& papa, const UpstreamCollector& o)
        : papa(papa) {}

    UpstreamCollector* clone() const 
        { assert(false); throw std::logic_error("UpstreamCollector unclonable"); }

    BaseSource* makeSource() { return Forwarder::makeSource(); }

    AtEnd traits_changed( TraitsRef r, Link* from ) {
        DEBUG("Traits changed from upstream in " << this  << " from " << from << " to " << r.get() );
        assert( upstream_traits() == r );
        Link::traits_changed(r,from);
        AtEnd rv;
        for ( simparm::NodeChoiceEntry<LinkAdaptor>::iterator i = papa.choices.beginChoices(); i != papa.choices.endChoices(); ++i ) {
            rv = i->link().traits_changed( r, this );
            if ( upstream_traits() != r ) return AtEnd();
        }
        return rv;
    }

    std::string name() const { throw std::logic_error("Not implemented"); }
    std::string description() const { throw std::logic_error("Not implemented"); }
};

class Alternatives::UpstreamLink
: public Link 
{
    UpstreamCollector& collector;

  public:
    UpstreamLink( UpstreamCollector& collector ) : collector(collector) {}
    /** This clone() implementation returns no object. The new upstream links
     *  are inserted by the Alternatives class. */
    UpstreamLink* clone() const { return NULL; }
    void registerNamedEntries( simparm::Node& ) {}
    std::string name() const { throw std::logic_error("Not implemented"); }
    std::string description() const { throw std::logic_error("Not implemented"); }
    BaseSource* makeSource() { return collector.makeSource(); }
    AtEnd traits_changed( TraitsRef, Link* ) { /* TODO: This method should be used for traits injection. */
       throw std::logic_error("Not implemented"); }
    void insert_new_node( std::auto_ptr<Link>, Place ) { throw std::logic_error("Not implemented"); }
};

Alternatives::Alternatives(std::string name, std::string desc, bool auto_select)
: Choice(name, desc, auto_select), collector( new UpstreamCollector(*this) )
{
}

Alternatives::Alternatives(const Alternatives& o)
: Choice(o), collector( new UpstreamCollector(*this, *o.collector) )
{
    for ( simparm::NodeChoiceEntry<LinkAdaptor>::iterator i = choices.beginChoices(); i != choices.endChoices(); ++i ) {
        i->link().insert_new_node( std::auto_ptr< Link >(new UpstreamLink(*collector) ), Anywhere );
    }
}

Alternatives::~Alternatives() {
}

void Alternatives::add_choice( std::auto_ptr<Link> choice ) 
{
    choice->insert_new_node( std::auto_ptr< Link >(new UpstreamLink(*collector) ), Anywhere );
    Choice::add_choice(choice);
}

void Alternatives::insert_new_node( std::auto_ptr<Link> link, Place p )
{
    collector->insert_new_node(link,p);
}

void Alternatives::registerNamedEntries( simparm::Node& node ) {
    collector->registerNamedEntries(node);
    Choice::registerNamedEntries(node);
}

}
}
}
