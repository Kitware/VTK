/* @(#)spatter.sl	1.1 (Pixar - RenderMan Division) 10/10/89 */

/*-______________________________________________________________________
** 
** Copyright (c) 1989 PIXAR.  All rights reserved.  This program or
** documentation contains proprietary confidential information and trade
** secrets of PIXAR.  Reverse engineering of object code is prohibited.
** Use of copyright notice is precautionary and does not imply
** publication.
** 
**                      RESTRICTED RIGHTS NOTICE
** 
** Use, duplication, or disclosure by the Government is subject to the 
** following restrictions:  For civilian agencies, subparagraphs (a) through
** (d) of the Commercial Computer Software--Restricted Rights clause at
** 52.227-19 of the FAR; and, for units of the Department of Defense, DoD
** Supplement to the FAR, clause 52.227-7013 (c)(1)(ii), Rights in
** Technical Data and Computer Software.
** 
** Pixar
** 3240 Kerner Blvd.
** San Rafael, CA  94901
** 
** ______________________________________________________________________
*/

/*---------------------------------------------------------------------------
 *	spatter - make surface look dark blue with white paint spatters,
 *	like on that camping cookware. (Or any base color with any color
 *	paint spatters).
 *
 *	Ka, Kd, Ks, roughness, specularcolor - the usual meaning
 *	specksize - size of the smallest paint specks
 *	sizes - number of different sizes of paint specks (each double
 *		the previous size)
 *	basecolor - the background color on which to spatter paint
 *	spattercolor - the color of the paint spatters
 *--------------------------------------------------------------------------*/
surface
spatter(float Ka = 1, Kd = 0.5, Ks = 0.7, roughness = 0.2, specksize = .01,
	sizes = 5; color specularcolor = 1, basecolor = color (.1,.1,0.5),
	spattercolor = color (1,1,1))
{
	varying float speckle,size,scalefac,
		      threshold = 0.6;
	varying color paint;
	varying point Nf, V;

	Nf = faceforward(normalize(N),I);
	V = -normalize(I);

    /* "OR" together various-sized noise spots. If any white, make paint white*/
	scalefac = 1/specksize;
	paint = basecolor;
	for (size=1; size<=sizes; size +=1) {
		speckle = noise(transform("shader",P)*scalefac);
		if (speckle > threshold) {
			paint = spattercolor;
			break;
		}
		scalefac /= 2;
	}

    /* get final color */
	Oi = Os;
	Ci = Os * (paint * (Ka * ambient() + Kd * diffuse(Nf)) +
		specularcolor * Ks * specular(Nf,V,roughness));
}
