//VTK::System::Dec
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

//VTK::Camera::Dec

/// A comma separated list of strings with one or more of the following options:
/// 1. The shape of the abstract patch which will be tessellated - isolines, triangles, quads.
/// 2. The type of spacing between tessellated vertices - cw, ccw
/// 3. The ordering of tessellated vertices that primitive generator must use - isolines, triangles, quads
/// 4. Whether points must be rendered - point_mode
layout({TessellationOptions}) in;

// The indices of all DOFs for all cells of this shape.
uniform highp isamplerBuffer shape_conn;
// The (x,y,z) points of each DOF in the entire mesh.
uniform samplerBuffer shape_vals;

patch in int cellIdTCSOutput;
patch in int sideIdTCSOutput;
patch in int instanceIdTCSOutput;
in vec3 pcoordTCSOutput[];

flat out int cellIdTESOutput;
flat out int instanceIdTESOutput;
#if {UsesGeometryShaders}
smooth out vec3 patchDistanceTESOutput;
#endif

smooth out vec3 normalVCTESOutput;
smooth out vec4 vertexVCTESOutput;
smooth out vec3 pcoordTESOutput;

{commonDefs}
{cellEval}
{cellUtil}

vec3 interpolate_2(in vec3 v0, in vec3 v1)
{{
  return vec3(gl_TessCoord.x) * v0 + vec3(1.0 - gl_TessCoord.x) * v1;
}}
vec4 interpolate_2(in vec4 v0, in vec4 v1)
{{
  return vec4(gl_TessCoord.x) * v0 + vec4(1.0 - gl_TessCoord.x) * v1;
}}
vec3 interpolate_3(in vec3 v0, in vec3 v1, in vec3 v2)
{{
  return vec3(gl_TessCoord.x) * v0 + vec3(gl_TessCoord.y) * v1 + vec3(gl_TessCoord.z) * v2;
}}
vec4 interpolate_3(in vec4 v0, in vec4 v1, in vec4 v2)
{{
  return vec4(gl_TessCoord.x) * v0 + vec4(gl_TessCoord.y) * v1 + vec4(gl_TessCoord.z) * v2;
}}
vec3 interpolate_4(in vec3 v0, in vec3 v1, in vec3 v2, in vec3 v3)
{{
  vec3 a = mix(v0, v1, gl_TessCoord.x);
  vec3 b = mix(v3, v2, gl_TessCoord.x);
  return mix(a, b, gl_TessCoord.y);
}}
vec4 interpolate_4(in vec4 v0, in vec4 v1, in vec4 v2, in vec4 v3)
{{
  vec4 a = mix(v0, v1, gl_TessCoord.x);
  vec4 b = mix(v3, v2, gl_TessCoord.x);
  return mix(a, b, gl_TessCoord.y);
}}

void main()
{{
  float shapeValues[{ShapeCoeffPerCell}];
  vec3 pcoord;
  vec4 position;
  vec3 vertexNormalVC;
  float coordinateArray[{ShapeNumValPP}];
  vec3 eyeNormalMC = vec3(0.0f, 0.0f, 1.0f);

  if ({PatchSize} == 2)
  {{
    pcoord = interpolate_2(pcoordTCSOutput[0], pcoordTCSOutput[1]);
  }}
  else if ({PatchSize} == 3)
  {{
    pcoord = interpolate_3(pcoordTCSOutput[0], pcoordTCSOutput[1], pcoordTCSOutput[2]);
  }}
  else if ({PatchSize} == 4)
  {{
    pcoord = interpolate_4(pcoordTCSOutput[0], pcoordTCSOutput[1], pcoordTCSOutput[2], pcoordTCSOutput[3]);
  }}
  shapeValuesForCell(cellIdTCSOutput, shapeValues);
  shapeEvaluateAt(pcoord, shapeValues, coordinateArray);
  if ({ShapeNumValPP} == 3)
  {{
    position.x = coordinateArray[0];
    position.y = coordinateArray[1];
    position.z = coordinateArray[2];
  }}
  else
  {{
    position.xyz = vec3(1.0); // fallback
  }}
  position.w = 1.0;
  vec3 vertexNormalMC = normalToSideAt(sideIdTCSOutput, shapeValues, pcoord, -eyeNormalMC);

  // Normal vectors are transformed in a different way than vertices.
  // Instead of pre-multiplying with MCDCMatrix, a different matrix is used.
  // This `normalMatrix` is computed on the CPU. It must be the result of the following:
  // normalMatrix = inverse(ModelToWorld X WorldToView)
  // Read more about normal matrix at http://www.songho.ca/opengl/gl_normaltransform.html
  if ((DrawingCellsNotSides && NumPtsPerCell <= 2) || (!DrawingCellsNotSides && NumPtsPerSide <= 2))
  {{
    // for lines or vertices, the normal will always face the camera.
    vertexNormalVC = vec3(vertexNormalMC.xy, 1.0f);
  }}
  else
  {{
    vertexNormalVC = normalMatrix * vertexNormalMC;
  }}

  // Transform the vertex by the model-to-device coordinate matrix.
  // This matrix must be the result of the following multiplication:
  // MCDCMatrix = ModelToWorld X WorldToView X ViewToDisplay
  gl_Position = MCDCMatrix * position;
  cellIdTESOutput = cellIdTCSOutput;
  instanceIdTESOutput = instanceIdTCSOutput;
  vertexVCTESOutput = MCVCMatrix * position;
  normalVCTESOutput = vertexNormalVC;
#if {UsesGeometryShaders}
  patchDistanceTESOutput = gl_TessCoord;
#endif
  pcoordTESOutput = pcoord;
}}
