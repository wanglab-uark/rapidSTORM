#ifndef TESTPLUGIN_REPEATTRIGGER_H
#define TESTPLUGIN_REPEATTRIGGER_H

#include <simparm/TriggerEntry.h>
#include <dStorm/output/Output.h>
#include <dStorm/output/OutputBuilder.h>
#include <dStorm/Engine.h>

struct Repeat
: public dStorm::output::Output
{
    struct Config {
        static std::string get_name() { return "RepeatTrigger"; }
        static std::string get_description() { return "Repeat trigger"; }
        void attach_ui( simparm::NodeHandle ) {}
        bool can_work_with(const dStorm::output::Capabilities&)
            {return true;}
    };
    dStorm::Engine *r;
    simparm::TriggerEntry repeat;
    simparm::BaseAttribute::ConnectionStore listening;

    void attach_ui_( simparm::NodeHandle at ) { 
        listening = repeat.value.notify_on_value_change( 
            boost::bind( &Repeat::repeat_results, this ) );
        repeat.attach_ui( at ); 
    }
    Repeat(const Config&) 
        : r(NULL), repeat("Repeat", "Repeat results")
        { repeat.hide();  }
    Repeat* clone() const { return new Repeat(*this); }

    AdditionalData announceStormSize(const Announcement& a) { 
        r = a.engine;
        repeat.set_visibility( r );
        return AdditionalData(); 
    }
    void receiveLocalizations(const EngineResult&) {}

private:
    void repeat_results() {
        if ( r && repeat.triggered() ) { 
            repeat.untrigger();
            r->repeat_results(); 
            LOG("Repeating results"); 
        } 
    }
};

#endif
