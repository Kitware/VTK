### Program common data
commonDefs = """
// NB: NumIntPt does *NOT* have to be equivalent to NumPtsPerCell.
//     They are different when the "shape" attribute has a different
//     interpolation order than the (field) "color" attribute.

// Macros for things specific to the current cell-type, cell-attribute, and side/cell:
// Number of integration points per cell. (Tetrahedral C1 HDIV ⇒  4)
#define NumIntPt 4
// Number of field values per integration point (For HDIV B_Field, this is 1)
#define NumValPP 1
// Number Of corner vertices per (triangular) side
#define NumPtsPerSide 3
// Number of corner vertices per (tetrahedral) cell
#define NumPtsPerCell 4
// The location in side_offsets to use when looking up a location in side_local.
#define ShapeIndex 0
// Non-zero (true) when coloring by a scalar field (i.e., field_vals is provided)
#define HaveFields true

"""

### Cell evaluation
# C1 HGRAD I1 Tet
cellEval_C1_HGRAD_I1_Tet = """
void basisAt(in vec3 point, out float[NumIntPt] basis)
{
  basis[0] = 1.0 - point.x - point.y - point.z;
  basis[1] = point.x;
  basis[2] = point.y;
  basis[3] = point.z;
}

void basisGradientAt(in vec3 xx, out vec3 basisGradient[NumIntPt])
{
  basisGradient[0] = vec3(-1., -1., -1.);
  basisGradient[1] = vec3(+1.,  0.,  0.);
  basisGradient[2] = vec3( 0., +1.,  0.);
  basisGradient[3] = vec3( 0.,  0., +1.);
}

int axisPermutationForSide(in int side)
{
  switch(side)
  {
  case 0: return 2; // (+r, +t) = -S
  case 1: return 6; // (-r+s,-r-s+t) = +RST
  case 2: return 0; // (-s, +t) = -R
  case 3: return 4; // (-r, -s) = -T
  }
}
"""
# DG HCURL I1 Tet
cellEval_DG_HCURL_I1_Tet = """
void basisAt(in vec3 point, in vec3[NumIntPt] shape, out vec3[NumIntPt] basis)
{
}
"""

# Utilities
# depends on commonDefs
cellUtil = """
void shapeGradientAt(in vec3 xx, in vec3 coord[NumIntPt], out vec3 dxdr, out vec3 dxds, out vec3 dxdt)
{
  dxdr = vec3(0.);
  dxds = vec3(0.);
  dxdt = vec3(0.);
  vec3 basisGradient[NumIntPt];
  basisGradientAt(xx, basisGradient);
  for (int ii = 0; ii < NumIntPt; ++ii)
  {
    dxdr += coord[ii] * basisGradient[ii].x;
    dxds += coord[ii] * basisGradient[ii].y;
    dxdt += coord[ii] * basisGradient[ii].z;
  }
}

void evaluateAt(in vec3 rr, in float cellData[NumIntPt * NumValPP], out float value[NumValPP])
{
  float basis[NumIntPt];
  basisAt(rr, basis);
  for (int cc = 0; cc < NumValPP; ++cc)
  {
    value[cc] = 0.0;
  }
  for (int pp = 0; pp < NumIntPt; ++pp)
  {
    for (int cc = 0; cc < NumValPP; ++cc)
    {
      value[cc] += cellData[pp * NumValPP + cc] * basis[pp];
    }
  }
}

vec3 normalToSideAt(int sideId, vec3 corners[NumIntPt], vec3 rst)
{
  vec3 dxdr;
  vec3 dxds;
  vec3 dxdt;
  vec3 nn;
  int axisPerm = axisPermutationForSide(sideId);
  if (axisPerm < 6)
  {
    shapeGradientAt(rst, corners, dxdr, dxds, dxdt);
  }
  else
  {
    dxdr = corners[2] - corners[1];
    dxds = corners[3] - corners[1];
  }
  if (axisPerm == 0) { nn = cross(dxdt, dxds); /* -R */ } else
  if (axisPerm == 1) { nn = cross(dxds, dxdt); /* +R */ } else
  if (axisPerm == 2) { nn = cross(dxdr, dxdt); /* -S */ } else
  if (axisPerm == 3) { nn = cross(dxdt, dxdr); /* +S */ } else
  if (axisPerm == 4) { nn = cross(dxds, dxdr); /* -T */ } else
  if (axisPerm == 5) { nn = cross(dxdr, dxds); /* +T */ } else

  /* Below is a special case for tetrahedra which have a +RST face that is not axis-aligned: */
  if (axisPerm == 6) { nn = cross(dxdr, dxds); /* +RST */ } else

  /* camera facing */ { nn = vec3(0,0,-1); }
  /* camera facing { nn = cameraVector; } */
  return normalize(nn);
}
"""

