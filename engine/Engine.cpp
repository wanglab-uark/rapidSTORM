#define DSTORM_ENGINE_CPP

#include "engine/Engine.h"
#include "config.h"

#include <dStorm/data-c++/Vector.h>
#include <cassert>
#include <errno.h>
#include <string.h>
#include <algorithm>

#include "engine/SigmaGuesser.h"
#include <dStorm/engine/SpotFinder.h>
#include "engine/SpotFitter.h"
#include <dStorm/engine/Image.h>
#include <dStorm/engine/Config.h>
#include <dStorm/engine/Input.h>
#include <dStorm/outputs/Crankshaft.h>
#include <sstream>
#include "engine/EngineDebug.h"
#include <dStorm/input/Slot.h>
#include <dStorm/input/Source.h>
#include <dStorm/input/ImageTraits.h>
#include "doc/help/context.h"

#ifdef DSTORM_MEASURE_TIMES
#include <time.h>
clock_t smooth_time = 0, search_time = 0, fit_time = 0;
#endif

#include "debug.h"

using namespace dStorm;
using namespace std;
using namespace dStorm::input;
using namespace dStorm::output;
using namespace dStorm::outputs;

namespace dStorm {
namespace engine {

bool Engine::globalStop=false;

class EngineThread : public ost::Thread {
  private:
    Engine &engine;
    auto_ptr<string> nm;
  public:
    EngineThread(Engine &engine, auto_ptr<string> name) 
        : ost::Thread(name->c_str()), engine(engine), nm(name) {}
    ~EngineThread() {
        DEBUG("Collecting piston");
        join(); 
    }
    void run() throw() {
        engine.safeRunPiston();
    }
};

Engine::Engine(Config &config, Input &input, Output& output)
: Object("EngineStatus", "Computation status"),
  simparm::Node::Callback( Node::ValueChanged ),
  config(config),
  stopper("EngineStopper", "Stop computation"),
  input(input), output(&output), emergencyStop(false),
  error(false)
{
    DEBUG("Constructing engine");

    stopper.helpID = HELP_StopEngine;
    receive_changes_from( stopper.value );
    push_back( config.sigma_x);
    push_back( config.sigma_y);
    push_back( config.sigma_xy);
    push_back( stopper );
}

void Engine::addPiston() {
    int pistonCount = pistons.size();
    auto_ptr<string> pistonName( new string("Piston 00") );
    (*pistonName)[7] += pistonCount / 10;
    (*pistonName)[8] += pistonCount % 10;
    auto_ptr<ost::Thread> new_piston(new EngineThread(*this, pistonName));
    DEBUG("Spawning new piston");
    new_piston->start();
    pistons.push_back( new_piston );
}

void Engine::restart() {
    DEBUG("Restarting");
    emergencyStop = false;
    error = false;
    DEBUG("Cleaning input");
    input.makeAllUntouched();
    DEBUG("Deleting all results");
    output->propagate_signal( Output::Engine_is_restarted );
    DEBUG("Restarted");
}

void Engine::collectPistons() {
    pistons.clear();
}

output::Traits Engine::convert_traits( const InputTraits& in ) {
    return output::Traits( 
        in.get_other_dimensionality<Localization::Dim>() );
}

void Engine::run() 
{
    DEBUG("Started run");

    int numPistons = config.pistonCount();

    DEBUG("Making SigmaGuesser");
    std::auto_ptr<Crankshaft> temporaryCrankshaft;
    if ( ! config.fixSigma() ) {
        Crankshaft *crankshaft = dynamic_cast<Crankshaft*>(output);
        std::auto_ptr<Output> guesser
            (new SigmaGuesserMean( config, input ));
        if ( crankshaft != NULL )  {
            crankshaft->push_front( guesser, Crankshaft::State );
        } else {
            temporaryCrankshaft.reset( new Crankshaft() );
            temporaryCrankshaft->add( guesser, Crankshaft::State );
            temporaryCrankshaft->add( *output );
            output = temporaryCrankshaft.get();
        }
    } else
        input.setDiscardingLicense( true );

    DEBUG("Announcing size");
    Output::Announcement announcement(convert_traits(input.getTraits()));
    DEBUG("Built announcement structure");

    Output::AdditionalData data 
        = output->announceStormSize(announcement);
    if ( data.test( output::Capabilities::ClustersWithSources ) ) {
        cerr << "The engine module cannot provide localization traces."
             << "Please select an appropriate transmission.\n";
        return;
    }

    DEBUG("Ready to fork " << numPistons << " computation threads");
#ifdef DSTORM_MEASURE_TIMES
    cerr << "Warning: Time measurement probes active" << endl;
    numPistons = 1;
#endif
    while (true) {
        for (int i = 0; i < numPistons-1; i++)
            addPiston();
        safeRunPiston();

        DEBUG("Collecting running pistons");
        collectPistons();
        DEBUG("Collected pistons");

        if ( globalStop)
            break;
        else if (emergencyStop) {
            if (error) {
                output->propagate_signal( Output::Engine_run_failed );
                break;
            } else
                restart();
        } else {
            output->propagate_signal( Output::Engine_run_succeeded );
            break;
        }
    }
    DEBUG("Finished run");
}

void Engine::safeRunPiston() throw()
{
    try {
        runPiston();
    } catch (const std::bad_alloc& e) {
        cerr << PACKAGE_NAME << " ran out of memory. Try removing the filter "
                "output module or reducing image resolution "
                "enhancement." << endl;
        emergencyStop = error = true;
    } catch (const std::exception& e) {
        cerr << PACKAGE_NAME << ": Error in computation: " << e.what() << endl;
        emergencyStop = error = true;
    } catch (...) {
        cerr << PACKAGE_NAME << ": Unknown engine failure." << endl;
        emergencyStop = error = true;
    }
}

void Engine::runPiston() 
{
    int maximumLimit = 20;
    const input::Traits<Image>& imProp = input.getTraits();
    DEBUG("Started piston");
    DEBUG("Building spot finder with dimensions " << imProp.dimx() <<
           " " << imProp.dimy());
    if ( ! config.spotFindingMethod.isValid() )
        throw std::runtime_error("No spot finding method selected.");
    auto_ptr<SpotFinder> finder
        = config.spotFindingMethod().make_SpotFinder
            (config, imProp.dimx(), imProp.dimy() );

    DEBUG("Building spot fitter");
    auto_ptr<SpotFitter> fitter(SpotFitter::factory(config));

    DEBUG("Building fit buffer");
    data_cpp::Vector<Localization> buffer;

    DEBUG("Building maximums");
    CandidateTree<SmoothedPixel> maximums
        (config.x_maskSize(), config.y_maskSize(), 1, 1);
    maximums.setLimit(maximumLimit);

    DEBUG("Initialized maximums");
    int origMotivation = config.motivation();

    DEBUG("Initialized motivation");
    Output::EngineResult resultStructure;
    resultStructure.smoothed = &finder->getSmoothedImage();
    resultStructure.candidates = &maximums;

    output->propagate_signal(Output::Engine_run_is_starting);

    for (Input::iterator i = input.begin(); i != input.end(); i++)
    {
        DEBUG("Intake (" << i->index() << ")");

        Claim< Image > claim = i->claim();
        if ( ! claim.isGood() ) continue;
        Image& image = *claim;

        PROGRESS("Compression (" << image.getImageNumber() << ")");
        IF_DSTORM_MEASURE_TIMES( clock_t prepre = clock() );
        finder->smooth(image);
        IF_DSTORM_MEASURE_TIMES( smooth_time += clock() - prepre );

        CandidateTree<SmoothedPixel>::iterator cM = maximums.begin();
        int motivation;
        recompress:  /* We jump here if maximum limit proves too small */
        IF_DSTORM_MEASURE_TIMES( clock_t pre = clock() );
        finder->findCandidates( maximums );
        PROGRESS("Found " << maximums.size() << " spots");

        IF_DSTORM_MEASURE_TIMES( clock_t search_start = clock() );
        IF_DSTORM_MEASURE_TIMES( search_time += search_start - pre );

        /* Motivational fitting */
        motivation = origMotivation;
        for ( cM = maximums.begin(); cM.hasMore() && motivation > 0; cM++){
            LOCKING("Trying candidate");
            const Spot& s = cM->second;
            /* Get the next spot to fit and fit it. */
            Localization *candidate = buffer.allocate();
            int found_number = 
                fitter->fitSpot(s, image, claim.index(), candidate);
            if ( found_number > 0 ) {
                LOCKING("Good fit");
                motivation = origMotivation;
                buffer.commit(found_number);
            } else if ( found_number < 0 ) {
                LOCKING("Bad fit");
                motivation += found_number;
            }
        }
        if (motivation > 0 && cM.limitReached()) {
            maximumLimit *= 2;
            PROGRESS("Raising maximumLimit to " << maximumLimit);
            maximums.setLimit(maximumLimit);
            buffer.clear();
            goto recompress;
        }

        PROGRESS("Found " << buffer.size() << " localizations");
        IF_DSTORM_MEASURE_TIMES( fit_time += clock() - search_start );

        PROGRESS("Power");
        resultStructure.forImage = claim.index();
        resultStructure.first = buffer.ptr();
        resultStructure.number = buffer.size();
        resultStructure.source = &image;

        Output::Result r = 
            output->receiveLocalizations( resultStructure );
        if (r == Output::RestartEngine || r == Output::StopEngine ) 
        {
            DEBUG("Emergency stop: Engine restart requested");
            output->propagate_signal( Output::Engine_run_is_aborted );
            emergencyStop = true;
            error = ( r == Output::StopEngine );
        }

        PROGRESS("Exhaust");
        buffer.clear();

        if (emergencyStop || globalStop) {
            PROGRESS("Emergency stop");
            break;
        }
    }
    DEBUG("Finished piston");
}

Engine::~Engine() {
    DEBUG("Destructing engine");
    emergencyStop = true;
    collectPistons();
    DEBUG("Destructed engine");
}

}
}
