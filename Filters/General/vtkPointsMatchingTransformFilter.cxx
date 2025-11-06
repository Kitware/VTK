// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPointsMatchingTransformFilter.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkPointSet.h"
#include "vtkSmartPointer.h"
#include "vtkSphericalPointIterator.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"

#include <array>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPointsMatchingTransformFilter);
vtkCxxSetSmartPointerMacro(vtkPointsMatchingTransformFilter, SourceMatrix, vtkMatrix4x4);
vtkCxxSetSmartPointerMacro(vtkPointsMatchingTransformFilter, TargetMatrix, vtkMatrix4x4);

namespace
{
//------------------------------------------------------------------------------
vtkSmartPointer<vtkMatrix3x3> ExtractRotationFromMatrix4x4(vtkMatrix4x4* M)
{
  vtkNew<vtkMatrix3x3> rotation;
  double data[9];
  for (int i = 0; i < 9; ++i)
  {
    data[i] = M->GetElement(i / 3, i % 3);
  }
  rotation->SetData(data);
  return rotation;
}

//------------------------------------------------------------------------------
void SetRotationInMatrix4x4(vtkMatrix3x3* source, vtkMatrix4x4* dest)
{
  for (int i = 0; i < 9; ++i)
  {
    dest->SetElement(i / 3, i % 3, source->GetElement(i / 3, i % 3));
  }
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkMatrix3x3> PolarDecomposition(vtkMatrix3x3* M)
{
  // Compute the SVD of the input
  double arrayM[3][3];
  for (int i = 0; i < 3; ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      arrayM[i][j] = M->GetElement(i, j);
    }
  }
  double U[3][3], Vt[3][3], S[3];
  vtkMath::SingularValueDecomposition3x3(arrayM, U, S, Vt);

  // Estimate the scale factor by averaging the singular values
  double scale = (S[0] + S[1] + S[2]) / 3.0;

  // Determine the polar matrix
  vtkNew<vtkMatrix3x3> polar;
  vtkNew<vtkMatrix3x3> matrixU;
  vtkNew<vtkMatrix3x3> matrixVt;
  matrixU->SetData(U[0]);
  matrixVt->SetData(Vt[0]);
  vtkMatrix3x3::Multiply3x3(matrixU, matrixVt, polar);
  for (int i = 0; i < 3; ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      polar->SetElement(i, j, scale * polar->GetElement(i, j));
    }
  }

  return polar;
}

} // anonymous namespace

//------------------------------------------------------------------------------
vtkPointsMatchingTransformFilter::vtkPointsMatchingTransformFilter()
{
  this->SourceMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  this->TargetMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  constexpr std::array<double, 16> init = { 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1 };
  this->SourceMatrix->SetData(init.data());
  this->TargetMatrix->SetData(init.data());
}

//------------------------------------------------------------------------------
int vtkPointsMatchingTransformFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkSmartPointer<vtkPointSet> input = vtkPointSet::GetData(inputVector[0]);
  vtkSmartPointer<vtkPointSet> output = vtkPointSet::GetData(outputVector);
  if (!input)
  {
    vtkErrorMacro(<< "Invalid or missing input");
    return 0;
  }

  // Build transform matrix
  vtkNew<vtkMatrix4x4> srcInv;
  srcInv->DeepCopy(this->SourceMatrix);
  if (std::abs(srcInv->Determinant()) < 10e-4)
  {
    vtkWarningMacro("Source matrix is not invertible. Source points are likely coplanar.");
    output->ShallowCopy(input);
    return 1;
  }
  srcInv->Invert();
  vtkNew<vtkMatrix4x4> transformMatrix;
  vtkMatrix4x4::Multiply4x4(this->TargetMatrix, srcInv, transformMatrix);

  if (this->RigidTransform)
  {
    vtkSmartPointer<vtkMatrix3x3> rotation = ::ExtractRotationFromMatrix4x4(transformMatrix);
    rotation = ::PolarDecomposition(rotation);
    ::SetRotationInMatrix4x4(rotation, transformMatrix);
  }

  // Apply the transform
  vtkNew<vtkTransform> transform;
  transform->SetMatrix(transformMatrix);
  vtkNew<vtkTransformFilter> transformFilter;
  transformFilter->SetInputData(input);
  transformFilter->SetTransform(transform);
  transformFilter->Update();

  output->ShallowCopy(transformFilter->GetOutput());
  return 1;
}

//------------------------------------------------------------------------------
void vtkPointsMatchingTransformFilter::SetSourcePoint1(double x, double y, double z)
{
  this->SetSourcePoint(0, x, y, z);
}

//------------------------------------------------------------------------------
void vtkPointsMatchingTransformFilter::SetSourcePoint2(double x, double y, double z)
{
  this->SetSourcePoint(1, x, y, z);
}

//------------------------------------------------------------------------------
void vtkPointsMatchingTransformFilter::SetSourcePoint3(double x, double y, double z)
{
  this->SetSourcePoint(2, x, y, z);
}

//------------------------------------------------------------------------------
void vtkPointsMatchingTransformFilter::SetSourcePoint4(double x, double y, double z)
{
  this->SetSourcePoint(3, x, y, z);
}

//------------------------------------------------------------------------------
void vtkPointsMatchingTransformFilter::SetTargetPoint1(double x, double y, double z)
{
  this->SetTargetPoint(0, x, y, z);
}

//------------------------------------------------------------------------------
void vtkPointsMatchingTransformFilter::SetTargetPoint2(double x, double y, double z)
{
  this->SetTargetPoint(1, x, y, z);
}

//------------------------------------------------------------------------------
void vtkPointsMatchingTransformFilter::SetTargetPoint3(double x, double y, double z)
{
  this->SetTargetPoint(2, x, y, z);
}

//------------------------------------------------------------------------------
void vtkPointsMatchingTransformFilter::SetTargetPoint4(double x, double y, double z)
{
  this->SetTargetPoint(3, x, y, z);
}

//------------------------------------------------------------------------------
void vtkPointsMatchingTransformFilter::SetSourcePoint(int index, double x, double y, double z)
{
  this->SourceMatrix->SetElement(0, index, x);
  this->SourceMatrix->SetElement(1, index, y);
  this->SourceMatrix->SetElement(2, index, z);
}

//------------------------------------------------------------------------------
void vtkPointsMatchingTransformFilter::SetTargetPoint(int index, double x, double y, double z)
{
  this->TargetMatrix->SetElement(0, index, x);
  this->TargetMatrix->SetElement(1, index, y);
  this->TargetMatrix->SetElement(2, index, z);
}

//------------------------------------------------------------------------------
vtkMTimeType vtkPointsMatchingTransformFilter::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  vtkMTimeType time;

  if (this->SourceMatrix != nullptr)
  {
    time = this->SourceMatrix->GetMTime();
    mTime = std::max(time, mTime);
  }
  if (this->TargetMatrix != nullptr)
  {
    time = this->TargetMatrix->GetMTime();
    mTime = std::max(time, mTime);
  }

  return mTime;
}

//------------------------------------------------------------------------------
void vtkPointsMatchingTransformFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Source Matrix: " << this->SourceMatrix << "\n";
  os << indent << "Target Matrix: " << this->SourceMatrix << "\n";
  os << indent << "Rigid Transform: " << this->RigidTransform << "\n";
}
VTK_ABI_NAMESPACE_END
