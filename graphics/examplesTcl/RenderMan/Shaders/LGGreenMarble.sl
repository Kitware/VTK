/*
 * greenmarble.sl -- RenderMan compatible shader for green veined marble.
 *
 * DESCRIPTION:
 *   Makes a marble-like surface using a turbulence function.
 *   
 * 
 * PARAMETERS:
 *   Ka, Kd, Ks, roughness, specularcolor - work just like the plastic
 *   txtscale - overall scaling for the texture
 *   darkcolor, lightcolor - colors of the underlying substrate
 *   veincolor - color of the bright veins
 *   veinfreq - controls the frequency of the veining effects
 *   sharpness - how sharp the veins appear
 *
 *
 * AUTHOR: Larry Gritz, the George Washington University
 *         email: gritz@seas.gwu.edu
 *
 *
 * last modified  11 July 1994 by Larry Gritz
 */



surface
LGGreenMarble (float Ka = 0.1, Kd = 0.6, Ks = 0.4, roughness = 0.01;
	     color specularcolor = 1;
	     color darkcolor = color(0.01, 0.12, 0.004);
	     color lightcolor = color(0.06, 0.18, 0.02);
	     color veincolor = color(0.47, 0.57, 0.03);
	     float veinfreq = 1;
	     float sharpness = 25;
            )
{
#define snoise(x) (2*noise(x)-1)
  point PP, offset;
  float cmi;
  point Nf, V;
  color Ct;
  float pixelsize, twice, scale, freq;
  float turbsum, turb, i;

  PP = transform ("shader", P);

  /*
   * First calculate the underlying color of the substrate
   *    Use turbulence - use frequency clamping
   */
  pixelsize = sqrt(area(PP));
  twice = 2 * pixelsize;
  turb = 0;
  for (scale = 1; scale > twice; scale /= 2)
      turb += scale * abs(noise(PP/scale)-0.5);
  if (scale > pixelsize)
      turb += clamp ((scale/pixelsize)-1, 0, 1) * scale * abs(noise(PP/scale)-0.5);

  Ct = mix (darkcolor, lightcolor, smoothstep(0.1,.35,turb));

  /*
   * Now we layer on the veins
   */

  /* perturb the lookup */
  freq = 1;
  offset = point(35.2,-21.9,6.25);
  /* This offset makes it uncorrelated to the substrate pattern */
  for (i = 0;  i < 6;  i += 1) {
      offset += (point noise (freq * PP) - point(.5,.5,.5))  / freq;
      freq *= 2;
    }
  PP += offset;

  /* Now calculate the veining function for the lookup area */
  turbsum = 0;  freq = 1;
  PP *= veinfreq;
  for (i = 0;  i < 3;  i += 1) {
      turb = abs (snoise (PP*freq));
      turb = pow (smoothstep (0.8, 1, 1 - turb), sharpness) / freq;
      turbsum += (1-turbsum) * turb;
      freq *= 2;
    }
  turbsum *= smoothstep (-0.1, 0.05, snoise(2*(PP+point(-4.4,8.34,27.1))));

  Ct = mix (Ct, veincolor, turbsum);
  
  /*
   * Shade like plastic
   */
  Oi = Os;
  Nf = faceforward (normalize(N), I);
  V = -normalize (I);
  Ci = Os * (Ct * (Ka*ambient() + Kd*diffuse(Nf)) +
	     specularcolor * Ks * specular(Nf,V,roughness));
}
