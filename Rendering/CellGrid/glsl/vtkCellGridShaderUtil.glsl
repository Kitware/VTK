// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
void shapeGradientAt(in vec3 param, in float coord[{ShapeCoeffPerCell}], out vec3 dxdr, out vec3 dxds, out vec3 dxdt)
{{
  dxdr = vec3(0.);
  dxds = vec3(0.);
  dxdt = vec3(0.);
  float basisGradient[3 * {ShapeCellBasisSize}];
  shapeBasisGradientAt(param, basisGradient);
  for (int ii = 0; ii < {ShapeNumBasisFun}; ++ii)
  {{
    int start = ii * {ShapeMultiplicity};
    vec3 point = vec3(coord[start], coord[start + 1], coord[start + 2]);
    dxdr += point * basisGradient[ii * {ShapeMultiplicity} + 0];
    dxds += point * basisGradient[ii * {ShapeMultiplicity} + 1];
    dxdt += point * basisGradient[ii * {ShapeMultiplicity} + 2];
  }}
}}

void colorEvaluateAt(
  in vec3 rr,
  in float shapeData[{ShapeCoeffPerCell}],
  in float cellData[{ColorCoeffPerCell}],
  out float value[{ColorNumValPP}])
{{
  float basis[{ColorCellBasisSize}];
  colorBasisAt(rr, basis);
  for (int cc = 0; cc < {ColorNumValPP}; ++cc)
  {{
    value[cc] = 0.0;
  }}
  for (int pp = 0; pp < {ColorNumBasisFun}; ++pp)
  {{
    for (int cc = 0; cc < {ColorMultiplicity}; ++cc)
    {{
      for (int bb = 0; bb < {ColorBasisSize}; ++bb)
      {{
        value[cc * {ColorBasisSize} + bb] += cellData[pp * {ColorMultiplicity} + cc] * basis[pp * {ColorBasisSize} + bb];
      }}
    }}
  }}
#if {ColorScaleInverseJacobian}
  {{
    // Scale the value by the inverse Jacobian of the shape attribute.
    vec3 dxdr;
    vec3 dxds;
    vec3 dxdt;
    mat3 jac;
    shapeGradientAt(rr, shapeData, dxdr, dxds, dxdt);
    jac = transpose(mat3(dxdr, dxds, dxdt));
    // jac = mat3(dxdr, dxds, dxdt);
    mat3 ijac = inverse(jac);
    for (int cc = 0; cc < {ColorNumValPP} / 3; ++cc)
    {{
      vec3 unscaled = vec3(value[cc * 3], value[cc * 3 + 1], value[cc * 3 + 2]);
      vec3 scaled = ijac * unscaled;
      value[cc * 3    ] = scaled.x;
      value[cc * 3 + 1] = scaled.y;
      value[cc * 3 + 2] = scaled.z;
    }}
  }}
#elif {ColorScaleScaledJacobian}
  {{
    // Scale the value by the determinant of the Jacobian *and*
    // transform the value by the Jacobian.
    vec3 dxdr;
    vec3 dxds;
    vec3 dxdt;
    mat3 jac;
    shapeGradientAt(rr, shapeData, dxdr, dxds, dxdt);
    // jac = transpose(mat3(dxdr, dxds, dxdt));
    jac = mat3(dxdr, dxds, dxdt);
    float jdet = determinant(jac);
    for (int cc = 0; cc < {ColorNumValPP} / 3; ++cc)
    {{
      vec3 unscaled = vec3(value[cc * 3], value[cc * 3 + 1], value[cc * 3 + 2]);
      vec3 scaled = jac * unscaled / jdet;
      value[cc * 3    ] = scaled.x;
      value[cc * 3 + 1] = scaled.y;
      value[cc * 3 + 2] = scaled.z;
    }}
  }}
#endif
}}

void shapeEvaluateAt(in vec3 rr, in float shapeVals[{ShapeCoeffPerCell}], out float value[{ShapeNumValPP}])
{{
  float basis[{ShapeCellBasisSize}];
  shapeBasisAt(rr, basis);
  for (int cc = 0; cc < {ShapeNumValPP}; ++cc)
  {{
    value[cc] = 0.0;
  }}
  for (int pp = 0; pp < {ShapeNumBasisFun}; ++pp)
  {{
    for (int cc = 0; cc < {ShapeNumValPP}; ++cc)
    {{
      value[cc] += shapeVals[pp * {ShapeNumValPP} + cc] * basis[pp];
    }}
  }}
}}

// Fetch point coordinates and per-integration-point field values for
// the entire cell.
//
// NB: Currently, a cell-grid's shape attribute *must* be continuous
// (i.e., share degrees of freedom at cell boundaries). This makes
// fetching shapeValues much simpler than fetching colorValues.
//
// NB: This method expects `shape_conn` and `shape_vals` declared as samplerBuffer uniforms.
void shapeValuesForCell(in int cellId, out float shapeValues[{ShapeCoeffPerCell}])
{{
  for (int ii = 0; ii < {ShapeNumBasisFun}; ++ii)
  {{
    int vertexId = texelFetchBuffer(shape_conn, cellId * {ShapeNumBasisFun} + ii).s;
    for (int jj = 0; jj < {ShapeMultiplicity}; ++jj)
    {{
      shapeValues[ii * {ShapeMultiplicity} + jj] = texelFetchBuffer(shape_vals, vertexId * {ShapeMultiplicity} + jj).x;
    }}
  }}
}}
