/* Listing 16.15  Surface shader providing  wood-grain texture*/
/*
 *  wood(): calculate a solid wood texture using noise()
 */
surface
wood(
	float	ringscale	= 10;
	color	lightwood	= color (0.3, 0.12, 0.03),
		darkwood	= color (0.05, 0.01, 0.005);
	float	Ka		= 0.2,
		Kd		= 0.4,
		Ks		= 0.6,
		roughness	= 0.1 )
{
	 point NN, V;
	 point PP;
	 float y, z, r;
	/*
	 * Compute the forward-facing normal NN and the vector
	 * toward the ray origin V, both normalized.
	 * These vectors are used by "specular" and "diffuse". */
	NN = faceforward(normalize(N),I);
	V = -normalize(I);

	/* put point in shader space and perturb it to add irregularity */
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
	Ci = Oi * Ci * (Ka * ambient() + Kd * diffuse(NN))
		 + (0.3*r + 0.7) * Ks * specular(NN, V, roughness);
}


