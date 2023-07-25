// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkArrayListTemplate
 * @brief   thread-safe and efficient data attribute processing
 *
 *
 * vtkArrayListTemplate supplements the vtkDataSetAttributes class to provide
 * threaded processing of data arrays. It is also more efficient for certain
 * interpolation operations. The expectation is that it will be replaced one
 * day once vtkPointData, vtkCellData, vtkDataSetAttributes, and vtkFieldData
 * properly support multithreading and/or are redesigned. Note that this
 * implementation does not support incremental operations (like InsertNext()).
 *
 * Generally the way this helper class is used is to first invoke
 * vtkDataSetAttributes::CopyInterpolate() or InterpolateAllocate() which
 * performs the initial magic of constructing input and output arrays. Then
 * the input attributes, and output attributes, are passed to initialize the
 * internal structures via the AddArrays() method. Essentially these internal
 * structures are templated pairs of arrays of the same type, which can be
 * efficiently accessed and assigned. The operations on these array pairs
 * (e.g., interpolation) occur using a typeless, virtual dispatch base class.
 *
 * @warning
 * vtkDataSetAttributes is not in general thread safe due to the use of its
 * vtkFieldData::BasicIterator RequiredArrays data member. This class augments
 * vtkDataSetAttributes for thread safety.
 *
 * @sa
 * vtkFieldData vtkDataSetAttributes vtkPointData vtkCellData
 */

#ifndef vtkArrayListTemplate_h
#define vtkArrayListTemplate_h

#include "vtkAbstractArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"

#include <algorithm>
#include <vector>

// Create a generic class supporting virtual dispatch to type-specific
// subclasses.
VTK_ABI_NAMESPACE_BEGIN
struct BaseArrayPair
{
  vtkIdType Num;
  int NumComp;
  vtkSmartPointer<vtkAbstractArray> OutputArray;

  BaseArrayPair(vtkIdType num, int numComp, vtkAbstractArray* outArray)
    : Num(num)
    , NumComp(numComp)
    , OutputArray(outArray)
  {
  }
  virtual ~BaseArrayPair() = default;

  virtual void Copy(vtkIdType inId, vtkIdType outId) = 0;
  virtual void Interpolate(
    int numWeights, const vtkIdType* ids, const double* weights, vtkIdType outId) = 0;
  virtual void InterpolateOutput(
    int numWeights, const vtkIdType* ids, const double* weights, vtkIdType outId) = 0;
  virtual void Average(int numPts, const vtkIdType* ids, vtkIdType outId) = 0;
  virtual void WeightedAverage(
    int numPts, const vtkIdType* ids, const double* weights, vtkIdType outId) = 0;
  virtual void InterpolateEdge(vtkIdType v0, vtkIdType v1, double t, vtkIdType outId) = 0;
  virtual void AssignNullValue(vtkIdType outId) = 0;
#ifdef VTK_USE_64BIT_IDS
  virtual void Copy(unsigned int inId, unsigned int outId) = 0;
  virtual void Interpolate(
    int numWeights, const unsigned int* ids, const double* weights, unsigned int outId) = 0;
  virtual void InterpolateOutput(
    int numWeights, const unsigned int* ids, const double* weights, unsigned int outId) = 0;
  virtual void Average(int numPts, const unsigned int* ids, unsigned int outId) = 0;
  virtual void WeightedAverage(
    int numPts, const unsigned int* ids, const double* weights, unsigned int outId) = 0;
  virtual void InterpolateEdge(unsigned int v0, unsigned int v1, double t, unsigned int outId) = 0;
  virtual void AssignNullValue(unsigned int outId) = 0;
#endif
  virtual void Copy(unsigned short inId, unsigned short outId) = 0;
  virtual void Interpolate(
    int numWeights, const unsigned short* ids, const double* weights, unsigned short outId) = 0;
  virtual void InterpolateOutput(
    int numWeights, const unsigned short* ids, const double* weights, unsigned short outId) = 0;
  virtual void Average(int numPts, const unsigned short* ids, unsigned short outId) = 0;
  virtual void WeightedAverage(
    int numPts, const unsigned short* ids, const double* weights, unsigned short outId) = 0;
  virtual void InterpolateEdge(
    unsigned short v0, unsigned short v1, double t, unsigned short outId) = 0;
  virtual void AssignNullValue(unsigned short outId) = 0;

  virtual void Realloc(vtkIdType sze) = 0;
};

