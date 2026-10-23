// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Checks vtkLiutex against closed-form values. The velocity fields are linear,
// so the gradient computed on image data is exact.

#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkLiutex.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStringFormatter.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

namespace
{
using Matrix = double[3][3];

double Tolerance = 1e-6;

vtkSmartPointer<vtkImageData> MakeLinearField(const Matrix j, bool asFloat = false)
{
  auto image = vtkSmartPointer<vtkImageData>::New();
  image->SetDimensions(5, 5, 5);
  image->SetOrigin(-0.5, -0.5, -0.5);
  image->SetSpacing(0.25, 0.25, 0.25);

  vtkSmartPointer<vtkDataArray> velocity;
  if (asFloat)
  {
    velocity = vtkSmartPointer<vtkFloatArray>::New();
  }
  else
  {
    velocity = vtkSmartPointer<vtkDoubleArray>::New();
  }
  velocity->SetName("velocity");
  velocity->SetNumberOfComponents(3);
  velocity->SetNumberOfTuples(image->GetNumberOfPoints());

  vtkNew<vtkDoubleArray> gradient;
  gradient->SetName("gradient");
  gradient->SetNumberOfComponents(9);
  gradient->SetNumberOfTuples(image->GetNumberOfPoints());

  for (vtkIdType id = 0; id < image->GetNumberOfPoints(); ++id)
  {
    double x[3];
    image->GetPoint(id, x);
    for (int i = 0; i < 3; ++i)
    {
      velocity->SetComponent(id, i, j[i][0] * x[0] + j[i][1] * x[1] + j[i][2] * x[2]);
      for (int k = 0; k < 3; ++k)
      {
        gradient->SetComponent(id, 3 * i + k, j[i][k]);
      }
    }
  }
  image->GetPointData()->AddArray(velocity);
  image->GetPointData()->AddArray(gradient);
  return image;
}

// Rotation by angle about a unit axis (Rodrigues' formula).
void Rotation(const double axis[3], double angle, Matrix q)
{
  const double c = std::cos(angle);
  const double s = std::sin(angle);
  const double k[3][3] = { { 0., -axis[2], axis[1] }, { axis[2], 0., -axis[0] },
    { -axis[1], axis[0], 0. } };
  for (int i = 0; i < 3; ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      double kk = 0.;
      for (int m = 0; m < 3; ++m)
      {
        kk += k[i][m] * k[m][j];
      }
      q[i][j] = (i == j ? 1. : 0.) + s * k[i][j] + (1. - c) * kk;
    }
  }
}

bool CheckScalar(vtkDataArray* array, double expected, const std::string& label)
{
  if (!array)
  {
    std::cerr << label << ": missing array" << std::endl;
    return false;
  }
  for (vtkIdType i = 0; i < array->GetNumberOfTuples(); ++i)
  {
    if (std::abs(array->GetComponent(i, 0) - expected) >
      Tolerance * std::max(1., std::abs(expected)))
    {
      std::cerr << label << ": value " << array->GetComponent(i, 0) << " at " << i << ", expected "
                << expected << std::endl;
      return false;
    }
  }
  return true;
}

bool CheckVector(vtkDataArray* array, const double expected[3], const std::string& label)
{
  if (!array || array->GetNumberOfComponents() != 3)
  {
    std::cerr << label << ": missing array" << std::endl;
    return false;
  }
  for (vtkIdType i = 0; i < array->GetNumberOfTuples(); ++i)
  {
    for (int c = 0; c < 3; ++c)
    {
      if (std::abs(array->GetComponent(i, c) - expected[c]) > Tolerance)
      {
        std::cerr << label << ": component " << c << " is " << array->GetComponent(i, c) << " at "
                  << i << ", expected " << expected[c] << std::endl;
        return false;
      }
    }
  }
  return true;
}

