// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkLinearTransform.h"

#include "vtkArrayDispatch.h"
#include "vtkArrayDispatchDataSetArrayList.h"
#include "vtkDataArray.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkPoints.h"
#include "vtkSMPTools.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
void vtkLinearTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

namespace
{ // anonymous

//------------------------------------------------------------------------------
template <class T1, class T2, class T3>
inline void vtkLinearTransformPoint(T1 matrix[4][4], T2 in[3], T3 out[3])
{
  T3 x = static_cast<T3>(
    matrix[0][0] * in[0] + matrix[0][1] * in[1] + matrix[0][2] * in[2] + matrix[0][3]);
  T3 y = static_cast<T3>(
    matrix[1][0] * in[0] + matrix[1][1] * in[1] + matrix[1][2] * in[2] + matrix[1][3]);
  T3 z = static_cast<T3>(
    matrix[2][0] * in[0] + matrix[2][1] * in[1] + matrix[2][2] * in[2] + matrix[2][3]);

  out[0] = x;
  out[1] = y;
  out[2] = z;
}

//------------------------------------------------------------------------------
template <class T1, class T2, class T3, class T4>
inline void vtkLinearTransformDerivative(T1 matrix[4][4], T2 in[3], T3 out[3], T4 derivative[3][3])
{
  vtkLinearTransformPoint(matrix, in, out);

  for (int i = 0; i < 3; i++)
  {
    derivative[0][i] = static_cast<T4>(matrix[0][i]);
    derivative[1][i] = static_cast<T4>(matrix[1][i]);
    derivative[2][i] = static_cast<T4>(matrix[2][i]);
  }
}

//------------------------------------------------------------------------------
template <class T1, class T2, class T3>
inline void vtkLinearTransformVector(T1 matrix[4][4], T2 in[3], T3 out[3])
{
  T3 x = static_cast<T3>(matrix[0][0] * in[0] + matrix[0][1] * in[1] + matrix[0][2] * in[2]);
  T3 y = static_cast<T3>(matrix[1][0] * in[0] + matrix[1][1] * in[1] + matrix[1][2] * in[2]);
  T3 z = static_cast<T3>(matrix[2][0] * in[0] + matrix[2][1] * in[1] + matrix[2][2] * in[2]);

  out[0] = x;
  out[1] = y;
  out[2] = z;
}

//------------------------------------------------------------------------------
template <class T1, class T2, class T3>
inline void vtkLinearTransformNormal(T1 mat[4][4], T2 in[3], T3 out[3])
{
  // to transform the normal, multiply by the transposed inverse matrix
  T1 matrix[4][4];
  memcpy(*matrix, *mat, 16 * sizeof(T1));
  vtkMatrix4x4::Invert(*matrix, *matrix);
  vtkMatrix4x4::Transpose(*matrix, *matrix);

  vtkLinearTransformVector(matrix, in, out);

  vtkMath::Normalize(out);
}

//------------------------------------------------------------------------------
struct vtkLinearTransformPointsWorker
{
  template <class TArrayIn, class TArrayOut, class T>
  void operator()(TArrayIn* inArray, TArrayOut* outArray, vtkIdType outBeginTuple, T matrix[4][4])
  {
    // We use THRESHOLD to test if the data size is small enough
    // to execute the functor serially. It's faster for a smaller number of transformation.
    vtkSMPTools::For(0, inArray->GetNumberOfTuples(), vtkSMPTools::THRESHOLD,
      [&](vtkIdType ptId, vtkIdType endPtId)
      {
        auto pin = inArray->GetPointer(3 * ptId);
        auto pout = outArray->GetPointer(3 * (outBeginTuple + ptId));
        for (; ptId < endPtId; ++ptId, pin += 3, pout += 3)
        {
          vtkLinearTransformPoint(matrix, pin, pout);
        }
      });
  }
};

//------------------------------------------------------------------------------
struct vtkLinearTransformVectorsWorker
{
  template <class TArrayIn, class TArrayOut, class T>
  void operator()(TArrayIn* inArray, TArrayOut* outArray, vtkIdType outBeginTuple, T matrix[4][4])
  {
    // We use THRESHOLD to test if the data size is small enough
    // to execute the functor serially. It's faster for a smaller number of transformation.
    vtkSMPTools::For(0, inArray->GetNumberOfTuples(), vtkSMPTools::THRESHOLD,
      [&](vtkIdType ptId, vtkIdType endPtId)
      {
        auto pin = inArray->GetPointer(3 * ptId);
        auto pout = outArray->GetPointer(3 * (outBeginTuple + ptId));
        for (; ptId < endPtId; ++ptId, pin += 3, pout += 3)
        {
          vtkLinearTransformVector(matrix, pin, pout);
        }
      });
  }
};

//------------------------------------------------------------------------------
struct vtkLinearTransformNormalsWorker
{
  template <class TArrayIn, class TArrayOut, class T>
  void operator()(TArrayIn* inArray, TArrayOut* outArray, vtkIdType outBeginTuple, T matrix[4][4])
  {
    // We use THRESHOLD to test if the data size is small enough
    // to execute the functor serially. It's faster for a smaller number of transformation.
    vtkSMPTools::For(0, inArray->GetNumberOfTuples(), vtkSMPTools::THRESHOLD,
      [&](vtkIdType ptId, vtkIdType endPtId)
      {
        auto pin = inArray->GetPointer(3 * ptId);
        auto pout = outArray->GetPointer(3 * (outBeginTuple + ptId));
        for (; ptId < endPtId; ++ptId, pin += 3, pout += 3)
        {
          // matrix has been transposed & inverted, so use TransformVector
          vtkLinearTransformVector(matrix, pin, pout);
          vtkMath::Normalize(pout);
        }
      });
  }
};

} // anonymous namespace

