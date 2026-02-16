// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkImageTransform.h"

#include "vtkArrayDispatch.h"
#include "vtkArrayDispatchDataSetArrayList.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkSMPTools.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkImageTransform);

//============================================================================
//------------------------------------------------------------------------------
// Functors to support threaded execution
namespace
{ // anonymous

template <typename TArray, typename T = vtk::GetAPIType<TArray>>
struct InPlaceTranslatePointsFunctor
{
  TArray* Points;
  const double* Translation;

  InPlaceTranslatePointsFunctor(const double t[3], TArray* pts)
    : Points(pts)
    , Translation(t)
  {
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    auto pIn = vtk::DataArrayValueRange(this->Points, 3 * ptId).begin();

    for (; ptId < endPtId; ++ptId)
    {
      *pIn++ += this->Translation[0];
      *pIn++ += this->Translation[1];
      *pIn++ += this->Translation[2];
    }
  }
}; // InPlaceTransformPoints

struct InPlaceTranslatePointsWorker
{
  template <class TArray>
  void operator()(TArray* normals, const double* m3)
  {
    InPlaceTranslatePointsFunctor<TArray> transform(m3, normals);
    vtkSMPTools::For(0, normals->GetNumberOfTuples(), transform);
  }
};

template <typename TArray, typename T = vtk::GetAPIType<TArray>>
struct InPlaceTransformPointsFunctor
{
  TArray* Points;
  const double* M4;

  InPlaceTransformPointsFunctor(const double* m4, TArray* pts)
    : Points(pts)
    , M4(m4)
  {
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    auto pIn = vtk::DataArrayValueRange(this->Points, 3 * ptId).begin();

    for (; ptId < endPtId; ++ptId)
    {
      double x = M4[0] * pIn[0] + M4[1] * pIn[1] + M4[2] * pIn[2] + M4[3];
      double y = M4[4] * pIn[0] + M4[5] * pIn[1] + M4[6] * pIn[2] + M4[7];
      double z = M4[8] * pIn[0] + M4[9] * pIn[1] + M4[10] * pIn[2] + M4[11];
      *pIn++ = static_cast<T>(x);
      *pIn++ = static_cast<T>(y);
      *pIn++ = static_cast<T>(z);
    }
  }
}; // InPlaceTransformPointsFunctor

struct InPlaceTransformPointsWorker
{
  template <class TArray>
  void operator()(TArray* points, const double* m4d)
  {
    InPlaceTransformPointsFunctor<TArray> transform(m4d, points);
    vtkSMPTools::For(0, points->GetNumberOfTuples(), transform);
  }
};

template <typename TArray, typename T = vtk::GetAPIType<TArray>>
struct InPlaceTransformNormalsFunctor
{
  TArray* Normals;
  const double* M3;

  InPlaceTransformNormalsFunctor(const double* m3, TArray* n)
    : Normals(n)
    , M3(m3)
  {
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    auto nIn = vtk::DataArrayValueRange(this->Normals, 3 * ptId).begin();
    double vec[3];

    for (; ptId < endPtId; ++ptId)
    {
      vec[0] = M3[0] * nIn[0] + M3[1] * nIn[1] + M3[2] * nIn[2];
      vec[1] = M3[3] * nIn[0] + M3[4] * nIn[1] + M3[5] * nIn[2];
      vec[2] = M3[6] * nIn[0] + M3[7] * nIn[1] + M3[8] * nIn[2];
      vtkMath::Normalize(vec);
      *nIn++ = static_cast<T>(vec[0]);
      *nIn++ = static_cast<T>(vec[1]);
      *nIn++ = static_cast<T>(vec[2]);
    }
  }
}; // InPlaceTransformNormals

struct InPlaceTransformNormalsWorker
{
  template <class TArray>
  void operator()(TArray* normals, const double* m3)
  {
    InPlaceTransformNormalsFunctor<TArray> transform(m3, normals);
    vtkSMPTools::For(0, normals->GetNumberOfTuples(), transform);
  }
};

template <typename TArray, typename T = vtk::GetAPIType<TArray>>
struct InPlaceTransformVectorsFunctor
{
  TArray* Vectors;
  const double* M3;

  InPlaceTransformVectorsFunctor(const double* m3, TArray* v)
    : Vectors(v)
    , M3(m3)
  {
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    auto nIn = vtk::DataArrayValueRange(this->Vectors, 3 * ptId).begin();

    for (; ptId < endPtId; ++ptId)
    {
      double x = M3[0] * nIn[0] + M3[1] * nIn[1] + M3[2] * nIn[2];
      double y = M3[3] * nIn[0] + M3[4] * nIn[1] + M3[5] * nIn[2];
      double z = M3[6] * nIn[0] + M3[7] * nIn[1] + M3[8] * nIn[2];
      *nIn++ = static_cast<T>(x);
      *nIn++ = static_cast<T>(y);
      *nIn++ = static_cast<T>(z);
    }
  }
}; // InPlaceTransformVectors

struct InPlaceTransformVectorsWorker
{
  template <class TArray>
  void operator()(TArray* vectors, const double* m3)
  {
    InPlaceTransformVectorsFunctor<TArray> transform(m3, vectors);
    vtkSMPTools::For(0, vectors->GetNumberOfTuples(), transform);
  }
};

} // anonymous namespace