// Type specific interpolation on a matched pair of data arrays
template <typename T>
struct ArrayPair : public BaseArrayPair
{
  T* Input;
  T* Output;
  T NullValue;

  ArrayPair(T* in, T* out, vtkIdType num, int numComp, vtkAbstractArray* outArray, T null)
    : BaseArrayPair(num, numComp, outArray)
    , Input(in)
    , Output(out)
    , NullValue(null)
  {
  }
  ~ArrayPair() override = default; // calm down some finicky compilers
protected:
  template <typename IdTypeT>
  void Copy(IdTypeT inId, IdTypeT outId)
  {
    for (int j = 0; j < this->NumComp; ++j)
    {
      this->Output[outId * this->NumComp + j] =
        static_cast<T>(this->Input[inId * this->NumComp + j]);
    }
  }

  template <typename IdTypeT>
  void Interpolate(int numWeights, const IdTypeT* ids, const double* weights, IdTypeT outId)
  {
    for (int j = 0; j < this->NumComp; ++j)
    {
      double v = 0.0;
      for (int i = 0; i < numWeights; ++i)
      {
        v += weights[i] * static_cast<double>(this->Input[ids[i] * this->NumComp + j]);
      }
      this->Output[outId * this->NumComp + j] = static_cast<T>(v);
    }
  }

  template <typename IdTypeT>
  void InterpolateOutput(int numWeights, const IdTypeT* ids, const double* weights, IdTypeT outId)
  {
    for (int j = 0; j < this->NumComp; ++j)
    {
      double v = 0.0;
      for (int i = 0; i < numWeights; ++i)
      {
        v += weights[i] * static_cast<double>(this->Output[ids[i] * this->NumComp + j]);
      }
      this->Output[outId * this->NumComp + j] = static_cast<T>(v);
    }
  }

  template <typename IdTypeT>
  void Average(int numPts, const IdTypeT* ids, IdTypeT outId)
  {
    for (int j = 0; j < this->NumComp; ++j)
    {
      double v = 0.0;
      for (int i = 0; i < numPts; ++i)
      {
        v += static_cast<double>(this->Input[ids[i] * this->NumComp + j]);
      }
      v /= static_cast<double>(numPts);
      this->Output[outId * this->NumComp + j] = static_cast<T>(v);
    }
  }

  template <typename IdTypeT>
  void WeightedAverage(int numPts, const IdTypeT* ids, const double* weights, IdTypeT outId)
  {
    for (int j = 0; j < this->NumComp; ++j)
    {
      double v = 0.0;
      for (int i = 0; i < numPts; ++i)
      {
        v += (weights[i] * static_cast<double>(this->Input[ids[i] * this->NumComp + j]));
      }
      this->Output[outId * this->NumComp + j] = static_cast<T>(v);
    }
  }

  template <typename IdTypeT>
  void InterpolateEdge(IdTypeT v0, IdTypeT v1, double t, IdTypeT outId)
  {
    double v;
    for (int j = 0; j < this->NumComp; ++j)
    {
      v = this->Input[v0 * this->NumComp + j] +
        t * (this->Input[v1 * this->NumComp + j] - this->Input[v0 * this->NumComp + j]);
      this->Output[outId * this->NumComp + j] = static_cast<T>(v);
    }
  }

