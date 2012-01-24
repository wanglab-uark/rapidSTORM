#include "debug.h"
#include "plugin.h"
#include <stdint.h>
#include "Viewer.h"
#include <limits>
#include <cassert>
#include <dStorm/engine/Image.h>

#include <simparm/ChoiceEntry_Impl.hh>
#include <simparm/Message.hh>
#include <dStorm/display/Manager.h>
#include <boost/thread/locks.hpp>
#include <boost/thread/recursive_mutex.hpp>

#include "ColourScheme.h"
#include "colour_schemes/hot_config.h"

using namespace std;

using namespace dStorm::display;
using namespace dStorm::outputs;
using namespace dStorm::output;

namespace dStorm {
namespace viewer {

Viewer::Viewer(const Viewer::Config& config)
: Status(config),
  OutputObject("Display", "Display status"),
  simparm::Node::Callback( simparm::Event::ValueChanged ),
  output_mutex(NULL),
  repeater( NULL )
{
    DEBUG("Building viewer");

    this->registerNamedEntries( *this );
    Status::add_listener( *this );

    DEBUG("Built viewer");
}

Viewer::~Viewer() {
    DEBUG("Destructing Viewer " << this);
}


void Viewer::receiveLocalizations(const EngineResult& er)
{
    forwardOutput->receiveLocalizations(er);
}

Output::AdditionalData 
Viewer::announceStormSize(const Announcement &a) {
    announcement = a;
    repeater = a.engine;
    output_mutex = a.output_chain_mutex;
    this->manager = &a.display_manager();
    implementation = config.colourScheme.value().make_backend(this->config, *this);
    implementation->set_output_mutex( output_mutex );
    implementation->set_job_name( a.description );
    forwardOutput = &implementation->getForwardOutput();
    return forwardOutput->announceStormSize(a);
}

Viewer::RunRequirements Viewer::announce_run(const RunAnnouncement& a) {
    return forwardOutput->announce_run(a);
}

void Viewer::store_results() {
    forwardOutput->store_results();
    if (config.outputFile)
        writeToFile(config.outputFile());
}

void Viewer::operator()(const simparm::Event& e) {
    if ( ! output_mutex ) return;
    if (&e.source == &config.showOutput.value) {
        boost::lock_guard<boost::recursive_mutex> lock(*output_mutex);
        adapt_to_changed_config();
    } else if (&e.source == &save.value) {
        if ( save.triggered() ) {
            boost::lock_guard<boost::recursive_mutex> lock(*output_mutex);
            /* Save image */
            save.untrigger();
            if ( config.outputFile ) {
                writeToFile( config.outputFile() );
            }
        }
    } else if (&e.source == &config.histogramPower.value) {
        /* Change histogram power */
        boost::lock_guard<boost::recursive_mutex> lock(*output_mutex);
        implementation->set_histogram_power(config.histogramPower());
    } else if ( announcement ) {
        /* Store the old implementation past the mutex lock to allow mutex 
         * locking in the course of the destructor. This is needed when the
         * live backend is destructed because a last update is fetched by
         * the display thread. */
        std::auto_ptr<Backend> old_implementation;
        {
            boost::lock_guard<boost::recursive_mutex> lock(*output_mutex);
            old_implementation = implementation;
            implementation = config.colourScheme.value().make_backend(this->config, *this);
            implementation->set_output_mutex( output_mutex );
            implementation->set_job_name( announcement->description );
            forwardOutput = &implementation->getForwardOutput();
            forwardOutput->announceStormSize(*announcement);
            if ( repeater ) repeater->repeat_results();
        }
        old_implementation.reset();
    }
}

void Viewer::adapt_to_changed_config() {
    DEBUG("Changing implementation, showing output is " << config.showOutput());
    if ( implementation.get() ) {
        implementation = implementation->adapt( implementation, *this );
        implementation->set_output_mutex( output_mutex );
        forwardOutput = &implementation->getForwardOutput();
    }
    DEBUG("Changed implementation to " << implementation.get());
}

void Viewer::writeToFile(const string &name) {
    try {
        implementation->save_image(name, config);
    } catch ( const std::runtime_error& e ) {
        simparm::Message m( "Writing result image failed", "Writing result image failed: " + std::string(e.what()) );
        send( m );
    }
}

void Viewer::check_for_duplicate_filenames
        (std::set<std::string>& present_filenames)
{
    insert_filename_with_check( config.outputFile(), present_filenames );
}

std::auto_ptr<output::OutputSource> make_output_source() {
    return std::auto_ptr<output::OutputSource>( new output::FileOutputBuilder<Viewer>() );
}

}
}
