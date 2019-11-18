/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageTransform.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageTransform.h"

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkSMPTools.h"

vtkStandardNewMacro(vtkImageTransform);

//============================================================================
//----------------------------------------------------------------------------
// Functors to support threaded execution
namespace
{ // anonymous

template <typename T>
struct InPlaceTranslatePoints
{
  T* Points;
  double* Translation;

  InPlaceTranslatePoints(double t[3], T* pts)
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
  static void Execute(double t[3], vtkIdType num, T* pts)
  {
    InPlaceTranslatePoints<T> translate(t, pts);
    vtkSMPTools::For(0, num, translate);
  }
}; // InPlaceTransformPoints

template <typename T>
struct InPlaceTransformPoints
{
  T* Points;
  vtkMatrix4x4* M4;

  InPlaceTransformPoints(vtkMatrix4x4* m4, T* pts)
    : Points(pts)
    , M4(m4)
  {
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    T* pIn = this->Points + 3 * ptId;
    T tmp[3] = { 0, 0, 0 };

    for (; ptId < endPtId; ++ptId)
    {
      tmp[0] = M4->GetElement(0, 0) * pIn[0] + M4->GetElement(0, 1) * pIn[1] +
        M4->GetElement(0, 2) * pIn[2] + M4->GetElement(0, 3);
      tmp[1] = M4->GetElement(1, 0) * pIn[0] + M4->GetElement(1, 1) * pIn[1] +
        M4->GetElement(1, 2) * pIn[2] + M4->GetElement(1, 3);
      tmp[2] = M4->GetElement(2, 0) * pIn[0] + M4->GetElement(2, 1) * pIn[1] +
        M4->GetElement(2, 2) * pIn[2] + M4->GetElement(2, 3);
      *pIn++ = tmp[0];
      *pIn++ = tmp[1];
      *pIn++ = tmp[2];
    }
  }

  // Interface to vtkSMPTools
  static void Execute(vtkMatrix4x4* m4, vtkIdType num, T* pts)
  {
    InPlaceTransformPoints<T> transform(m4, pts);
    vtkSMPTools::For(0, num, transform);
  }
}; // InPlaceTransformPoints

template <typename T>
struct InPlaceTransformNormals
{
  T* Normals;
  vtkMatrix3x3* M3;
  double Determinant;
  double* Spacing;

  InPlaceTransformNormals(vtkMatrix3x3* m3, double* spacing, T* n)
    : Normals(n)
    , M3(m3)
    , Determinant(m3->Determinant())
    , Spacing(spacing)
  {
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    T* nIn = this->Normals + 3 * ptId;
    T tmp[3] = { 0, 0, 0 };
    T toUnit = 0;

    for (; ptId < endPtId; ++ptId)
    {
      nIn[0] = nIn[0] / this->Spacing[0];
      nIn[1] = nIn[1] / this->Spacing[1];
      nIn[2] = nIn[2] / this->Spacing[2];
      tmp[0] = M3->GetElement(0, 0) * nIn[0] + M3->GetElement(0, 1) * nIn[1] +
        M3->GetElement(0, 2) * nIn[2];
      tmp[1] = M3->GetElement(1, 0) * nIn[0] + M3->GetElement(1, 1) * nIn[1] +
        M3->GetElement(1, 2) * nIn[2];
      tmp[2] = M3->GetElement(2, 0) * nIn[0] + M3->GetElement(2, 1) * nIn[1] +
        M3->GetElement(2, 2) * nIn[2];
      tmp[0] *= this->Determinant;
      tmp[1] *= this->Determinant;
      tmp[2] *= this->Determinant;
      toUnit = 1 / sqrt(tmp[0] * tmp[0] + tmp[1] * tmp[1] + tmp[2] * tmp[2]);
      *nIn++ = tmp[0] * toUnit;
      *nIn++ = tmp[1] * toUnit;
      *nIn++ = tmp[2] * toUnit;
    }
  }

  // Interface to vtkSMPTools
  static void Execute(vtkMatrix3x3* m3, double* spacing, vtkIdType num, T* n)
  {
    InPlaceTransformNormals<T> transform(m3, spacing, n);
    vtkSMPTools::For(0, num, transform);
  }
}; // InPlaceTransformNormals

template <typename T>
struct InPlaceTransformVectors
{
  T* Vectors;
  vtkMatrix3x3* M3;
  double* Spacing;