  template <typename IdTypeT>
  void AssignNullValue(IdTypeT outId)
  {
    for (int j = 0; j < this->NumComp; ++j)
    {
      this->Output[outId * this->NumComp + j] = this->NullValue;
    }
  }

public:
  void Copy(vtkIdType inId, vtkIdType outId) override { this->Copy<vtkIdType>(inId, outId); }
  void Interpolate(
    int numWeights, const vtkIdType* ids, const double* weights, vtkIdType outId) override
  {
    this->Interpolate<vtkIdType>(numWeights, ids, weights, outId);
  }
  void InterpolateOutput(
    int numWeights, const vtkIdType* ids, const double* weights, vtkIdType outId) override
  {
    this->InterpolateOutput<vtkIdType>(numWeights, ids, weights, outId);
  }
  void Average(int numPts, const vtkIdType* ids, vtkIdType outId) override
  {
    this->Average<vtkIdType>(numPts, ids, outId);
  }
  void WeightedAverage(
    int numPts, const vtkIdType* ids, const double* weights, vtkIdType outId) override
  {
    this->WeightedAverage<vtkIdType>(numPts, ids, weights, outId);
  }
  void InterpolateEdge(vtkIdType v0, vtkIdType v1, double t, vtkIdType outId) override
  {
    this->InterpolateEdge<vtkIdType>(v0, v1, t, outId);
  }
  void AssignNullValue(vtkIdType outId) override { this->AssignNullValue<vtkIdType>(outId); }
#ifdef VTK_USE_64BIT_IDS
  void Copy(unsigned int inId, unsigned int outId) override
  {
    this->Copy<unsigned int>(inId, outId);
  }
  void Interpolate(
    int numWeights, const unsigned int* ids, const double* weights, unsigned int outId) override
  {
    this->Interpolate<unsigned int>(numWeights, ids, weights, outId);
  }
  void InterpolateOutput(
    int numWeights, const unsigned int* ids, const double* weights, unsigned int outId) override
  {
    this->InterpolateOutput<unsigned int>(numWeights, ids, weights, outId);
  }
  void Average(int numPts, const unsigned int* ids, unsigned int outId) override
  {
    this->Average<unsigned int>(numPts, ids, outId);
  }
  void WeightedAverage(
    int numPts, const unsigned int* ids, const double* weights, unsigned int outId) override
  {
    this->WeightedAverage<unsigned int>(numPts, ids, weights, outId);
  }
  void InterpolateEdge(unsigned int v0, unsigned int v1, double t, unsigned int outId) override
  {
    this->InterpolateEdge<unsigned int>(v0, v1, t, outId);
  }
  void AssignNullValue(unsigned int outId) override { this->AssignNullValue<unsigned int>(outId); }
#endif
  void Copy(unsigned short inId, unsigned short outId) override
  {
    this->Copy<unsigned short>(inId, outId);
  }
  void Interpolate(
    int numWeights, const unsigned short* ids, const double* weights, unsigned short outId) override
  {
    this->Interpolate<unsigned short>(numWeights, ids, weights, outId);
  }
  void InterpolateOutput(
    int numWeights, const unsigned short* ids, const double* weights, unsigned short outId) override
  {
    this->InterpolateOutput<unsigned short>(numWeights, ids, weights, outId);
  }
  void Average(int numPts, const unsigned short* ids, unsigned short outId) override
  {
    this->Average<unsigned short>(numPts, ids, outId);
  }
  void WeightedAverage(
    int numPts, const unsigned short* ids, const double* weights, unsigned short outId) override
  {
    this->WeightedAverage<unsigned short>(numPts, ids, weights, outId);
  }
  void InterpolateEdge(
    unsigned short v0, unsigned short v1, double t, unsigned short outId) override
  {
    this->InterpolateEdge<unsigned short>(v0, v1, t, outId);
  }
  void AssignNullValue(unsigned short outId) override
  {
    this->AssignNullValue<unsigned short>(outId);
  }

  void Realloc(vtkIdType sze) override
  {
    this->OutputArray->Resize(sze);
    this->OutputArray->SetNumberOfTuples(sze);
    this->Output = static_cast<T*>(this->OutputArray->GetVoidPointer(0));
  }
};

template <>
struct ArrayPair<vtkStdString> : public BaseArrayPair
{
  vtkStdString* Input;
  vtkStdString* Output;
  double NullValue;

  ArrayPair(vtkStdString* in, vtkStdString* out, vtkIdType num, int numComp,
    vtkAbstractArray* outArray, double null)
    : BaseArrayPair(num, numComp, outArray)
    , Input(in)
    , Output(out)
    , NullValue(null)
  {
  }
  ~ArrayPair() override = default; // calm down some finicky compilers
protected:
  template <typename IdTypeT>
  void Copy(IdTypeT inId, IdTypeT outId)
  {
    for (int j = 0; j < this->NumComp; ++j)
    {
      this->Output[outId * this->NumComp + j] =
        static_cast<vtkStdString>(this->Input[inId * this->NumComp + j]);
    }
  }
  template <typename IdTypeT>
  void Interpolate(
    int numWeights, const IdTypeT* ids, const double* vtkNotUsed(weights), IdTypeT outId)
  {
    for (int i = 0; i < numWeights; ++i)
    {
      this->Copy(ids[i], outId);
    }
  }
  template <typename IdTypeT>
  void InterpolateOutput(int vtkNotUsed(numWeights), const IdTypeT* vtkNotUsed(ids),
    const double* vtkNotUsed(weights), IdTypeT vtkNotUsed(outId))
  {
  }
  template <typename IdTypeT>
  void Average(int numPts, const IdTypeT* ids, IdTypeT outId)
  {
    for (int i = 0; i < numPts; ++i)
    {
      this->Copy(ids[i], outId);
    }
  }
  template <typename IdTypeT>
  void WeightedAverage(
    int numPts, const IdTypeT* ids, const double* vtkNotUsed(weights), IdTypeT outId)
  {
    for (int i = 0; i < numPts; ++i)
    {
      this->Copy(ids[i], outId);
    }
  }
  template <typename IdTypeT>
  void InterpolateEdge(IdTypeT v0, IdTypeT v1, double vtkNotUsed(t), IdTypeT outId)
  {
    vtkStdString s;
    for (int j = 0; j < this->NumComp; ++j)
    {
      s = std::string(this->Input[v0 * this->NumComp + j]) +
        std::string(this->Input[v1 * this->NumComp + j]);
      this->Output[outId * this->NumComp + j] = s;
    }
  }

