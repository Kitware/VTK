/*
 *  ivory(): modifed from prman/tutorial/ch16/plastic.sl
 */
surface 
ivory( 
	float	Ks			= .5, 
		Kd			= .5, 
		Ka			=  1, 
		roughness		= .1; 
	color	specularcolor		=  1;
        )
{
	point OP;

	point Nf;
	point V;

	string texturename = "marks.txt";

	point NN;
	point PP;
	float x, y, z, r;

	PP = transform("object", P);

	/* ivory                                                     */

	Nf = faceforward(normalize(N), I );
	V = normalize(-I);

	Oi = Os;
	/* Intensity falls off away from direction to camera,        */
	/* simulating subsurface scattering in ivory                 */
	/* (idea from Pat Hanrahan)                                  */
	Ci = Os * ( Cs * (Ka*ambient() + Kd*diffuse(Nf)) + 
		specularcolor * Ks * specular(Nf,V,roughness) ) *
		(normalize(-I) . normalize(N));
}