//============================================================================
//------------------------------------------------------------------------------
// Here is the VTK class proper.
//------------------------------------------------------------------------------
// A convenience function to transform points (in the point set) as well as
// associated normals and vectors.
void vtkImageTransform::TransformPointSet(vtkImageData* im, vtkPointSet* ps)
{
  vtkImageTransform::TransformPointSet(im, ps, true, true);
}

//------------------------------------------------------------------------------
// A convenience method to transform a point set, with the ability to control
// whether normals and vectors are transformed as well.
void vtkImageTransform::TransformPointSet(
  vtkImageData* im, vtkPointSet* ps, bool transformNormals, bool transformVectors)
{
  // Check input
  if (im == nullptr || ps == nullptr)
  {
    return;
  }

  // Nothing to do if the direction matrix is the identity
  vtkMatrix4x4* m4 = im->GetIndexToPhysicalMatrix();
  if (m4->IsIdentity())
  {
    return;
  }

  // Make sure points are available
  vtkIdType numPts = ps->GetNumberOfPoints();
  if (numPts < 1)
  {
    return;
  }

  // Grab the points-related-data and process as appropriate
  vtkDataArray* pts = ps->GetPoints()->GetData();
  vtkMatrix3x3* m3 = im->GetDirectionMatrix();
  const double* ar = im->GetSpacing();

  // If there is no rotation or spacing, only translate
  if (m3->IsIdentity() && ar[0] == 1.0 && ar[1] == 1.0 && ar[2] == 1.0)
  {
    vtkImageTransform::TranslatePoints(im->GetOrigin(), pts);
    return;
  }

  // Otherwise, need to transform points and optionally
  // vectors and normals.
  vtkImageTransform::TransformPoints(m4, pts);

  if (transformNormals)
  {
    vtkDataArray* normals = ps->GetPointData()->GetNormals();
    if (normals != nullptr)
    {
      vtkImageTransform::TransformNormals(m3, ar, normals);
    }
    normals = ps->GetCellData()->GetNormals();
    if (normals != nullptr)
    {
      vtkImageTransform::TransformNormals(m3, ar, normals);
    }
  }

  if (transformVectors)
  {
    vtkDataArray* vectors = ps->GetPointData()->GetVectors();
    if (vectors != nullptr)
    {
      vtkImageTransform::TransformVectors(m3, ar, vectors);
    }
    vectors = ps->GetCellData()->GetVectors();
    if (vectors != nullptr)
    {
      vtkImageTransform::TransformVectors(m3, ar, vectors);
    }
  }
}

//------------------------------------------------------------------------------
void vtkImageTransform::TranslatePoints(const double t[3], vtkDataArray* da)
{
  InPlaceTranslatePointsWorker worker;
  if (!vtkArrayDispatch::DispatchByArray<vtkArrayDispatch::PointArrays>::Execute(da, worker, t))
  {
    worker(da, t);
  }
}

//------------------------------------------------------------------------------
void vtkImageTransform::TransformPoints(vtkMatrix4x4* m4, vtkDataArray* da)
{
  const double* m4d = m4->GetData();
  InPlaceTransformPointsWorker worker;
  if (!vtkArrayDispatch::DispatchByArray<vtkArrayDispatch::PointArrays>::Execute(da, worker, m4d))
  {
    worker(da, m4d);
  }
}

//------------------------------------------------------------------------------
void vtkImageTransform::TransformNormals(
  vtkMatrix3x3* m3, const double spacing[3], vtkDataArray* da)
{
  // The determinant of the image Direction is 1 or -1, we use it to flip
  // the normals to the expected orientation for proper visualization
  double determinant = m3->Determinant();
  double m3n[9];
  vtkMatrix3x3::Invert(m3->GetData(), m3n);
  vtkMatrix3x3::Transpose(m3n, m3n);

  for (int i = 0; i < 3; ++i)
  {
    if (spacing[i] != 0.0)
    {
      m3n[i] = m3n[i] / spacing[i] * determinant;
      m3n[3 + i] = m3n[3 + i] / spacing[i] * determinant;
      m3n[6 + i] = m3n[6 + i] / spacing[i] * determinant;
    }
    else
    {
      m3n[i] = 0.0;
      m3n[3 + i] = 0.0;
      m3n[6 + i] = 0.0;
    }
  }

  InPlaceTransformNormalsWorker worker;
  if (!vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals>::Execute(da, worker, m3n))
  {
    worker(da, m3n);
  }
}

//------------------------------------------------------------------------------
void vtkImageTransform::TransformVectors(
  vtkMatrix3x3* m3, const double spacing[3], vtkDataArray* da)
{
  // Here we assume that the vectors are gradient vectors, therefore
  // the transposed inverse matrix is used to apply the transformation
  double m3v[9];
  vtkMatrix3x3::Invert(m3->GetData(), m3v);
  vtkMatrix3x3::Transpose(m3v, m3v);

  for (int i = 0; i < 3; ++i)
  {
    if (spacing[i] != 0.0)
    {
      m3v[i] = m3v[i] / spacing[i];
      m3v[3 + i] = m3v[3 + i] / spacing[i];
      m3v[6 + i] = m3v[6 + i] / spacing[i];
    }
    else
    {
      m3v[i] = 0.0;
      m3v[3 + i] = 0.0;
      m3v[6 + i] = 0.0;
    }
  }

  InPlaceTransformVectorsWorker worker;
  if (!vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals>::Execute(da, worker, m3v))
  {
    worker(da, m3v);
  }
}

//------------------------------------------------------------------------------
void vtkImageTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
