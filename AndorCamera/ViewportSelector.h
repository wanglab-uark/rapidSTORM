#ifndef ANDORDIRECT_VIEWPORTSELECTOR_H
#define ANDORDIRECT_VIEWPORTSELECTOR_H

#include <dStorm/helpers/thread.h>
#include <dStorm/helpers/DisplayManager.h>
#include <simparm/Entry.hh>
#include <simparm/NumericEntry.hh>
#include <simparm/FileEntry.hh>
#include <simparm/TriggerEntry.hh>
#include <simparm/Set.hh>
#include <boost/optional.hpp>
#include <map>
#include "AndorDirect.h"

#include <dStorm/ImageTraits.h>
#include "InputChainLink_decl.h"
#include <dStorm/input/chain/Context.h>
#include <boost/smart_ptr/scoped_ptr.hpp>
#include <boost/thread/thread.hpp>

namespace dStorm {
namespace AndorCamera {

/** The Display provides a window in which Entry 
    *  elements defining the acquisition rectangle can be displayed
    *  and configured interactively. */
class Display : public simparm::Set,
                private simparm::Node::Callback, 
                public dStorm::Display::DataSource
{
  private:
    struct FetchHandler;

    boost::scoped_ptr<CameraConnection> cam;
    Method& config;

    simparm::StringEntry       status;
    simparm::TriggerEntry      stopAim;
    simparm::TriggerEntry      pause;
    simparm::FileEntry         imageFile;
    simparm::TriggerEntry      save;

    void operator()(const simparm::Event&);

    /** Reference to the config element to be configured. */
    bool aimed;

    ost::Mutex mutex;
    /** Flag is set to true when the display should be paused. */
    bool paused;

    boost::optional<int> camBorders[4];

    /** Buffer image for acquisition. Made class member to allow 
        *  saving to file. */
    std::auto_ptr<dStorm::Display::Change> change;
    std::auto_ptr<dStorm::Display::Manager::WindowHandle> handle;

    CamTraits traits;
    /** Currently used normalization boundaries. Will be set for each new
     *  image when \c lock_normalization is not set. */
    CamImage::PixelPair normalization_factor;
    /** If set to true, \c normalization_factor is fixed at the current
     *  level. */
    bool lock_normalization, redeclare_key;
    simparm::optional< boost::units::quantity<boost::units::camera::intensity> >
        lower_user_limit, upper_user_limit;

    /** Saved data of the last camera image to enable saving. */
    dStorm::Image<dStorm::Pixel,2> last_image;
    boost::shared_ptr<const input::chain::Context> context;
    boost::thread image_acquirer;

    /** Subthread for image acquisition. */
    virtual void run() throw();
    /** Method implementing data source interface. */
    virtual std::auto_ptr<dStorm::Display::Change> get_changes();
    /** Method implementing data source interface. */
    virtual void notice_closed_data_window();
    /** Method implementing data source interface. */
    virtual void notice_drawn_rectangle(int, int, int, int);
    /** Method implementing data source interface. */
    void notice_user_key_limits(int, bool, std::string);

    void registerNamedEntries();
    void acquire();
    void configure_camera();
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
    Display ( std::auto_ptr<CameraConnection>, Mode, Method& );
    /** Destructor, will join the subthread and close the display
        *  window. */
    virtual ~Display();

    void terminate();
    void context_changed( boost::shared_ptr<const input::chain::Context> );
};

}
}

#endif
