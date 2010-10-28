#ifndef ANDORDIRECT_VIEWPORTSELECTOR_H
#define ANDORDIRECT_VIEWPORTSELECTOR_H

#include <dStorm/helpers/thread.h>
#include <dStorm/helpers/DisplayManager.h>
#include <simparm/Entry.hh>
#include <simparm/NumericEntry.hh>
#include <simparm/FileEntry.hh>
#include <simparm/TriggerEntry.hh>
#include <simparm/Set.hh>
#include <map>
#include <AndorCamera/CameraReference.h>
#include "AndorDirect.h"
#include "Context.h"

#include <dStorm/ImageTraits.h>

namespace AndorCamera {

class Acquisition;

namespace ViewportSelector {

class Config;

/** The Display provides a window in which Entry 
    *  elements defining the acquisition rectangle can be displayed
    *  and configured interactively. */
class Display : public simparm::Set,
                private simparm::Node::Callback, 
                private ost::Thread,
                public dStorm::Display::DataSource
{
  private:
    AndorCamera::CameraReference cam;
    Config& config;
    const Context& context;

    simparm::Object            statusBox;
    simparm::TriggerEntry      stopAim;
    simparm::TriggerEntry      pause;
    simparm::FileEntry         imageFile;
    simparm::TriggerEntry      save;

    void operator()(const simparm::Event&);

    /** Reference to the config element to be configured. */
    ImageReadout* aimed;

    ost::Mutex mutex;
    /** Flag is set to true when the display should be paused. */
    bool paused;

    /** Buffer image for acquisition. Made class member to allow 
        *  saving to file. */
    std::auto_ptr<dStorm::Display::Change> change;
    std::auto_ptr<dStorm::Display::Manager::WindowHandle> handle;

    /** Size of displayed camera image. */
    CamImage::Size size;
    /** Currently used normalization boundaries. Will be set for each new
     *  image when \c lock_normalization is not set. */
    CamImage::PixelPair normalization_factor;
    /** If set to true, \c normalization_factor is fixed at the current
     *  level. */
    bool lock_normalization;

    /** Saved data of the last camera image to enable saving. */
    dStorm::Image<dStorm::Pixel,2> last_image;

    /** Subthread for image acquisition. */
    virtual void run() throw();
    /** Method implementing data source interface. */
    virtual std::auto_ptr<dStorm::Display::Change> get_changes();
    /** Method implementing data source interface. */
    virtual void notice_closed_data_window();
    /** Method implementing data source interface. */
    virtual void notice_drawn_rectangle(int, int, int, int);

    void registerNamedEntries();
    void acquire();
    void configure_camera(AndorCamera::Acquisition&);
    void initialize_display();
    void draw_image(const CamImage& data);

    dStorm::Display::ResizeChange getSize() const;

    /** Undefined copy constructor to avoid implicit copy construction. */
    Display(const Display&);
    /** Undefined assignment to avoid implicit assignment. */
    Display& operator=(const Display&);

  public:
    enum Mode { SelectROI, ViewROI };

    /** Constructor and only public interface.
        *  This is a fire-and-forget class: The constructor starts a
        *  subthread that will open the acquisition and update the
        *  display window, and then return control. */
    Display ( const CameraReference&, Mode, Config&, const Context& );
    /** Destructor, will join the subthread and close the display
        *  window. */
    virtual ~Display();

    void terminate();
    void context_changed();
};

/** Configuration items for the viewport selection window that
    *  opens when the "aim" button is pressed. */
class Config 
: public simparm::Object, public simparm::Node::Callback 
{
    AndorCamera::CameraReference cam;
    /* Use a copy of the context to avoid race conditions. */
    Context context;

    void registerNamedEntries();

    void startAiming();
    void stopAiming();

  public:
    simparm::TriggerEntry select_ROI, view_ROI;

    Config(const AndorCamera::CameraReference& cam, 
           Context::ConstPtr context);
    Config(const Config &c);
    ~Config();
    Config* clone();

    Config& operator=(const Config&);

    void operator()(const simparm::Event&);

    void delete_active_selector();
    void context_changed(Context::ConstPtr new_context);

  private:
    ost::Mutex active_selector_mutex;
    ost::Condition active_selector_changed;
    std::auto_ptr<Display> active_selector;

    void set_entry_viewability();
    void make_display( Display::Mode mode );
};

}
}

#endif