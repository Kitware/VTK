// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// NB: The functions named NoneXxxI0_basisAt() are dummy functions used when
// not coloring by a field. They are never called.

// Forward-declare the "generic" shape gradient so that specializations of
// {{ShapeName}}_normalToSideAt() can call it. This way things will just
// work, even with curved elements.
void shapeGradientAt(in vec3 param, in float coeff[{ShapeCoeffPerCell}], out vec3 dxdr, out vec3 dxds, out vec3 dxdt);

/** Pyramidal function space interpolation */
#ifdef BASIS_HGradPyrI0
void HGradPyrI0_basisAt(in vec3 param, out float basis[1])
{{
  basis[0] = 1.0;
}}

void HGradPyrI0_basisGradientAt(in vec3 param, out float basisGradient[3])
{{
  basisGradient[0] = 0.;
  basisGradient[1] = 0.;
  basisGradient[2] = 0.;
}}
#endif /* BASIS_HGradPyrI0 */

#ifdef BASIS_HGradPyrC1
void HGradPyrC1_basisAt(in vec3 param, out float basis[5])
{{
  // constexpr float eps = std::numeric_limits<float>::epsilon();
  float eps = 1.19209e-07;

  float tt = param.z;
  if (abs(tt - 1.0) < eps)
  {{
    if (tt <= 1.0) {{ tt = 1.0 - eps; }}
    else {{ tt = 1.0 + eps; }}
  }}

  float ttTerm = 0.25/(1.0 - tt);
  basis[0] = (1.0 - param.x - tt) * (1.0 - param.y - tt) * ttTerm;
  basis[1] = (1.0 + param.x - tt) * (1.0 - param.y - tt) * ttTerm;
  basis[2] = (1.0 + param.x - tt) * (1.0 + param.y - tt) * ttTerm;
  basis[3] = (1.0 - param.x - tt) * (1.0 + param.y - tt) * ttTerm;
  basis[4] = tt;
}}

void HGradPyrC1_basisGradientAt(in vec3 param, out float basisGradient[15]) // 5 * 3
{{
  // constexpr float eps = std::numeric_limits<float>::epsilon();
  float eps = 1.19209e-07;

  float rr = param.x;
  float ss = param.y;
  float tt = param.z;
  // Be sure that the basis functions are defined when tt is very close to 1.
  // Warning: the derivatives are discontinuous in (0, 0, 1).
  if (abs(tt-1.0) < eps)
  {{
    if (tt <= 1.0) {{ tt = 1.0 - eps; }}
    else {{ tt = 1.0 + eps; }}
  }}

  float ttTerm = 0.25/(1.0 - tt);
  float ttTerm2 = 4.0 * ttTerm * ttTerm;

  basisGradient[ 0] = (ss + tt - 1.0) * ttTerm;
  basisGradient[ 1] = (rr + tt - 1.0) * ttTerm;
  basisGradient[ 2] = rr * ss * ttTerm2 - 0.25;

  basisGradient[ 3] = (1.0 - ss - tt) * ttTerm;
  basisGradient[ 4] = (tt - rr - 1.0) * ttTerm;
  basisGradient[ 5] = rr * ss * ttTerm2 - 0.25;

  basisGradient[ 6] = (1.0 + ss - tt) * ttTerm;
  basisGradient[ 7] = (1.0 + rr - tt) * ttTerm;
  basisGradient[ 8] = rr * ss * ttTerm2 - 0.25;

  basisGradient[ 9] = (tt - ss - 1.0) * ttTerm;
  basisGradient[10] = (1.0 - rr - tt) * ttTerm;
  basisGradient[11] = rr * ss * ttTerm2 - 0.25;

  basisGradient[12] = 0.0;
  basisGradient[13] = 0.0;
  basisGradient[14] = 1.0;
}}
#endif /* BASIS_HGradPyrC1 */

#ifdef SHAPE_pyramid
int pyramid_axisPermutationForSide(in int side)
{{
  switch(side)
  {{
  case 0: return 2; // (+r, +t) = -S
  case 1: return 1; // (+s, +t) = +R
  case 2: return 3; // (-r, +t) = +S
  case 3: return 0; // (-s, +t) = -R
  case 4: return 4; // (-r, -s) = -T
  // case 5: return 5; // (+r, +s) = +T
  // case 1: return 6; // (-r+s,-r-s+t) = +RST
  // case 2: return 0; // (-s, +t) = -R
  // case 3: return 4; // (-r, -s) = -T
  }}
}}

vec3 pyramid_normalToSideAt(in int sideId, in float shapeVals[{ShapeCoeffPerCell}], in vec3 rst, in vec3 cameraVector)
{{
  vec3 dxdr;
  vec3 dxds;
  vec3 dxdt;
  vec3 nn;
  shapeGradientAt(rst, shapeVals, dxdr, dxds, dxdt);
  vec3 corners[5];
  if (sideId < 4)
  {{
    for (int ii = 0; ii < 5; ++ii)
    {{
      corners[ii] = vec3(shapeVals[ii * 3], shapeVals[ii * 3 + 1], shapeVals[ii * 3 + 2]);
    }}
  }}
  switch (sideId)
  {{
  // Triangles
#if 1
     // NB: These work for a linear pyramid but will not compute normals for higher-order pyramids.
  case  0: nn = cross(corners[1] - corners[0], corners[4] - corners[0]); break;
  case  1: nn = cross(corners[2] - corners[1], corners[4] - corners[1]); break;
  case  2: nn = cross(corners[3] - corners[2], corners[4] - corners[2]); break;
  case  3: nn = cross(corners[0] - corners[3], corners[4] - corners[3]); break;
#else
     // NB: These would work in theory for higher-order pyramids, but fail due to a degeneracy in the parameter space.
  case  0: nn = cross( dxdr, 0.5 * ( dxdr + dxds) + dxdt); break; // -S -dxds + dxdt  cross(dxdr, dxdt)
  case  1: nn = cross( dxds, 0.5 * (-dxdr + dxds) + dxdt); break; // +R  dxdr + dxdt  cross(dxds, dxdt)
  case  2: nn = cross(-dxdr, 0.5 * (-dxdr - dxds) + dxdt); break; // +S  dxds + dxdt  cross(dxdt, dxdr)
  case  3: nn = cross(-dxds, 0.5 * ( dxdr - dxds) + dxdt); break; // -R -dxdr + dxdt  cross(dxdt, dxds)
#endif
  // Quadrilateral
  case  4: nn = cross(dxds, dxdr); break; // -T
  // Edges around base
  case  5: nn = cross(dxdr, dxdt); break; // Or cross(dxds, dxdr); // Edge is +R, norm -S or -T
  case  6: nn = cross(dxds, dxdt); break; // Or cross(dxds, dxdr); // Edge is +S, norm +R or -T
  case  7: nn = cross(dxdt, dxds); break; // Or cross(dxds, dxdr); // Edge is -R, norm +S or -T
  case  8: nn = cross(dxdt, dxdr); break; // Or cross(dxds, dxdr); // Edge is -S, norm -R or -T
  // Edges to tip
  case  9: nn = cross(dxdr,  dxds + 2 * dxdt); break; // Or cross(dxdr + 2 * dxdt, -dxds);
  case 10: nn = cross(dxds, -dxdr + 2 * dxdt); break; // Or cross(dxds + 2 * dxdt,  dxdr);
  case 11: nn = cross(dxdr, -dxds + 2 * dxdt); break; // Or cross(dxdr + 2 * dxdt,  dxds);
  case 12: nn = cross(dxds,  dxdr + 2 * dxdt); break; // Or cross(dxds + 2 * dxdt, -dxdr);
  // Base vertices
  case 13:
  case 14:
  case 15:
  case 16:
           nn = -dxdt; break;
  case 17: nn =  dxdt; break;
  default: nn = cameraVector; break;
  }}
  return normalize(nn);
}}

void NonePyrI0_basisAt(in vec3 param, out float[1] basis) {{ }}
void NonePyrC1_basisAt(in vec3 param, out float[1] basis) {{ }}
#endif /* SHAPE_pyramid */

/** Wedge function space interpolation */
#ifdef BASIS_HGradWdgI0
void HGradWdgI0_basisAt(in vec3 param, out float basis[1])
{{
  basis[0] = 1.0;
}}

void HGradWdgI0_basisGradientAt(in vec3 param, out float basisGradient[3])
{{
  basisGradient[0] = 0.;
  basisGradient[1] = 0.;
  basisGradient[2] = 0.;
}}
#endif /* BASIS_HGradWdgI0 */

#ifdef BASIS_HGradWdgC1
void HGradWdgC1_basisAt(in vec3 param, out float basis[6])
{{
  basis[0] = (1.0 - param.x - param.y)*(1.0 - param.z)/2.0;
  basis[1] = param.x*(1.0 - param.z)/2.0;
  basis[2] = param.y*(1.0 - param.z)/2.0;
  basis[3] = (1.0 - param.x - param.y)*(1.0 + param.z)/2.0;
  basis[4] = param.x*(1.0 + param.z)/2.0;
  basis[5] = param.y*(1.0 + param.z)/2.0;
}}

