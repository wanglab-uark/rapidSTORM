#ifndef SIMPARM_WX_UI_TRIGGERNODE_H
#define SIMPARM_WX_UI_TRIGGERNODE_H

#include "WindowNode.h"
#include <simparm/Attribute.h>

namespace simparm {
namespace wx_ui {

class TriggerNode : public Node {
    std::string description;
    simparm::Attribute<unsigned long>* value;
    boost::shared_ptr< wxWindow* > my_window;

public:
    TriggerNode( boost::shared_ptr<Node> n )
        : Node(n) {}
    ~TriggerNode();
    virtual void set_description( std::string d ) { description = d; }
    void initialization_finished();
    void add_attribute( simparm::BaseAttribute& a ) {
        if ( a.get_name() == "value" ) value = dynamic_cast< simparm::Attribute<unsigned long>* >(&a);
    }
};

}
}

#endif
