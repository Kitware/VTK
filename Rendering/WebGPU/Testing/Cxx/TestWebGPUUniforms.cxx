// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkTestErrorObserver.h"
#include "vtkWebGPUUniforms.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace
{
bool Check(bool condition, const char* message)
{
  if (!condition)
  {
    std::cerr << "FAILED: " << message << "\n";
  }
  return condition;
}
}

int TestWebGPUUniforms(int, char*[])
{
  bool success = true;
  vtkNew<vtkWebGPUUniforms> uniforms;

  uniforms->SetUniformf("strength", 0.5f);
  const float tint[3] = { 1.f, 0.5f, 0.25f };
  uniforms->SetUniform3f("tint", tint);
  float strength = 0.f;
  float readTint[3] = {};
  success &=
    Check(uniforms->GetUniformf("strength", strength) && strength == 0.5f, "a scalar reads back");
  success &=
    Check(uniforms->GetUniform3f("tint", readTint) && readTint[1] == 0.5f, "a vector reads back");
  success &= Check(uniforms->GetNumberOfUniforms() == 2 &&
      std::string(uniforms->GetNthUniformName(0)) == "strength" &&
      std::string(uniforms->GetNthUniformName(1)) == "tint",
    "uniforms are listed by name");

  const vtkMTimeType listTime = uniforms->GetUniformListMTime();
  uniforms->SetUniformf("strength", 0.75f);
  success &= Check(uniforms->GetUniformListMTime() == listTime,
    "changing a value does not change the uniform list");
  uniforms->SetUniformi("count", 3);
  success &= Check(uniforms->GetUniformListMTime() > listTime, "adding a uniform does");

  const float values[2] = { 1.f, 2.f };
  uniforms->SetUniform1fv("weights", 2, values);
  success &= Check(uniforms->GetUniformNumberOfTuples("weights") == 2 &&
      uniforms->GetUniformNumberOfTuples("strength") == 1,
    "arrays keep their number of tuples");

  const unsigned char color[3] = { 255, 0, 51 };
  uniforms->SetUniform3uc("color", color);
  float readColor[3] = {};
  success &=
    Check(uniforms->GetUniform3f("color", readColor) && readColor[0] == 1.f && readColor[2] == 0.2f,
      "unsigned char colors are normalized");

  vtkNew<vtkMatrix4x4> matrix;
  matrix->SetElement(0, 1, 2.0);
  uniforms->SetUniformMatrix("model", matrix);
  float readMatrix[16] = {};
  success &= Check(uniforms->GetUniformMatrix4x4("model", readMatrix) && readMatrix[1] == 2.f &&
      readMatrix[4] == 0.f,
    "matrices are stored row by row");

  vtkNew<vtkTest::ErrorObserver> errors;
  uniforms->AddObserver(vtkCommand::ErrorEvent, errors);
  uniforms->SetUniformi("strength", 1);
  success &= Check(errors->GetError() && uniforms->GetUniformScalarType("strength") == VTK_FLOAT,
    "a uniform keeps its type");

  uniforms->RemoveUniform("count");
  int count = 0;
  success &= Check(!uniforms->GetUniformi("count", count), "a removed uniform is gone");
  uniforms->RemoveAllUniforms();
  success &= Check(uniforms->GetNumberOfUniforms() == 0, "all uniforms can be removed");

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