void HGradWdgC1_basisGradientAt(in vec3 param, out float basisGradient[18]) // 6 * 3
{{
  basisGradient[ 0] = -(1.0 - param.z)/2.0;
  basisGradient[ 1] = -(1.0 - param.z)/2.0;
  basisGradient[ 2] = -(1.0 - param.x - param.y)/2.0;

  basisGradient[ 3] =  (1.0 - param.z)/2.0;
  basisGradient[ 4] =   0.0;
  basisGradient[ 5] =  -param.x/2.0;

  basisGradient[ 6] =   0.0;
  basisGradient[ 7] =  (1.0 - param.z)/2.0;
  basisGradient[ 8] =  -param.y/2.0;

  basisGradient[ 9] = -(1.0 + param.z)/2.0;
  basisGradient[10] = -(1.0 + param.z)/2.0;
  basisGradient[11] =  (1.0 - param.x - param.y)/2.0;

  basisGradient[12] =  (1.0 + param.z)/2.0;
  basisGradient[13] =   0.0;
  basisGradient[14] =   param.x/2.0;

  basisGradient[15] =   0.0;
  basisGradient[16] =  (1.0 + param.z)/2.0;
  basisGradient[17] =   param.y/2.0;
}}
#endif /* BASIS_HGradWdgC1 */

#ifdef SHAPE_wedge
int wedge_axisPermutationForSide(in int side)
{{
  switch(side)
  {{
  case 0: return 2; // (+r, +t) = -S
  case 1: return 6; // (-r+s, +t) = +RS
  case 2: return 0; // (-s, +t) = -R
  case 3: return 4; // (-r, -s) = -T
  case 4: return 5; // (+r, +s) = +T
  }}
}}

vec3 wedge_normalToSideAt(in int sideId, in float shapeVals[{ShapeCoeffPerCell}], in vec3 rst, in vec3 cameraVector)
{{
  vec3 dxdr;
  vec3 dxds;
  vec3 dxdt;
  vec3 nn;
  shapeGradientAt(rst, shapeVals, dxdr, dxds, dxdt);
  switch (sideId)
  {{
  // Quadrilaterals
  case 0: nn = cross(dxdr, dxdt); break; // -S
  case 1: nn = cross(dxds - dxdr, dxdt); break; // +RS
  case 2: nn = cross(dxdt, dxds); break; // -R
  // Triangles
  case 3: nn = cross(dxds, dxdr); break; // -T
  case 4: nn = cross(dxdr, dxds); break; // +T
  // Bottom edges (in -T plane)
  case 5:
  case 6:
  case 7:
          nn = cross(dxds, dxdr); break; // Or cross(±dxd{{rs}}, dxdt);
  // Vertical (+T-directed) edges
  case 8: nn = -(dxdr + dxds); break;
  case 9: nn = dxdr - 0.5 * dxds; break;
  case 10: nn = dxds - 0.5 * dxdr; break;
  // Top edges (in +T plane)
  case 11: nn = cross(dxdr + dxds, dxdt); break; // Or cross(dxds - dxdr, dxdr + dxds);
  case 12: nn = cross(dxdt, dxds); break; // Or cross(dxds, dxdr);
  case 13: nn = cross(dxdt, dxds); break; // Or cross(dxds, dxdr);
  // Vertices (bottom, then top)
  case 14: nn = -(dxdr + dxds + 2*dxdt); break;
  case 15: nn = dxdr - 0.5 * dxds - 2 * dxdt; break;
  case 16: nn = dxds - 0.5 * dxdr - 2 * dxdt; break;
  case 17: nn = -(dxdr + dxds - 2*dxdt); break;
  case 18: nn = dxdr - 0.5 * dxds + 2 * dxdt; break;
  case 19: nn = dxds - 0.5 * dxdr + 2 * dxdt; break;
  default: nn = cameraVector; break;
  }}
  return normalize(nn);
}}

void NoneWdgI0_basisAt(in vec3 param, out float[1] basis) {{ }}
void NoneWdgC1_basisAt(in vec3 param, out float[1] basis) {{ }}
#endif /* SHAPE_wedge */

/** Hexahedral function space interpolation */
#if defined(BASIS_HGradHexI0) || defined(BASIS_HGradHexC0)
void HGradHexI0_basisAt(in vec3 param, out float basis[1])
{{
  basis[0] = 1.0;
}}

void HGradHexI0_basisGradientAt(in vec3 param, out float basisGradient[3])
{{
  basisGradient[0] = 0.;
  basisGradient[1] = 0.;
  basisGradient[2] = 0.;
}}

void HGradHexC0_basisAt(in vec3 param, out float basis[1])
{{
  HGradHexI0_basisAt(param, basis);
}}

void HGradHexC0_basisGradientAt(in vec3 param, out float basisGradient[3])
{{
  HGradHexI0_basisGradientAt(param, basisGradient);
}}
#endif /* BASIS_HGradHexI0 */

#ifdef BASIS_HGradHexC1
void HGradHexC1_basisAt(in vec3 param, out float basis[8])
{{
  basis[0] = (1.0 - param.x)*(1.0 - param.y)*(1.0 - param.z)/8.0;
  basis[1] = (1.0 + param.x)*(1.0 - param.y)*(1.0 - param.z)/8.0;
  basis[2] = (1.0 + param.x)*(1.0 + param.y)*(1.0 - param.z)/8.0;
  basis[3] = (1.0 - param.x)*(1.0 + param.y)*(1.0 - param.z)/8.0;

  basis[4] = (1.0 - param.x)*(1.0 - param.y)*(1.0 + param.z)/8.0;
  basis[5] = (1.0 + param.x)*(1.0 - param.y)*(1.0 + param.z)/8.0;
  basis[6] = (1.0 + param.x)*(1.0 + param.y)*(1.0 + param.z)/8.0;
  basis[7] = (1.0 - param.x)*(1.0 + param.y)*(1.0 + param.z)/8.0;
}}

void HGradHexC1_basisGradientAt(in vec3 param, out float basisGradient[24]) // 8 * 3
{{
  basisGradient[ 0] = -(1.0 - param.y)*(1.0 - param.z)/8.0;
  basisGradient[ 1] = -(1.0 - param.x)*(1.0 - param.z)/8.0;
  basisGradient[ 2] = -(1.0 - param.x)*(1.0 - param.y)/8.0;

  basisGradient[ 3] =  (1.0 - param.y)*(1.0 - param.z)/8.0;
  basisGradient[ 4] = -(1.0 + param.x)*(1.0 - param.z)/8.0;
  basisGradient[ 5] = -(1.0 + param.x)*(1.0 - param.y)/8.0;

  basisGradient[ 6] =  (1.0 + param.y)*(1.0 - param.z)/8.0;
  basisGradient[ 7] =  (1.0 + param.x)*(1.0 - param.z)/8.0;
  basisGradient[ 8] = -(1.0 + param.x)*(1.0 + param.y)/8.0;

  basisGradient[ 9] = -(1.0 + param.y)*(1.0 - param.z)/8.0;
  basisGradient[10] =  (1.0 - param.x)*(1.0 - param.z)/8.0;
  basisGradient[11] = -(1.0 - param.x)*(1.0 + param.y)/8.0;

  basisGradient[12] = -(1.0 - param.y)*(1.0 + param.z)/8.0;
  basisGradient[13] = -(1.0 - param.x)*(1.0 + param.z)/8.0;
  basisGradient[14] =  (1.0 - param.x)*(1.0 - param.y)/8.0;

  basisGradient[15] =  (1.0 - param.y)*(1.0 + param.z)/8.0;
  basisGradient[16] = -(1.0 + param.x)*(1.0 + param.z)/8.0;
  basisGradient[17] =  (1.0 + param.x)*(1.0 - param.y)/8.0;

  basisGradient[18] =  (1.0 + param.y)*(1.0 + param.z)/8.0;
  basisGradient[19] =  (1.0 + param.x)*(1.0 + param.z)/8.0;
  basisGradient[20] =  (1.0 + param.x)*(1.0 + param.y)/8.0;

  basisGradient[21] = -(1.0 + param.y)*(1.0 + param.z)/8.0;
  basisGradient[22] =  (1.0 - param.x)*(1.0 + param.z)/8.0;
  basisGradient[23] =  (1.0 - param.x)*(1.0 + param.y)/8.0;
}}
#endif /* BASIS_HGradHexC1 */

