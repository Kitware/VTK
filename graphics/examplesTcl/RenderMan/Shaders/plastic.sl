/* @(#)plastic.sl	1.7	(Pixar - RenderMan Division)	10/3/88 */

/*-______________________________________________________________________
** 
** Copyright (c) 1988 PIXAR.  All rights reserved.  This program or
** documentation contains proprietary confidential information and trade
** secrets of PIXAR.  Reverse engineering of object code is prohibited.
** Use of copyright notice is precautionary and does not imply
** publication.
** 
**                      RESTRICTED RIGHTS NOTICE
** 
** Use, duplication, or disclosure by the Government is subject to
** restrictions as set forth in subdivision (b)(3)(ii) of the Rights in
** Technical Data and Computer Software clause at 252.227-7013.
** 
** Pixar
** 3240 Kerner Blvd.
** San Rafael, CA  94901
** 
** ______________________________________________________________________
*/

surface 
plastic( float Ks=.5, Kd=.5, Ka=1, roughness=.1; color specularcolor=1 )
{
    point Nf, V;

    Nf = faceforward( normalize(N), I );
    V = -normalize(I);

    Oi = Os;
    Ci = Os * ( Cs * (Ka*ambient() + Kd*diffuse(Nf)) + 
	 	specularcolor * Ks * phong(Nf,V,1.0/roughness) );
}
