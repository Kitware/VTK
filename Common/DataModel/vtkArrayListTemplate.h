/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayListTemplate.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkArrayListTemplate
 * @brief   thread-safe and efficient data attribute manipulation
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
 * internal structures. Essentially these internal structures are pairs of
 * arrays of the same type, which can be efficiently accessed and
 * assigned. The operations on these array pairs (e.g., interpolation) occur
 * using a typeless, virtual dispatch base class.
 *
 * @sa
 * vtkFieldData vtkDataSetAttributes vtkPointData vtkCellData
*/

#ifndef vtkArrayListTemplate_h
#define vtkArrayListTemplate_h

#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"

#include <vector>
#include <algorithm>

// Create a generic class supporting virtual dispatch to type-specific
// subclasses.
struct BaseArrayPair
{
  vtkIdType Num;
  int NumComp;
  vtkSmartPointer<vtkDataArray> OutputArray;

  BaseArrayPair(vtkIdType num, int numComp, vtkDataArray *outArray) :
    Num(num), NumComp(numComp), OutputArray(outArray)
  {
  }
  virtual ~BaseArrayPair()
  {
  }

  virtual void Copy(vtkIdType inId, vtkIdType outId) = 0;
  virtual void Interpolate(int numWeights, const vtkIdType *ids,
                           const double *weights, vtkIdType outId) = 0;
  virtual void InterpolateEdge(vtkIdType v0, vtkIdType v1,
                               double t, vtkIdType outId) = 0;
  virtual void AssignNullValue(vtkIdType outId) = 0;
  virtual void Realloc(vtkIdType sze) = 0;
};

// Type specific interpolation on a matched pair of data arrays
template <typename T>
struct ArrayPair : public BaseArrayPair
{
  T *Input;
  T *Output;
  T  NullValue;

  ArrayPair(T *in, T *out, vtkIdType num, int numComp, vtkDataArray *outArray, T null) :
    BaseArrayPair(num,numComp,outArray), Input(in), Output(out), NullValue(null)
  {
  }
  ~ArrayPair() override  //calm down some finicky compilers
  {
  }

  void Copy(vtkIdType inId, vtkIdType outId) override
  {
    for (int j=0; j < this->NumComp; ++j)
    {
      this->Output[outId*this->NumComp+j] = this->Input[inId*this->NumComp+j];
    }
  }

  void Interpolate(int numWeights, const vtkIdType *ids,
                           const double *weights, vtkIdType outId) override
  {
    for (int j=0; j < this->NumComp; ++j)
    {
      double v = 0.0;
      for (vtkIdType i=0; i < numWeights; ++i)
      {
        v += weights[i] * static_cast<double>(this->Input[ids[i]*this->NumComp+j]);
      }
      this->Output[outId*this->NumComp+j] = static_cast<T>(v);
    }
  }

  void InterpolateEdge(vtkIdType v0, vtkIdType v1, double t, vtkIdType outId) override
  {
    double v;
    vtkIdType numComp=this->NumComp;
    for (int j=0; j < numComp; ++j)
    {
      v = this->Input[v0*numComp+j] +
        t * (this->Input[v1*numComp+j] - this->Input[v0*numComp+j]);
      this->Output[outId*numComp+j] = static_cast<T>(v);
    }
  }

  void AssignNullValue(vtkIdType outId) override
  {
    for (int j=0; j < this->NumComp; ++j)
    {
      this->Output[outId*this->NumComp+j] = this->NullValue;
    }
  }

  void Realloc(vtkIdType sze) override
  {
      this->OutputArray->WriteVoidPointer(0,sze*this->NumComp);
      this->Output = static_cast<T*>(this->OutputArray->GetVoidPointer(0));
  }

};

// Type specific interpolation on a pair of data arrays with different types, where the
// output type is expected to be a real type (i.e., float or double).
template <typename TInput, typename TOutput>
struct RealArrayPair : public BaseArrayPair
{
  TInput *Input;
  TOutput *Output;
  TOutput  NullValue;

  RealArrayPair(TInput *in, TOutput *out, vtkIdType num, int numComp, vtkDataArray *outArray, TOutput null) :
    BaseArrayPair(num,numComp,outArray), Input(in), Output(out), NullValue(null)
  {
  }
  ~RealArrayPair() override  //calm down some finicky compilers
  {
  }

  void Copy(vtkIdType inId, vtkIdType outId) override
  {
    for (int j=0; j < this->NumComp; ++j)
    {
      this->Output[outId*this->NumComp+j] = static_cast<TOutput>(this->Input[inId*this->NumComp+j]);
    }
  }

  void Interpolate(int numWeights, const vtkIdType *ids,
                           const double *weights, vtkIdType outId) override
  {
    for (int j=0; j < this->NumComp; ++j)
    {
      double v = 0.0;
      for (vtkIdType i=0; i < numWeights; ++i)
      {
        v += weights[i] * static_cast<double>(this->Input[ids[i]*this->NumComp+j]);
      }
      this->Output[outId*this->NumComp+j] = static_cast<TOutput>(v);
    }
  }