#ifdef BASIS_HGradHexC2
// NB: This is a complete 27-node hex.
//     Intrepid calls this a C2 vs an I2 hex since the full set
//     of tri-quadratic basis functions is included.
//     (C stands for Complete; I for Incomplete.)
void HGradHexC2_basisAt(in vec3 param, out float basis[27])
{{
  basis[ 0] = 0.125*(-1. + param.x)*param.x*(-1. + param.y)*param.y*(-1. + param.z)*param.z;
  basis[ 1] = 0.125*param.x*(1.+ param.x)*(-1. + param.y)*param.y*(-1. + param.z)*param.z;
  basis[ 2] = 0.125*param.x*(1.+ param.x)*param.y*(1.+ param.y)*(-1. + param.z)*param.z;
  basis[ 3] = 0.125*(-1. + param.x)*param.x*param.y*(1.+ param.y)*(-1. + param.z)*param.z;
  basis[ 4] = 0.125*(-1. + param.x)*param.x*(-1. + param.y)*param.y*param.z*(1.+ param.z);
  basis[ 5] = 0.125*param.x*(1.+ param.x)*(-1. + param.y)*param.y*param.z*(1.+ param.z);
  basis[ 6] = 0.125*param.x*(1.+ param.x)*param.y*(1.+ param.y)*param.z*(1.+ param.z);
  basis[ 7] = 0.125*(-1. + param.x)*param.x*param.y*(1.+ param.y)*param.z*(1.+ param.z);

  basis[ 8] = 0.25*(1. - param.x)*(1. + param.x)*(-1. + param.y)*param.y*(-1. + param.z)*param.z;
  basis[ 9] = 0.25*param.x*(1.+ param.x)*(1. - param.y)*(1. + param.y)*(-1. + param.z)*param.z;
  basis[10] = 0.25*(1. - param.x)*(1. + param.x)*param.y*(1.+ param.y)*(-1. + param.z)*param.z;
  basis[11] = 0.25*(-1. + param.x)*param.x*(1. - param.y)*(1. + param.y)*(-1. + param.z)*param.z;

  basis[12] = 0.25*(-1. + param.x)*param.x*(-1. + param.y)*param.y*(1. - param.z)*(1. + param.z);
  basis[13] = 0.25*param.x*(1.+ param.x)*(-1. + param.y)*param.y*(1. - param.z)*(1. + param.z);
  basis[14] = 0.25*param.x*(1.+ param.x)*param.y*(1.+ param.y)*(1. - param.z)*(1. + param.z);
  basis[15] = 0.25*(-1. + param.x)*param.x*param.y*(1.+ param.y)*(1. - param.z)*(1. + param.z);

  basis[16] = 0.25*(1. - param.x)*(1. + param.x)*(-1. + param.y)*param.y*param.z*(1.+ param.z);
  basis[17] = 0.25*param.x*(1.+ param.x)*(1. - param.y)*(1. + param.y)*param.z*(1.+ param.z);
  basis[18] = 0.25*(1. - param.x)*(1. + param.x)*param.y*(1.+ param.y)*param.z*(1.+ param.z);
  basis[19] = 0.25*(-1. + param.x)*param.x*(1. - param.y)*(1. + param.y)*param.z*(1.+ param.z);

  basis[20] = (1. - param.x)*(1. + param.x)*(1. - param.y)*(1. + param.y)*(1. - param.z)*(1. + param.z);

  basis[21] = 0.5*(1. - param.x)*(1. + param.x)*(1. - param.y)*(1. + param.y)*(-1. + param.z)*param.z;
  basis[22] = 0.5*(1. - param.x)*(1. + param.x)*(1. - param.y)*(1. + param.y)*param.z*(1.+ param.z);
  basis[23] = 0.5*(-1. + param.x)*param.x*(1. - param.y)*(1. + param.y)*(1. - param.z)*(1. + param.z);
  basis[24] = 0.5*param.x*(1.+ param.x)*(1. - param.y)*(1. + param.y)*(1. - param.z)*(1. + param.z);
  basis[25] = 0.5*(1. - param.x)*(1. + param.x)*(-1. + param.y)*param.y*(1. - param.z)*(1. + param.z);
  basis[26] = 0.5*(1. - param.x)*(1. + param.x)*param.y*(1.+ param.y)*(1. - param.z)*(1. + param.z);
}}

