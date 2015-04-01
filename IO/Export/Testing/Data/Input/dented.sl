/* Listing 16.16  Displacement shader to dent a surface*/
/*
 * dented(): Create a worn surface.
 */
displacement
dented (float	Km	= 1.0 )
{
	float	size 	  = 1.0,
		magnitude = 0.0,
		i;
	point P2;
        normal Ndiff;

	P2 = transform("shader", P);
	for (i = 0; i < 6.0; i += 1.0) {
		/* Calculate a simple fractal 1/f noise function */
		magnitude += abs(.5 - noise(P2 * size)) / size;
		size *= 2.0;
	}
        Ndiff = normalize(N) - normalize(Ng);
	P2 = P - normalize(N) * (magnitude * magnitude * magnitude) * Km;

        /* correct the normal */
        N = normalize(calculatenormal (P2)) + Ndiff;
}
