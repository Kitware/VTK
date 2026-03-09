// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkLight.h"
#include "vtkMathUtilities.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"

#include <array>
#include <cstdlib>

namespace
{

bool FuzzyEqual3(const double* a, const double* b)
{
  return vtkMathUtilities::FuzzyCompare(a[0], b[0]) && vtkMathUtilities::FuzzyCompare(a[1], b[1]) &&
    vtkMathUtilities::FuzzyCompare(a[2], b[2]);
}
}

int TestLightShallowClone(int, char*[])
{
  vtkNew<vtkLight> light;

  light->SetFocalPoint(1.1, 2.2, 3.3);
  light->SetPosition(4.4, 5.5, 6.6);
  light->SetAmbientColor(0.1, 0.2, 0.3);
  light->SetDiffuseColor(0.4, 0.5, 0.6);
  light->SetSpecularColor(0.7, 0.8, 0.9);
  light->SetAttenuationValues(0.3, 0.2, 0.1);
  light->SetIntensity(0.75);
  light->SetSwitch(0);
  light->SetPositional(1);
  light->SetExponent(12.5);
  light->SetConeAngle(67.0);
  light->SetLightTypeToCameraLight();
  light->SetShadowAttenuation(0.5f);

  vtkNew<vtkMatrix4x4> matrix;
  matrix->Identity();
  matrix->SetElement(0, 3, 10.0);
  matrix->SetElement(1, 3, -20.0);
  matrix->SetElement(2, 3, 30.0);
  light->SetTransformMatrix(matrix);

  std::array<double, 3> sourceTransformedPosition{};
  std::array<double, 3> sourceTransformedFocalPoint{};
  light->GetTransformedPosition(sourceTransformedPosition.data());
  light->GetTransformedFocalPoint(sourceTransformedFocalPoint.data());

  vtkLight* clone = light->ShallowClone();

  std::array<double, 3> cloneTransformedPosition{};
  std::array<double, 3> cloneTransformedFocalPoint{};
  clone->GetTransformedPosition(cloneTransformedPosition.data());
  clone->GetTransformedFocalPoint(cloneTransformedFocalPoint.data());

  const bool matches = FuzzyEqual3(clone->GetFocalPoint(), light->GetFocalPoint()) &&
    FuzzyEqual3(clone->GetPosition(), light->GetPosition()) &&
    FuzzyEqual3(clone->GetAmbientColor(), light->GetAmbientColor()) &&
    FuzzyEqual3(clone->GetDiffuseColor(), light->GetDiffuseColor()) &&
    FuzzyEqual3(clone->GetSpecularColor(), light->GetSpecularColor()) &&
    FuzzyEqual3(clone->GetAttenuationValues(), light->GetAttenuationValues()) &&
    vtkMathUtilities::FuzzyCompare(clone->GetIntensity(), light->GetIntensity()) &&
    clone->GetSwitch() == light->GetSwitch() && clone->GetPositional() == light->GetPositional() &&
    vtkMathUtilities::FuzzyCompare(clone->GetExponent(), light->GetExponent()) &&
    vtkMathUtilities::FuzzyCompare(clone->GetConeAngle(), light->GetConeAngle()) &&
    clone->GetLightType() == light->GetLightType() &&
    vtkMathUtilities::FuzzyCompare(clone->GetShadowAttenuation(), light->GetShadowAttenuation()) &&
    clone->GetTransformMatrix() == light->GetTransformMatrix() &&
    FuzzyEqual3(cloneTransformedPosition.data(), sourceTransformedPosition.data()) &&
    FuzzyEqual3(cloneTransformedFocalPoint.data(), sourceTransformedFocalPoint.data());

  clone->Delete();

  return matches ? EXIT_SUCCESS : EXIT_FAILURE;
}