// J0 = [[a, g - w, 0], [w, a, 0], [0, 0, -2a]]: rotation w about z, shear g and
// axial stretching. Its eigenvalues are a +/- i sqrt(w (w - g)) and -2a, the
// vorticity along z is 2w - g, so R = (2w - g) - |g| = 2 (w - g) for 0 < g < w.
// The field is rotated by Q, which rotates the Liutex vector to Q (0, 0, R).
bool TestRotatedField(bool useGradientInput, bool asFloat)
{
  const double w = 2., g = 0.5, a = 0.2;
  const Matrix j0 = { { a, g - w, 0. }, { w, a, 0. }, { 0., 0., -2. * a } };
  const double axis[3] = { 1. / std::sqrt(14.), 2. / std::sqrt(14.), 3. / std::sqrt(14.) };
  Matrix q;
  Rotation(axis, 0.7, q);
  Matrix j;
  for (int r = 0; r < 3; ++r)
  {
    for (int c = 0; c < 3; ++c)
    {
      j[r][c] = 0.;
      for (int m = 0; m < 3; ++m)
      {
        for (int n = 0; n < 3; ++n)
        {
          j[r][c] += q[r][m] * j0[m][n] * q[c][n];
        }
      }
    }
  }

  Tolerance = asFloat ? 1e-4 : 1e-6;
  auto image = MakeLinearField(j, asFloat);
  vtkNew<vtkLiutex> liutex;
  liutex->SetInputData(image);
  liutex->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, useGradientInput ? "gradient" : "velocity");
  liutex->ComputeOmegaLiutexOn();
  liutex->Update();
  vtkPointData* pd = vtkDataSet::SafeDownCast(liutex->GetOutput())->GetPointData();

  const std::string label = std::string("Rotated field") +
    (useGradientInput ? " from gradient" : " from velocity") + (asFloat ? " (float)" : "");

  const double magnitude = 2. * (w - g);
  const double expected[3] = { q[0][2] * magnitude, q[1][2] * magnitude, q[2][2] * magnitude };

  const double lambdaCi2 = w * (w - g);
  const double beta2 = 0.25 * (2. * w - g) * (2. * w - g);
  const double alpha2 = beta2 - lambdaCi2;
  const double omega = beta2 / (beta2 + alpha2 + a * a + 0.5 * 4. * a * a + 0.001 * lambdaCi2);

  if (asFloat && !vtkFloatArray::SafeDownCast(pd->GetArray("Liutex")))
  {
    std::cerr << label << ": a float velocity should give float output" << std::endl;
    return false;
  }
  return CheckVector(pd->GetArray("Liutex"), expected, label + " Liutex") &&
    CheckScalar(pd->GetArray("LiutexMagnitude"), magnitude, label + " LiutexMagnitude") &&
    CheckScalar(pd->GetArray("OmegaLiutex"), omega, label + " OmegaLiutex");
}

// Pure shear has vorticity but no local rotation.
bool TestPureShear()
{
  Tolerance = 1e-6;
  const Matrix j = { { 0., 1., 0. }, { 0., 0., 0. }, { 0., 0., 0. } };
  vtkNew<vtkLiutex> liutex;
  liutex->SetInputData(MakeLinearField(j));
  liutex->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "velocity");
  liutex->ComputeOmegaLiutexOn();
  liutex->Update();
  vtkPointData* pd = vtkDataSet::SafeDownCast(liutex->GetOutput())->GetPointData();
  const double zero[3] = { 0., 0., 0. };
  return CheckVector(pd->GetArray("Liutex"), zero, "Pure shear Liutex") &&
    CheckScalar(pd->GetArray("OmegaLiutex"), 0., "Pure shear OmegaLiutex");
}

// Two blocks in rigid rotation at rates 1 and 2. Liutex equals vorticity, and
// epsilon comes from the larger rate in either block: Omega_R = w^2 / (w^2 + 0.004).
bool TestComposite()
{
  Tolerance = 1e-6;
  vtkNew<vtkMultiBlockDataSet> blocks;
  blocks->SetNumberOfBlocks(2);
  for (int b = 0; b < 2; ++b)
  {
    const double w = b + 1.;
    const Matrix j = { { 0., -w, 0. }, { w, 0., 0. }, { 0., 0., 0. } };
    blocks->SetBlock(b, MakeLinearField(j));
  }

  vtkNew<vtkLiutex> liutex;
  liutex->SetInputData(blocks);
  liutex->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "velocity");
  liutex->ComputeOmegaLiutexOn();
  liutex->ComputeLiutexVectorOff();
  liutex->Update();

  auto output = vtkMultiBlockDataSet::SafeDownCast(liutex->GetOutputDataObject(0));
  if (!output || output->GetNumberOfBlocks() != 2)
  {
    std::cerr << "Composite: expected a multiblock output with two blocks" << std::endl;
    return false;
  }
  for (int b = 0; b < 2; ++b)
  {
    const double w = b + 1.;
    vtkPointData* pd = vtkImageData::SafeDownCast(output->GetBlock(b))->GetPointData();
    const std::string label = "Composite block " + vtk::to_string(b);
    if (pd->GetArray("Liutex"))
    {
      std::cerr << label << ": Liutex vector was not requested" << std::endl;
      return false;
    }
    if (!CheckScalar(pd->GetArray("LiutexMagnitude"), 2. * w, label + " LiutexMagnitude") ||
      !CheckScalar(pd->GetArray("OmegaLiutex"), w * w / (w * w + 0.004), label + " OmegaLiutex"))
    {
      return false;
    }
  }
  if (vtkImageData::SafeDownCast(blocks->GetBlock(0))->GetPointData()->GetArray("OmegaLiutex"))
  {
    std::cerr << "Composite: the input was modified" << std::endl;
    return false;
  }
  return true;
}