  void InterpolateEdge(vtkIdType v0, vtkIdType v1, double t, vtkIdType outId) override
  {
    double v;
    vtkIdType numComp=this->NumComp;
    for (int j=0; j < numComp; ++j)
    {
      v = this->Input[v0*numComp+j] +
        t * (this->Input[v1*numComp+j] - this->Input[v0*numComp+j]);
      this->Output[outId*numComp+j] = static_cast<TOutput>(v);
    }
  }

  void AssignNullValue(vtkIdType outId) override
  {
    for (int j=0; j < this->NumComp; ++j)
    {
      this->Output[outId*this->NumComp+j] = this->NullValue;
    }
  }

  void Realloc(vtkIdType sze) override
  {
      this->OutputArray->WriteVoidPointer(0,sze*this->NumComp);
      this->Output = static_cast<TOutput*>(this->OutputArray->GetVoidPointer(0));
  }

};

// Forward declarations. This makes working with vtkTemplateMacro easier.
struct ArrayList;

template <typename T>
void CreateArrayPair(ArrayList *list, T *inData, T *outData,
                     vtkIdType numTuples, int numComp, T nullValue);


// A list of the arrays to interpolate, and a method to invoke interpolation on the list
struct ArrayList
{
  // The list of arrays, and the arrays not to process
  std::vector<BaseArrayPair*> Arrays;
  std::vector<vtkDataArray*> ExcludedArrays;

  // Add the arrays to interpolate here (from attribute data)
  void AddArrays(vtkIdType numOutPts, vtkDataSetAttributes *inPD,
                 vtkDataSetAttributes *outPD, double nullValue=0.0,
                 vtkTypeBool promote=true);

  // Add an array that interpolates from its own attribute values
  void AddSelfInterpolatingArrays(vtkIdType numOutPts, vtkDataSetAttributes *attr,
                                  double nullValue=0.0);

  // Add a pair of arrays (manual insertion). Returns the output array created,
  // if any. No array may be created if \c inArray was previously marked as
  // excluded using ExcludeArray().
  vtkDataArray* AddArrayPair(vtkIdType numTuples, vtkDataArray *inArray,
                             vtkStdString &outArrayName, double nullValue,
                             vtkTypeBool promote);

  // Any array excluded here is not added by AddArrays() or AddArrayPair, hence not
  // processed. Also check whether an array is excluded.
  void ExcludeArray(vtkDataArray *da);
  vtkTypeBool IsExcluded(vtkDataArray *da);

  // Loop over the array pairs and copy data from one to another
  void Copy(vtkIdType inId, vtkIdType outId)
  {
      for (std::vector<BaseArrayPair*>::iterator it = Arrays.begin();
           it != Arrays.end(); ++it)
      {
        (*it)->Copy(inId, outId);
      }
  }

  // Loop over the arrays and have them interpolate themselves
  void Interpolate(int numWeights, const vtkIdType *ids, const double *weights, vtkIdType outId)
  {
      for (std::vector<BaseArrayPair*>::iterator it = Arrays.begin();
           it != Arrays.end(); ++it)
      {
        (*it)->Interpolate(numWeights, ids, weights, outId);
      }
  }

  // Loop over the arrays perform edge interpolation
  void InterpolateEdge(vtkIdType v0, vtkIdType v1, double t, vtkIdType outId)
  {
      for (std::vector<BaseArrayPair*>::iterator it = Arrays.begin();
           it != Arrays.end(); ++it)
      {
        (*it)->InterpolateEdge(v0, v1, t, outId);
      }
  }

  // Loop over the arrays and assign the null value
  void AssignNullValue(vtkIdType outId)
  {
      for (std::vector<BaseArrayPair*>::iterator it = Arrays.begin();
           it != Arrays.end(); ++it)
      {
        (*it)->AssignNullValue(outId);
      }
  }

  // Extend (realloc) the arrays
  void Realloc(vtkIdType sze)
  {
      for (std::vector<BaseArrayPair*>::iterator it = Arrays.begin();
           it != Arrays.end(); ++it)
      {
        (*it)->Realloc(sze);
      }
  }

  // Only you can prevent memory leaks!
  ~ArrayList()
  {
      for (std::vector<BaseArrayPair*>::iterator it = Arrays.begin();
           it != Arrays.end(); ++it)
      {
        delete (*it);
      }
  }

  // Return the number of arrays
  vtkIdType GetNumberOfArrays()
  {
    return static_cast<vtkIdType>(Arrays.size());
  }

};


#include "vtkArrayListTemplate.txx"

#endif
// VTK-HeaderTest-Exclude: vtkArrayListTemplate.h