  template <typename IdTypeT>
  void AssignNullValue(IdTypeT outId)
  {
    for (int j = 0; j < this->NumComp; ++j)
    {
      this->Output[outId * this->NumComp + j] = std::to_string(this->NullValue);
    }
  }

public:
  void Copy(vtkIdType inId, vtkIdType outId) override { this->Copy<vtkIdType>(inId, outId); }
  void Interpolate(
    int numWeights, const vtkIdType* ids, const double* weights, vtkIdType outId) override
  {
    this->Interpolate<vtkIdType>(numWeights, ids, weights, outId);
  }
  void InterpolateOutput(
    int numWeights, const vtkIdType* ids, const double* weights, vtkIdType outId) override
  {
    this->InterpolateOutput<vtkIdType>(numWeights, ids, weights, outId);
  }
  void Average(int numPts, const vtkIdType* ids, vtkIdType outId) override
  {
    this->Average<vtkIdType>(numPts, ids, outId);
  }
  void WeightedAverage(
    int numPts, const vtkIdType* ids, const double* weights, vtkIdType outId) override
  {
    this->WeightedAverage<vtkIdType>(numPts, ids, weights, outId);
  }
  void InterpolateEdge(vtkIdType v0, vtkIdType v1, double t, vtkIdType outId) override
  {
    this->InterpolateEdge<vtkIdType>(v0, v1, t, outId);
  }
  void AssignNullValue(vtkIdType outId) override { this->AssignNullValue<vtkIdType>(outId); }
#ifdef VTK_USE_64BIT_IDS
  void Copy(unsigned int inId, unsigned int outId) override
  {
    this->Copy<unsigned int>(inId, outId);
  }
  void Interpolate(
    int numWeights, const unsigned int* ids, const double* weights, unsigned int outId) override
  {
    this->Interpolate<unsigned int>(numWeights, ids, weights, outId);
  }
  void InterpolateOutput(
    int numWeights, const unsigned int* ids, const double* weights, unsigned int outId) override
  {
    this->InterpolateOutput<unsigned int>(numWeights, ids, weights, outId);
  }
  void Average(int numPts, const unsigned int* ids, unsigned int outId) override
  {
    this->Average<unsigned int>(numPts, ids, outId);
  }
  void WeightedAverage(
    int numPts, const unsigned int* ids, const double* weights, unsigned int outId) override
  {
    this->WeightedAverage<unsigned int>(numPts, ids, weights, outId);
  }
  void InterpolateEdge(unsigned int v0, unsigned int v1, double t, unsigned int outId) override
  {
    this->InterpolateEdge<unsigned int>(v0, v1, t, outId);
  }
  void AssignNullValue(unsigned int outId) override { this->AssignNullValue<unsigned int>(outId); }
#endif
  void Copy(unsigned short inId, unsigned short outId) override
  {
    this->Copy<unsigned short>(inId, outId);
  }
  void Interpolate(
    int numWeights, const unsigned short* ids, const double* weights, unsigned short outId) override
  {
    this->Interpolate<unsigned short>(numWeights, ids, weights, outId);
  }
  void InterpolateOutput(
    int numWeights, const unsigned short* ids, const double* weights, unsigned short outId) override
  {
    this->InterpolateOutput<unsigned short>(numWeights, ids, weights, outId);
  }
  void Average(int numPts, const unsigned short* ids, unsigned short outId) override
  {
    this->Average<unsigned short>(numPts, ids, outId);
  }
  void WeightedAverage(
    int numPts, const unsigned short* ids, const double* weights, unsigned short outId) override
  {
    this->WeightedAverage<unsigned short>(numPts, ids, weights, outId);
  }
  void InterpolateEdge(
    unsigned short v0, unsigned short v1, double t, unsigned short outId) override
  {
    this->InterpolateEdge<unsigned short>(v0, v1, t, outId);
  }
  void AssignNullValue(unsigned short outId) override
  {
    this->AssignNullValue<unsigned short>(outId);
  }

