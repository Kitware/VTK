/* spotlight.sl - Standard spot light source for RenderMan Interface.
 * (c) Copyright 1988, Pixar.
 *
 * The RenderMan (R) Interface Procedures and RIB Protocol are:
 *     Copyright 1988, 1989, Pixar.  All rights reserved.
 * RenderMan (R) is a registered trademark of Pixar.
 */

light
spotlight ( float intensity = 1;
	    color lightcolor = 1;
	    point from = point "shader" (0,0,0);
	    point to = point "shader" (0,0,1);
	    float coneangle = radians(30);
	    float conedeltaangle = radians(5);
	    float beamdistribution = 2; )
{
  float atten, cosangle;
  uniform vector A = normalize(to-from);

  illuminate (from, A, coneangle) {
      cosangle = (L . A) / length(L);
      atten = pow (cosangle, beamdistribution) / (L . L);
      atten *= smoothstep (cos(coneangle), cos(coneangle-conedeltaangle),
			   cosangle);
      Cl = atten * intensity * lightcolor ;
    }
}

