/* 
 * granite(): Provide a diffuse granite-like surface texture. 
 */
surface 
granite(
	float	Kd	= .8,
		Ka	= .2,
	        frequency = 2.5 )
{
	float sum = 0;
	float i;
        float freq = frequency;

	for (i = 0; i < 6; i = i + 1) {
		sum = sum + abs(.5 - noise( 4 * freq * I))/freq ;
		freq *= 2;
	}
	Ci = Cs * sum * (Ka + Kd * diffuse(faceforward( normalize(N), I )) ) ;
}
