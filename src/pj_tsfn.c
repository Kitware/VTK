/* determine small t */
#include <math.h>
#include <projects.h>
#define HALFPI		1.5707963267948966
	double
pj_tsfn(double phi, double sinphi, double e) {
	sinphi *= e;
	return (tan (.5 * (HALFPI - phi)) /
	   pow((1. - sinphi) / (1. + sinphi), .5 * e));
}
