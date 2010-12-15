#include "debug.h"

#include "LiveView.h"
#include <boost/thread/locks.hpp>
#include "AndorDirect.h"
#include <dStorm/Image.h>
#include <dStorm/Image_impl.h>

typedef ost::MutexLock guard;

using namespace boost::units;
using namespace cs_units::camera;
using namespace dStorm;

namespace AndorCamera {

LiveView::LiveView( 
    bool on_by_default,
    simparm::optional<Resolution> resolution,
    boost::units::quantity<cs_units::camera::frame_rate> cycle_time
    )
: Object("LiveView", "Live view options"),
  cycle_time( cycle_time ),
  resolution( resolution ),
  show_live("ShowLive", "Show camera image", on_by_default),
  change( new dStorm::Display::Change(1) )
{
    DEBUG("LiveView constructed");
    registerNamedEntries();
}

void LiveView::registerNamedEntries() {
    push_back( show_live );
}

LiveView::~LiveView() {
    DEBUG("LiveView destructing");
}

void LiveView::show_window(CamImage::Size size) {
    if ( window.get() == NULL ) {
        DEBUG("Showing live view window");
        dStorm::Display::Manager::WindowProperties props;
        props.name = "Live camera view";
        props.flags.close_window_on_unregister();
        props.initial_size.size = size;
        props.initial_size.keys.push_back( 
            dStorm::Display::KeyDeclaration("ADC", "A/D counts", 256) );
        props.initial_size.keys.back().can_set_lower_limit = true;
        props.initial_size.keys.back().can_set_upper_limit = true;
        props.initial_size.keys.back().lower_limit = "";
        props.initial_size.keys.back().upper_limit = "";
        if ( resolution.is_set() )
            props.initial_size.pixel_size = *resolution;
        else
            props.initial_size.pixel_size = 0 * cs_units::camera::pixels_per_meter;

        window = dStorm::Display::Manager::getSingleton()
            .register_data_source( props, *this );
        DEBUG("Success in showing live view window");
    }
}

void LiveView::hide_window() {
    if ( window.get() != NULL ) {
        DEBUG("Hiding window");
        show_live = false;
        window.reset( NULL );
    }
}

std::auto_ptr<Display::Change> LiveView::get_changes()
{
    DEBUG("Locking changes");
    std::auto_ptr<Display::Change> fresh( new Display::Change(1) );
    ost::MutexLock lock(change_mutex);
    DEBUG("Getting live view changes");
    compute_image_change( current_image_content );
    std::swap( fresh, change );
    DEBUG("Got live view changes");
    return fresh;
}

void LiveView::compute_image_change
    ( const CamImage& image )
{
    DEBUG("Computing minmax for change " << change.get() << " and validity " << image.is_invalid());
    if ( ! image.is_invalid() ) {
        CamImage::PixelPair minmax;
        if ( ! lower_user_limit.is_set() || ! upper_user_limit.is_set() )
            minmax = image.minmax();
        if ( lower_user_limit.is_set() )
            minmax.first = (*lower_user_limit) / cs_units::camera::ad_count;
        if ( upper_user_limit.is_set() )
            minmax.second = (*upper_user_limit) / cs_units::camera::ad_count;
        
        DEBUG("Normalizing");
        change->do_change_image = true;
        change->image_change.new_image = 
            image.normalize<dStorm::Pixel>( minmax );
        DEBUG("Making key");
        change->make_linear_key( minmax );
    }
}

void LiveView::show( const CamImage& image, int number) {
    DEBUG("Showing image");
    guard lock(window_mutex);
    DEBUG("Got mutex for showing image");
    if ( show_live() ) {
        DEBUG("Performing show");
        show_window(image.sizes());

        current_image_content = image;
        DEBUG("Performed show");
    } else {
        DEBUG("Hiding window");
        if ( window.get() != NULL ) hide_window();
    }
}

void LiveView::notice_closed_data_window() {
    guard lock(window_mutex);
    hide_window();
}

void LiveView::notice_user_key_limits(int key_index, bool lower, std::string input)
{
    guard lock( window_mutex );
    simparm::optional< boost::units::quantity<cs_units::camera::intensity> > v;
    if ( input != "" )
        v = atof(input.c_str()) * cs_units::camera::ad_count;
    if ( lower )
        lower_user_limit = v;
    else
        upper_user_limit = v;
}

}
