/*  Fish scales shader  */
#include "rmannotes.sl"

surface fish_scales
(
  float Ka = 0.5,
        Kd = 0.5,
        Ks = 1,
        roughness = 0.1;
  color specularcolor = color (1, 1, 1),
        top_fish_color = 1,
        bottom_fish_color = color (0.9, 0.2, 0.2),
        top_odd_scale = color (0.2, 0.9, 1),
        bottom_odd_scale = color (1, 0.5, 0.2),
        top_even_scale = color (0, 1, 0),
        bottom_even_scale = color (1, 0, 0),
        divider_color = color (1, 0, 0);
  float sfreq = 14, 
        tfreq = 10,
        divider_width = 0.03,
        divider_opac = 1,
        fuzz = 0.03,
        head_tiles = 1,
        head_opac = 1,
        odd_opac = 0.5,
        even_opac = 0.5;
)

{
  float ss, tt;
  color surface_color, layer_color;
  float layer_opac;
  float row, odd_col, even_col;
  point Nf, V;
  float this_parab, top_left_parab, top_right_parab;
  float scale_mix;


  Nf = faceforward(normalize(N), I);
  V = normalize(-I);

  /*---  layer 0  ---*/
  /* mix from top fish color to bottom fish color  */

  surface_color = mix(top_fish_color, bottom_fish_color, t);


  /*  For the rest of the layers, the pattern is tiled  */

  ss = repeat(s, sfreq);
  tt = repeat(t, tfreq);

  row = whichtile(t, tfreq);
  odd_col = whichtile(s, sfreq);
  even_col = floor(s * sfreq + 0.5);
  if (even(row))
    ss = mod(ss + 0.5, 1.0);

  this_parab = 4 * ss * ss - 4 * ss + 1;
  top_left_parab = 4 * ss * ss - 1;
  top_right_parab = 4 * ss * ss - 8 * ss + 3;

  /*--------------------  layer 1  -----------------------*/
  /*  Fill the area below the parabola                    */

  layer_opac = smoothstep(this_parab - fuzz / 2, this_parab + fuzz / 2, tt);
  scale_mix = (tt - this_parab) / 2;

  if (row < head_tiles)
  {
    layer_color = divider_color;
    layer_opac *= head_opac;
  }
  else if (even(row) && even(even_col) || odd(row) && odd(odd_col))
  {
    layer_color = mix(top_even_scale, bottom_even_scale, scale_mix);
    layer_opac *= even_opac;
  }
  else
  {
    layer_color = mix(top_odd_scale, bottom_odd_scale, scale_mix);
    layer_opac *= odd_opac;
  }

  surface_color = mix(surface_color, layer_color, layer_opac);


  /*--------------------  layer 2  -----------------------*/
  /*  Fill the area to the top left of the parabola       */

  layer_opac = pulse(top_left_parab, this_parab, fuzz, tt);
  layer_opac = intersection(layer_opac, 
                 complement(smoothstep(0.5 - fuzz / 2, 0.5 + fuzz / 2, ss)));
  scale_mix = (tt - top_left_parab) / 2;

  if (row <= head_tiles)
  {
    layer_color = divider_color;
    layer_opac *= head_opac;
  }
  else if (even(row) && even(even_col) || odd(row) && even(odd_col))
  {
    layer_color = mix(top_even_scale, bottom_even_scale, scale_mix);
    layer_opac *= even_opac;
  }
  else
  {
    layer_color = mix(top_odd_scale, bottom_odd_scale, scale_mix);
    layer_opac *= odd_opac;
  }

  surface_color = mix(surface_color, layer_color, layer_opac);


  /*--------------------  layer 3  -----------------------*/
  /*  Fill the area to the top right of the parabola      */

  layer_opac = pulse(top_right_parab, this_parab, fuzz, tt);
  layer_opac = intersection(layer_opac, 
                 smoothstep(0.5 - fuzz / 2, 0.5 + fuzz / 2, ss));
  scale_mix = (tt - top_right_parab) / 2;

  if (row <= head_tiles)
  {
    layer_color = divider_color;
    layer_opac *= head_opac;
  }
  else if (even(row) && odd(even_col) || odd(row) && odd(odd_col))
  {
    layer_color = mix(top_even_scale, bottom_even_scale, scale_mix);
    layer_opac *= even_opac;
  }
  else
  {
    layer_color = mix(top_odd_scale, bottom_odd_scale, scale_mix);
    layer_opac *= odd_opac;
  }

  surface_color = mix(surface_color, layer_color, layer_opac);



  /*-----------------------  layer 4  -----------------------------*/
  /*  Put the dividers around the scales  */

  layer_color = divider_color;
  layer_opac = pulse(this_parab - divider_width, this_parab + divider_width, 
                     fuzz, tt);
  layer_opac *= divider_opac;
  surface_color = mix(surface_color, layer_color, layer_opac);


  /*  illumination  */

  Oi = Os;
  Ci = Os * (surface_color * (Ka * ambient() + Kd * diffuse(Nf)) +
          specularcolor * Ks * specular(Nf, V, roughness));

}
