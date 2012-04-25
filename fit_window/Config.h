#ifndef DSTORM_FIT_WINDOW_CONFIG_H
#define DSTORM_FIT_WINDOW_CONFIG_H

#include <dStorm/UnitEntries/Nanometre.h>
#include <simparm/Set.hh>

namespace dStorm {
namespace fit_window {

/** This class collects configuration options for the GUF fitter. */
struct Config
{
    static const int maximum_plane_count = 9;

    Config();
    void registerNamedEntries( simparm::Node& at );
    dStorm::FloatNanometreEntry fit_window_size;
    simparm::BoolEntry allow_disjoint, double_computation;
};

}
}

#endif