  InPlaceTransformVectors(vtkMatrix3x3* m3, double* spacing, T* v)
    : Vectors(v)
    , M3(m3)
    , Spacing(spacing)
  {
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    T* nIn = this->Vectors + 3 * ptId;
    T tmp[3] = { 0, 0, 0 };

    for (; ptId < endPtId; ++ptId)
    {
      nIn[0] = nIn[0] / this->Spacing[0];
      nIn[1] = nIn[1] / this->Spacing[1];
      nIn[2] = nIn[2] / this->Spacing[2];
      tmp[0] = M3->GetElement(0, 0) * nIn[0] + M3->GetElement(0, 1) * nIn[1] +
        M3->GetElement(0, 2) * nIn[2];
      tmp[1] = M3->GetElement(1, 0) * nIn[0] + M3->GetElement(1, 1) * nIn[1] +
        M3->GetElement(1, 2) * nIn[2];
      tmp[2] = M3->GetElement(2, 0) * nIn[0] + M3->GetElement(2, 1) * nIn[1] +
        M3->GetElement(2, 2) * nIn[2];
      *nIn++ = tmp[0];
      *nIn++ = tmp[1];
      *nIn++ = tmp[2];
    }
  }

  // Interface to vtkSMPTools
  static void Execute(vtkMatrix3x3* m3, double* spacing, vtkIdType num, T* v)
  {
    InPlaceTransformVectors<T> transform(m3, spacing, v);
    vtkSMPTools::For(0, num, transform);
  }
}; // InPlaceTransformVectors

} // anonymous namespace

//============================================================================
//----------------------------------------------------------------------------
// Here is the VTK class proper.
//----------------------------------------------------------------------------
void vtkImageTransform::TransformPointSet(vtkImageData* im, vtkPointSet* ps)
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
  double* ar = im->GetSpacing();

  // If there is no rotation or spacing, only translate
  if (m3->IsIdentity() && ar[0] == 1 && ar[1] == 1 && ar[2] == 1)
  {
    vtkImageTransform::TranslatePoints(im->GetOrigin(), pts);
    return;
  }

  vtkImageTransform::TransformPoints(m4, pts);

  vtkDataArray* normals = ps->GetPointData()->GetNormals();
  if (normals != nullptr)
  {
    vtkImageTransform::TransformNormals(m3, ar, normals);
  }

  vtkDataArray* vectors = ps->GetPointData()->GetVectors();
  if (vectors != nullptr)
  {
    vtkImageTransform::TransformVectors(m3, ar, vectors);
  }

  // Grab the cells-related-data and process as appropriate
  normals = ps->GetCellData()->GetNormals();
  if (normals != nullptr)
  {
    vtkImageTransform::TransformNormals(m3, ar, normals);
  }

  vectors = ps->GetCellData()->GetVectors();
  if (vectors != nullptr)
  {
    vtkImageTransform::TransformVectors(m3, ar, vectors);
  }
}

//----------------------------------------------------------------------------
void vtkImageTransform::TranslatePoints(double* t, vtkDataArray* da)
{
  void* pts = da->GetVoidPointer(0);
  vtkIdType num = da->GetNumberOfTuples();

  switch (da->GetDataType())
  {
    vtkTemplateMacro(InPlaceTranslatePoints<VTK_TT>::Execute(t, num, (VTK_TT*)pts));
  }
}

//----------------------------------------------------------------------------
void vtkImageTransform::TransformPoints(vtkMatrix4x4* m4, vtkDataArray* da)
{
  void* pts = da->GetVoidPointer(0);
  vtkIdType num = da->GetNumberOfTuples();

  switch (da->GetDataType())
  {
    vtkTemplateMacro(InPlaceTransformPoints<VTK_TT>::Execute(m4, num, (VTK_TT*)pts));
  }
}

//----------------------------------------------------------------------------
void vtkImageTransform::TransformNormals(vtkMatrix3x3* m3, double spacing[3], vtkDataArray* da)
{
  void* n = da->GetVoidPointer(0);
  vtkIdType num = da->GetNumberOfTuples();

  switch (da->GetDataType())
  {
    vtkTemplateMacro(InPlaceTransformNormals<VTK_TT>::Execute(m3, spacing, num, (VTK_TT*)n));
  }
}

//----------------------------------------------------------------------------
void vtkImageTransform::TransformVectors(vtkMatrix3x3* m3, double spacing[3], vtkDataArray* da)
{
  void* v = da->GetVoidPointer(0);
  vtkIdType num = da->GetNumberOfTuples();

  switch (da->GetDataType())
  {
    vtkTemplateMacro(InPlaceTransformVectors<VTK_TT>::Execute(m3, spacing, num, (VTK_TT*)v));
  }
}

//----------------------------------------------------------------------------
void vtkImageTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