//------------------------------------------------------------------------------
void vtkLinearTransform::InternalTransformPoint(const float in[3], float out[3])
{
  vtkLinearTransformPoint(this->Matrix->Element, in, out);
}

//------------------------------------------------------------------------------
void vtkLinearTransform::InternalTransformPoint(const double in[3], double out[3])
{
  vtkLinearTransformPoint(this->Matrix->Element, in, out);
}

//------------------------------------------------------------------------------
void vtkLinearTransform::InternalTransformNormal(const float in[3], float out[3])
{
  vtkLinearTransformNormal(this->Matrix->Element, in, out);
}

//------------------------------------------------------------------------------
void vtkLinearTransform::InternalTransformNormal(const double in[3], double out[3])
{
  vtkLinearTransformNormal(this->Matrix->Element, in, out);
}

//------------------------------------------------------------------------------
void vtkLinearTransform::InternalTransformVector(const float in[3], float out[3])
{
  vtkLinearTransformVector(this->Matrix->Element, in, out);
}

//------------------------------------------------------------------------------
void vtkLinearTransform::InternalTransformVector(const double in[3], double out[3])
{
  vtkLinearTransformVector(this->Matrix->Element, in, out);
}

//------------------------------------------------------------------------------
void vtkLinearTransform::InternalTransformDerivative(
  const float in[3], float out[3], float derivative[3][3])
{
  vtkLinearTransformDerivative(this->Matrix->Element, in, out, derivative);
}

//------------------------------------------------------------------------------
void vtkLinearTransform::InternalTransformDerivative(
  const double in[3], double out[3], double derivative[3][3])
{
  vtkLinearTransformDerivative(this->Matrix->Element, in, out, derivative);
}

