/*
 * velvet.sl -- velvet
 *
 * DESCRIPTION:
 *   An attempt at a velvet surface.
 *   This phenomenological model contains three compnents:
 *   - A retroreflective lobe (back toward the light source)
 *   - Scattering near the horizon, regardless of incident direction
 *   - A diffuse color
 * 
 * PARAMETERS:
 *   Ks:	controls retroreflective lobe
 *   Kd:	scales diffuse color
 *   Ka:	ambient component (affects diffuse color only)
 *   sheen:	color of retroreflective lobe and horizon scattering
 *   roughness: shininess of fabric (controls retroreflection only)
 *   Cs:	diffuse color
 *
 * ANTIALIASING: should antialias itself fairly well
 *
 * AUTHOR: written by Stephen H. Westin, Ford Motor Company
 *
 * HISTORY:
 *
 * last modified  28 January 1997 S. H. Westin
 */

#define SQR(A) ((A)*(A))

surface
velvet (float Ka = 0.05,
              Kd = 0.1,
	      Ks = 0.1;
	color sheen = .25;
	float roughness = .1;
  )
{
  point Nf;			 /* Normalized normal vector */
  vector V;			 /* Normalized eye vector */
  vector H;			 /* Bisector vector for Phong/Blinn */
  vector Ln;			 /* Normalized vector to light */
  color shiny;			 /* Non-diffuse components */
  float cosine, sine;		 /* Components for horizon scatter */

  Nf = faceforward (normalize(N), I);
  V = -normalize (I);

  shiny = 0;
  illuminance ( P, Nf, 1.57079632679489661923 /* Hemisphere */ ) {
    Ln = normalize ( L );
    /* Retroreflective lobe */
    cosine = max ( -Nf.V, 0 );
    shiny += pow ( cosine, 1.0/roughness ) / ( Ln.Nf ) * Cl * sheen;
    /* Horizon scattering */
    cosine = max ( Nf.V, 0 );
    sine = sqrt (1.0-SQR(cosine));
    shiny += pow ( sine, 10.0 ) * Ln.Nf * Cl * sheen;
  }

  Oi = Os;
  /* Add in diffuse color */
  Ci = Os * (Ka*ambient() + Kd*diffuse(Nf)) * Cs + shiny;

}


