/*
 * dusty.sl - surface for dusty or cloudy surfaces
 *
 * 
 * DESCRIPTION:
 *     Makes a surface (or film) composed of microscopic dust grains,
 *   such as soot, Saturn's rings, or the Moon's surface.  (Note that a
 *   Lambertian surface description such as the "matte" shader doesn't
 *   produce quite the right shading in these cases.
 *     The surface uses statistical methods to simulate the effect of
 *   the microscopic particles on the light.  This method is based on
 *   Blinn's paper from the proceedings of Siggraph '82.
 *
 * 
 * PARAMETERS:
 *   Ka - ambient reflection coefficient (totally fake)
 *   Kd - Albedo of the particles
 *   thickness - thickness of the surface
 *   particle_density - number of particles per unit volume
 *   particle_radius - radius of each individual particle
 *   density - 0 = cloud-like, 1 = solid
 *   g - controls scattering (0=isotropic, 1=totally backscattering,
 *                            -1=totally forward scattering)
 *
 *
 * HINTS:
 *   Here are some sample values for materials:
 *                                            particle     particle
 *        Material               Kd      g    density       radius
 *        ------------------------------------------------------------
 *        moon                    1      0     100000        0.01
 *
 *
 * AUTHOR: written by Larry Gritz
 *
 * REFERENCES:
 *     [Blinn82] Blinn, James.  "Simulation of Cloudy and Dusty Surfaces",
 *              ACM Computer Graphics 16(2) (Proceedings of Siggraph '82),
 *              pp. ??, July, 1982.
 *
 * HISTORY:
 *      14 June 1994 -- written by Larry Gritz
 *
 * last modified  15 June 1994 by Larry Gritz
 */



surface
dusty (float Ka = 1;
       float Kd = .5;                   /* albedo of the particles */
       float thickness = 1;
       float particle_density = 10000;
       float particle_radius = 0.002;
       float g = 0.325;               /* the Hanyey-Greenstein constant */
       float density = 0;             /* 0 = cloud-like, near 1 = solid */
  )
{
  point Nf, IN, Eye;
  float mu, mu0, cos_phase_angle, phase_angle;
  float D;
  float B = 0;
  float tau;   /* Optical depth */
  float phase_function, scattering_prob;
  float transparency;
  float n;
  color lightC = color(0,0,0);

  Nf = faceforward (N,I);
  IN = normalize (I);
  Eye = -IN;
  D = clamp (density, 0, 0.999);
  n = particle_density / (1-D);

  mu = Nf . Eye;
  tau = particle_density * PI * particle_radius*particle_radius * thickness / (1-D);

  illuminance (P, Nf, PI/2) {
      mu0 = Nf . L;
      if (mu > 0) {
          cos_phase_angle = L . Eye;
          /* We should now choose a phase function */

          /* Constant scattering is the simplest, and is appropriate if the
           * particle size is much less than the wavelength of light.
           *
           *    phase_function = 1;
           */

          /* Anisotropic is a little better than constant, accounting for
           * the fact that more light should be reflected back toward the
           * light than forward.
           *
           *   phase_function = 1 + 0.5 * cos_phase_angle;
           */

          /* If you think your particles have Lambertian reflectance,
           * try this:
           *     phase_angle = acos (cos_phase_angle);
           *     phase_function = 2.66667 * PI * (sin(phase_angle) + 
           *                           (PI-phase_angle) * cos_phase_angle);
           */

          /* Rayleigh Scattering (if the particles are small compared to
           * the light wavelength, diffraction effects predominate.
           *   phase_function = 0.75 * (1 + cos_phase_angle*cos_phase_angle);
           */
          
          /* Henyey-Greenstein uses the g parameter to make scattering which
           * primarily forward scattering (g<0), backward scattering (g>0),
           * or isotropic (g=0).  Blinn notes that g=.325 is a good value
           * for dark rough surfaces like furnace slag.
           */
          /* We use this one!  the others are just comments/suggestions
           */
          phase_function = (1-g*g) / pow (1+g*g - 2*g*cos_phase_angle, 1.5);

          /* Sum of Henyey-Greenstein functions can be used when there is
           * a mixture of large and small particles.  The following formula
           * is good for handling the back-scattering of large particles and
           * forward scattering of small particles.  Blinn uses the following
           * constants which he found helpful for simulating Saturn's rings:
           * w1 = .596, g1 = 0.5, w2 = .404, g2 = -.5
           *
           * phase_function = w1 * (1-g1*g1) / pow (1+g1*g1 - 2*g1*cos_phase_angle, 1.5)
           * + w2 * (1-g2*g2) / pow (1+g2*g2 - 2*g2*cos_phase_angle, 1.5);
           */


          /* The rest here is used no matter what the phase function is */
          if (cos_phase_angle >= 0)
              B = 1 - exp (-tau * (1/mu0 + 1/mu));
          else B = exp (tau / mu0) - exp (-tau / mu);
          B *= phase_function * (mu0 / (mu+mu0));
          lightC += Cl * B;
        }
    }

  transparency = exp (-tau / mu);

  Oi = Os * (1-transparency);
  Ci = Os * Cs * (Ka*ambient() + Kd*lightC);
}

