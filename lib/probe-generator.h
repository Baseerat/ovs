#ifndef PROBE_GENERATOR_H
#define PROBE_GENERATOR_H

#include <config.h>

struct dp_packet;

#ifdef PROBE_GENERATOR

int probe_generator_init(int argc, char **argv);

//#include "util.h"
//
//static inline int
//probe_generator_init(int argc, char **argv)
//{
//    // TODO: Check for input.
//    return 0;
//}

#endif /* PROBE_GENERATOR */
#endif /* PROBE_GENERATOR_H */