  void Realloc(vtkIdType sze) override
  {
    this->OutputArray->Resize(sze);
    this->OutputArray->SetNumberOfTuples(sze);
    this->Output = static_cast<vtkStdString*>(this->OutputArray->GetVoidPointer(0));
  }
};

// Type specific interpolation on a pair of data arrays with different types, where the
// output type is expected to be a real type (i.e., float or double).
template <typename TInput, typename TOutput>
struct RealArrayPair : public BaseArrayPair
{
  TInput* Input;
  TOutput* Output;
  TOutput NullValue;

  RealArrayPair(
    TInput* in, TOutput* out, vtkIdType num, int numComp, vtkAbstractArray* outArray, TOutput null)
    : BaseArrayPair(num, numComp, outArray)
    , Input(in)
    , Output(out)
    , NullValue(null)
  {
  }
  ~RealArrayPair() override = default; // calm down some finicky compilers
protected:
  template <typename IdTypeT>
  void Copy(IdTypeT inId, IdTypeT outId)
  {
    for (int j = 0; j < this->NumComp; ++j)
    {
      this->Output[outId * this->NumComp + j] =
        static_cast<TOutput>(this->Input[inId * this->NumComp + j]);
    }
  }

  template <typename IdTypeT>
  void Interpolate(int numWeights, const IdTypeT* ids, const double* weights, IdTypeT outId)
  {
    for (int j = 0; j < this->NumComp; ++j)
    {
      double v = 0.0;
      for (int i = 0; i < numWeights; ++i)
      {
        v += weights[i] * static_cast<double>(this->Input[ids[i] * this->NumComp + j]);
      }
      this->Output[outId * this->NumComp + j] = static_cast<TOutput>(v);
    }
  }

  template <typename IdTypeT>
  void InterpolateOutput(int numWeights, const IdTypeT* ids, const double* weights, IdTypeT outId)
  {
    for (int j = 0; j < this->NumComp; ++j)
    {
      double v = 0.0;
      for (int i = 0; i < numWeights; ++i)
      {
        v += weights[i] * static_cast<double>(this->Output[ids[i] * this->NumComp + j]);
      }
      this->Output[outId * this->NumComp + j] = static_cast<TOutput>(v);
    }
  }

  template <typename IdTypeT>
  void Average(int numPts, const IdTypeT* ids, IdTypeT outId)
  {
    for (int j = 0; j < this->NumComp; ++j)
    {
      double v = 0.0;
      for (int i = 0; i < numPts; ++i)
      {
        v += static_cast<double>(this->Input[ids[i] * this->NumComp + j]);
      }
      v /= static_cast<double>(numPts);
      this->Output[outId * this->NumComp + j] = static_cast<TOutput>(v);
    }
  }

  template <typename IdTypeT>
  void WeightedAverage(int numPts, const IdTypeT* ids, const double* weights, IdTypeT outId)
  {
    for (int j = 0; j < this->NumComp; ++j)
    {
      double v = 0.0;
      for (int i = 0; i < numPts; ++i)
      {
        v += (weights[i] * static_cast<double>(this->Input[ids[i] * this->NumComp + j]));
      }
      this->Output[outId * this->NumComp + j] = static_cast<TOutput>(v);
    }
  }

  template <typename IdTypeT>
  void InterpolateEdge(IdTypeT v0, IdTypeT v1, double t, IdTypeT outId)
  {
    double v;
    for (int j = 0; j < this->NumComp; ++j)
    {
      v = this->Input[v0 * this->NumComp + j] +
        t * (this->Input[v1 * this->NumComp + j] - this->Input[v0 * this->NumComp + j]);
      this->Output[outId * this->NumComp + j] = static_cast<TOutput>(v);
    }
  }

