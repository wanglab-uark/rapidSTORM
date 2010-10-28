#ifndef DSTORM_JOBSTARTER_H
#define DSTORM_JOBSTARTER_H

#include <simparm/TriggerEntry.hh>
#include <dStorm/Config.h>
#include <dStorm/JobMaster.h>

namespace dStorm {

class JobStarter
: public simparm::TriggerEntry,
  simparm::Listener
{
    JobMaster* master;
    const Config* config;

    void operator()( const simparm::Event& );
  public:
    JobStarter( JobMaster* );
    void setConfig( const Config& config ) 
        { this->config= &config; }
};

}

#endif