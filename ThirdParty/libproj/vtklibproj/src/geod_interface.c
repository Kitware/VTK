#include "projects.h"
#include "geod_interface.h"

/* DEG_IN is a crock to work around the problem that dmstor.c uses the wrong
 * value for pi/180 (namely .0174532925199433 which is an inaccurately
 * truncated version of DEG_TO_RAD).
 */
#define DEG_IN .0174532925199433
#define DEG_OUT DEG_TO_RAD;

void geod_ini(void) {
  geod_init(&GlobalGeodesic, geod_a, geod_f);
}

void geod_pre(void) {
  double
    lat1 = phi1 / DEG_IN, lon1 = lam1 / DEG_IN, azi1 = al12 / DEG_IN;
  geod_lineinit(&GlobalGeodesicLine, &GlobalGeodesic, lat1, lon1, azi1, 0U);
}

void geod_for(void) {
  double
    s12 = geod_S, lat2, lon2, azi2;
  geod_position(&GlobalGeodesicLine, s12, &lat2, &lon2, &azi2);
  azi2 += azi2 >= 0 ? -180 : 180; /* Compute back azimuth */
  phi2 = lat2 * DEG_OUT;
  lam2 = lon2 * DEG_OUT;
  al21 = azi2 * DEG_OUT;
}

void geod_inv(void) {
  double
    lat1 = phi1 / DEG_IN, lon1 = lam1 / DEG_IN,
    lat2 = phi2 / DEG_IN, lon2 = lam2 / DEG_IN,
    azi1, azi2, s12;
  geod_inverse(&GlobalGeodesic, lat1, lon1, lat2, lon2, &s12, &azi1, &azi2);
  azi2 += azi2 >= 0 ? -180 : 180; /* Compute back azimuth */
  al12 = azi1 * DEG_OUT; al21 = azi2 * DEG_OUT; geod_S = s12;
}
