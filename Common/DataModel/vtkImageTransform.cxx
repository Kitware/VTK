// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkImageTransform.h"

#include "vtkCellData.h"
#include "vtkDataArray.h"
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

template <typename T>
struct InPlaceTranslatePoints
{
  T* Points;
  const double* Translation;

  InPlaceTranslatePoints(const double t[3], T* pts)
    : Points(pts)
    , Translation(t)
  {
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    T* pIn = this->Points + 3 * ptId;
    T* pOut = pIn;

    for (; ptId < endPtId; ++ptId)
    {
      *pIn++ = *pOut++ + this->Translation[0];
      *pIn++ = *pOut++ + this->Translation[1];
      *pIn++ = *pOut++ + this->Translation[2];
    }
  }

  // Interface to vtkSMPTools
  static void Execute(const double t[3], vtkIdType num, T* pts)
  {
    InPlaceTranslatePoints<T> translate(t, pts);
    vtkSMPTools::For(0, num, translate);
  }
}; // InPlaceTransformPoints

template <typename T>
struct InPlaceTransformPoints
{
  T* Points;
  const double* M4;

  InPlaceTransformPoints(const double* m4, T* pts)
    : Points(pts)
    , M4(m4)
  {
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    T* pIn = this->Points + 3 * ptId;

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

  // Interface to vtkSMPTools
  static void Execute(const double* m4, vtkIdType num, T* pts)
  {
    InPlaceTransformPoints<T> transform(m4, pts);
    vtkSMPTools::For(0, num, transform);
  }
}; // InPlaceTransformPoints

template <typename T>
struct InPlaceTransformNormals
{
  T* Normals;
  const double* M3;

  InPlaceTransformNormals(const double* m3, T* n)
    : Normals(n)
    , M3(m3)
  {
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    T* nIn = this->Normals + 3 * ptId;
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

  // Interface to vtkSMPTools
  static void Execute(const double* m3, vtkIdType num, T* n)
  {
    InPlaceTransformNormals<T> transform(m3, n);
    vtkSMPTools::For(0, num, transform);
  }
}; // InPlaceTransformNormals

template <typename T>
struct InPlaceTransformVectors
{
  T* Vectors;
  const double* M3;

  InPlaceTransformVectors(const double* m3, T* v)
    : Vectors(v)
    , M3(m3)
  {
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    T* nIn = this->Vectors + 3 * ptId;

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

  // Interface to vtkSMPTools
  static void Execute(const double* m3, vtkIdType num, T* v)
  {
    InPlaceTransformVectors<T> transform(m3, v);
    vtkSMPTools::For(0, num, transform);
  }
}; // InPlaceTransformVectors

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
  void* pts = da->GetVoidPointer(0);
  vtkIdType num = da->GetNumberOfTuples();

  switch (da->GetDataType())
  {
    vtkTemplateMacro(InPlaceTranslatePoints<VTK_TT>::Execute(t, num, static_cast<VTK_TT*>(pts)));
  }
}

//------------------------------------------------------------------------------
void vtkImageTransform::TransformPoints(vtkMatrix4x4* m4, vtkDataArray* da)
{
  void* pts = da->GetVoidPointer(0);
  vtkIdType num = da->GetNumberOfTuples();
  const double* m4d = m4->GetData();

  switch (da->GetDataType())
  {
    vtkTemplateMacro(InPlaceTransformPoints<VTK_TT>::Execute(m4d, num, static_cast<VTK_TT*>(pts)));
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

  void* n = da->GetVoidPointer(0);
  vtkIdType num = da->GetNumberOfTuples();

  switch (da->GetDataType())
  {
    vtkTemplateMacro(InPlaceTransformNormals<VTK_TT>::Execute(m3n, num, static_cast<VTK_TT*>(n)));
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

  void* v = da->GetVoidPointer(0);
  vtkIdType num = da->GetNumberOfTuples();

  switch (da->GetDataType())
  {
    vtkTemplateMacro(InPlaceTransformVectors<VTK_TT>::Execute(m3v, num, static_cast<VTK_TT*>(v)));
  }
}

//------------------------------------------------------------------------------
void vtkImageTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