void HGradHexC2_basisGradientAt(in vec3 param, out float basisGradient[81]) // 27 * 3
{{
  basis[ 0 * 3 + 0] = (-0.125 + 0.25*param.x)*(-1. + param.y)*param.y*(-1. + param.z)*param.z;
  basis[ 0 * 3 + 1] = (-1. + param.x)*param.x*(-0.125 + 0.25*param.y)*(-1. + param.z)*param.z;
  basis[ 0 * 3 + 2] = (-1. + param.x)*param.x*(-1. + param.y)*param.y*(-0.125 + 0.25*param.z);

  basis[ 1 * 3 + 0] = (0.125 + 0.25*param.x)*(-1. + param.y)*param.y*(-1. + param.z)*param.z;
  basis[ 1 * 3 + 1] = param.x*(1. + param.x)*(-0.125 + 0.25*param.y)*(-1. + param.z)*param.z;
  basis[ 1 * 3 + 2] = param.x*(1. + param.x)*(-1. + param.y)*param.y*(-0.125 + 0.25*param.z);

  basis[ 2 * 3 + 0] = (0.125 + 0.25*param.x)*param.y*(1. + param.y)*(-1. + param.z)*param.z;
  basis[ 2 * 3 + 1] = param.x*(1. + param.x)*(0.125 + 0.25*param.y)*(-1. + param.z)*param.z;
  basis[ 2 * 3 + 2] = param.x*(1. + param.x)*param.y*(1. + param.y)*(-0.125 + 0.25*param.z);

  basis[ 3 * 3 + 0] = (-0.125 + 0.25*param.x)*param.y*(1. + param.y)*(-1. + param.z)*param.z;
  basis[ 3 * 3 + 1] = (-1. + param.x)*param.x*(0.125 + 0.25*param.y)*(-1. + param.z)*param.z;
  basis[ 3 * 3 + 2] = (-1. + param.x)*param.x*param.y*(1. + param.y)*(-0.125 + 0.25*param.z);

  basis[ 4 * 3 + 0] = (-0.125 + 0.25*param.x)*(-1. + param.y)*param.y*param.z*(1. + param.z);
  basis[ 4 * 3 + 1] = (-1. + param.x)*param.x*(-0.125 + 0.25*param.y)*param.z*(1. + param.z);
  basis[ 4 * 3 + 2] = (-1. + param.x)*param.x*(-1. + param.y)*param.y*(0.125 + 0.25*param.z);

  basis[ 5 * 3 + 0] = (0.125 + 0.25*param.x)*(-1. + param.y)*param.y*param.z*(1. + param.z);
  basis[ 5 * 3 + 1] = param.x*(1. + param.x)*(-0.125 + 0.25*param.y)*param.z*(1. + param.z);
  basis[ 5 * 3 + 2] = param.x*(1. + param.x)*(-1. + param.y)*param.y*(0.125 + 0.25*param.z);

  basis[ 6 * 3 + 0] = (0.125 + 0.25*param.x)*param.y*(1. + param.y)*param.z*(1. + param.z);
  basis[ 6 * 3 + 1] = param.x*(1. + param.x)*(0.125 + 0.25*param.y)*param.z*(1. + param.z);
  basis[ 6 * 3 + 2] = param.x*(1. + param.x)*param.y*(1. + param.y)*(0.125 + 0.25*param.z);

  basis[ 7 * 3 + 0] = (-0.125 + 0.25*param.x)*param.y*(1. + param.y)*param.z*(1. + param.z);
  basis[ 7 * 3 + 1] = (-1. + param.x)*param.x*(0.125 + 0.25*param.y)*param.z*(1. + param.z);
  basis[ 7 * 3 + 2] = (-1. + param.x)*param.x*param.y*(1. + param.y)*(0.125 + 0.25*param.z);

  basis[ 8 * 3 + 0] = -0.5*param.x*(-1. + param.y)*param.y*(-1. + param.z)*param.z;
  basis[ 8 * 3 + 1] = (1. - param.x)*(1. + param.x)*(-0.25 + 0.5*param.y)*(-1. + param.z)*param.z;
  basis[ 8 * 3 + 2] = (1. - param.x)*(1. + param.x)*(-1. + param.y)*param.y*(-0.25 + 0.5*param.z);

  basis[ 9 * 3 + 0] = (0.25 + 0.5*param.x)*(1. - param.y)*(1. + param.y)*(-1. + param.z)*param.z;
  basis[ 9 * 3 + 1] = param.x*(1. + param.x)*(-0.5*param.y)*(-1. + param.z)*param.z;
  basis[ 9 * 3 + 2] = param.x*(1. + param.x)*(1. - param.y)*(1. + param.y)*(-0.25 + 0.5*param.z);

  basis[10 * 3 + 0] = -0.5*param.x*param.y*(1. + param.y)*(-1. + param.z)*param.z;
  basis[10 * 3 + 1] = (1. - param.x)*(1. + param.x)*(0.25 + 0.5*param.y)*(-1. + param.z)*param.z;
  basis[10 * 3 + 2] = (1. - param.x)*(1. + param.x)*param.y*(1. + param.y)*(-0.25 + 0.5*param.z);

  basis[11 * 3 + 0] = (-0.25 + 0.5*param.x)*(1. - param.y)*(1. + param.y)*(-1. + param.z)*param.z;
  basis[11 * 3 + 1] = (-1. + param.x)*param.x*(-0.5*param.y)*(-1. + param.z)*param.z;
  basis[11 * 3 + 2] = (-1. + param.x)*param.x*(1. - param.y)*(1. + param.y)*(-0.25 + 0.5*param.z);

  basis[12 * 3 + 0] = (-0.25 + 0.5*param.x)*(-1. + param.y)*param.y*(1. - param.z)*(1. + param.z);
  basis[12 * 3 + 1] = (-1. + param.x)*param.x*(-0.25 + 0.5*param.y)*(1. - param.z)*(1. + param.z);
  basis[12 * 3 + 2] = (-1. + param.x)*param.x*(-1. + param.y)*param.y*(-0.5*param.z);

  basis[13 * 3 + 0] = (0.25 + 0.5*param.x)*(-1. + param.y)*param.y*(1. - param.z)*(1. + param.z);
  basis[13 * 3 + 1] = param.x*(1. + param.x)*(-0.25 + 0.5*param.y)*(1. - param.z)*(1. + param.z);
  basis[13 * 3 + 2] = param.x*(1. + param.x)*(-1. + param.y)*param.y*(-0.5*param.z);

  basis[14 * 3 + 0] = (0.25 + 0.5*param.x)*param.y*(1. + param.y)*(1. - param.z)*(1. + param.z);
  basis[14 * 3 + 1] = param.x*(1. + param.x)*(0.25 + 0.5*param.y)*(1. - param.z)*(1. + param.z);
  basis[14 * 3 + 2] = param.x*(1. + param.x)*param.y*(1. + param.y)*(-0.5*param.z);

  basis[15 * 3 + 0] = (-0.25 + 0.5*param.x)*param.y*(1. + param.y)*(1. - param.z)*(1. + param.z);
  basis[15 * 3 + 1] = (-1. + param.x)*param.x*(0.25 + 0.5*param.y)*(1. - param.z)*(1. + param.z);
  basis[15 * 3 + 2] = (-1. + param.x)*param.x*param.y*(1. + param.y)*(-0.5*param.z);

  basis[16 * 3 + 0] = -0.5*param.x*(-1. + param.y)*param.y*param.z*(1. + param.z);
  basis[16 * 3 + 1] = (1. - param.x)*(1. + param.x)*(-0.25 + 0.5*param.y)*param.z*(1. + param.z);
  basis[16 * 3 + 2] = (1. - param.x)*(1. + param.x)*(-1. + param.y)*param.y*(0.25 + 0.5*param.z);

  basis[17 * 3 + 0] = (0.25 + 0.5*param.x)*(1. - param.y)*(1. + param.y)*param.z*(1. + param.z);
  basis[17 * 3 + 1] = param.x*(1. + param.x)*(-0.5*param.y)*param.z*(1. + param.z);
  basis[17 * 3 + 2] = param.x*(1. + param.x)*(1. - param.y)*(1. + param.y)*(0.25 + 0.5*param.z);

  basis[18 * 3 + 0] = -0.5*param.x*param.y*(1. + param.y)*param.z*(1. + param.z);
  basis[18 * 3 + 1] = (1. - param.x)*(1. + param.x)*(0.25 + 0.5*param.y)*param.z*(1. + param.z);
  basis[18 * 3 + 2] = (1. - param.x)*(1. + param.x)*param.y*(1. + param.y)*(0.25 + 0.5*param.z);

  basis[19 * 3 + 0] = (-0.25 + 0.5*param.x)*(1. - param.y)*(1. + param.y)*param.z*(1. + param.z);
  basis[19 * 3 + 1] = (-1. + param.x)*param.x*(-0.5*param.y)*param.z*(1. + param.z);
  basis[19 * 3 + 2] = (-1. + param.x)*param.x*(1. - param.y)*(1. + param.y)*(0.25 + 0.5*param.z);

  basis[20 * 3 + 0] = -2.*param.x*(1. - param.y)*(1. + param.y)*(1. - param.z)*(1. + param.z);
  basis[20 * 3 + 1] = (1. - param.x)*(1. + param.x)*(-2.*param.y)*(1. - param.z)*(1. + param.z);
  basis[20 * 3 + 2] = (1. - param.x)*(1. + param.x)*(1. - param.y)*(1. + param.y)*(-2.*param.z);

  basis[21 * 3 + 0] = -param.x*(1. - param.y)*(1. + param.y)*(-1. + param.z)*param.z;
  basis[21 * 3 + 1] = (1. - param.x)*(1. + param.x)*(-param.y)*(-1. + param.z)*param.z;
  basis[21 * 3 + 2] = (1. - param.x)*(1. + param.x)*(1. - param.y)*(1. + param.y)*(-0.5 + param.z);

  basis[22 * 3 + 0] = -param.x*(1. - param.y)*(1. + param.y)*param.z*(1. + param.z);
  basis[22 * 3 + 1] = (1. - param.x)*(1. + param.x)*(-param.y)*param.z*(1. + param.z);
  basis[22 * 3 + 2] = (1. - param.x)*(1. + param.x)*(1. - param.y)*(1. + param.y)*(0.5 + param.z);

  basis[23 * 3 + 0] = (-0.5 + param.x)*(1. - param.y)*(1. + param.y)*(1. - param.z)*(1. + param.z);
  basis[23 * 3 + 1] = (-1. + param.x)*param.x*(-param.y)*(1. - param.z)*(1. + param.z);
  basis[23 * 3 + 2] = (-1. + param.x)*param.x*(1. - param.y)*(1. + param.y)*(-param.z);

  basis[24 * 3 + 0] = (0.5 + param.x)*(1. - param.y)*(1. + param.y)*(1. - param.z)*(1. + param.z);
  basis[24 * 3 + 1] = param.x*(1. + param.x)*(-param.y)*(1. - param.z)*(1. + param.z);
  basis[24 * 3 + 2] = param.x*(1. + param.x)*(1. - param.y)*(1. + param.y)*(-param.z);

  basis[25 * 3 + 0] = -param.x*(-1. + param.y)*param.y*(1. - param.z)*(1. + param.z);
  basis[25 * 3 + 1] = (1. - param.x)*(1. + param.x)*(-0.5 + param.y)*(1. - param.z)*(1. + param.z);
  basis[25 * 3 + 2] = (1. - param.x)*(1. + param.x)*(-1. + param.y)*param.y*(-param.z);

  basis[26 * 3 + 0] = -param.x*param.y*(1. + param.y)*(1. - param.z)*(1. + param.z);
  basis[26 * 3 + 1] = (1. - param.x)*(1. + param.x)*(0.5 + param.y)*(1. - param.z)*(1. + param.z);
  basis[26 * 3 + 2] = (1. - param.x)*(1. + param.x)*param.y*(1. + param.y)*(-param.z);
}}
#endif /* BASIS_HGradHexC2 */

#ifdef BASIS_HGradHexI2
// NB: This is a 20-node hex (mid-edge but not mid-face or mid-body points).
//     Intrepid calls this an I2 vs a C2 hex since it omits mid-face/body points.
void HGradHexI2_basisAt(in vec3 param, out float basis[20])
{{
  basis[ 0] = 0.125*(1.0 - param.x)*(1.0 - param.y)*(1.0 - param.z)*(-param.x - param.y - param.z - 2.0);
  basis[ 1] = 0.125*(1.0 + param.x)*(1.0 - param.y)*(1.0 - param.z)*( param.x - param.y - param.z - 2.0);
  basis[ 2] = 0.125*(1.0 + param.x)*(1.0 + param.y)*(1.0 - param.z)*( param.x + param.y - param.z - 2.0);
  basis[ 3] = 0.125*(1.0 - param.x)*(1.0 + param.y)*(1.0 - param.z)*(-param.x + param.y - param.z - 2.0);
  basis[ 4] = 0.125*(1.0 - param.x)*(1.0 - param.y)*(1.0 + param.z)*(-param.x - param.y + param.z - 2.0);
  basis[ 5] = 0.125*(1.0 + param.x)*(1.0 - param.y)*(1.0 + param.z)*( param.x - param.y + param.z - 2.0);
  basis[ 6] = 0.125*(1.0 + param.x)*(1.0 + param.y)*(1.0 + param.z)*( param.x + param.y + param.z - 2.0);
  basis[ 7] = 0.125*(1.0 - param.x)*(1.0 + param.y)*(1.0 + param.z)*(-param.x + param.y + param.z - 2.0);

  basis[ 8] = 0.25*(1.0 - param.x*param.x)*(1.0 - param.y)*(1.0 - param.z);
  basis[ 9] = 0.25*(1.0 + param.x)*(1.0 - param.y*param.y)*(1.0 - param.z);
  basis[10] = 0.25*(1.0 - param.x*param.x)*(1.0 + param.y)*(1.0 - param.z);
  basis[11] = 0.25*(1.0 - param.x)*(1.0 - param.y*param.y)*(1.0 - param.z);

  basis[12] = 0.25*(1.0 - param.x)*(1.0 - param.y)*(1.0 - param.z*param.z);
  basis[13] = 0.25*(1.0 + param.x)*(1.0 - param.y)*(1.0 - param.z*param.z);
  basis[14] = 0.25*(1.0 + param.x)*(1.0 + param.y)*(1.0 - param.z*param.z);
  basis[15] = 0.25*(1.0 - param.x)*(1.0 + param.y)*(1.0 - param.z*param.z);

  basis[16] = 0.25*(1.0 - param.x*param.x)*(1.0 - param.y)*(1.0 + param.z);
  basis[17] = 0.25*(1.0 + param.x)*(1.0 - param.y*param.y)*(1.0 + param.z);
  basis[18] = 0.25*(1.0 - param.x*param.x)*(1.0 + param.y)*(1.0 + param.z);
  basis[19] = 0.25*(1.0 - param.x)*(1.0 - param.y*param.y)*(1.0 + param.z);
}}

