#include "ProgressMeter.h"
#include "help_context.h"

namespace dStorm {

ProgressMeter::Config::Config() 
: simparm::Object( "Progress", "Display progress" ) 
{
    userLevel = Intermediate;
}

ProgressMeter::ProgressMeter(const Config &)
    : Output(),
      Object("ProgressMeter", "Progress status"),
      progress("Progress", "Progress on this job") 
    {
        progress.helpID = HELP_ProgressMeter_Progress;
        progress.setEditable(false);
        progress.setViewable(true);
        progress.setUserLevel(simparm::Entry::Beginner);
        progress.setIncrement(0.02);
        push_back(progress);
    }

}
