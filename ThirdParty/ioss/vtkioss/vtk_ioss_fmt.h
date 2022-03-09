#ifndef vtkioss_fmt_h
#define vtkioss_fmt_h

/**
 * Ioss uses fmt API not yet available in a release. To avoid forcing an fmt
 * update to a development version, we use this hack.
 */
#include "vtk_fmt.h"
#define group_digits(x) format("{}", x)

#endif
