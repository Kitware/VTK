/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVectorNorm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVectorNorm.h"

#include "vtkArrayDispatch.h"
#include "vtkCellData.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSMPTools.h"

#include <cmath>

vtkStandardNewMacro(vtkVectorNorm);

namespace
{
// The heart of the algorithm plus interface to the SMP tools. Double templated
// over point and scalar types.
template <class TV>
struct vtkVectorNormAlgorithm
{
  TV* Vectors = nullptr;
  float* Scalars = nullptr;
};
// Interface dot product computation to SMP tools.
template <class T>
struct NormOp
{
  vtkVectorNormAlgorithm<T>* Algo;
  vtkSMPThreadLocal<double> Max;
  NormOp(vtkVectorNormAlgorithm<T>* algo)
    : Algo(algo)
    , Max(VTK_DOUBLE_MIN)
  {
  }
  void operator()(vtkIdType k, vtkIdType end)
  {
    using ValueType = vtk::GetAPIType<T>;

    double& max = this->Max.Local();
    auto vectorRange = vtk::DataArrayTupleRange<3>(this->Algo->Vectors, k, end);
    float* s = this->Algo->Scalars + k;
    for (auto v : vectorRange)
    {
      const ValueType mag = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
      *s = static_cast<float>(sqrt(static_cast<double>(mag)));
      max = (*s > max ? *s : max);
      s++;
    }
  }
};

struct vtkVectorNormDispatch // Interface between VTK and templated functions.
{
  template <typename ArrayT>
  void operator()(ArrayT* vectors, bool normalize, vtkIdType num, float* scalars) const
  {

    // Populate data into local storage
    vtkVectorNormAlgorithm<ArrayT> algo;

    algo.Vectors = vectors;
    algo.Scalars = scalars;

    // Okay now generate samples using SMP tools
    NormOp<ArrayT> norm(&algo);
    vtkSMPTools::For(0, num, norm);

    // Have to roll up the thread local storage and get the overall range
    double max = VTK_DOUBLE_MIN;
    vtkSMPThreadLocal<double>::iterator itr;
    for (itr = norm.Max.begin(); itr != norm.Max.end(); ++itr)
    {
      if (*itr > max)
      {
        *itr = max;
      }
    }

    if (max > 0.0 && normalize)
    {
      vtkSMPTools::For(0, num, [&](vtkIdType i, vtkIdType end) {
        float* s = algo.Scalars + i;
        for (; i < end; ++i)
        {
          *s++ /= max;
        }
      });
    }
  }
};
}

//=================================Begin class proper=========================
//----------------------------------------------------------------------------
// Construct with normalize flag off.
vtkVectorNorm::vtkVectorNorm()
{
  this->Normalize = 0;
  this->AttributeMode = VTK_ATTRIBUTE_MODE_DEFAULT;
}

//----------------------------------------------------------------------------
int vtkVectorNorm::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType numVectors;
  int computePtScalars = 1, computeCellScalars = 1;
  vtkFloatArray* newScalars;
  vtkDataArray *ptVectors, *cellVectors;
  vtkPointData *pd = input->GetPointData(), *outPD = output->GetPointData();
  vtkCellData *cd = input->GetCellData(), *outCD = output->GetCellData();

  // Initialize
  vtkDebugMacro(<< "Computing norm of vectors!");

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  ptVectors = pd->GetVectors();
  cellVectors = cd->GetVectors();
  if (!ptVectors || this->AttributeMode == VTK_ATTRIBUTE_MODE_USE_CELL_DATA)
  {
    computePtScalars = 0;
  }

  if (!cellVectors || this->AttributeMode == VTK_ATTRIBUTE_MODE_USE_POINT_DATA)
  {
    computeCellScalars = 0;
  }

  if (!computeCellScalars && !computePtScalars)
  {
    vtkErrorMacro(<< "No vector norm to compute!");
    return 1;
  }

  // Needed for point and cell vector normals computation
  vtkVectorNormDispatch normDispatch;
  bool normalize = (this->GetNormalize() != 0);

  // Allocate / operate on point data
  if (computePtScalars)
  {
    numVectors = ptVectors->GetNumberOfTuples();
    newScalars = vtkFloatArray::New();
    newScalars->SetNumberOfTuples(numVectors);

    if (!vtkArrayDispatch::Dispatch::Execute(
          ptVectors, normDispatch, normalize, numVectors, newScalars->GetPointer(0)))
    {
      normDispatch(ptVectors, normalize, numVectors, newScalars->GetPointer(0));
    }

    int idx = outPD->AddArray(newScalars);
    outPD->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
    newScalars->Delete();
    outPD->CopyScalarsOff();
  } // if computing point scalars

  this->UpdateProgress(0.50);

  // Allocate / operate on cell data
  if (computeCellScalars)
  {
    numVectors = cellVectors->GetNumberOfTuples();
    newScalars = vtkFloatArray::New();
    newScalars->SetNumberOfTuples(numVectors);

    if (!vtkArrayDispatch::Dispatch::Execute(
          cellVectors, normDispatch, normalize, numVectors, newScalars->GetPointer(0)))
    {
      normDispatch(cellVectors, normalize, numVectors, newScalars->GetPointer(0));
    }

    int idx = outCD->AddArray(newScalars);
    outCD->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
    newScalars->Delete();
    outCD->CopyScalarsOff();
  } // if computing cell scalars

  // Pass appropriate data through to output
  outPD->PassData(pd);
  outCD->PassData(cd);

  return 1;
}

//----------------------------------------------------------------------------
// Return the method for generating scalar data as a string.
const char* vtkVectorNorm::GetAttributeModeAsString()
{
  if (this->AttributeMode == VTK_ATTRIBUTE_MODE_DEFAULT)
  {
    return "Default";
  }
  else if (this->AttributeMode == VTK_ATTRIBUTE_MODE_USE_POINT_DATA)
  {
    return "UsePointData";
  }
  else
  {
    return "UseCellData";
  }
}

//----------------------------------------------------------------------------
void vtkVectorNorm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Normalize: " << (this->Normalize ? "On\n" : "Off\n");
  os << indent << "Attribute Mode: " << this->GetAttributeModeAsString() << endl;
}