  template <typename IdTypeT>
  void AssignNullValue(IdTypeT outId)
  {
    for (int j = 0; j < this->NumComp; ++j)
    {
      this->Output[outId * this->NumComp + j] = this->NullValue;
    }
  }

public:
  void Copy(vtkIdType inId, vtkIdType outId) override { this->Copy<vtkIdType>(inId, outId); }
  void Interpolate(
    int numWeights, const vtkIdType* ids, const double* weights, vtkIdType outId) override
  {
    this->Interpolate<vtkIdType>(numWeights, ids, weights, outId);
  }
  void InterpolateOutput(
    int numWeights, const vtkIdType* ids, const double* weights, vtkIdType outId) override
  {
    this->InterpolateOutput<vtkIdType>(numWeights, ids, weights, outId);
  }
  void Average(int numPts, const vtkIdType* ids, vtkIdType outId) override
  {
    this->Average<vtkIdType>(numPts, ids, outId);
  }
  void WeightedAverage(
    int numPts, const vtkIdType* ids, const double* weights, vtkIdType outId) override
  {
    this->WeightedAverage<vtkIdType>(numPts, ids, weights, outId);
  }
  void InterpolateEdge(vtkIdType v0, vtkIdType v1, double t, vtkIdType outId) override
  {
    this->InterpolateEdge<vtkIdType>(v0, v1, t, outId);
  }
  void AssignNullValue(vtkIdType outId) override { this->AssignNullValue<vtkIdType>(outId); }
#ifdef VTK_USE_64BIT_IDS
  void Copy(unsigned int inId, unsigned int outId) override
  {
    this->Copy<unsigned int>(inId, outId);
  }
  void Interpolate(
    int numWeights, const unsigned int* ids, const double* weights, unsigned int outId) override
  {
    this->Interpolate<unsigned int>(numWeights, ids, weights, outId);
  }
  void InterpolateOutput(
    int numWeights, const unsigned int* ids, const double* weights, unsigned int outId) override
  {
    this->InterpolateOutput<unsigned int>(numWeights, ids, weights, outId);
  }
  void Average(int numPts, const unsigned int* ids, unsigned int outId) override
  {
    this->Average<unsigned int>(numPts, ids, outId);
  }
  void WeightedAverage(
    int numPts, const unsigned int* ids, const double* weights, unsigned int outId) override
  {
    this->WeightedAverage<unsigned int>(numPts, ids, weights, outId);
  }
  void InterpolateEdge(unsigned int v0, unsigned int v1, double t, unsigned int outId) override
  {
    this->InterpolateEdge<unsigned int>(v0, v1, t, outId);
  }
  void AssignNullValue(unsigned int outId) override { this->AssignNullValue<unsigned int>(outId); }
#endif
  void Copy(unsigned short inId, unsigned short outId) override
  {
    this->Copy<unsigned short>(inId, outId);
  }
  void Interpolate(
    int numWeights, const unsigned short* ids, const double* weights, unsigned short outId) override
  {
    this->Interpolate<unsigned short>(numWeights, ids, weights, outId);
  }
  void InterpolateOutput(
    int numWeights, const unsigned short* ids, const double* weights, unsigned short outId) override
  {
    this->InterpolateOutput<unsigned short>(numWeights, ids, weights, outId);
  }
  void Average(int numPts, const unsigned short* ids, unsigned short outId) override
  {
    this->Average<unsigned short>(numPts, ids, outId);
  }
  void WeightedAverage(
    int numPts, const unsigned short* ids, const double* weights, unsigned short outId) override
  {
    this->WeightedAverage<unsigned short>(numPts, ids, weights, outId);
  }
  void InterpolateEdge(
    unsigned short v0, unsigned short v1, double t, unsigned short outId) override
  {
    this->InterpolateEdge<unsigned short>(v0, v1, t, outId);
  }
  void AssignNullValue(unsigned short outId) override
  {
    this->AssignNullValue<unsigned short>(outId);
  }

  void Realloc(vtkIdType sze) override
  {
    this->OutputArray->Resize(sze);
    this->OutputArray->SetNumberOfTuples(sze);
    this->Output = static_cast<TOutput*>(this->OutputArray->GetVoidPointer(0));
  }
};

// Forward declarations. This makes working with vtkTemplateMacro easier.
struct ArrayList;

template <typename T>
void CreateArrayPair(
  ArrayList* list, T* inData, T* outData, vtkIdType numTuples, int numComp, T nullValue);

// A list of the arrays to interpolate, and a method to invoke interpolation on the list
struct ArrayList
{
  // The list of arrays, and the arrays not to process
  std::vector<BaseArrayPair*> Arrays;
  std::vector<vtkAbstractArray*> ExcludedArrays;

