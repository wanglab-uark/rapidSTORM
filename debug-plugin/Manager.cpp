#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "Manager.h"
#include "md5.h"
#include <iomanip>
#include <fstream>
#include <sstream>
#include <boost/units/io.hpp>
#include <simparm/ChoiceEntry_Impl.hh>
#include <simparm/NumericEntry.hh>
#include <simparm/TriggerEntry.hh>
#include <dStorm/helpers/Variance.h>
#include <dStorm/image/iterator.h>
#include <dStorm/log.h>

class Manager::ControlConfig
: public simparm::Object, public simparm::Listener, private boost::noncopyable
{
    Manager& m;
    simparm::DataChoiceEntry<int> which_window;
    simparm::UnsignedLongEntry which_key;
    simparm::StringEntry new_limit;
    simparm::TriggerEntry close, set_lower_limit, set_upper_limit;

    void send_key_update(bool lower) {
        if ( ! which_window.isValid() ) return;
        int number = which_window(), key = which_key();
        ost::MutexLock lock(m.mutex);
        SourcesMap::iterator i = m.sources_by_number.find(number);
        if ( i != m.sources_by_number.end() ) {
            i->second->handler.notice_user_key_limits( key, lower, new_limit() );
        }
    }

  public:
    ControlConfig(Manager& m) 
        : Object("DummyDisplayManagerConfig", "Dummy display manager"), m(m),
          which_window("WhichWindow", "Select window"),
          which_key("WhichKey", "Select key", 1),
          new_limit("NewLimit", "New limit for selected key"),
          close("Close", "Close Window"),
          set_lower_limit("SetLowerLimit", "Set lower key limit"),
          set_upper_limit("SetUpperLimit", "Set upper key limit")
    {
        which_window.set_auto_selection( false );
        push_back( which_window );
        push_back( close );
        push_back( which_key );
        push_back( new_limit );
        push_back( set_lower_limit );
        push_back( set_upper_limit );
        receive_changes_from( close.value );
        receive_changes_from( set_lower_limit.value );
        receive_changes_from( set_upper_limit.value );
    }

    void added_choice( int n ) {
        std::stringstream entrynum;
        entrynum << n;
        which_window.addChoice( n, "Window" + entrynum.str(), "Window " + entrynum.str());
    }

    void removed_choice( int n ) {
        which_window.removeChoice(n);
    }

    void operator()(const simparm::Event& e) {
        if ( &e.source == &close.value && close.triggered() ) {
            close.untrigger();
            if ( ! which_window.isValid() ) return;
            int number = which_window();
            ost::MutexLock lock(m.mutex);
            SourcesMap::iterator i = m.sources_by_number.find(number);
            if ( i != m.sources_by_number.end() ) {
                i->second->handler.notice_closed_data_window();
            }
        } else if ( &e.source == &set_lower_limit.value && set_lower_limit.triggered() ) {
            set_lower_limit.untrigger();
            send_key_update(true);
        } else if ( &e.source == &set_upper_limit.value && set_upper_limit.triggered() ) {
            set_upper_limit.untrigger();
            send_key_update(false);
        }
    }
};

class Manager::Handle 
: public dStorm::Display::Manager::WindowHandle {
    Sources::iterator i;
    Manager& m;

  public:
    Handle( 
        const WindowProperties& properties,
        dStorm::Display::DataSource& source,
        Manager& m ) : m(m) 
    {
        guard lock(m.mutex);
        i = m.sources.insert( 
            m.sources.end(),
            Source(properties, source, m.number++) );
        m.sources_by_number.insert( make_pair( i->number, i ) );
        m.control_config->added_choice( i->number );
        LOG( "Created new window number " << i->number );
    }
    ~Handle() {
        guard lock(m.mutex);
        m.print_status(*i, "Destructing ", true);
        m.sources.erase( i );
        m.control_config->removed_choice(i->number);
    }

    std::auto_ptr<dStorm::Display::Change>
        get_state();
};

Manager::Source::Source(
    const WindowProperties& properties,
    dStorm::Display::DataSource& source,
    int n)
: handler(source), state(0), number(n),
  wants_closing(false), may_close(false)
{
    LOG( "Listening to window " << properties.name );
    handle_resize( properties.initial_size );
}

void Manager::Source::handle_resize( 
    const dStorm::Display::ResizeChange& r)
{
    LOG( "Sizing display number " << number << " to " 
              << r.size.x() << " " << r.size.y() << " with "
              << ((r.keys.empty()) ? 0 : r.keys.front().size) << " grey levels and pixel "
                 "size " << r.pixel_size );
    for (unsigned int i = 0; i < r.keys.size(); ++i ) {
        LOG( "Window " << number << " key " << i << " is measured in " << r.keys[i].unit << " and has " << r.keys[i].size << " levels and the user "
             "can " << ((r.keys[i].can_set_lower_limit) ? "" : "not ") << "set the lower limit" );
    }
    current_display = Image( r.size );
    current_display.fill(0);
    state.do_resize = true;
    state.resize_image = r;
    state.changed_keys.resize( r.keys.size() );
    for (size_t i = 0; i < r.keys.size(); ++i)
        state.changed_keys[i].resize( r.keys[i].size );
}

