#include "Config.h"
#include <dStorm/helpers/thread.h>
#include <simparm/ChoiceEntry_Impl.hh>

#define NAME "Acquisition"
#define DESC "Acquisition parameters"

namespace AndorCamera {
_Config::_Config()
: Set(NAME, DESC),
  targetTemperature("ReferenceTemperature", "Desired CCD temperature",-75),
  outputAmp("OutputAmplifier", "Output Amplifier"),
  VS_Speed("VSSpeed", "Vertical shift time"),
  HS_Speed("HSSpeed", "Horizontal shift speed (MHz)"),
  emccdGain("EMCCDGain", "EM CCD Gain multiplier"),
  realExposureTime("RealExposureTime", "Used exposure time (s)"),
  cycleTime("KineticCycleTime", "Used kinetic cycle time (s)")
{
    PROGRESS("Start Config");

    targetTemperature.setMax(30);
    targetTemperature.setMin(-100);
    targetTemperature.setHelp("Try to cool the camera to this value.");
    targetTemperature.setUserLevel(simparm::Entry::Beginner);

    realExposureTime.editable = false;
    cycleTime.editable = false;

    outputAmp.addChoice( 
            ElectronMultiplication, "EM", "Electron multiplying" );
    outputAmp.addChoice(
            ConventionalAmplification, "Conventional","Conventional");

    PROGRESS("End Config");
}

void _Config::registerNamedEntries() {
    push_back(targetTemperature);
    push_back(outputAmp);
    push_back(VS_Speed);
    push_back(HS_Speed);
    push_back(emccdGain);
    push_back(realExposureTime);
    push_back(cycleTime);
}

}