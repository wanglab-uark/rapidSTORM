#ifndef TESTPLUGIN_BASENAME_PRINTER_H
#define TESTPLUGIN_BASENAME_PRINTER_H

#include <simparm/Entry.hh>
#include <simparm/NumericEntry.hh>
#include <dStorm/output/Output.h>
#include <dStorm/output/FileOutputBuilder.h>
#include <iostream>
#include <stdexcept>
#include <boost/units/io.hpp>

struct BasenamePrinter
: public dStorm::output::OutputObject
{
    struct _Config;
    typedef simparm::Structure<_Config> Config;
    typedef dStorm::output::FileOutputBuilder<BasenamePrinter> Source;

    BasenamePrinter(const Config& config);
    BasenamePrinter* clone() const;

    AdditionalData announceStormSize(const Announcement& a) 
        { return AdditionalData(); }
    Result receiveLocalizations(const EngineResult& er) 
        { return KeepRunning; }
    void propagate_signal(ProgressSignal s) {
    }

};

struct BasenamePrinter::_Config
 : public simparm::Object 
{
    simparm::FileEntry outputFile;

    _Config();
    void registerNamedEntries() {}
    bool can_work_with(const dStorm::output::Capabilities&)
        {return true;}
};

BasenamePrinter::_Config::_Config()
 : simparm::Object("BasenamePrinter", "BasenamePrinter"),
   outputFile("ToFile", "")
{
}

BasenamePrinter* BasenamePrinter::clone() const { 
    return new BasenamePrinter(*this); 
}

BasenamePrinter::BasenamePrinter( const Config& config )
        : OutputObject("SegFault", "SegFault")
{
    std::cerr << "Output file basename is " << config.outputFile() << "\n";
}

#endif
