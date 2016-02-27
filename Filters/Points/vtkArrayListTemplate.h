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
// .NAME vtkArrayListTemplate - thread-safe and efficient data attribute manipulation

// .SECTION Description
// vtkArrayListTemplate supplements the vtkDataSetAttributes class to provide
// threaded processing of data arrays. It is also more efficient for certain
// interpolation operations. The expectation is that it will be replaced one
// day once vtkPointData, vtkCellData, vtkDataSetAttributes, and vtkFieldData
// properly support multithreading and/or are redesigned. Note that this
// implementation does not support incremental operations (like InsertNext()).
//
// Generally the way this helper class is used is to first invoke
// vtkDataSetAttributes::CopyInterpolate() or InterpolateAllocate() which
// performs the initial magic of constructing input and output arrays. Then
// the input attributes, and output attributes, are passed to initialize the
// internal structures. Internally, pairs of typed arrays are created; the
// operations (e.g., interpolate) occur on these typed arrays using a
// typeless, virtual dispatch base class.


// .SECTION See Also
// vtkFieldData vtkDataSetAttributes vtkPointData vtkCellData

#ifndef vtkArrayListTemplate_h
#define vtkArrayListTemplate_h

#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"

// Create a generic class supporting virtual dispatch to type-specific
// subclasses.
struct BaseArrayPair
{
  vtkIdType Num;
  int NumComp;

  BaseArrayPair(vtkIdType num, int numComp) : Num(num), NumComp(numComp)
    {
    }
  virtual ~BaseArrayPair()
    {
    }

  virtual void Copy(vtkIdType inId, vtkIdType outId) = 0;
  virtual void Interpolate(int numWeights, const vtkIdType *ids,
                           const double *weights, vtkIdType outPtId) = 0;
  virtual void AssignNullValue(vtkIdType outId) = 0;
};

// Type specific interpolation on a matched pair of data arrays
template <typename T>
struct ArrayPair : public BaseArrayPair
{
  T *Input;
  T *Output;
  T  NullValue;

  ArrayPair(T *in, T *out, vtkIdType num, int numComp, T null) :
    BaseArrayPair(num,numComp), Input(in), Output(out), NullValue(null)
    {
    }
  virtual ~ArrayPair()  //calm down some finicky compilers
    {
    }

  virtual void Copy(vtkIdType inId, vtkIdType outId)
    {
    for (int j=0; j < this->NumComp; ++j)
      {
      this->Output[outId*this->NumComp+j] = this->Input[inId*this->NumComp+j];
      }
    }

  virtual void Interpolate(int numWeights, const vtkIdType *ids,
                           const double *weights, vtkIdType outPtId)
    {
    for (int j=0; j < this->NumComp; ++j)
      {
      double v = 0.0;
      for (vtkIdType i=0; i < numWeights; ++i)
        {
        v += weights[i] * static_cast<double>(this->Input[ids[i]*this->NumComp+j]);
        }
      this->Output[outPtId*this->NumComp+j] = static_cast<T>(v);
      }
    }

  virtual void AssignNullValue(vtkIdType outId)
    {
    for (int j=0; j < this->NumComp; ++j)
      {
      this->Output[outId*this->NumComp+j] = this->NullValue;
      }
    }

};

// Forward declarations. This makes working with vtkTemplateMacro easier.
struct ArrayList;

template <typename T>
void CreateArrayPair(ArrayList *list, T *inData, T *outData,
                     vtkIdType numPts, int numComp, T nullValue);


// A list of the arrays to interpolate, and a method to invoke interpolation on the list
struct ArrayList
{
  // The list of arrays
  std::vector<BaseArrayPair*> Arrays;

  // Add the arrays to interpolate here
  void AddArrays(vtkIdType numOutPts, vtkDataSetAttributes *inPD,
                 vtkDataSetAttributes *outPD, double nullValue=0.0);

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
  void Interpolate(int numWeights, const vtkIdType *ids, const double *weights, vtkIdType outPtId)
    {
      for (std::vector<BaseArrayPair*>::iterator it = Arrays.begin();
           it != Arrays.end(); ++it)
        {
        (*it)->Interpolate(numWeights, ids, weights, outPtId);
        }
    }

  // Loop over the arrays and have them interpolate themselves
  void AssignNullValue(vtkIdType outId)
    {
      for (std::vector<BaseArrayPair*>::iterator it = Arrays.begin();
           it != Arrays.end(); ++it)
        {
        (*it)->AssignNullValue(outId);
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
};


#include "vtkArrayListTemplate.txx"

#endif
// VTK-HeaderTest-Exclude: vtkArrayListTemplate.h