bool Manager::Source::get_and_handle_change() {
    std::auto_ptr<dStorm::Display::Change> c
        = handler.get_changes();

    if ( c.get() == NULL ) {
        LOG( "Error in display handling: NULL change pointer" );
        return false;
    }

    bool has_changed = c->do_resize || c->do_clear ||
                       c->do_change_image || 
                       (c->change_pixels.size() > 0);
    for (dStorm::Display::Change::Keys::const_iterator i = c->changed_keys.begin(); 
                                      i != c->changed_keys.end(); ++i)
        has_changed = has_changed || (i->size() > 0);

    if ( c->do_resize ) {
        handle_resize( c->resize_image );
    } 
    if ( c->do_clear ) {
        state.do_clear = true;
        state.clear_image = c->clear_image;
        current_display.fill(
            state.clear_image.background );
    } 
    if ( c->do_change_image ) {
        LOG( "Replacing image with " << c->image_change.new_image.frame_number().value() );
        current_display = c->image_change.new_image;
    }
    
    for( dStorm::Display::Change::PixelQueue::const_iterator
                i = c->change_pixels.begin(); 
                i != c->change_pixels.end(); i++)
    {
         current_display(i->x,i->y) = i->color;
    }

    for ( size_t j = 0; j < c->changed_keys.size(); ++j)
        for ( int i = 0; i < c->changed_keys[j].size(); i++ )
        {
            dStorm::Display::KeyChange kc = c->changed_keys[j][i];
            if ( i == 31 )
                LOG("Window " << number << " key " << j << " value 31 has value " << kc.value);
            state.changed_keys[j][kc.index] = kc;
        }

    return has_changed;
}

void Manager::run() { dispatch_events(); }

std::auto_ptr<dStorm::Display::Manager::WindowHandle>
    Manager::register_data_source
(
    const WindowProperties& props,
    dStorm::Display::DataSource& handler
) {
    {
        guard lock(mutex);
        if ( !running ) {
            running = true;
            ost::Thread::start();
        }
    }
    return 
        std::auto_ptr<dStorm::Display::Manager::WindowHandle>( new Handle(props, handler, *this) );
}

std::ostream& operator<<(std::ostream& o, dStorm::Display::Color c)
{
    return ( o << int(c.red()) << " " << int(c.green()) << " " << int(c.blue()) );
}

void Manager::print_status(Source& s, std::string prefix, bool p)
{
    bool has_changed = s.get_and_handle_change();
    if (!has_changed && !p) return;
    
    md5_state_t pms;
    md5_byte_t digest[16];
    md5_init( &pms );
    md5_append( &pms, 
        (md5_byte_t*)s.current_display.ptr(),
        s.current_display.size_in_pixels()*
            sizeof(dStorm::Pixel) / sizeof(md5_byte_t));
    md5_finish( &pms, digest );

    std::stringstream sdigest;
    for (int j = 0; j < 16; j++)
        sdigest << std::hex << int(digest[j]);

    float sum = 0;
    int count = 0;
    for ( Source::Image::const_iterator j = s.current_display.begin(); 
                j != s.current_display.end(); ++j )
    {
        sum += int(j->red()) + j->blue() + j->green();
        if ( j->red() != 0 || j->blue() != 0 || j->green() != 0 )
            count++;
    }
    LOG( prefix << "window " << s.number << " with digest " << sdigest.str() << ", mean is " 
                << sum / s.current_display.size_in_pixels() << " and count of nonzero pixels is " << count
                );
}

void Manager::dispatch_events() {
    while ( running ) {
      {
        guard lock(mutex);
        for ( Sources::iterator i = sources.begin(); i != sources.end(); i++ )
        {
            if ( i->wants_closing ) { i->may_close = true; }
            print_status(*i, "Changing ");
        }
        gui_run.broadcast(); 
      }
#ifdef HAVE_USLEEP
      usleep(100000);
#elif HAVE_WINDOWS_H
      Sleep(100);
#endif
    }
}

void Manager::run_in_GUI_thread( ost::Runnable* )
{
}

Manager::Manager(dStorm::Display::Manager *p)
: ost::Thread("DisplayManager"),
  gui_run(mutex),
  running(false),
  number(0),
  previous(p)
{
    control_config.reset( new ControlConfig(*this) );
}

Manager::~Manager()
{
    if ( running ) {
        running = false;
        this->join();
    }
}

void Manager::store_image(
        std::string filename,
        const dStorm::Display::Change& image )
{
    std::cerr << "Storing result image under " << filename << std::endl;
    previous->store_image(filename, image);
}

std::auto_ptr<dStorm::Display::Change>
     Manager::Handle::get_state()
{
    guard lock(m.mutex);
    m.gui_run.wait();
    std::auto_ptr<dStorm::Display::Change>
        rv( new dStorm::Display::Change(i->state) );

    if ( ! rv->do_resize )
        throw std::runtime_error("State not yet initialized");
    rv->do_change_image = true;
    rv->image_change.new_image = i->current_display;

    return rv;
}

simparm::Node* Manager::getConfig()
{
    return control_config.get();
}