### Vertex shader
vertShaderSource = """//VTK::System::Dec
// camera and actor matrix values
//VTK::Camera::Dec

uniform samplerBuffer vertices;

// The parametric coordinates of each shape-attribute integration point.
uniform samplerBuffer cell_parametrics;

// Tuples of (cell-id, side-id) to render.
uniform isamplerBuffer sides;
// Look up the offset at which local cell-side connectivity starts in \a side_local.
uniform isamplerBuffer side_offsets;
// The subset of DOFs in shape_conn that correspond to each side in cells of this shape.
uniform isamplerBuffer side_local;
// The indices of all DOFs for all cells of this shape.
uniform isamplerBuffer shape_conn;
// The (x,y,z) points of each DOF in the entire mesh.
uniform samplerBuffer shape_vals;
// We may need this if the field and shape have different interpolation orders.
// uniform isamplerBuffer field_conn;
// The coefficient of each basis function at each integration point in each cell.
uniform samplerBuffer field_vals;
// For vector- or tensor-valued field attributes, which should determine the color?
// If -1 or -2, the L1-norm or L2-norm are used.
uniform int field_component;

// Position of vertex in view coordinates.
//VTK::PositionVC::Dec
// Color output per vertex in the rendered primitives.
//VTK::Color::Dec

// The normal of the output primitive in view coordinates.
//VTK::Normal::Dec

//VTK::Draw::Dec

smooth out vec3 LightVector0;
smooth out vec3 EyeNormal;
flat out int sideIdVS;
flat out int fieldComponentVS;

/// View coordinate normal for this vertex.
smooth out vec3 vertexNormalVCVS;

{{commonDefs}}
{{cellEval}}
{{cellUtil}}

/// Parametric coordinates of this vertex.
smooth out vec3 pcoordVS;
/// Normal vector for this vertex.
smooth out vec3 vertexNormalMC;

// NB: We might be able optimize the shapeValuesVS and fieldValuesVS down
//     to just NumPtsPerSide and NumIntPtPerSide (assuming the restriction
//     of the shape/field attributes can be defined only in terms of the
//     subset of values that "live" on the side). However, this would
//     require separate side_offsets and side_local samplers for the
//     shape and field values in order for us to subset them properly.

flat out vec3 shapeValuesVS[NumPtsPerCell];
flat out float fieldValuesVS[NumIntPt * NumValPP];

void main()
{
  ivec2 cellAndSide = texelFetch(sides, gl_InstanceID).st;
  int sideOffset = texelFetch(side_offsets, ShapeIndex).s;

  // Fetch point coordinates and per-integration-point field values for
  // the entire cell, passing them in bulk to the fragment shader using
  // a "flat" keyword.
  for (int ii = 0; ii < NumPtsPerCell; ++ii)
  {
    int vertexId = texelFetch(shape_conn, cellAndSide.s * NumPtsPerCell + ii).s;
    shapeValuesVS[ii] = texelFetch(shape_vals, vertexId).xyz;
  }
  if (HaveFields)
  {
    for (int ii = 0; ii < NumIntPt; ++ii)
    {
      for (int jj = 0; jj < NumValPP; ++jj)
      {
        fieldValuesVS[ii * NumValPP + jj] =
          // (cellAndSide.s * NumIntPt + ii) * NumValPP + jj;
          texelFetch(field_vals, (cellAndSide.s * NumIntPt + ii) * NumValPP + jj).r;
      }
    }
  }

  int sideVertexIndex = texelFetch(side_local, sideOffset + cellAndSide.t * NumPtsPerSide + gl_VertexID).s;
  int vertexId = texelFetch(shape_conn, cellAndSide.s * NumPtsPerCell + sideVertexIndex).s;
  vec4 vertexMC = vec4(texelFetch(shape_vals, vertexId).xyz, 1.f);
  gl_Position = MCDCMatrix * vertexMC;
  LightVector0 = vec3(0., 0., +1.);
  EyeNormal = mat3(MCDCMatrix) * vec3(0., 0., +1.);
  pcoordVS = texelFetch(cell_parametrics, sideVertexIndex).xyz;
  vertexNormalMC = normalToSideAt(cellAndSide.t, shapeValuesVS, pcoordVS);
  vertexPositionVCVS = MCVCMatrix * vertexMC;
  vertexNormalVCVS = normalMatrix * vertexNormalMC;
  fieldComponentVS = 0;
  sideIdVS = cellAndSide.t;
}
""".replace('{{commonDefs}}', commonDefs).replace('{{cellEval}}', cellEval_C1_HGRAD_I1_Tet).replace('{{cellUtil}}', cellUtil)