void HGradHexI2_basisGradientAt(in vec3 param, out float basisGradient[60]) // 20 * 3
{{
  basisGradient[0 * 3 + 0] = -0.125*(1.0-param.y)*(1.0-param.z)*(-param.x-param.y-param.z-2.0) - 0.125*(1.0-param.x)*(1.0-param.y)*(1.0-param.z);
  basisGradient[0 * 3 + 1] = -0.125*(1.0-param.x)*(1.0-param.z)*(-param.x-param.y-param.z-2.0) - 0.125*(1.0-param.x)*(1.0-param.y)*(1.0-param.z);
  basisGradient[0 * 3 + 2] = -0.125*(1.0-param.x)*(1.0-param.y)*(-param.x-param.y-param.z-2.0) - 0.125*(1.0-param.x)*(1.0-param.y)*(1.0-param.z);

  basisGradient[1 * 3 + 0] =  0.125*(1.0-param.y)*(1.0-param.z)*( param.x-param.y-param.z-2.0) + 0.125*(1.0+param.x)*(1.0-param.y)*(1.0-param.z);
  basisGradient[1 * 3 + 1] = -0.125*(1.0+param.x)*(1.0-param.z)*( param.x-param.y-param.z-2.0) - 0.125*(1.0+param.x)*(1.0-param.y)*(1.0-param.z);
  basisGradient[1 * 3 + 2] = -0.125*(1.0+param.x)*(1.0-param.y)*( param.x-param.y-param.z-2.0) - 0.125*(1.0+param.x)*(1.0-param.y)*(1.0-param.z);

  basisGradient[2 * 3 + 0] =  0.125*(1.0+param.y)*(1.0-param.z)*( param.x+param.y-param.z-2.0) + 0.125*(1.0+param.x)*(1.0+param.y)*(1.0-param.z);
  basisGradient[2 * 3 + 1] =  0.125*(1.0+param.x)*(1.0-param.z)*( param.x+param.y-param.z-2.0) + 0.125*(1.0+param.x)*(1.0+param.y)*(1.0-param.z);
  basisGradient[2 * 3 + 2] = -0.125*(1.0+param.x)*(1.0+param.y)*( param.x+param.y-param.z-2.0) - 0.125*(1.0+param.x)*(1.0+param.y)*(1.0-param.z);

  basisGradient[3 * 3 + 0] = -0.125*(1.0+param.y)*(1.0-param.z)*(-param.x+param.y-param.z-2.0) - 0.125*(1.0-param.x)*(1.0+param.y)*(1.0-param.z);
  basisGradient[3 * 3 + 1] =  0.125*(1.0-param.x)*(1.0-param.z)*(-param.x+param.y-param.z-2.0) + 0.125*(1.0-param.x)*(1.0+param.y)*(1.0-param.z);
  basisGradient[3 * 3 + 2] = -0.125*(1.0-param.x)*(1.0+param.y)*(-param.x+param.y-param.z-2.0) - 0.125*(1.0-param.x)*(1.0+param.y)*(1.0-param.z);

  basisGradient[4 * 3 + 0] = -0.125*(1.0-param.y)*(1.0+param.z)*(-param.x-param.y+param.z-2.0) - 0.125*(1.0-param.x)*(1.0-param.y)*(1.0+param.z);
  basisGradient[4 * 3 + 1] = -0.125*(1.0-param.x)*(1.0+param.z)*(-param.x-param.y+param.z-2.0) - 0.125*(1.0-param.x)*(1.0-param.y)*(1.0+param.z);
  basisGradient[4 * 3 + 2] =  0.125*(1.0-param.x)*(1.0-param.y)*(-param.x-param.y+param.z-2.0) + 0.125*(1.0-param.x)*(1.0-param.y)*(1.0+param.z);

  basisGradient[5 * 3 + 0] =  0.125*(1.0-param.y)*(1.0+param.z)*( param.x-param.y+param.z-2.0) + 0.125*(1.0+param.x)*(1.0-param.y)*(1.0+param.z);
  basisGradient[5 * 3 + 1] = -0.125*(1.0+param.x)*(1.0+param.z)*( param.x-param.y+param.z-2.0) - 0.125*(1.0+param.x)*(1.0-param.y)*(1.0+param.z);
  basisGradient[5 * 3 + 2] =  0.125*(1.0+param.x)*(1.0-param.y)*( param.x-param.y+param.z-2.0) + 0.125*(1.0+param.x)*(1.0-param.y)*(1.0+param.z);

  basisGradient[6 * 3 + 0] =  0.125*(1.0+param.y)*(1.0+param.z)*( param.x+param.y+param.z-2.0) + 0.125*(1.0+param.x)*(1.0+param.y)*(1.0+param.z);
  basisGradient[6 * 3 + 1] =  0.125*(1.0+param.x)*(1.0+param.z)*( param.x+param.y+param.z-2.0) + 0.125*(1.0+param.x)*(1.0+param.y)*(1.0+param.z);
  basisGradient[6 * 3 + 2] =  0.125*(1.0+param.x)*(1.0+param.y)*( param.x+param.y+param.z-2.0) + 0.125*(1.0+param.x)*(1.0+param.y)*(1.0+param.z);

  basisGradient[7 * 3 + 0] = -0.125*(1.0+param.y)*(1.0+param.z)*(-param.x+param.y+param.z-2.0) - 0.125*(1.0-param.x)*(1.0+param.y)*(1.0+param.z);
  basisGradient[7 * 3 + 1] =  0.125*(1.0-param.x)*(1.0+param.z)*(-param.x+param.y+param.z-2.0) + 0.125*(1.0-param.x)*(1.0+param.y)*(1.0+param.z);
  basisGradient[7 * 3 + 2] =  0.125*(1.0-param.x)*(1.0+param.y)*(-param.x+param.y+param.z-2.0) + 0.125*(1.0-param.x)*(1.0+param.y)*(1.0+param.z);

  basisGradient[8 * 3 + 0] = -0.5*param.x*(1.0-param.y)*(1.0-param.z);
  basisGradient[8 * 3 + 1] = -0.25*(1.0-param.x*param.x)*(1.0-param.z);
  basisGradient[8 * 3 + 2] = -0.25*(1.0-param.x*param.x)*(1.0-param.y);

  basisGradient[9 * 3 + 0] =  0.25*(1.0-param.y*param.y)*(1.0-param.z);
  basisGradient[9 * 3 + 1] = -0.5*param.y*(1.0+param.x)*(1.0-param.z);
  basisGradient[9 * 3 + 2] = -0.25*(1.0+param.x)*(1.0-param.y*param.y);

  basisGradient[10 * 3 + 0] = -0.5*param.x*(1.0+param.y)*(1.0-param.z);
  basisGradient[10 * 3 + 1] =  0.25*(1.0-param.x*param.x)*(1.0-param.z);
  basisGradient[10 * 3 + 2] = -0.25*(1.0-param.x*param.x)*(1.0+param.y);

  basisGradient[11 * 3 + 0] = -0.25*(1.0-param.y*param.y)*(1.0-param.z);
  basisGradient[11 * 3 + 1] = -0.5*param.y*(1.0-param.x)*(1.0-param.z);
  basisGradient[11 * 3 + 2] = -0.25*(1.0-param.x)*(1.0-param.y*param.y);

  basisGradient[12 * 3 + 0] = -0.25*(1.0-param.y)*(1.0-param.z*param.z);
  basisGradient[12 * 3 + 1] = -0.25*(1.0-param.x)*(1.0-param.z*param.z);
  basisGradient[12 * 3 + 2] = -0.5*param.z*(1.0-param.x)*(1.0-param.y);

  basisGradient[13 * 3 + 0] =  0.25*(1.0-param.y)*(1.0-param.z*param.z);
  basisGradient[13 * 3 + 1] = -0.25*(1.0+param.x)*(1.0-param.z*param.z);
  basisGradient[13 * 3 + 2] = -0.5*param.z*(1.0+param.x)*(1.0-param.y);

  basisGradient[14 * 3 + 0] =  0.25*(1.0+param.y)*(1.0-param.z*param.z);
  basisGradient[14 * 3 + 1] =  0.25*(1.0+param.x)*(1.0-param.z*param.z);
  basisGradient[14 * 3 + 2] = -0.5*param.z*(1.0+param.x)*(1.0+param.y);

  basisGradient[15 * 3 + 0] = -0.25*(1.0+param.y)*(1.0-param.z*param.z);
  basisGradient[15 * 3 + 1] =  0.25*(1.0-param.x)*(1.0-param.z*param.z);
  basisGradient[15 * 3 + 2] = -0.5*param.z*(1.0-param.x)*(1.0+param.y);

  basisGradient[16 * 3 + 0] = -0.5*param.x*(1.0-param.y)*(1.0+param.z);
  basisGradient[16 * 3 + 1] = -0.25*(1.0-param.x*param.x)*(1.0+param.z);
  basisGradient[16 * 3 + 2] =  0.25*(1.0-param.x*param.x)*(1.0-param.y);

  basisGradient[17 * 3 + 0] =  0.25*(1.0-param.y*param.y)*(1.0+param.z);
  basisGradient[17 * 3 + 1] = -0.5*param.y*(1.0+param.x)*(1.0+param.z);
  basisGradient[17 * 3 + 2] =  0.25*(1.0+param.x)*(1.0-param.y*param.y);

  basisGradient[18 * 3 + 0] = -0.5*param.x*(1.0+param.y)*(1.0+param.z);
  basisGradient[18 * 3 + 1] =  0.25*(1.0-param.x*param.x)*(1.0+param.z);
  basisGradient[18 * 3 + 2] =  0.25*(1.0-param.x*param.x)*(1.0+param.y);

  basisGradient[19 * 3 + 0] = -0.25*(1.0-param.y*param.y)*(1.0+param.z);
  basisGradient[19 * 3 + 1] = -0.5*param.y*(1.0-param.x)*(1.0+param.z);
  basisGradient[19 * 3 + 2] =  0.25*(1.0-param.x)*(1.0-param.y*param.y);
}}
#endif /* BASIS_HGradHexI2 */

