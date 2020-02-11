/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAdaptiveTemporalInterpolator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAdaptiveTemporalInterpolator.h"

#include "vtkCellCenters.h"
#include "vtkCellData.h"
#include "vtkDataArraySelection.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPMergeArrays.h"
#include "vtkPassSelectedArrays.h"
#include "vtkPointData.h"
#include "vtkPointDataToCellData.h"
#include "vtkPointSet.h"
#include "vtkResampleWithDataSet.h"
#include "vtkStaticCellLocator.h"

#include <vector>

vtkStandardNewMacro(vtkAdaptiveTemporalInterpolator);

//----------------------------------------------------------------------------
class vtkAdaptiveTemporalInterpolator::ResamplingHelperImpl
{
public:
  ResamplingHelperImpl()
  {
    // Build the resampling pipeline which will produce the previous timestep
    // analog (geometry/topology of the next time step, with cell and point data
    // sampled from the previous time step).
    stripAllArrays->GetPointDataArraySelection()->SetUnknownArraySetting(0);
    stripAllArrays->GetCellDataArraySelection()->SetUnknownArraySetting(0);
    stripAllArrays->GetFieldDataArraySelection()->SetUnknownArraySetting(1); // TODO: ? field data ?

    keepOnlyCellArrays->GetPointDataArraySelection()->SetUnknownArraySetting(0);
    keepOnlyCellArrays->GetCellDataArraySelection()->SetUnknownArraySetting(1);
    keepOnlyCellArrays->GetFieldDataArraySelection()->SetUnknownArraySetting(0);

    keepOnlyPointArrays->GetPointDataArraySelection()->SetUnknownArraySetting(1);
    keepOnlyPointArrays->GetCellDataArraySelection()->SetUnknownArraySetting(0);
    keepOnlyPointArrays->GetFieldDataArraySelection()->SetUnknownArraySetting(0);

    pointDataResampler->SetSourceConnection(keepOnlyPointArrays->GetOutputPort());
    pointDataResampler->SetInputConnection(stripAllArrays->GetOutputPort());
    pointDataResamplerLocator->SetAutomatic(true);
    pointDataResampler->SetCellLocatorPrototype(pointDataResamplerLocator);

    cellCenters->SetInputConnection(stripAllArrays->GetOutputPort());
    cellCenters->SetVertexCells(1);

    cellDataResampler->SetSourceConnection(keepOnlyCellArrays->GetOutputPort());
    cellDataResampler->SetInputConnection(cellCenters->GetOutputPort());
    cellDataResamplerLocator->SetAutomatic(true);
    cellDataResampler->SetCellLocatorPrototype(cellDataResamplerLocator);

    pointToCell->SetInputConnection(cellDataResampler->GetOutputPort());
    pointToCell->ProcessAllArraysOn();

    appendAttributes->AddInputConnection(stripAllArrays->GetOutputPort());
    appendAttributes->AddInputConnection(pointDataResampler->GetOutputPort());
    appendAttributes->AddInputConnection(pointToCell->GetOutputPort());
  }

  ~ResamplingHelperImpl() {}

  vtkPointSet* GetResampledDataObject(vtkPointSet* t0, vtkPointSet* t1)
  {
    keepOnlyCellArrays->RemoveAllInputs();
    keepOnlyPointArrays->RemoveAllInputs();
    stripAllArrays->RemoveAllInputs();

    keepOnlyCellArrays->SetInputData(t0);
    keepOnlyPointArrays->SetInputData(t0);
    stripAllArrays->SetInputData(t1);

    appendAttributes->Update();

    return vtkPointSet::SafeDownCast(appendAttributes->GetOutputDataObject(0));
  }

  vtkNew<vtkPassSelectedArrays> keepOnlyCellArrays;
  vtkNew<vtkPassSelectedArrays> keepOnlyPointArrays;
  vtkNew<vtkPassSelectedArrays> stripAllArrays;

  vtkNew<vtkResampleWithDataSet> pointDataResampler;
  vtkNew<vtkStaticCellLocator> pointDataResamplerLocator;
  vtkNew<vtkResampleWithDataSet> cellDataResampler;
  vtkNew<vtkStaticCellLocator> cellDataResamplerLocator;

  vtkNew<vtkCellCenters> cellCenters;

  vtkNew<vtkPointDataToCellData> pointToCell;

  vtkNew<vtkPMergeArrays> appendAttributes;
};

//----------------------------------------------------------------------------
vtkAdaptiveTemporalInterpolator::vtkAdaptiveTemporalInterpolator()
{
  this->ResampleImpl = nullptr;
}

