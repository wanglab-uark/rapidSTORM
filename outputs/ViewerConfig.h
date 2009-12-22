#ifndef DSTORM_VIEWER_CONFIG_H
#define DSTORM_VIEWER_CONFIG_H

#include <outputs/Viewer.h>
#include <simparm/ChoiceEntry.hh>
#include <simparm/ChoiceEntry_Impl.hh>

namespace dStorm {
namespace output {

class Viewer::_Config : public simparm::Object {
  public:

    simparm::BoolEntry showOutput;
    simparm::FileEntry outputFile;
    simparm::DoubleEntry res_enh;
    simparm::UnsignedLongEntry refreshCycle;
    simparm::DoubleEntry histogramPower;
    simparm::ChoiceEntry colourScheme;
    simparm::DoubleEntry hue, saturation;
    simparm::BoolEntry invert, close_on_completion;

    _Config();

    void registerNamedEntries();
    static bool can_work_with(Capabilities) { return true; }
};

}
}

#endif
