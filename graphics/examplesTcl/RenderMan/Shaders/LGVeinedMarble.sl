/*
 * veinedmarble.sl -- surface shader for a nice veined marble.
 *
 * DESCRIPTION:
 *   Makes solid marble texture with strong veins.  The "veincolor" parameter
 *   controls the color of the veins.  The background color is given by the
 *   surface color (Cs).
 * 
 * PARAMETERS:
 *   Ka, Kd, Ks, roughness, specularcolor - same as plastic
 *   veinfreq - controls fhe lowest frequency of the color veins
 *   veinlevels - how many "levels" of vein tendrills it has
 *   warpfreq - lowest frequency of the turbulent warping in the marble
 *   warping - controls how much turbulent warping there will be
 *   veincolor - the color of the veins
 *   sharpness - controls how sharp or fuzzy the veins are (higher = sharper)
 *
 *
 * AUTHOR: Larry Gritz, the George Washington University
 *         email: gritz@seas.gwu.edu
 *
 * HISTORY:
 *
 * last modified  29 Jun 1994 by Larry Gritz
 */





surface
LGVeinedMarble (float Ka = .5;
	      float Kd = .8;
	      float Ks = .4;
	      float roughness = .005;
	      color specularcolor = 1;
	      float veinfreq = 1;
	      float veinlevels = 2;
	      float warpfreq = 1;
	      float warping = .5;
	      color veincolor = color(.6,.5,.1);
	      float sharpness = 8;
             )
{
#define snoise(x) (2*noise(x)-1)
  color Ct;
  point Nf;
  point PP, offset;
  float i, turb, freq, j;
  float turbsum;

  PP = transform ("shader", P);

  /* perturb the lookup */
  freq = 1;
  offset = 0;
  for (i = 0;  i < 6;  i += 1) {
      offset += 2 * warping * (point noise (warpfreq * freq * PP) - 0.5)  / freq;
      freq *= 2;
    }
  PP += offset;

  /* Now calculate the veining function for the lookup area */
  turbsum = 0;  freq = 1;
  PP *= veinfreq;
  for (i = 0;  i < veinlevels;  i += 1) {
      turb = abs (snoise (PP));
      turb = pow (smoothstep (0.8, 1, 1 - turb), sharpness) / freq;
      turbsum += (1-turbsum) * turb;
      freq *= 3;
      PP *= 3;
    }

  Ct = mix (Cs, veincolor, turbsum);

  Oi = Os;
  Nf = faceforward (normalize(N),I);
  Ci = Os * ( Ct * (Ka*ambient() + Kd*diffuse(Nf)) +
	      specularcolor * Ks*specular(Nf,-normalize(I),roughness));
}
