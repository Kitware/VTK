#ifdef RCSIDS
static char rcsid[] = "Id";
#endif

/*
 * distantlight.sl
 * $Revision$
 * $Date$
 *
 * Log: distantlight.sl
 * Revision 1.1  1995-12-13 11:46:36-08  lg
 * Initial revision
 *
 */

/* distantlight.sl - Standard distant light source for RenderMan Interface.
 * (c) Copyright 1988, Pixar.
 *
 * The RenderMan (R) Interface Procedures and RIB Protocol are:
 *     Copyright 1988, 1989, Pixar.  All rights reserved.
 * RenderMan (R) is a registered trademark of Pixar.
 */

light
distantlight ( float intensity = 1;
	       color lightcolor = 1;
	       point from = point "shader" (0,0,0);
	       point to = point "shader" (0,0,1); )
{
  solar (to-from, 0)
      Cl = intensity * lightcolor;
}

