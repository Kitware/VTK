//VTK::System::Dec
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
//VTK::Camera::Dec

// The parametric coordinates of each shape-attribute integration point.
uniform samplerBuffer cell_parametrics;

// Tuples of (cell-id, side-id) to render.
uniform highp isamplerBuffer sides;
// Look up the offset at which local cell-side connectivity starts in \a side_local.
uniform highp isamplerBuffer side_offsets;
// The subset of DOFs in shape_conn that correspond to each side in cells of this shape.
uniform highp isamplerBuffer side_local;
// The indices of all DOFs for all cells of this shape.
uniform highp isamplerBuffer shape_conn;
// The (x,y,z) points of each DOF in the entire mesh.
uniform samplerBuffer shape_vals;

flat out int cellIdVSOutput;
flat out int sideIdVSOutput;
flat out int instanceIdVSOutput;

{commonDefs}
{cellEval}
{cellUtil}

/// Parametric coordinates of this vertex.
smooth out vec3 pcoordVSOutput;

#if !{UsesTessellationShaders}
/// View coordinate normal for this vertex.
//VTK::Normal::Dec

/// View coordinate position for this vertex.
//VTK::PositionVC::Dec
#endif

void main()
{{
  // Identify the cell ID and side ID (if applicable) we are to render.
  ivec2 cellAndSide;
  if (DrawingCellsNotSides)
  {{
     cellAndSide = ivec2(gl_InstanceID, 0);
     // These two VS outputs are to be used for picking:
     cellIdVSOutput = cellAndSide.s;
     sideIdVSOutput = -1;
     // Note sideIdVSOutput != cellAndSide.t; this is so that we can properly
     // compute the cell connectivity (using the zero offset in cellAndSide.t)
     // while also computing the normal (using sideIdVSOutput).
  }}
  else
  {{
     cellAndSide = texelFetchBuffer(sides, gl_InstanceID).st;
     // These two VS outputs are to be used for picking:
     cellIdVSOutput = cellAndSide.s;
     sideIdVSOutput = cellAndSide.t;
  }}
  instanceIdVSOutput = gl_InstanceID;

  // Fetch the offset into the (ragged) side_local table:
  int sideRaggedOffset = texelFetchBuffer(side_offsets, {ShapeIndex}).s;

  // Now compute the location of the current vertex inside the current side or cell.
  // NB: {{SideOffset}} = {SideOffset} is the offset to apply to side IDs so we can
  // look up their connectivity offset from sideRaggedOffset (the start in side_local
  // for connectivity of sides of our type).
  int sideVertexIndex =
    texelFetchBuffer(side_local,
      sideRaggedOffset + (cellAndSide.t - {SideOffset}) * NumPtsPerSide + gl_VertexID).s;
  int vertexId = texelFetchBuffer(shape_conn, cellIdVSOutput * {ShapeNumBasisFun} + sideVertexIndex).s;
  // Parametric coordinate for this vertex.
  pcoordVSOutput = texelFetchBuffer(cell_parametrics, sideVertexIndex).xyz;
  // position for this vertex as defined in vtk data model.
  vec4 vertexMC = vec4(
      texelFetchBuffer(shape_vals, vertexId * 3).x,
      texelFetchBuffer(shape_vals, vertexId * 3 + 1).x,
      texelFetchBuffer(shape_vals, vertexId * 3 + 2).x,
      1.0f);
#if !{UsesTessellationShaders}
  float shapeValues[{ShapeCoeffPerCell}];
  shapeValuesForCell(cellIdVSOutput, shapeValues);
  // default eye direction in model coordinates.
  vec3 eyeNormalMC = vec3(0.0f, 0.0f, 1.0f);
  vec3 vertexNormalMC = normalToSideAt(sideIdVSOutput, shapeValues, pcoordVSOutput, -eyeNormalMC);

  // Transform the vertex by the model-to-device coordinate matrix.
  // This matrix must be the result of the following multiplication:
  // MCDCMatrix = ModelToWorld X WorldToView X ViewToDisplay
  gl_Position = MCDCMatrix * vertexMC;

  // Transform vertex posittion to view coordinate.
  // Some operations in fragment shader want it that way.
  vertexVCVSOutput = MCVCMatrix * vertexMC;

  // Normal vectors are transformed in a different way than vertices.
  // Instead of pre-multiplying with MCDCMatrix, a different matrix is used.
  // This `normalMatrix` is computed on the CPU. It must be the result of the following:
  // normalMatrix = inverse(ModelToWorld X WorldToView)
  // Read more about normal matrix at http://www.songho.ca/opengl/gl_normaltransform.html
  if ((DrawingCellsNotSides && NumPtsPerCell <= 2) || (!DrawingCellsNotSides && NumPtsPerSide <= 2))
  {{
    // for lines or vertices, the normal will always face the camera.
    normalVCVSOutput = vec3(vertexNormalMC.xy, 1.0f);
  }}
  else
  {{
    normalVCVSOutput = normalMatrix * vertexNormalMC;
  }}
#else
  gl_Position = vertexMC;
#endif
}}
