/*
 *  ivory(): modifed from prman/tutorial/ch16/plastic.sl
 */
surface 
ivorywood( 
	float	Ks			= .5, 
		Kd			= .5, 
		Ka			=  1, 
		roughness		= .1; 
	color	specularcolor		=  1;
/*	float	ringscale	= 500;*/
	float	ringscale	= .2;
	color	lightwood	= color (0.3, 0.0, 0.0),
		darkwood	= color (0.1, 0.0, 0.0);
	float	woodKa		= 0.2,
		woodKd		= 0.4,
		woodKs		= 0.8,
		woodroughness	= 0.1 )
{
	point OP;

	point Nf;
	point V;

	string texturename = "marks.txt";



	point NN;
	point PP;
	float x, y, z, r;

	PP = transform("object", P);

	if (ycomp(PP) > 0.081) {
		/* ivory                                                     */

		Nf = faceforward(normalize(N), I );
		V = normalize(-I);

		/* Dent the ivory using displacement mapping                 */
		/* Texture is from bowling pin                               */
		/*
		x = xcomp(normalize(I)) + 0.5;
		y = ycomp(normalize(I)) + 0.5;
		z = float texture(texturename,x,y);
		P -= 0.05 * z * normalize(N);
		N = calculatenormal(P);
		*/

		Oi = Os;
		/* Intensity falls off away from direction to camera,        */
		/* simulating subsurface scattering in ivory                 */
		/* (idea from Pat Hanrahan)                                  */
		Ci = Os * ( Cs * (Ka*ambient() + Kd*diffuse(Nf)) + 
			specularcolor * Ks * specular(Nf,V,roughness) ) *
			(normalize(-I) . normalize(N));

	}
	else {
		/* rosewood                                                  */

		/*
		 * Compute the forward-facing normal NN and the vector
		 * toward the ray origin V, both normalized.
		 * These vectors are used by "specular" and "diffuse". */
		NN = faceforward(normalize(N),I);
		V = -normalize(I);

		/* put point in shader space                                 */
		/* and perturb it to add irregularity                        */
		PP = transform("shader", P);
		PP += noise(PP);

		/* compute radial distance r from PP to axis of "tree" */
		y = ycomp(PP);
		z = zcomp(PP);
		r = sqrt(y*y + z*z);

		/* map radial distance r into ring position [0,1] */
		r *= ringscale;
		r += abs(noise(r));
		r -= floor(r);			/* == mod(r,1) */

		/* use ring position r to select wood color */
		r = smoothstep(0, 0.8, r) - smoothstep(0.83, 1.0, r);
		Ci = mix(lightwood, darkwood, r);

		/* shade using r to vary shininess */
		Oi = Os;
		Ci = Oi * Cs * Ci * (Ka * ambient() + Kd * diffuse(NN))
			 + (0.3*r + 0.7) * Ks * specular(NN, V, roughness);
	}
}
