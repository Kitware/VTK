/* Perlin's "Bozo's Donut"
 * Perlin, Ken, "An Image Synthesizer", SIGGRAPH '85
 */


surface
bozo(
    float k             = 4,      /* noise amplification */
          Ka            = 1,
          Kd            = 0.5,
          Ks            = 0.5,
          roughness     = 0.1;
    color specularcolor = 1
)
{
    normal Nf = faceforward(normalize(N),I);
    vector V = normalize(-I);
    color cc;
    float i;

    cc = color noise(k*P) ;

    /* map ranges of noise values into different colors */
    for ( i=0.0; i<3.0; i+=1.0) {
        if (comp(cc,i) < 0.3) setcomp(cc,i,0.3);
        else
            if (comp(cc,i) < 0.6) setcomp(cc,i,0.6);
            else setcomp(cc,i,1.0);
    }

    /* specular reflection model */
    Oi = Os;
    Ci = Os * (Cs * (Ka * ambient() + cc * Kd * diffuse(Nf))
        + specularcolor * Ks * specular(Nf,V,roughness));
}
