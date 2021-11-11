/* reduce argument to range +/- PI */
#include <math.h>

#include "proj.h"
#include "proj_internal.h"

double adjlon (double lon) {
    /* Let lon slightly overshoot, to avoid spurious sign switching at the date line */
    if (fabs (lon) < M_PI + 1e-12)
        return lon;

    /* adjust to 0..2pi range */
    lon += M_PI;

    /* remove integral # of 'revolutions'*/
    lon -= M_TWOPI * floor(lon / M_TWOPI);

    /* adjust back to -pi..pi range */
    lon -= M_PI;

    return lon;
}