#ifdef BASIS_HCurlHexI1
void HCurlHexI1_basisAt(in vec3 param, out float basis[36]) // 12 * 3
{{
  basis[0 * 3 + 0] = (1.0 - param.y)*(1.0 - param.z)/8.0;
  basis[0 * 3 + 1] = 0.0;
  basis[0 * 3 + 2] = 0.0;

  basis[1 * 3 + 0] = 0.0;
  basis[1 * 3 + 1] = (1.0 + param.x)*(1.0 - param.z)/8.0;
  basis[1 * 3 + 2] = 0.0;

  basis[2 * 3 + 0] = -(1.0 + param.y)*(1.0 - param.z)/8.0;
  basis[2 * 3 + 1] = 0.0;
  basis[2 * 3 + 2] = 0.0;

  basis[3 * 3 + 0] = 0.0;
  basis[3 * 3 + 1] = -(1.0 - param.x)*(1.0 - param.z)/8.0;
  basis[3 * 3 + 2] = 0.0;

  basis[4 * 3 + 0] = (1.0 - param.y)*(1.0 + param.z)/8.0;
  basis[4 * 3 + 1] = 0.0;
  basis[4 * 3 + 2] = 0.0;

  basis[5 * 3 + 0] = 0.0;
  basis[5 * 3 + 1] = (1.0 + param.x)*(1.0 + param.z)/8.0;
  basis[5 * 3 + 2] = 0.0;

  basis[6 * 3 + 0] = -(1.0 + param.y)*(1.0 + param.z)/8.0;
  basis[6 * 3 + 1] = 0.0;
  basis[6 * 3 + 2] = 0.0;

  basis[7 * 3 + 0] = 0.0;
  basis[7 * 3 + 1] = -(1.0 - param.x)*(1.0 + param.z)/8.0;
  basis[7 * 3 + 2] = 0.0;

  basis[8 * 3 + 0] = 0.0;
  basis[8 * 3 + 1] = 0.0;
  basis[8 * 3 + 2] = (1.0 - param.x)*(1.0 - param.y)/8.0;

  basis[9 * 3 + 0] = 0.0;
  basis[9 * 3 + 1] = 0.0;
  basis[9 * 3 + 2] = (1.0 + param.x)*(1.0 - param.y)/8.0;

  basis[10 * 3 + 0] = 0.0;
  basis[10 * 3 + 1] = 0.0;
  basis[10 * 3 + 2] = (1.0 + param.x)*(1.0 + param.y)/8.0;

  basis[11 * 3 + 0] = 0.0;
  basis[11 * 3 + 1] = 0.0;
  basis[11 * 3 + 2] = (1.0 - param.x)*(1.0 + param.y)/8.0;
}}
#endif /* BASIS_HCurlHexI1 */

#ifdef BASIS_HDivHexI1
void HDivHexI1_basisAt(in vec3 param, out float basis[18]) // 6 * 3
{{
  basis[0 * 3 + 0] = 0.0;
  basis[0 * 3 + 1] = (param.y - 1.0)/8.0;
  basis[0 * 3 + 2] = 0.0;

  basis[1 * 3 + 0] = (1.0 + param.x)/8.0;
  basis[1 * 3 + 1] = 0.0;
  basis[1 * 3 + 2] = 0.0;

  basis[2 * 3 + 0] = 0.0;
  basis[2 * 3 + 1] = (1.0 + param.y)/8.0;
  basis[2 * 3 + 2] = 0.0;

  basis[3 * 3 + 0] = (param.x - 1.0)/8.0;
  basis[3 * 3 + 1] = 0.0;
  basis[3 * 3 + 2] = 0.0;

  basis[4 * 3 + 0] = 0.0;
  basis[4 * 3 + 1] = 0.0;
  basis[4 * 3 + 2] = (param.z - 1.0)/8.0;

  basis[5 * 3 + 0] = 0.0;
  basis[5 * 3 + 1] = 0.0;
  basis[5 * 3 + 2] = (1.0 + param.z)/8.0;
}}
#endif /* BASIS_HDivHexI1 */

#ifdef SHAPE_hexahedron
int hexahedron_axisPermutationForSide(in int side)
{{
  switch(side)
  {{
  case 0: return 2; // (+r, +t) = -S
  case 1: return 1; // (+s, +t) = +R
  case 2: return 3; // (-r, +t) = +S
  case 3: return 0; // (-s, +t) = -R
  case 4: return 4; // (-r, -s) = -T
  case 5: return 5; // (+r, +s) = +T
  // case 1: return 6; // (-r+s,-r-s+t) = +RST
  // case 2: return 0; // (-s, +t) = -R
  // case 3: return 4; // (-r, -s) = -T
  }}
}}

vec3 hexahedron_normalToSideAt(in int sideId, in float shapeVals[{ShapeCoeffPerCell}], in vec3 rst, in vec3 cameraVector)
{{
  vec3 dxdr;
  vec3 dxds;
  vec3 dxdt;
  vec3 nn;
  shapeGradientAt(rst, shapeVals, dxdr, dxds, dxdt);
  switch (sideId)
  {{
  // Quadrilaterals
  case  0: nn = cross(dxdr, dxdt); break; // -S
  case  1: nn = cross(dxds, dxdt); break; // +R
  case  2: nn = cross(dxdt, dxdr); break; // +S
  case  3: nn = cross(dxdt, dxds); break; // -R
  case  4: nn = cross(dxds, dxdr); break; // -T
  case  5: nn = cross(dxdr, dxds); break; // +T
  // Bottom edges (in -T plane)
  case  6: nn = -dxds - dxdt; break;
  case  7: nn =  dxdr - dxdt; break;
  case  8: nn =  dxds - dxdt; break;
  case  9: nn = -dxdr - dxdt; break;
  // Vertical (+T-directed) edges
  case 10: nn = -dxdr - dxds; break;
  case 11: nn =  dxdr - dxds; break;
  case 12: nn =  dxdr + dxds; break;
  case 13: nn = -dxdr + dxds; break;
  // Top edges (in +T plane)
  case 14: nn = -dxds + dxdt; break;
  case 15: nn =  dxdr + dxdt; break;
  case 16: nn =  dxds + dxdt; break;
  case 17: nn = -dxdr + dxdt; break;
  // Vertices (bottom, then top)
  case 18: nn = -dxdr - dxds - dxdt; break;
  case 19: nn =  dxdr - dxds - dxdt; break;
  case 20: nn =  dxdr + dxds - dxdt; break;
  case 21: nn = -dxdr + dxds - dxdt; break;
  case 22: nn = -dxdr - dxds + dxdt; break;
  case 23: nn =  dxdr - dxds + dxdt; break;
  case 24: nn =  dxdr + dxds + dxdt; break;
  case 25: nn = -dxdr + dxds + dxdt; break;
  default: nn = cameraVector; break;
  }}
  return normalize(nn);
}}

