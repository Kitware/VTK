/*
 * stucco.sl -- displacement shader for stucco
 *
 * DESCRIPTION:
 *   Displacees a surface to make it look like stucco.
 * 
 * PARAMETERS:
 *   Km 	   	the amplitude of the stucco pimples
 *   power	   	controls the shape of the pimples
 *   frequency  	the frequency of the pimples
 *
 *
 * AUTHOR: written by Larry Gritz
 *
 * HISTORY:
 *      May 1992 -- written by lg
 *      12 Jan 1994 -- recoded by lg in correct shading language.
 *
 * last modified  12 Jan 1994 by Larry Gritz
 */



displacement
stucco ( float Km = 0.05, power = 5, frequency = 10; )
{
  float size;
  float magnitude;
  point PP;
  point Ndiff;

  PP = transform ("shader", P);
  magnitude = Km * pow (noise (PP*frequency), power);
  Ndiff = normalize(N) - normalize(Ng);
  P += magnitude * normalize (N);

  /* correct the normal */
  N = normalize(calculatenormal (P)) + Ndiff;
}
