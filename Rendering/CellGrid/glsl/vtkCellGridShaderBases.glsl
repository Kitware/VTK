// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// NB: The functions named NoneXxxI0_basisAt() are dummy functions used when
// not coloring by a field. They are never called.

// Forward-declare the "generic" shape gradient so that specializations of
// {{ShapeName}}_normalToSideAt() can call it. This way things will just
// work, even with curved elements.
void shapeGradientAt(in vec3 param, in float coeff[{ShapeCoeffPerCell}], out vec3 dxdr, out vec3 dxds, out vec3 dxdt);

#define RealT float

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
#if 0
     // NB: These work for a linear pyramid but will not compute normals for higher-order pyramids.
  case  0: nn = cross(corners[1] - corners[0], corners[4] - corners[0]); break;
  case  1: nn = cross(corners[2] - corners[1], corners[4] - corners[1]); break;
  case  2: nn = cross(corners[3] - corners[2], corners[4] - corners[2]); break;
  case  3: nn = cross(corners[0] - corners[3], corners[4] - corners[3]); break;
#else
     // NB: These would work in theory for higher-order pyramids, but fail due to a degeneracy in the parameter space.
  case  0: nn = cross( dxdr,  dxds + 0.5 * dxdt); break; // -S -dxds + dxdt  cross(dxdr, dxdt)
  case  1: nn = cross( dxds, -dxdr + 0.5 * dxdt); break; // +R  dxdr + dxdt  cross(dxds, dxdt)
  case  2: nn = cross(-dxdr, -dxds + 0.5 * dxdt); break; // +S  dxds + dxdt  cross(dxdt, dxdr)
  case  3: nn = cross(-dxds,  dxdr + 0.5 * dxdt); break; // -R -dxdr + dxdt  cross(dxdt, dxds)
#endif
  // Quadrilateral
  case  4: nn = cross(dxds, dxdr); break; // -T
  // Edges and vertices should not be shaded.
  default: nn = cameraVector; break;
  }}
  return normalize(nn);
}}

void NonePyrI0_basisAt(in vec3 param, out float[1] basis) {{ }}
void NonePyrC1_basisAt(in vec3 param, out float[1] basis) {{ }}
#endif /* SHAPE_pyramid */

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
  // Edges and vertices should not be shaded.
  default: nn = cameraVector; break;
  }}
  return normalize(nn);
}}

void NoneWdgI0_basisAt(in vec3 param, out float[1] basis) {{ }}
void NoneWdgC1_basisAt(in vec3 param, out float[1] basis) {{ }}
#endif /* SHAPE_wedge */

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
  // Edges and vertices should be rendered without shading.
  default: nn = cameraVector; break;
  }}
  return normalize(nn);
}}

void NoneHexI0_basisAt(in vec3 param, out float[1] basis) {{ }}
void NoneHexC1_basisAt(in vec3 param, out float[1] basis) {{ }}
#endif /* SHAPE_hexahedron */

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
  // Edges and vertices should not be shaded.
  default: nn = cameraVector; break;
  }}
  return normalize(nn);
}}

void NoneTetI0_basisAt(in vec3 param, out float[1] basis) {{ }}
void NoneTetC1_basisAt(in vec3 param, out float[1] basis) {{ }}
#endif /* SHAPE_tetrahedron */

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
  // Edges and vertices should not be shaded.
  default: nn = cameraVector; break;
  }}
  return normalize(nn);
}}
void NoneQuadI0_basisAt(in vec3 param, out float[1] basis) {{ }}
void NoneQuadC1_basisAt(in vec3 param, out float[1] basis) {{ }}
#endif /* SHAPE_quadrilateral */

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
  // Edges and vertices should not be shaded.
  default: nn = cameraVector; break;
  }}
  return normalize(nn);
}}

void NoneTriI0_basisAt(in vec3 param, out float[1] basis) {{ }}
void NoneTriC1_basisAt(in vec3 param, out float[1] basis) {{ }}
#endif /* SHAPE_triangle */

#ifdef SHAPE_edge
int edge_axisPermutationForSide(in int side)
{{
  if (side < 0) {{ return 7; }} // Camera-facing normal.
  return side; // side 0 = -R (0), side 1 = +R (1)
}}

vec3 edge_normalToSideAt(in int sideId, in float shapeVals[{ShapeCoeffPerCell}], in vec3 rst, in vec3 cameraVector)
{{
  return normalize(cameraVector);
}}

void NoneEdgeI0_basisAt(in vec3 param, out float[1] basis) {{ }}
void NoneEdgeC1_basisAt(in vec3 param, out float[1] basis) {{ }}
#endif /* SHAPE_edge */

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
