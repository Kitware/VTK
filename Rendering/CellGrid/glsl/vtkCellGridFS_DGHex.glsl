//VTK::System::Dec

//VTK::Define::Dec

// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Template for the cellgrid mapper fragment shader

// look up cell points, types and vertex positions from texture buffers.
uniform samplerBuffer fieldCoefficients;
uniform vec2 fieldRange;

// These two help debug parametric coordinates and basis function values.
uniform int visualizePCoord;
uniform int visualizeBasisFunction;
uniform int mapScalars;

//VTK::CustomUniforms::Dec

// Camera prop
//VTK::Camera::Dec

// Input parametric coord from geometry shader
in vec3 pCoordGSOutput;

// Input VTK cell ID from vertex shader
flat in int vtkCellIdGSOutput;

// optional color passed in from the vertex shader, vertexColor
//VTK::Color::Dec

// cell Normal used to light up and shade the pixels.
//VTK::Normal::Dec

// Lights
//VTK::Light::Dec

// the output of this shader
//VTK::Output::Dec

//----------------------------------------------------------------
/**
 * Evaluate a texture coordinate based on the field value at pos.
 *
 * coefficients - field coefficients [size = (numCells, 8)]
 * pos          - position of fragment in cell parametric space
 * cellId       - the index of the vtkCell that owns this fragment.
 *
 * Note: When visualizing basis functions, this function returns the
 * basis function value at pos.
 *
 */
vec2 ComputeScalarTexCoord_Hex_C1(in samplerBuffer coefficients, in vec3 pos, in int cellId)
{
  float coeff[8]; // coefficients for cellId'th cell.
  for (int i = 0; i < 8; ++i)
  {
    coeff[i] = texelFetch(coefficients, i + 8 * cellId).r;
  }
  float x = pos.x;
  float y = pos.y;
  float z = pos.z;

  // https://github.com/trilinos/Trilinos/blob/master/packages/intrepid/src/Discretization/Basis/Intrepid_HGRAD_HEX_C1_FEMDef.hpp
  // Evaluate basis functions.
  float basis[8];
  basis[0] = (1.0 - x)*(1.0 - y)*(1.0 - z)/8.0;
  basis[1] = (1.0 + x)*(1.0 - y)*(1.0 - z)/8.0;
  basis[2] = (1.0 + x)*(1.0 + y)*(1.0 - z)/8.0;
  basis[3] = (1.0 - x)*(1.0 + y)*(1.0 - z)/8.0;
  basis[4] = (1.0 - x)*(1.0 - y)*(1.0 + z)/8.0;
  basis[5] = (1.0 + x)*(1.0 - y)*(1.0 + z)/8.0;
  basis[6] = (1.0 + x)*(1.0 + y)*(1.0 + z)/8.0;
  basis[7] = (1.0 - x)*(1.0 + y)*(1.0 + z)/8.0;

  int visualizeBFuncId = visualizeBasisFunction >= 0 ? visualizeBasisFunction % 8 : -1;
  if (visualizeBFuncId >= 0)
  {
    return vec2(basis[visualizeBFuncId], 0.0f);
  }

  // Calculate field value from basis functions and the coefficients.
  float value = 0.0;
  for (int i = 0; i < 8; ++i)
  {
    value += basis[i] * coeff[i];
  }
  return vec2(value / (fieldRange.y - fieldRange.x), 0.0f);
}

//----------------------------------------------------------------
/**
 * Transform a value from bi-unit space to unit space
 * i.e, a -1....1 space to 0....1 space.
 */
float BiUnitToUnitDomain(in float value)
{
  return 0.5f * (1.0f + value);
}

//----------------------------------------------------------------
void main()
{
  vec2 texCoord;
  if (mapScalars != 0)
  {
    texCoord = ComputeScalarTexCoord_Hex_C1(fieldCoefficients, pCoordGSOutput, vtkCellIdGSOutput);
  }
  else
  {
    texCoord = vec2(0.5, 0);
  }

  // visualize the parametric coordinate if requested.
  int visualizePCoordId = visualizePCoord >= 0 ? visualizePCoord % 6 : -1;
  switch (visualizePCoordId)
  {
    case 0:
      texCoord = vec2(BiUnitToUnitDomain(pCoordGSOutput.x), 0);
      break;
    case 1:
      texCoord = vec2(BiUnitToUnitDomain(pCoordGSOutput.y), 0);
      break;
    case 2:
      texCoord = vec2(BiUnitToUnitDomain(pCoordGSOutput.z), 0);
      break;
    case 3:
      texCoord = vec2(BiUnitToUnitDomain(length(pCoordGSOutput.xy)), 0);
      break;
    case 4:
      texCoord = vec2(BiUnitToUnitDomain(length(pCoordGSOutput.yz)), 0);
      break;
    case 5:
      texCoord = vec2(BiUnitToUnitDomain(length(pCoordGSOutput.zx)), 0);
      break;
    default:
      break;
  }

  //VTK::Normal::Impl

  //VTK::Color::Impl

  //VTK::Light::Impl
}
