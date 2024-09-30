// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkIdentityTransform.h"

#include "vtkDataArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkSMPTools.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkIdentityTransform);

//------------------------------------------------------------------------------
vtkIdentityTransform::vtkIdentityTransform() = default;

//------------------------------------------------------------------------------
vtkIdentityTransform::~vtkIdentityTransform() = default;

//------------------------------------------------------------------------------
void vtkIdentityTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkIdentityTransform::InternalDeepCopy(vtkAbstractTransform*)
{
  // nothing to do
}

//------------------------------------------------------------------------------
vtkAbstractTransform* vtkIdentityTransform::MakeTransform()
{
  return vtkIdentityTransform::New();
}

//------------------------------------------------------------------------------
template <class T2, class T3>
void vtkIdentityTransformPoint(T2 in[3], T3 out[3])
{
  out[0] = in[0];
  out[1] = in[1];
  out[2] = in[2];
}

//------------------------------------------------------------------------------
template <class T2, class T3, class T4>
void vtkIdentityTransformDerivative(T2 in[3], T3 out[3], T4 derivative[3][3])
{
  out[0] = in[0];
  out[1] = in[1];
  out[2] = in[2];

  vtkMath::Identity3x3(derivative);
}

//------------------------------------------------------------------------------
void vtkIdentityTransform::InternalTransformPoint(const float in[3], float out[3])
{
  vtkIdentityTransformPoint(in, out);
}

//------------------------------------------------------------------------------
void vtkIdentityTransform::InternalTransformPoint(const double in[3], double out[3])
{
  vtkIdentityTransformPoint(in, out);
}

//------------------------------------------------------------------------------
void vtkIdentityTransform::InternalTransformNormal(const float in[3], float out[3])
{
  vtkIdentityTransformPoint(in, out);
  vtkMath::Normalize(out);
}

//------------------------------------------------------------------------------
void vtkIdentityTransform::InternalTransformNormal(const double in[3], double out[3])
{
  vtkIdentityTransformPoint(in, out);
  vtkMath::Normalize(out);
}

//------------------------------------------------------------------------------
void vtkIdentityTransform::InternalTransformVector(const float in[3], float out[3])
{
  vtkIdentityTransformPoint(in, out);
}

//------------------------------------------------------------------------------
void vtkIdentityTransform::InternalTransformVector(const double in[3], double out[3])
{
  vtkIdentityTransformPoint(in, out);
}

//------------------------------------------------------------------------------
void vtkIdentityTransform::InternalTransformDerivative(
  const float in[3], float out[3], float derivative[3][3])
{
  vtkIdentityTransformDerivative(in, out, derivative);
}

//------------------------------------------------------------------------------
void vtkIdentityTransform::InternalTransformDerivative(
  const double in[3], double out[3], double derivative[3][3])
{
  vtkIdentityTransformDerivative(in, out, derivative);
}

//------------------------------------------------------------------------------
// Transform the normals and vectors using the derivative of the
// transformation.  Either inNms or inVrs can be set to nullptr.
// Normals are multiplied by the inverse transpose of the transform
// derivative, while vectors are simply multiplied by the derivative.
// Note that the derivative of the inverse transform is simply the
// inverse of the derivative of the forward transform.
void vtkIdentityTransform::TransformPointsNormalsVectors(vtkPoints* inPts, vtkPoints* outPts,
  vtkDataArray* inNms, vtkDataArray* outNms, vtkDataArray* inVrs, vtkDataArray* outVrs,
  int nOptionalVectors, vtkDataArray** inVrsArr, vtkDataArray** outVrsArr)
{
  this->TransformPoints(inPts, outPts);
  if (inNms)
  {
    this->TransformNormals(inNms, outNms);
  }
  if (inVrs)
  {
    this->TransformVectors(inVrs, outVrs);
  }
  if (inVrsArr)
  {
    for (int iArr = 0; iArr < nOptionalVectors; iArr++)
    {
      this->TransformVectors(inVrsArr[iArr], outVrsArr[iArr]);
    }
  }
}

//------------------------------------------------------------------------------
void vtkIdentityTransform::TransformPoints(vtkPoints* inPts, vtkPoints* outPts)
{
  vtkIdType n = inPts->GetNumberOfPoints();
  vtkIdType m = outPts->GetNumberOfPoints();
  outPts->SetNumberOfPoints(m + n);

  vtkSMPTools::For(0, n, vtkSMPTools::THRESHOLD,
    [&](vtkIdType ptId, vtkIdType endPtId)
    {
      double point[3];
      for (; ptId < endPtId; ++ptId)
      {
        inPts->GetPoint(ptId, point);
        outPts->SetPoint(m + ptId, point);
      }
    });
}

//------------------------------------------------------------------------------
void vtkIdentityTransform::TransformNormals(vtkDataArray* inNms, vtkDataArray* outNms)
{
  vtkIdType n = inNms->GetNumberOfTuples();
  vtkIdType m = outNms->GetNumberOfTuples();
  outNms->SetNumberOfTuples(m + n);

  vtkSMPTools::For(0, n, vtkSMPTools::THRESHOLD,
    [&](vtkIdType ptId, vtkIdType endPtId)
    {
      double normal[3];
      for (; ptId < endPtId; ++ptId)
      {
        inNms->GetTuple(ptId, normal);
        outNms->SetTuple(m + ptId, normal);
      }
    });
}

//------------------------------------------------------------------------------
void vtkIdentityTransform::TransformVectors(vtkDataArray* inVrs, vtkDataArray* outVrs)
{
  vtkIdType n = inVrs->GetNumberOfTuples();
  vtkIdType m = outVrs->GetNumberOfTuples();
  outVrs->SetNumberOfTuples(m + n);

  vtkSMPTools::For(0, n, vtkSMPTools::THRESHOLD,
    [&](vtkIdType ptId, vtkIdType endPtId)
    {
      double vect[3];
      for (; ptId < endPtId; ++ptId)
      {
        inVrs->GetTuple(ptId, vect);
        outVrs->SetTuple(m + ptId, vect);
      }
    });
}
VTK_ABI_NAMESPACE_END