  // Add the arrays to interpolate here (from attribute data). Note that this method is
  // not thread-safe due to its use of vtkDataSetAttributes.
  void AddArrays(vtkIdType numOutPts, vtkDataSetAttributes* inPD, vtkDataSetAttributes* outPD,
    double nullValue = 0.0, vtkTypeBool promote = true);

  // Add an array that interpolates from its own attribute values
  void AddSelfInterpolatingArrays(
    vtkIdType numOutPts, vtkDataSetAttributes* attr, double nullValue = 0.0);

  // Add a pair of arrays (manual insertion). Returns the output array created,
  // if any. No array may be created if \c inArray was previously marked as
  // excluded using ExcludeArray().
  vtkAbstractArray* AddArrayPair(vtkIdType numTuples, vtkAbstractArray* inArray,
    vtkStdString& outArrayName, double nullValue, vtkTypeBool promote);

  // Any array excluded here is not added by AddArrays() or AddArrayPair, hence not
  // processed. Also check whether an array is excluded.
  void ExcludeArray(vtkAbstractArray* da);
  vtkTypeBool IsExcluded(vtkAbstractArray* da);

  // Only you can prevent memory leaks!
  ~ArrayList()
  {
    for (auto& array : this->Arrays)
    {
      delete array;
    }
  }

protected:
  template <typename TIdType>
  void Copy(TIdType inId, TIdType outId)
  {
    for (auto& array : this->Arrays)
    {
      array->Copy(inId, outId);
    }
  }

  template <typename TIdType>
  void Interpolate(int numWeights, const TIdType* ids, const double* weights, TIdType outId)
  {
    for (auto& array : this->Arrays)
    {
      array->Interpolate(numWeights, ids, weights, outId);
    }
  }

  template <typename TIdType>
  void InterpolateOutput(int numWeights, const TIdType* ids, const double* weights, TIdType outId)
  {
    for (auto& array : this->Arrays)
    {
      array->InterpolateOutput(numWeights, ids, weights, outId);
    }
  }

  template <typename TIdType>
  void Average(int numPts, const TIdType* ids, TIdType outId)
  {
    for (auto& array : this->Arrays)
    {
      array->Average(numPts, ids, outId);
    }
  }

  template <typename TIdType>
  void WeightedAverage(int numPts, const TIdType* ids, const double* weights, TIdType outId)
  {
    for (auto& array : this->Arrays)
    {
      array->WeightedAverage(numPts, ids, weights, outId);
    }
  }

  template <typename TIdType>
  void InterpolateEdge(TIdType v0, TIdType v1, double t, TIdType outId)
  {
    for (auto& array : this->Arrays)
    {
      array->InterpolateEdge(v0, v1, t, outId);
    }
  }

