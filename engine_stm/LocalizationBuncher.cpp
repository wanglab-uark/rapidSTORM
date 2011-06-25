#include "debug.h"

#include "LocalizationBuncher.h"
#include <dStorm/input/Source_impl.h>
#include <dStorm/output/Output.h>
#include <boost/variant/apply_visitor.hpp>

using namespace dStorm::output;

namespace dStorm {
namespace engine_stm {

enum VisitResult { KeepComing, IAmFinished, FinishedAndReject };

class Visitor 
: public boost::static_visitor<VisitResult>
{
    std::auto_ptr<output::LocalizedImage> can;
    simparm::optional<frame_index> my_image;

  public:
    ~Visitor() {}
    VisitResult operator()( const dStorm::Localization& l ) 
    {
        if ( my_image.is_set() ) {
            if ( l.frame_number() != *my_image )
                return FinishedAndReject;
        } else {
            my_image = l.frame_number();
            can.reset( new output::LocalizedImage() );
            can->forImage = *my_image;
        }
        can->push_back( l );
        return KeepComing;
    }

    VisitResult operator()( const dStorm::LocalizationFile::EmptyLine& i ) 
    {
        if ( ! my_image.is_set() ) {
            my_image = i.number;
            can.reset( new output::LocalizedImage() );
            can->forImage = *my_image;
        }
        return IAmFinished;
    }

    template <typename Type> VisitResult add(Type& argument);
    frame_index for_frame() { assert(my_image.is_set()); return *my_image; }
    std::auto_ptr<output::LocalizedImage> get_result() { return can; }
};

template <typename Type>
VisitResult 
Visitor::add(Type& argument)
{
    return boost::apply_visitor( *this, argument );
}

template <>
VisitResult 
Visitor::add<Localization>(Localization& argument)
{
    return (*this)( argument );
}

template <typename Input>
LocalizationBuncher<Input>::LocalizationBuncher(
    const Config& config,
    Source<Input>& master,
    bool end)
: master(master)
{
    if ( end ) {
        outputImage = master.lastImage;
    } else {
        claim_image();
        search_output_image();
    }
}

template <typename Input>
void LocalizationBuncher<Input>::claim_image()
{
    ost::MutexLock lock(master.mutex);
    outputImage = master.next_image;
    master.next_image += 1 * camera::frame;
}

template <typename Input>
void LocalizationBuncher<Input>::increment() {
    output.reset();
    claim_image();
    search_output_image();
}

template <typename Input>
bool LocalizationBuncher<Input>::equal(const LocalizationBuncher<Input>& o) const
{
    return outputImage == o.outputImage;
}

template <typename Input>
void LocalizationBuncher<Input>::search_output_image() {
    ost::MutexLock lock( master.mutex );
    typename Source<Input>::Canned::iterator canned_output
        = master.canned.find(outputImage);
    if ( canned_output != master.canned.end() ) {
        output.reset( master.canned.release(canned_output).release() );
        return;
    }

    DEBUG("Reading " << outputImage << " into can");
    while ( master.current != master.base_end ) {
        Visitor v;
        for ( ; master.current != master.base_end; ++master.current )  {
            VisitResult r = v.add(*master.current);
            if ( r == FinishedAndReject )
                break;
            else if ( r == IAmFinished ) {
                ++master.current;
                break;
            }
        }
        DEBUG("Visitor declares to be finished with " << v.for_frame() << ", want " << outputImage);
        if ( v.for_frame() == outputImage ) {
            output = v.get_result();
            break;
        } else
            master.canned.insert( v.for_frame(), v.get_result() );
    }
    if ( output.get() == NULL )
        /* End of file reached. Insert empty can. */
        output.reset( new output::LocalizedImage() );

    DEBUG("Serving " << output->forImage);
}

template <typename Input>
LocalizationBuncher<Input>::LocalizationBuncher
    (const LocalizationBuncher& o)
: master( const_cast< Source<Input>& >(o.master) ),
  output(o.output),
  outputImage(o.outputImage)
{
}

template <typename Input>
LocalizationBuncher<Input>::~LocalizationBuncher() {
}

template <typename InputType>
typename Source<InputType>::TraitsPtr
Source<InputType>::get_traits()
{
    input::Source<Localization>::TraitsPtr traits  = base->get_traits();
    traits::ImageNumber::RangeType& r = traits->image_number().range();

    current = base->begin();
    base_end = base->end();

    if ( ! r.first.is_set() || ! r.second.is_set() )
        throw std::runtime_error("Total number of frames in STM file must be known");
    firstImage = *r.first;
    lastImage = *r.second + 1 * camera::frame;
    firstImage = std::min(firstImage, lastImage);

    return TraitsPtr( new TraitsPtr::element_type( *traits ) );
}

template <class InputType>
Source<InputType>::Source( const Config& c, std::auto_ptr<Input> base )
: Base(config, Capabilities(base->flags).reset( MultipleConcurrentIterators ) ), config(c), base(base) {}

template <class InputType>
Source<InputType>::~Source()
{
    DEBUG("Deleting base input for " << this);
    canned.clear();
    DEBUG("Deleting iterators and input");
    current = base_end = InputIterator();
    DEBUG("Deleting rest");
}

template <class InputType>
void Source<InputType>::dispatch(Messages m)
{
    if ( m.test( RepeatInput ) ) {
        m.reset( RepeatInput );
        current = base->begin();
        base_end = base->end();
    }
    m.reset( WillNeverRepeatAgain );
    base->dispatch(m);
}

template class Source<LocalizationFile::Record>;
template class Source<Localization>;

}
}