void NoneHexI0_basisAt(in vec3 param, out float[1] basis) {{ }}
void NoneHexC1_basisAt(in vec3 param, out float[1] basis) {{ }}
#endif /* SHAPE_hexahedron */

/** Tetrahedral function space interpolation */
#if defined(BASIS_HGradTetI0) || defined(BASIS_HGradTetC0)
void HGradTetC0_basisAt(in vec3 param, out float basis[1])
{{
  basis[0] = 1.0;
}}

void HGradTetC0_basisGradientAt(in vec3 xx, out float basisGradient[3])
{{
  basisGradient[0] = 0.;
  basisGradient[1] = 0.;
  basisGradient[2] = 0.;
}}

void HGradTetI0_basisAt(in vec3 param, out float basis[1])
{{
  HGradTetC0_basisAt(param, basis);
}}

void HGradTetI0_basisGradientAt(in vec3 xx, out float basisGradient[3])
{{
  HGradTetC0_basisGradientAt(xx, basisGradient);
}}
#endif /* BASIS_HGradTetI0 || BASIS_HGradTetC0 */

#ifdef BASIS_HGradTetC1
void HGradTetC1_basisAt(in vec3 param, out float basis[4])
{{
  basis[0] = 1.0 - param.x - param.y - param.z;
  basis[1] = param.x;
  basis[2] = param.y;
  basis[3] = param.z;
}}

void HGradTetC1_basisGradientAt(in vec3 xx, out float basisGradient[12]) // 4 * 3
{{
  basisGradient[ 0] = -1.;
  basisGradient[ 1] = -1.;
  basisGradient[ 2] = -1.;

  basisGradient[ 3] = +1.;
  basisGradient[ 4] =  0.;
  basisGradient[ 5] =  0.;

  basisGradient[ 6] =  0.;
  basisGradient[ 7] = +1.;
  basisGradient[ 8] =  0.;

  basisGradient[ 9] =  0.;
  basisGradient[10] =  0.;
  basisGradient[11] = +1.;
}}
#endif /* BASIS_HGradTetC1 */

#ifdef BASIS_HCurlTetI1
void HCurlTetI1_basisAt(in vec3 param, out float basis[18]) // 6 * 3
{{
  basis[0 * 3 + 0] = 1.0 - param.y - param.z;
  basis[0 * 3 + 1] = param.x;
  basis[0 * 3 + 2] = param.x;

  basis[1 * 3 + 0] =-param.y;
  basis[1 * 3 + 1] = param.x;
  basis[1 * 3 + 2] = 0.0;

  basis[2 * 3 + 0] = -param.y;
  basis[2 * 3 + 1] = -1.0 + param.x + param.z;
  basis[2 * 3 + 2] = -param.y;

  basis[3 * 3 + 0] = param.z;
  basis[3 * 3 + 1] = param.z;
  basis[3 * 3 + 2] = 1.0 - param.x - param.y;

  basis[4 * 3 + 0] =-param.z;
  basis[4 * 3 + 1] = 0.0;
  basis[4 * 3 + 2] = param.x;

  basis[5 * 3 + 0] = 0.0;
  basis[5 * 3 + 1] =-param.z;
  basis[5 * 3 + 2] = param.y;
}}
#endif /* BASIS_HCurlTetI1 */

#ifdef BASIS_HDivTetI1
void HDivTetI1_basisAt(in vec3 param, out float basis[12]) // 4 * 3
{{
  basis[0 * 3 + 0] = 2.0*param.x;
  basis[0 * 3 + 1] = 2.0*(param.y - 1.0);
  basis[0 * 3 + 2] = 2.0*param.z;

  basis[1 * 3 + 0] = 2.0*param.x;
  basis[1 * 3 + 1] = 2.0*param.y;
  basis[1 * 3 + 2] = 2.0*param.z;

  basis[2 * 3 + 0] = 2.0*(param.x - 1.0);
  basis[2 * 3 + 1] = 2.0*param.y;
  basis[2 * 3 + 2] = 2.0*param.z;

  basis[3 * 3 + 0] = 2.0*param.x;
  basis[3 * 3 + 1] = 2.0*param.y;
  basis[3 * 3 + 2] = 2.0*(param.z - 1.0);
}}
#endif /* BASIS_HDivTetI1 */

#ifdef SHAPE_tetrahedron
int tetrahedron_axisPermutationForSide(in int side)
{{
  switch(side)
  {{
  case 0: return 2; // (+r, +t) = -S
  case 1: return 6; // (-r+s,-r-s+t) = +RST
  case 2: return 0; // (-s, +t) = -R
  case 3: return 4; // (-r, -s) = -T
  }}
}}

vec3 tetrahedron_normalToSideAt(in int sideId, in float shapeVals[{ShapeCoeffPerCell}], in vec3 rst, in vec3 cameraVector)
{{
  vec3 dxdr;
  vec3 dxds;
  vec3 dxdt;
  vec3 nn;
  shapeGradientAt(rst, shapeVals, dxdr, dxds, dxdt);
  switch (sideId)
  {{
  // Triangles
  case  0: nn = cross(dxdr, dxdt); break; // -S
  case  1: nn = cross(-dxdr + dxds, dxdt - dxdr); break; // +RS
  case  2: nn = cross(dxdt, dxds); break; // -R
  case  3: nn = cross(dxds, dxdr); break; // -T
  // Edges
  case  4: nn = -dxds - dxdt; break;
  case  5: nn =  dxdr + dxds - dxdt; break;
  case  6: nn = -dxdr - dxdt; break;
  case  7: nn = -dxdr - dxds; break;
  case  8: nn =  dxdr - dxds + dxdt; break;
  case  9: nn = -dxdr + dxds + dxdt; break;
  // Vertices
  case 10: nn = -dxdr - dxds - dxdt; break;
  case 11: nn =  dxdr - dxds - dxdt; break;
  case 12: nn =  dxdr + dxds - dxdt; break;
  case 13: nn = -dxdr - dxds + dxdt; break;
  default: nn = cameraVector; break;
  }}
  return normalize(nn);
}}

void NoneTetI0_basisAt(in vec3 param, out float[1] basis) {{ }}
void NoneTetC1_basisAt(in vec3 param, out float[1] basis) {{ }}
#endif /* SHAPE_tetrahedron */

/** Quadrilateral function space interpolation */
#ifdef BASIS_HGradQuadI0
void HGradQuadI0_basisAt(in vec3 param, out float basis[1])
{{
  basis[0] = 1.0;
}}

void HGradQuadI0_basisGradientAt(in vec3 param, out float basisGradient[3])
{{
  basisGradient[0] = 0.;
  basisGradient[1] = 0.;
  basisGradient[2] = 0.;
}}
#endif /* BASIS_HGradQuadI0 */

#ifdef BASIS_HGradQuadC1
void HGradQuadC1_basisAt(in vec3 param, out float basis[4])
{{
  basis[0] = (1.0 - param.x)*(1.0 - param.y)/4.0;
  basis[1] = (1.0 + param.x)*(1.0 - param.y)/4.0;
  basis[2] = (1.0 + param.x)*(1.0 + param.y)/4.0;
  basis[3] = (1.0 - param.x)*(1.0 + param.y)/4.0;
}}

void HGradQuadC1_basisGradientAt(in vec3 param, out float basisGradient[12]) // 4 * 3
{{
  basisGradient[ 0] =  -(1.0 - param.y)/4.0;
  basisGradient[ 1] =  -(1.0 - param.x)/4.0;
  basisGradient[ 2] =    0.0;

  basisGradient[ 3] =   (1.0 - param.y)/4.0;
  basisGradient[ 4] =  -(1.0 + param.x)/4.0;
  basisGradient[ 5] =    0.0;

  basisGradient[ 6] =   (1.0 + param.y)/4.0;
  basisGradient[ 7] =   (1.0 + param.x)/4.0;
  basisGradient[ 8] =    0.0;

  basisGradient[ 9] =  -(1.0 + param.y)*(1.0 - param.z)/4.0;
  basisGradient[10] =   (1.0 - param.x)*(1.0 - param.z)/4.0;
  basisGradient[11] =    0.0;
}}
#endif /* BASIS_HGradQuadC1 */

#ifdef SHAPE_quadrilateral
int quadrilateral_axisPermutationForSide(in int side)
{{
  switch(side)
  {{
  case -1: return 5; // (+r, +s) = +T (rendering the cell itself)
  case 0: return 2; // (+r, +t) = -S
  case 1: return 1; // (+s, +t) = +R
  case 2: return 3; // (-r, +t) = +S
  case 3: return 0; // (-s, +t) = -R
  // case 4: return 4; // (-r, -s) = -T
  // case 5: return 5; // (+r, +s) = +T
  // case 1: return 6; // (-r+s,-r-s+t) = +RST
  // case 2: return 0; // (-s, +t) = -R
  // case 3: return 4; // (-r, -s) = -T
  }}
}}