// Lamb-Oseen (strain 0) and Burgers (strain a) vortices about the z axis, with
// swirl u_theta = (1 - exp(-r^2 / rc^2)) / r and, for Burgers, the strain field
// u_r = -a r / 2, u_z = a z. With a = du_theta/dr and b = u_theta / r the
// in-plane eigenvalues are -a/2 +/- i sqrt(a b) when a b > 0, the vorticity is
// a + b, and R = 2 min(a, b), which is 2 du_theta/dr inside the core and zero
// where du_theta/dr <= 0. The axial eigenvalue is the strain, so Burgers also
// checks the lambda_r and lambda_cr terms of Omega-Liutex.
bool TestAxisymmetricVortex(double strain, const std::string& label)
{
  constexpr double coreRadius = 0.5;
  constexpr double epsilon = 0.016;
  constexpr double centerX = 0.013;
  constexpr double centerY = -0.007;
  constexpr double spacing = 0.02;

  vtkNew<vtkImageData> image;
  image->SetDimensions(101, 101, 5);
  image->SetOrigin(-1., -1., 0.);
  image->SetSpacing(spacing, spacing, spacing);

  auto swirl = [&](double r2, double& uThetaOverR, double& duThetaDr)
  {
    const double e = std::exp(-r2 / (coreRadius * coreRadius));
    if (r2 < 1e-12)
    {
      uThetaOverR = duThetaDr = 1. / (coreRadius * coreRadius);
      return;
    }
    uThetaOverR = (1. - e) / r2;
    duThetaDr = 2. * e / (coreRadius * coreRadius) - uThetaOverR;
  };

  vtkNew<vtkDoubleArray> velocity;
  velocity->SetName("velocity");
  velocity->SetNumberOfComponents(3);
  velocity->SetNumberOfTuples(image->GetNumberOfPoints());
  for (vtkIdType id = 0; id < image->GetNumberOfPoints(); ++id)
  {
    double p[3];
    image->GetPoint(id, p);
    const double dx = p[0] - centerX;
    const double dy = p[1] - centerY;
    double b, a;
    swirl(dx * dx + dy * dy, b, a);
    velocity->SetTuple3(id, -b * dy - 0.5 * strain * dx, b * dx - 0.5 * strain * dy, strain * p[2]);
  }
  image->GetPointData()->AddArray(velocity);

  vtkNew<vtkLiutex> liutex;
  liutex->SetInputData(image);
  liutex->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "velocity");
  liutex->ComputeOmegaLiutexOn();
  liutex->SetOmegaLiutexEpsilon(epsilon);
  liutex->Update();
  vtkPointData* pd = vtkDataSet::SafeDownCast(liutex->GetOutput())->GetPointData();
  vtkDataArray* vectors = pd->GetArray("Liutex");
  vtkDataArray* magnitudes = pd->GetArray("LiutexMagnitude");
  vtkDataArray* omegas = pd->GetArray("OmegaLiutex");

  // The gradient is a central difference, so compare at interior points with a
  // tolerance relative to R on the axis, 2 / rc^2.
  const double tolerance = 1e-3 * 2. / (coreRadius * coreRadius);
  double maxError = 0.;
  double maxOmegaError = 0.;
  for (vtkIdType id = 0; id < image->GetNumberOfPoints(); ++id)
  {
    double p[3];
    image->GetPoint(id, p);
    if (std::abs(p[0]) > 0.9 || std::abs(p[1]) > 0.9 || p[2] < 0.5 * spacing ||
      p[2] > 3.5 * spacing)
    {
      continue;
    }
    const double dx = p[0] - centerX;
    const double dy = p[1] - centerY;
    double b, a;
    swirl(dx * dx + dy * dy, b, a);
    const bool rotating = a > 0. && b > 0.;
    const double magnitude = rotating ? 2. * std::min(a, b) : 0.;

    double omega = 0.;
    if (rotating)
    {
      const double beta2 = 0.25 * (a + b) * (a + b);
      const double alpha2 = beta2 - a * b;
      const double lambdaCr = -0.5 * strain;
      omega = beta2 / (beta2 + alpha2 + lambdaCr * lambdaCr + 0.5 * strain * strain + epsilon);
    }

    const double* vector = vectors->GetTuple3(id);
    maxError = std::max({ maxError, std::abs(magnitudes->GetComponent(id, 0) - magnitude),
      std::abs(vector[0]), std::abs(vector[1]), std::abs(vector[2] - magnitude) });
    // Omega-Liutex jumps from about 1/2 to 0 where du_theta/dr changes sign, so
    // points within the gradient error of that edge can fall on either side.
    if (std::abs(a) > 10. * tolerance)
    {
      maxOmegaError = std::max(maxOmegaError, std::abs(omegas->GetComponent(id, 0) - omega));
    }
  }

  if (maxError > tolerance || maxOmegaError > 1e-2)
  {
    std::cerr << label << ": largest Liutex error " << maxError << " (tolerance " << tolerance
              << "), largest Omega-Liutex error " << maxOmegaError << " (tolerance 0.01)"
              << std::endl;
    return false;
  }
  return true;
}
}

int TestLiutex(int, char*[])
{
  bool ok = TestRotatedField(false, false);
  ok = TestRotatedField(true, false) && ok;
  ok = TestRotatedField(false, true) && ok;
  ok = TestPureShear() && ok;
  ok = TestComposite() && ok;
  ok = TestAxisymmetricVortex(0., "Lamb-Oseen") && ok;
  ok = TestAxisymmetricVortex(4., "Burgers") && ok;
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
