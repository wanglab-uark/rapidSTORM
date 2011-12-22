#include "debug.h"

#include <boost/algorithm/string.hpp>
#include <dStorm/input/InputMutex.h>
#include <dStorm/input/chain/MetaInfo.h>
#include "chain/Choice.h"

#include "FileMethod.h"
#include "InputFileNameChange.h"

namespace dStorm {
namespace input {

using namespace chain;

class FileTypeChoice 
: public chain::Choice
{
    void insert_new_node( std::auto_ptr<chain::Link> l, Place p ) {
        if ( p == FileReader )
            chain::Choice::add_choice(l);
        else
            chain::Choice::insert_new_node(l,p);
    }

  public:
    FileTypeChoice() 
        : chain::Choice("FileType", "File type", true) {}
};

FileMethod::FileMethod()
: simparm::Set("FileMethod", "File"),
  chain::Forwarder(),
  simparm::Listener( simparm::Event::ValueChanged ),
  input_file("InputFile", "Input file")
{
    input_file.helpID = "InputFile";
    /* TODO: children.set_help_id( "FileType" ); */
    DEBUG("Created file method");
    receive_changes_from( input_file.value );
    chain::Forwarder::insert_here( std::auto_ptr<Link>( new FileTypeChoice() ) );
}

FileMethod::FileMethod(const FileMethod& o)
: simparm::Set(o),
  chain::Forwarder(o),
  simparm::Listener( simparm::Event::ValueChanged ),
  input_file(o.input_file)
{
    DEBUG("Copied file method " << this << " from " << &o);
    receive_changes_from( input_file.value );
}

FileMethod::~FileMethod() {}

void FileMethod::operator()( const simparm::Event& )
{
    ost::MutexLock lock( global_mutex() );
    DEBUG( "Sending callback for filename " << input_file() << " from " << this << " to " << current_meta_info().get() );
    if ( current_meta_info().get() != NULL ) {
        current_meta_info()->get_signal< InputFileNameChange >()( input_file() );
    }
}

class BasenameApplier {
    std::string basename;
    bool applied;

  public:
    BasenameApplier( std::string filename ) : basename(filename), applied(false) {}
    void operator()( const std::pair<std::string,std::string>& p ) {
        if ( applied ) return;
        std::string suffix = p.second;
        if ( basename.length() >= suffix.length() &&
            boost::iequals( basename.substr( basename.length() - suffix.length() ), suffix ) ) 
        {
            basename = basename.substr( 0, basename.length() - suffix.length() );
            applied = true;
        }
    }
    const std::string& get_result() const { return basename; }
};

void FileMethod::traits_changed( TraitsRef traits, Link* from )
{
    Link::traits_changed( traits, from );
    DEBUG( "Sending callback for filename " << input_file() << " from " << this << " to " << traits.get() );
    if ( traits.get() == NULL ) return update_current_meta_info( traits );
    DEBUG( "FileMethod " << this << " got traits " << traits->provides_nothing() );
    traits->get_signal< InputFileNameChange >()( input_file() );
    /* The signal might have forced a traits update that already did our
     * work for us. */
    if ( upstream_traits() != traits ) return;

    boost::shared_ptr<chain::MetaInfo> my_traits( new chain::MetaInfo(*traits) );
    if ( my_traits->suggested_output_basename.unformatted()() == "" ) {
        BasenameApplier a( input_file() );
        std::string new_basename = 
            std::for_each( my_traits->accepted_basenames.begin(), 
                        my_traits->accepted_basenames.end(),
                        BasenameApplier( input_file() ) ).get_result();
        my_traits->suggested_output_basename.unformatted() = new_basename;
    }
    my_traits->forbidden_filenames.insert( input_file() );
    update_current_meta_info( my_traits );
}

}
}

#include "inputs/TIFF.h"
#include "test-plugin/DummyFileInput.h"
#include "dejagnu.h"

namespace dStorm {
namespace input {

using namespace chain;

void FileMethod::unit_test( TestState& t ) {
    FileMethod file_method;

    file_method.insert_new_node( std::auto_ptr<Link>( new TIFF::ChainLink() ), FileReader );
    t.testrun( file_method.current_meta_info().get(), 
        "Test method publishes traits" );
    t.testrun( file_method.current_meta_info()->provides_nothing(), 
        "Test method provides nothing without an input file" );

    file_method.input_file = TIFF::test_file_name;
    t.testrun( file_method.current_meta_info().get(), 
        "Test method publishes traits for TIFF file name" );
    t.testrun( ! file_method.current_meta_info()->provides_nothing(), 
        "Test method provides something for TIFF file name" );
    t.testrun( file_method.current_meta_info()->traits< dStorm::engine::Image >(),
        "Test method provides correct image type" );
    t.testrun( file_method.current_meta_info()->traits< dStorm::engine::Image >()->size[1] == 42 * camera::pixel,
        "Test method provides correct width for TIFF file name" );

    std::auto_ptr<  dStorm::input::chain::Link > foo = dummy_file_input::make();
    file_method.insert_new_node( foo, FileReader );
    t.testrun( file_method.current_meta_info()->traits< dStorm::engine::Image >()->size[1] == 42 * camera::pixel,
        "Test method provides correct width for TIFF file name" );

    file_method.input_file = "foobar.dummy";
    t.testrun( file_method.current_meta_info().get() &&
               file_method.current_meta_info()->traits< dStorm::engine::Image >().get() &&
               file_method.current_meta_info()->traits< dStorm::engine::Image >()->size[1] == 50 * camera::pixel,
        "Test method can change file type" );

    FileMethod copy(file_method);
    copy.input_file = TIFF::test_file_name;
    t.testrun( copy.current_meta_info().get() && 
               copy.current_meta_info()->provides<dStorm::engine::Image>() &&
               copy.current_meta_info()->traits< dStorm::engine::Image >()->size[1] == 42 * camera::pixel,
        "Copied file method can adapt to input file" );
    file_method.input_file = "";
    t.testrun( copy.current_meta_info().get() && 
               copy.current_meta_info()->provides<dStorm::engine::Image>() &&
               copy.current_meta_info()->traits< dStorm::engine::Image >()->size[1] == 42 * camera::pixel,
        "Copied file method is not mutated by original" );
}

}
}