vec3 quadrilateral_normalToSideAt(in int sideId, in float shapeVals[{ShapeCoeffPerCell}], in vec3 rst, in vec3 cameraVector)
{{
  vec3 dxdr;
  vec3 dxds;
  vec3 dxdt; // Note this should always be zero.
  vec3 nn;
  shapeGradientAt(rst, shapeVals, dxdr, dxds, dxdt);
  switch (sideId)
  {{
  // Self
  case -1: nn = cross(dxdr, dxds); break; // +T
  // Edges
  case  0: nn = -dxds; break; // -S
  case  1: nn =  dxdr; break; // +R
  case  2: nn =  dxds; break; // +S
  case  3: nn = -dxdr; break; // -R
  // Vertices
  case  4: nn = -dxdr - dxds; break;
  case  5: nn =  dxdr - dxds; break;
  case  6: nn =  dxdr + dxds; break;
  case  7: nn = -dxdr + dxds; break;
  default: nn = cameraVector; break;
  }}
  return normalize(nn);
}}
void NoneQuadI0_basisAt(in vec3 param, out float[1] basis) {{ }}
void NoneQuadC1_basisAt(in vec3 param, out float[1] basis) {{ }}
#endif /* SHAPE_quadrilateral */

/** Triangle function space interpolation */
#ifdef BASIS_HGradTriI0
void HGradTriI0_basisAt(in vec3 param, out float basis[1])
{{
  basis[0] = 1.;
}}

void HGradTriI0_basisGradientAt(in vec3 xx, out float basisGradient[3])
{{
  basisGradient[0] = 0.;
  basisGradient[1] = 0.;
  basisGradient[2] = 0.;
}}
#endif /* BASIS_HGradTriI0 */

#ifdef BASIS_HGradTriC1
void HGradTriC1_basisAt(in vec3 param, out float basis[3])
{{
  basis[0] = 1.0 - param.x - param.y;
  basis[1] = param.x;
  basis[2] = param.y;
}}

void HGradTriC1_basisGradientAt(in vec3 xx, out float basisGradient[9]) // 3 * 3
{{
  basisGradient[0] = -1.;
  basisGradient[1] = -1.;
  basisGradient[2] =  0.;

  basisGradient[3] = +1.;
  basisGradient[4] =  0.;
  basisGradient[5] =  0.;

  basisGradient[6] =  0.;
  basisGradient[7] = +1.;
  basisGradient[8] =  0.;
}}
#endif /* BASIS_HGradTriC1 */

#ifdef BASIS_HCurlTriI1
void HCurlTriI1_basisAt(in vec3 param, out float[3 * 3] basis, in vec3 node[3])
{{
  vec3 edge0 = node[1] - node[0];
  vec3 edge1 = node[2] - node[1];
  vec3 edge2 = node[2] - node[0];
  vec3 edge3 = node[3] - node[0];
  vec3 edge4 = node[3] - node[1];
  vec3 edge5 = node[3] - node[2];

  float rr = param.x;
  float ss = param.y;
  float tt = param.z;
  float uu = 1.0 - param.x - param.y - param.z;

  basis[ 0] = rr * edge0.x;
  basis[ 1] = rr * edge0.y;
  basis[ 2] = rr * edge0.z;

  basis[ 3] = rr * edge0.x;
  basis[ 4] = rr * edge0.y;
  basis[ 5] = rr * edge0.z;

  basis[ 6] = rr * edge0.x;
  basis[ 7] = rr * edge0.y;
  basis[ 8] = rr * edge0.z;
}}
#endif /* BASIS_HCurlTriI1 */

#ifdef SHAPE_triangle
int triangle_axisPermutationForSide(in int side)
{{
  switch(side)
  {{
  case -1: return 5; // (+r, +s) = +T (normal to triangle itself)
  case 0: return 2; // (+r, +t) = -S
  case 1: return 6; // (-r+s,-r-s) = +RS (T)
  case 2: return 0; // (-s, +t) = -R
  // case 3: return 4; // (-r, -s) = -T
  }}
}}

vec3 triangle_normalToSideAt(in int sideId, in float shapeVals[{ShapeCoeffPerCell}], in vec3 rst, in vec3 cameraVector)
{{
  vec3 dxdr;
  vec3 dxds;
  vec3 dxdt; // This will always be zero.
  vec3 nn;
  shapeGradientAt(rst, shapeVals, dxdr, dxds, dxdt);
  switch (sideId)
  {{
  // Self
  case -1: nn = cross(dxdr, dxds); break; // +T
  // Edges
  case  0: nn = -dxds; break; // -S
  case  1: nn =  dxdr + dxds; break; // +RS
  case  2: nn = -dxdr; break; // -R
  // Vertices
  case  4: nn = -dxdr - dxds; break;
  case  5: nn =  dxdr - 0.5 * dxds; break;
  case  6: nn = -0.5 * dxdr + dxds; break;
  default: nn = cameraVector; break;
  }}
  return normalize(nn);
}}

void NoneTriI0_basisAt(in vec3 param, out float[1] basis) {{ }}
void NoneTriC1_basisAt(in vec3 param, out float[1] basis) {{ }}
#endif /* SHAPE_triangle */

/** Edge function space interpolation */
#ifdef BASIS_HGradEdgeI0
void HGradEdgeI0_basisAt(in vec3 param, out float basis[1])
{{
  basis[0] = 1.0;
}}

void HGradEdgeI0_basisGradientAt(in vec3 xx, out float basisGradient[3])
{{
  basisGradient[0] = 0.;
  basisGradient[1] = 0.;
  basisGradient[2] = 0.;
}}
#endif /* BASIS_HGradEdgeI0 */

#ifdef BASIS_HGradEdgeC1
void HGradEdgeC1_basisAt(in vec3 param, out float basis[2])
{{
  basis[0] = 1.0 - param.x;
  basis[1] = param.x;
}}

void HGradEdgeC1_basisGradientAt(in vec3 xx, out float basisGradient[6]) // 2 * 3
{{
  basisGradient[0] = -1.;
  basisGradient[1] =  0.;
  basisGradient[2] =  0.;

  basisGradient[3] = +1.;
  basisGradient[4] =  0.;
  basisGradient[5] =  0.;
}}
#endif /* BASIS_HGradEdgeC1 */

#ifdef SHAPE_edge
int edge_axisPermutationForSide(in int side)
{{
  if (side < 0) {{ return 7; }} // Camera-facing normal.
  return side; // side 0 = -R (0), side 1 = +R (1)
}}

vec3 edge_normalToSideAt(in int sideId, in float shapeVals[{ShapeCoeffPerCell}], in vec3 rst, in vec3 cameraVector)
{{
  float eps = 1.19209e-07;
  vec3 dxdr;
  vec3 dxds; // This will always be zero.
  vec3 dxdt; // This will always be zero.
  vec3 nn;
  shapeGradientAt(rst, shapeVals, dxdr, dxds, dxdt);
  switch (sideId)
  {{
  // Self
  case -1:
    nn = cross(dxdr, vec3(0, 1, 0));
    if (length(nn) < eps)
    {{
      nn = cross(dxdr, vec3(0, 0, 1));
    }}
    break;
  // Vertices
  case  0: nn = -dxdr; break; // -R
  case  1: nn =  dxdr; break; // +R
  default: nn = cameraVector; break;
  }}
  return normalize(nn);
}}

void NoneEdgeI0_basisAt(in vec3 param, out float[1] basis) {{ }}
void NoneEdgeC1_basisAt(in vec3 param, out float[1] basis) {{ }}
#endif /* SHAPE_edge */

/** Vertex function space interpolation */
#ifdef BASIS_HGradVertI0
void HGradVertI0_basisAt(in vec3 param, out float basis[0])
{{
  basis[0] = 1.0;
}}

void HGradVertI0_basisGradientAt(in vec3 xx, out float basisGradient[3])
{{
  basisGradient[0] = 0.;
  basisGradient[1] = 0.;
  basisGradient[2] = 0.;
}}
#endif /* BASIS_HGradVertI0 */

#ifdef BASIS_HGradVertC1
// TODO: Eliminate this as it makes no sense to have a linear basis over a null space.
void HGradVertC1_basisAt(in vec3 param, out float basis[1])
{{
  basis[0] = 1.0;
}}

void HGradVertC1_basisGradientAt(in vec3 xx, out float basisGradient[3])
{{
  basisGradient[0] = 0.;
  basisGradient[1] = 0.;
  basisGradient[2] = 0.;
}}
#endif /* BASIS_HGradVertC1 */

#ifdef SHAPE_vertex
int vertex_axisPermutationForSide(in int side)
{{
  return 7; // Camera-facing normal
}}

vec3 vertex_normalToSideAt(in int sideId, in float shapeVals[{ShapeCoeffPerCell}], in vec3 rst, in vec3 cameraVector)
{{
  vec3 nn;
  nn = cameraVector;
  return normalize(nn);
}}

void NoneVertI0_basisAt(in vec3 param, out float[1] basis) {{ }}
void NoneVertC1_basisAt(in vec3 param, out float[1] basis) {{ }}
#endif /* SHAPE_vertex */