//------------------------------------------------------------------------------
// Transform the normals and vectors using the derivative of the
// transformation.  Either inNms or inVrs can be set to nullptr.
// Normals are multiplied by the inverse transpose of the transform
// derivative, while vectors are simply multiplied by the derivative.
// Note that the derivative of the inverse transform is simply the
// inverse of the derivative of the forward transform.
void vtkLinearTransform::TransformPointsNormalsVectors(vtkPoints* inPts, vtkPoints* outPts,
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
void vtkLinearTransform::TransformPoints(vtkPoints* inPts, vtkPoints* outPts)
{
  vtkIdType n = inPts->GetNumberOfPoints();
  vtkIdType m = outPts->GetNumberOfPoints();
  double(*matrix)[4] = this->Matrix->Element;

  this->Update();

  // operate directly on the memory to avoid GetPoint()/SetPoint() calls.
  vtkDataArray* inArray = inPts->GetData();
  vtkDataArray* outArray = outPts->GetData();
  outArray->WriteVoidPointer(3 * m, 3 * n);

  vtkLinearTransformPointsWorker worker;
  if (!vtkArrayDispatch::Dispatch2ByArray<vtkArrayDispatch::AOSPointArrays,
        vtkArrayDispatch::AOSPointArrays>::Execute(inArray, outArray, worker, m, matrix))
  {
    // for anything that isn't float or double
    vtkSMPTools::For(0, n, vtkSMPTools::THRESHOLD,
      [&](vtkIdType ptId, vtkIdType endPtId)
      {
        double point[3];
        for (; ptId < endPtId; ++ptId)
        {
          inPts->GetPoint(ptId, point);
          vtkLinearTransformPoint(matrix, point, point);
          outPts->SetPoint(m + ptId, point);
        }
      });
  }
}

//------------------------------------------------------------------------------
void vtkLinearTransform::TransformNormals(vtkDataArray* inNms, vtkDataArray* outNms)
{
  vtkIdType n = inNms->GetNumberOfTuples();
  vtkIdType m = outNms->GetNumberOfTuples();
  double matrix[4][4];

  this->Update();

  // to transform the normal, multiply by the transposed inverse matrix
  vtkMatrix4x4::DeepCopy(*matrix, this->Matrix);
  vtkMatrix4x4::Invert(*matrix, *matrix);
  vtkMatrix4x4::Transpose(*matrix, *matrix);

  // operate directly on the memory to avoid GetTuple()/SetPoint() calls.
  outNms->WriteVoidPointer(3 * m, 3 * n);

  vtkLinearTransformNormalsWorker worker;
  if (!vtkArrayDispatch::Dispatch2ByArray<vtkArrayDispatch::AOSPointArrays,
        vtkArrayDispatch::AOSPointArrays>::Execute(inNms, outNms, worker, m, matrix))
  {
    // for anything that isn't float or double
    vtkSMPTools::For(0, n, vtkSMPTools::THRESHOLD,
      [&](vtkIdType ptId, vtkIdType endPtId)
      {
        double norm[3];
        for (; ptId < endPtId; ++ptId)
        {
          inNms->GetTuple(ptId, norm);
          // use TransformVector because matrix is already transposed & inverted
          vtkLinearTransformVector(matrix, norm, norm);
          vtkMath::Normalize(norm);
          outNms->SetTuple(m + ptId, norm);
        }
      });
  }
}

//------------------------------------------------------------------------------
void vtkLinearTransform::TransformVectors(vtkDataArray* inVrs, vtkDataArray* outVrs)
{
  vtkIdType n = inVrs->GetNumberOfTuples();
  vtkIdType m = outVrs->GetNumberOfTuples();

  double(*matrix)[4] = this->Matrix->Element;

  this->Update();

  // operate directly on the memory to avoid GetTuple()/SetTuple() calls.
  outVrs->WriteVoidPointer(3 * m, 3 * n);

  vtkLinearTransformVectorsWorker worker;
  if (!vtkArrayDispatch::Dispatch2ByArray<vtkArrayDispatch::AOSPointArrays,
        vtkArrayDispatch::AOSPointArrays>::Execute(inVrs, outVrs, worker, m, matrix))
  {
    // for anything that isn't float or double
    vtkSMPTools::For(0, n, vtkSMPTools::THRESHOLD,
      [&](vtkIdType ptId, vtkIdType endPtId)
      {
        double vec[3];
        for (; ptId < endPtId; ++ptId)
        {
          inVrs->GetTuple(ptId, vec);
          vtkLinearTransformVector(matrix, vec, vec);
          outVrs->SetTuple(m + ptId, vec);
        }
      });
  }
}
VTK_ABI_NAMESPACE_END
