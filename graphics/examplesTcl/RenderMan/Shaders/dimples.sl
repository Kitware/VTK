/*  S. Kirk Bowers  */

/*  Dimples displacement shader  */

#include "rmannotes.sl"

displacement dimples
(
  float  Km = -0.2;

  float  fuzz = 0.05;

  float  sfreq = 10,
         tfreq = 10;
)

{
  float  surface_mag, layer_mag;
  float  mag_opac;
  float  ss, tt;
  float  row, col;
  float  radius, d;
  float  noi;
  point  center;
  point sssttt;
  float  x, y;
  float  sss, ttt;

  surface_mag = 0;

  ss = s;
  tt = t;
  row = whichtile(tt, tfreq);
  col = whichtile(ss, sfreq);
  ss = repeat(ss, sfreq);
  tt = repeat(tt, tfreq);

  center = (0.5, 0.5, 0);

  for (x = -1; x <= 1; x += 1)
  {
    for (y = -1; y <= 1; y += 1)
    {
      sss = ss - x;
      ttt = tt - y;

  noi = noise((col + x) * 10 + 0.5, (row + y) * 10 + 0.5);

  sss = sss + udn(noi * 1183, -0.25, 0.25);
  ttt = ttt + udn(noi * 999, -0.25, 0.25);
  radius = 0.5 + udn(noi * 2747, -0.1, 0.1);

  sssttt = (sss, ttt, 0.0);
/*  d = distance(center, sssttt);*/
  d = length(center-sssttt);
  mag_opac = 1 - smoothstep(radius - fuzz, radius, d);
  mag_opac = mag_opac * (radius * radius - d * d);
  layer_mag = mag_opac;
  surface_mag = max(surface_mag, layer_mag);
    }
  }

  
  P += Km * surface_mag * normalize(N);
  N = calculatenormal(P);
}  