  template <typename TIdType>
  void AssignNullValue(TIdType outId)
  {
    for (auto& array : this->Arrays)
    {
      array->AssignNullValue(outId);
    }
  }

public:
  /**
   * Loop over the array pairs and copy data from one to another. This (and the following methods)
   * can be used within threads.
   */
  void Copy(vtkIdType inId, vtkIdType outId) { this->Copy<vtkIdType>(inId, outId); }
  /**
   * Loop over the arrays and have them interpolate themselves
   */
  void Interpolate(int numWeights, const vtkIdType* ids, const double* weights, vtkIdType outId)
  {
    this->Interpolate<vtkIdType>(numWeights, ids, weights, outId);
  }
  /**
   * Loop over the arrays and have them interpolate themselves based on the output arrays
   */
  void InterpolateOutput(
    int numWeights, const vtkIdType* ids, const double* weights, vtkIdType outId)
  {
    this->InterpolateOutput<vtkIdType>(numWeights, ids, weights, outId);
  }
  /**
   * Loop over the arrays and have them averaged.
   */
  void Average(int numPts, const vtkIdType* ids, vtkIdType outId)
  {
    this->Average<vtkIdType>(numPts, ids, outId);
  }
  /**
   * Loop over the arrays and weighted average the attributes. The weights should sum to 1.0.
   */
  void WeightedAverage(int numPts, const vtkIdType* ids, const double* weights, vtkIdType outId)
  {
    this->WeightedAverage<vtkIdType>(numPts, ids, weights, outId);
  }
  /**
   * Loop over the arrays perform edge interpolation.
   */
  void InterpolateEdge(vtkIdType v0, vtkIdType v1, double t, vtkIdType outId)
  {
    this->InterpolateEdge<vtkIdType>(v0, v1, t, outId);
  }
  /**
   * Loop over the arrays and assign the null value.
   */
  void AssignNullValue(vtkIdType outId) { this->AssignNullValue<vtkIdType>(outId); }
#ifdef VTK_USE_64BIT_IDS
  /**
   * Loop over the array pairs and copy data from one to another. This (and the following methods)
   * can be used within threads.
   */
  void Copy(unsigned int inId, unsigned int outId) { this->Copy<unsigned int>(inId, outId); }
  /**
   * Loop over the arrays and have them interpolate themselves
   */
  void Interpolate(
    int numWeights, const unsigned int* ids, const double* weights, unsigned int outId)
  {
    this->Interpolate<unsigned int>(numWeights, ids, weights, outId);
  }
  /**
   * Loop over the arrays and have them interpolate themselves based on the output arrays
   */
  void InterpolateOutput(
    int numWeights, const unsigned int* ids, const double* weights, unsigned int outId)
  {
    this->InterpolateOutput<unsigned int>(numWeights, ids, weights, outId);
  }
  /**
   * Loop over the arrays and have them averaged.
   */
  void Average(int numPts, const unsigned int* ids, unsigned int outId)
  {
    this->Average<unsigned int>(numPts, ids, outId);
  }
  /**
   * Loop over the arrays and weighted average the attributes. The weights should sum to 1.0.
   */
  void WeightedAverage(
    int numPts, const unsigned int* ids, const double* weights, unsigned int outId)
  {
    this->WeightedAverage<unsigned int>(numPts, ids, weights, outId);
  }
  /**
   * Loop over the arrays perform edge interpolation.
   */
  void InterpolateEdge(unsigned int v0, unsigned int v1, double t, unsigned int outId)
  {
    this->InterpolateEdge<unsigned int>(v0, v1, t, outId);
  }
  /**
   * Loop over the arrays and assign the null value.
   */
  void AssignNullValue(unsigned int outId) { this->AssignNullValue<unsigned int>(outId); }
#endif
  /**
   * Loop over the array pairs and copy data from one to another. This (and the following methods)
   * can be used within threads.
   */
  void Copy(unsigned short inId, unsigned short outId) { this->Copy<unsigned short>(inId, outId); }
  /**
   * Loop over the arrays and have them interpolate themselves
   */
  void Interpolate(
    int numWeights, const unsigned short* ids, const double* weights, unsigned short outId)
  {
    this->Interpolate<unsigned short>(numWeights, ids, weights, outId);
  }
  /**
   * Loop over the arrays and have them interpolate themselves based on the output arrays
   */
  void InterpolateOutput(
    int numWeights, const unsigned short* ids, const double* weights, unsigned short outId)
  {
    this->InterpolateOutput<unsigned short>(numWeights, ids, weights, outId);
  }
  /**
   * Loop over the arrays and have them averaged.
   */
  void Average(int numPts, const unsigned short* ids, unsigned short outId)
  {
    this->Average<unsigned short>(numPts, ids, outId);
  }
  /**
   * Loop over the arrays and weighted average the attributes. The weights should sum to 1.0.
   */
  void WeightedAverage(
    int numPts, const unsigned short* ids, const double* weights, unsigned short outId)
  {
    this->WeightedAverage<unsigned short>(numPts, ids, weights, outId);
  }
  /**
   * Loop over the arrays perform edge interpolation.
   */
  void InterpolateEdge(unsigned short v0, unsigned short v1, double t, unsigned short outId)
  {
    this->InterpolateEdge<unsigned short>(v0, v1, t, outId);
  }
  /**
   * Loop over the arrays and assign the null value.
   */
  void AssignNullValue(unsigned short outId) { this->AssignNullValue<unsigned short>(outId); }

  /**
   * Extend (realloc) the arrays.
   */
  void Realloc(vtkIdType sze)
  {
    for (auto& array : this->Arrays)
    {
      array->Realloc(sze);
    }
  }

  /**
   * Return the number of arrays.
   */
  vtkIdType GetNumberOfArrays() { return static_cast<vtkIdType>(this->Arrays.size()); }
};

VTK_ABI_NAMESPACE_END
#include "vtkArrayListTemplate.txx"

#endif
// VTK-HeaderTest-Exclude: vtkArrayListTemplate.h
