/* Listing 16.29  Plastic surface shader using a texture map*/
/* 
 *  txtplastic(): version of plastic() shader using an optional texture map
 */
surface 
txtplastic( 
	float	Ks		= .5, 
		Kd		= .5, 
		Ka		=  1, 
		roughness	= .1;
	color 	specularcolor = 1;
	string	mapname = "")
{
	point	Nf = faceforward( normalize(N), I );

	if( mapname != "" ) {
		Ci = color texture( mapname );	/* Use s and t */
	        Ci = Ci * Cs;
		Oi = color texture( mapname[3] );	/* Use s and t */
		setcomp (Oi, 1, comp (Oi, 0));
		setcomp (Oi, 2, comp (Oi, 0));
	        Oi = Oi * Os;
	}
	else {
		Ci = Cs;
		Oi = Os;
	}
	Ci = Oi * ( Ci * 
		    (Ka*ambient() + Kd*diffuse(Nf)) 
	 	+ specularcolor * Ks * specular(Nf,-I,roughness) );
}