### Fragment shader
fragShaderSource = """//VTK::System::Dec
//VTK::Output::Dec

{{commonDefs}}
{{cellEval}}
{{cellUtil}}

// Camera prop
//VTK::Camera::Dec

// optional color passed in from the vertex shader, vertexColor
//VTK::Color::Dec

// cell Normal used to light up and shade the pixels.
//VTK::Normal::Dec

// Lights
//VTK::Light::Dec

/// View coordinate normal for this vertex.
smooth in vec3 vertexNormalVCVS;
/// View coordinate position for this vertex.
//VTK::PositionVC::Dec
smooth in vec3 vertexNormalMC;

// basic material shading capabilities
uniform vec3 color_ambient;
uniform vec3 color_diffuse;
uniform vec3 color_specular;
uniform vec3 color_ambient_bf; // backface
uniform vec3 color_diffuse_bf; // backface
uniform vec3 color_specular_bf; // backface
uniform float intensity_ambient;
uniform float intensity_diffuse;
uniform float intensity_specular;
uniform float intensity_opacity;
uniform float intensity_ambient_bf; // backface
uniform float intensity_diffuse_bf; // backface
uniform float intensity_specular_bf; // backface
uniform float intensity_opacity_bf; // backface
uniform float power_specular;
uniform float power_specular_bf; // backface
uniform int enable_specular;

smooth in vec3 LightVector0;
smooth in vec3 EyeNormal;
smooth in vec3 pcoordVS;

uniform sampler2D color_map;
uniform vec3 field_range;

flat in vec3 shapeValuesVS[NumPtsPerCell];
flat in float fieldValuesVS[NumIntPt * NumValPP];
flat in int fieldComponentVS;
flat in int sideIdVS;

void main()
{
  vec3 eyeNormal;
  vec3 lightVector;
  float dotProduct;
  float fieldValue[NumValPP];
  float scalar; // The non-normalized scalar value computed from a fieldValue tuple.
  vec2 texCoord; // Used for color lookup.

  eyeNormal = normalize(vertexNormalMC); // Perspective transform requires re-normalization.
  lightVector = normalize(LightVector0);

  // Debug: color by normal vector (offset to avoid black):
  // vec3 tn = 0.25 * eyeNormal + 0.75 * vec3(1., 1., 1.);
  // gl_FragData[0] = vec4(tn, 1.0);

  // Debug: color by cell parametric coordinates:
  // vec3 tn = 0.5 * pcoordVS + 0.5 * vec3(1., 1., 1.);
  // gl_FragData[0] = vec4(tn, 1.0);

  // Evaluate the basis function at this fragment's parametric coords (pcoordVS),
  // yielding the \a fieldValue tuple.
  evaluateAt(pcoordVS, fieldValuesVS, fieldValue);

  // Choose how we map the \a fieldValue tuple into a texture-map coordinate.
  switch (fieldComponentVS)
  {
  case -2:
    {
      float mag = 0.0;
      for (int cc = 0; cc < NumValPP; ++cc)
      {
        mag += fieldValue[cc] * fieldValue[cc];
      }
      scalar = sqrt(mag);
    }
  case -1:
    {
      scalar = fieldValue[0];
      for (int cc = 1; cc < NumValPP; ++cc)
      {
        if (fieldValue[cc] > scalar)
        {
          scalar = fieldValue[cc];
        }
      }
    }
  default:
    scalar = fieldValue[fieldComponentVS];
  }
  texCoord = vec2((scalar - field_range[0]) / field_range[2], 0.0);
  // texCoord = vec2(fieldValuesVS[3], 0.);

  dotProduct = dot(eyeNormal, lightVector);
  // TODO: Should we take abs(dotProduct) here so inverted elements are still shaded properly?
  float val = clamp(abs(dotProduct), 0., 1.);
  vec4 color = texture(color_map, texCoord);

  vec3 ambientColor = intensity_ambient * color.rgb;
  vec3 diffuseColor = intensity_diffuse * color.rgb;
  vec3 specularColor = intensity_specular * color.rgb;
  float opacity = intensity_opacity * color.a;
  float specularPower = 0.0f;

  // float df = 0.75;
  // gl_FragData[0] = df * color + (1. - df) * vec4(val, val, val, 1.0);
  gl_FragData[0] = 0.75 * color + (1. - 0.75) * vec4(val, val, val, 1.0);
  // gl_FragData[0] = color; // vec4(color.r * val, color.g * val, color.b * val, color.a);
  //VTK::Normal::Impl

  //VTK::Color::Impl

  //VTK::Light::Impl
}
""".replace('{{commonDefs}}', commonDefs).replace('{{cellEval}}', cellEval_C1_HGRAD_I1_Tet).replace('{{cellUtil}}', cellUtil)
