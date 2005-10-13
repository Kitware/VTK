//
// Fragment shader for procedurally generated toy ball
//
// Author: Bill Licea-Kane
//
// Copyright (c) 2002-2003 ATI Research 
//
// See ATI-License.txt for license information
//

varying vec4 ECposition;   // surface position in eye coordinates
varying vec4 ECballCenter; // ball center in eye coordinates

uniform vec4  LightDir;     // light direction, should be normalized
uniform vec4  HVector;      // reflection vector for infinite light source
uniform vec4  SpecularColor;
uniform vec4  Red;
uniform vec4  Yellow;
uniform vec4  Blue;

uniform vec4  HalfSpace0;   // half-spaces used to define star pattern
uniform vec4  HalfSpace1;
uniform vec4  HalfSpace2;
uniform vec4  HalfSpace3;
uniform vec4  HalfSpace4;

uniform float InOrOutInit;  // = -3
uniform float StripeWidth;  // = 0.3
uniform float FWidth;       // = 0.005

void main(void)
{
    vec4  normal;              // Analytically computed normal
    vec4  p;                   // Point in shader space
    vec4  surfColor;           // Computed color of the surface
    float intensity;           // Computed light intensity
    vec4  distance;            // Computed distance values
    float inorout;             // Counter for computing star pattern

    p.xyz = normalize(ECposition.xyz - ECballCenter.xyz);    // Calculate p
    p.w   = 1.0;

    inorout = InOrOutInit;     // initialize inorout to -3

    distance[0] = dot(p, HalfSpace0);
    distance[1] = dot(p, HalfSpace1);
    distance[2] = dot(p, HalfSpace2);
    distance[3] = dot(p, HalfSpace3);

    distance = smoothstep(-FWidth, FWidth, distance);

    inorout += dot(distance, vec4(1.0));

    distance.x = dot(p, HalfSpace4);
    distance.y = StripeWidth - abs(p.z);
    distance = smoothstep(-FWidth, FWidth, distance);
    inorout += distance.x;

    inorout = clamp(inorout, 0.0, 1.0);

    surfColor = mix(Yellow, Red, inorout);
    surfColor = mix(surfColor, Blue, distance.y);

    // normal = point on surface for sphere at (0,0,0)
    normal = p;

    // Per fragment diffuse lighting
    intensity  = 0.2; // ambient
    intensity += 0.8 * clamp(dot(LightDir, normal), 0.0, 1.0);
    surfColor *= intensity;

    // Per fragment specular lighting
    intensity  = clamp(dot(HVector, normal), 0.0, 1.0);
    intensity  = pow(intensity, SpecularColor.a);
    surfColor += SpecularColor * intensity;

    gl_FragColor = surfColor;
}
