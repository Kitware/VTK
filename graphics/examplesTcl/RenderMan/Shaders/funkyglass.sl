/* funkyglass.sl - randomly colored "glass" (transparent, but no refl/refr).
 * (c) Copyright 1994, Larry Gritz
 *
 * The RenderMan (R) Interface Procedures and RIB Protocol are:
 *     Copyright 1988, 1989, Pixar.  All rights reserved.
 * RenderMan (R) is a registered trademark of Pixar.
 */



surface
funkyglass (float Ka = .2;
	    float Kd = .2;
	    float Ks = 1;
	    float roughness = .08;
	    color specularcolor = 1;)
{
  point PP, Nf, V;
  color Ct, Ot;

  V = normalize(I);
  Nf = faceforward (normalize(N),V);
  PP = transform ("shader", P);
  Ct = 2 * (color noise (PP) - .5) + .5;
  Ot = (comp(Ct,0) + comp(Ct,1) + comp(Ct,2))/3 + (1-Ct);

  Oi = Ot * (0.75 - 0.5*abs(V.Nf));
  Ci = ( Ct * (Ka*ambient() + Kd*diffuse(Nf)) +
	      specularcolor * Ks*specular(Nf,-V,roughness));
}