//----------------------------------------------------------------------------
vtkAdaptiveTemporalInterpolator::~vtkAdaptiveTemporalInterpolator()
{
  if (this->ResampleImpl != nullptr)
  {
    delete this->ResampleImpl;
  }
}

//----------------------------------------------------------------------------
void vtkAdaptiveTemporalInterpolator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkDataSet* vtkAdaptiveTemporalInterpolator ::InterpolateDataSet(
  vtkDataSet* in1, vtkDataSet* in2, double ratio)
{
  vtkDataSet* input[2];
  input[0] = in1;
  input[1] = in2;

  // Favor the latter dataset (later timestep).
  // When meshes are adaptively refined, the timestep previous to
  // refinement will have accumulated error until it is unacceptable
  // while the refinement must (in theory) have a significant
  // improvement or it would be abandoned. Thus we prefer the latter:
  const int sourceInput = 1;
  vtkDataSet* output = input[sourceInput]->NewInstance();
  output->CopyStructure(input[sourceInput]);

  //
  // Interpolate points if the dataset is a vtkPointSet
  //
  vtkPointSet* inPointSet1 = vtkPointSet::SafeDownCast(input[0]);
  vtkPointSet* inPointSet2 = vtkPointSet::SafeDownCast(input[1]);
  vtkPointSet* outPointSet = vtkPointSet::SafeDownCast(output);
  ArrayMatch status = MISMATCHED_COMPS;
  vtkPointSet* resampledInput = nullptr;
  if (inPointSet1 && inPointSet2)
  {
    vtkDataArray* outarray = nullptr;
    vtkPoints* outpoints = nullptr;

    if (inPointSet1->GetNumberOfPoints() > 0 && inPointSet2->GetNumberOfPoints() > 0)
    {
      vtkDataArray* arrays[2];
      arrays[0] = inPointSet1->GetPoints()->GetData();
      arrays[1] = inPointSet2->GetPoints()->GetData();

      // allocate double for output if input is double - otherwise float
      // do a quick check to see if all arrays have the same number of tuples
      status = this->VerifyArrays(arrays, 2);
      switch (status)
      {
        case MISMATCHED_COMPS:
        {
          vtkWarningMacro("Interpolation aborted for points because the number of "
                          "components in each time step are different");
        }
        break;
        case MISMATCHED_TUPLES:
        {
          // If the mesh topology does not match, then assume for now
          // that the same space is covered but that the areas where
          // mesh density is high varies between the datasets.
          // So, we will copy the points from \a sourceInput.
          outarray = arrays[sourceInput];
          outarray->Register(this); // Delete will be called below.
          resampledInput = this->ResampleDataObject(inPointSet1, inPointSet2, sourceInput);
          if (resampledInput)
          {
            input[sourceInput == 0 ? 1 : 0] = resampledInput;
          }
          vtkPoints* inpoints = (sourceInput == 0 ? inPointSet1 : inPointSet2)->GetPoints();
          outpoints = inpoints->NewInstance();
          outPointSet->SetPoints(outpoints);
        }
        break;
        case MATCHED:
        {
          outarray = this->InterpolateDataArray(ratio, arrays, arrays[0]->GetNumberOfTuples());
          // Do not shallow copy points from either input, because otherwise when
          // we set the actual point coordinate data we overwrite the original
          // we must instantiate a new points object
          // (ie we override the copystrucure above)
          vtkPoints* inpoints = inPointSet1->GetPoints();
          outpoints = inpoints->NewInstance();
          outPointSet->SetPoints(outpoints);
        }
        break;
      }
    }
    else
    {
      // not much we can do really
      outpoints = vtkPoints::New();
      outPointSet->SetPoints(outpoints);
    }

    if (outpoints != nullptr)
    {
      if (vtkArrayDownCast<vtkDoubleArray>(outarray))
      {
        outpoints->SetDataTypeToDouble();
      }
      else
      {
        outpoints->SetDataTypeToFloat();
      }
      outpoints->SetNumberOfPoints(outarray->GetNumberOfTuples());
      outpoints->SetData(outarray);
      outpoints->Delete();
    }

    if (outarray)
    {
      outarray->Delete();
    }
  }

  //
  // Interpolate pointdata if present
  //
  output->GetPointData()->ShallowCopy(input[sourceInput]->GetPointData());
  for (int s = 0; s < input[sourceInput == 0 ? 1 : 0]->GetPointData()->GetNumberOfArrays(); ++s)
  {
    std::vector<vtkDataArray*> arrays;
    char* scalarname = nullptr;
    for (int i = 0; i < 2; ++i)
    {
      //
      // On some data, the scalar arrays are consistent but ordered
      // differently on each time step, so we will fetch them by name if
      // possible.
      //
      if (i == 0 || (scalarname == nullptr))
      {
        vtkDataArray* dataarray = input[i]->GetPointData()->GetArray(s);
        scalarname = dataarray->GetName();
        arrays.push_back(dataarray);
      }
      else
      {
        vtkDataArray* dataarray = input[i]->GetPointData()->GetArray(scalarname);
        arrays.push_back(dataarray);
      }
    }
    if (arrays[1])
    {
      // do a quick check to see if all arrays have the same number of tuples
      if (this->VerifyArrays(&arrays[0], 2) == MATCHED)
      {
        // allocate double for output if input is double - otherwise float
        vtkDataArray* outarray =
          this->InterpolateDataArray(ratio, &arrays[0], arrays[0]->GetNumberOfTuples());
        output->GetPointData()->AddArray(outarray);
        outarray->Delete();
      }
      else
      {
        vtkWarningMacro(<< "Interpolation aborted for point array "
                        << (scalarname ? scalarname : "(unnamed array)")
                        << " because the number of tuples/components"
                        << " in each time step are different");
      }
    }
    else
    {
      vtkDebugMacro(<< "Interpolation aborted for point array "
                    << (scalarname ? scalarname : "(unnamed array)")
                    << " because the array was not found"
                    << " in the second time step");
    }
  }

  //
  // Interpolate celldata if present
  //
  output->GetCellData()->ShallowCopy(input[sourceInput]->GetCellData());
  for (int s = 0; s < input[sourceInput == 0 ? 1 : 0]->GetCellData()->GetNumberOfArrays(); ++s)
  {
    // copy the structure
    std::vector<vtkDataArray*> arrays;
    char* scalarname = nullptr;
    for (int i = 0; i < 2; ++i)
    {
      //
      // On some data, the scalar arrays are consistent but ordered
      // differently on each time step, so we will fetch them by name if
      // possible.
      //
      if (i == 0 || (scalarname == nullptr))
      {
        vtkDataArray* dataarray = input[i]->GetCellData()->GetArray(s);
        scalarname = dataarray->GetName();
        arrays.push_back(dataarray);
      }
      else
      {
        vtkDataArray* dataarray = input[i]->GetCellData()->GetArray(scalarname);
        arrays.push_back(dataarray);
      }
    }
    if (arrays[1])
    {
      // do a quick check to see if all arrays have the same number of tuples
      if (this->VerifyArrays(&arrays[0], 2) == MATCHED)
      {
        // allocate double for output if input is double - otherwise float
        vtkDataArray* outarray =
          this->InterpolateDataArray(ratio, &arrays[0], arrays[0]->GetNumberOfTuples());
        output->GetCellData()->AddArray(outarray);
        outarray->Delete();
      }
      else
      {
        vtkWarningMacro(<< "Interpolation aborted for cell array "
                        << (scalarname ? scalarname : "(unnamed array)")
                        << " because the number of tuples/components"
                        << " in each time step are different");
      }
    }
    else
    {
      vtkDebugMacro(<< "Interpolation aborted for cell array "
                    << (scalarname ? scalarname : "(unnamed array)")
                    << " because the array was not found"
                    << " in the second time step");
    }
  }

  return output;
}

//----------------------------------------------------------------------------
vtkPointSet* vtkAdaptiveTemporalInterpolator::ResampleDataObject(
  vtkPointSet*& a, vtkPointSet*& b, int sourceInput)
{
  vtkPointSet* input;
  vtkPointSet* source;
  if (sourceInput == 0)
  {
    input = vtkPointSet::SafeDownCast(a->NewInstance());
    input->CopyStructure(a);
    source = b;
  }
  else
  {
    input = vtkPointSet::SafeDownCast(b->NewInstance());
    input->CopyStructure(b);
    source = a;
  }

  if (this->ResampleImpl == nullptr)
  {
    this->ResampleImpl = new ResamplingHelperImpl();
  }

  vtkPointSet* resampled = this->ResampleImpl->GetResampledDataObject(source, input);
  input->Delete();

  if (resampled)
  {
    if (sourceInput == 0)
    {
      a = resampled;
    }
    else
    {
      b = resampled;
    }
  }

  return resampled;
}
