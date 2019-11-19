/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredGridAppend.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
#include "vtkStructuredGridAppend.h"

#include "vtkAlgorithmOutput.h"
#include "vtkArrayDispatch.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkUnsignedCharArray.h"
#include <cassert>

vtkStandardNewMacro(vtkStructuredGridAppend);

//----------------------------------------------------------------------------
vtkStructuredGridAppend::vtkStructuredGridAppend() = default;

//----------------------------------------------------------------------------
vtkStructuredGridAppend::~vtkStructuredGridAppend() = default;

//----------------------------------------------------------------------------
void vtkStructuredGridAppend::ReplaceNthInputConnection(int idx, vtkAlgorithmOutput* input)
{
  if (idx < 0 || idx >= this->GetNumberOfInputConnections(0))
  {
    vtkErrorMacro("Attempt to replace connection idx "
      << idx << " of input port " << 0 << ", which has only "
      << this->GetNumberOfInputConnections(0) << " connections.");
    return;
  }

  if (!input || !input->GetProducer())
  {
    vtkErrorMacro("Attempt to replace connection index "
      << idx << " for input port " << 0 << " with "
      << (!input ? "a null input." : "an input with no producer."));
    return;
  }

  this->SetNthInputConnection(0, idx, input);
}

//----------------------------------------------------------------------------
// The default vtkStructuredGridAlgorithm semantics are that SetInput() puts
// each input on a different port, we want all the structured grid inputs to
// go on the first port.
void vtkStructuredGridAppend::SetInputData(int idx, vtkDataObject* input)
{
  this->SetInputDataInternal(idx, input);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkStructuredGridAppend::GetInput(int idx)
{
  if (this->GetNumberOfInputConnections(0) <= idx)
  {
    return nullptr;
  }
  return vtkStructuredGrid::SafeDownCast(this->GetExecutive()->GetInputData(0, idx));
}

//----------------------------------------------------------------------------
// This method tells the output it will have more components
int vtkStructuredGridAppend::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  int unionExt[6];

  // Find the outMin/max of the appended axis for this input.
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), unionExt);
  for (int idx = 0; idx < this->GetNumberOfInputConnections(0); ++idx)
  {
    inInfo = inputVector[0]->GetInformationObject(idx);
    int* inExt = inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());

    // Compute union for preserving extents.
    if (inExt[0] < unionExt[0])
    {
      unionExt[0] = inExt[0];
    }
    if (inExt[1] > unionExt[1])
    {
      unionExt[1] = inExt[1];
    }
    if (inExt[2] < unionExt[2])
    {
      unionExt[2] = inExt[2];
    }
    if (inExt[3] > unionExt[3])
    {
      unionExt[3] = inExt[3];
    }
    if (inExt[4] < unionExt[4])
    {
      unionExt[4] = inExt[4];
    }
    if (inExt[5] > unionExt[5])
    {
      unionExt[5] = inExt[5];
    }
  }

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), unionExt, 6);

  return 1;
}

//----------------------------------------------------------------------------
int vtkStructuredGridAppend::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  // default input extent will be that of output extent
  for (int whichInput = 0; whichInput < this->GetNumberOfInputConnections(0); whichInput++)
  {
    int* inWextent;

    // Find the outMin/max of the appended axis for this input.
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(whichInput);
    inWextent = inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());

    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inWextent, 6);
  }

  return 1;
}

namespace
{
//----------------------------------------------------------------------------
// This templated implementation executes the filter for any type of data.
struct AppendWorker
{
  template <typename InArrayT, typename OutArrayT>
  void operator()(InArrayT *inArray,
                  OutArrayT *outArray,
                  int inExt[6],
                  int outExt[6],
                  vtkStructuredGrid *inData,
                  std::vector<int> &validValues,
                  vtkUnsignedCharArray *ghosts,
                  bool forCells)
  {
    const auto inTuples = vtk::DataArrayTupleRange(inArray);
    auto outTuples = vtk::DataArrayTupleRange(outArray);

    const int forPoints = forCells ? 0 : 1;
    vtkIdType inCounter = 0;

    for (int k = inExt[4]; k < inExt[5] + forPoints; k++)
    {
      for (int j = inExt[2]; j < inExt[3] + forPoints; j++)
      {
        for (int i = inExt[0]; i < inExt[1] + forPoints; i++)
        {
          const int ijk[3] = {i, j, k};
          bool skipValue = forCells
              ? !inData->IsCellVisible(inCounter)
              : !inData->IsPointVisible(inCounter);

          const vtkIdType outputIndex = forCells
              ? vtkStructuredData::ComputeCellIdForExtent(outExt, ijk)
              : vtkStructuredData::ComputePointIdForExtent(outExt, ijk);
          assert(static_cast<size_t>(outputIndex) < validValues.size());
          int &validValue = validValues[static_cast<std::size_t>(outputIndex)];

          if (skipValue && validValue <= 1)
          { // current output value for this is not set
            skipValue = false;
            validValue = 1; // value is from a blanked entity
          }
          else if(
            ghosts &&
            (ghosts->GetValue(inCounter) & vtkDataSetAttributes::DUPLICATECELL) &&
            validValue <= 2)
          {
            validValue = 2; // value is a ghost
            skipValue = false;
          }
          else if(validValue <= 3)
          {
            validValue = 3; // value is valid
            skipValue = false;
          }

          if(!skipValue)
          {
            outTuples[outputIndex] = inTuples[inCounter];
          }
          inCounter++;
        }
      }
    }
  }
};
} // end anon namespace

//----------------------------------------------------------------------------
int vtkStructuredGridAppend::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  int outExt[6];

  vtkStructuredGrid* output = vtkStructuredGrid::GetData(outputVector, 0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), outExt);
  output->SetExtent(outExt);

  // arrays to keep track of valid values that have been copied to the output
  // 0 means no value set, 1 means value set from a blanked entity, 2 means
  // value set from a ghost entity, 3 means value set from a non-ghost entity.
  // VTK assumes ghost entities have correct values in them but that may not
  // always be the case.
  vtkIdType numPoints = vtkStructuredData::GetNumberOfPoints(outExt);
  vtkIdType numCells = vtkStructuredData::GetNumberOfCells(outExt);
  std::vector<int> validValues;
  validValues.reserve(numPoints);

  // Dispatcher and worker implementation for append
  using Dispatcher = vtkArrayDispatch::Dispatch2SameValueType;
  AppendWorker worker;

  for (int idx1 = 0; idx1 < this->GetNumberOfInputConnections(0); ++idx1)
  {
    vtkStructuredGrid* input = vtkStructuredGrid::GetData(inputVector[0], idx1);
    if (input != nullptr)
    {
      // Get the input extent and output extent
      // the real out extent for this input may be clipped.
      vtkInformation* inInfo = inputVector[0]->GetInformationObject(idx1);

      int inExt[6];
      inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExt);
      // do a quick check to see if the input is used at all.
      if (inExt[0] <= inExt[1] && inExt[2] <= inExt[3] && inExt[4] <= inExt[5])
      {
        vtkDataArray* inArray;
        vtkDataArray* outArray;
        vtkIdType numComp;

        vtkUnsignedCharArray* ghosts = input->GetPointGhostArray();

        if (input->GetPointData()->GetNumberOfArrays())
        { // only zero out the array if we have point arrays
          validValues.resize(numPoints);
          for (vtkIdType i = 0; i < numPoints; i++)
          {
            validValues[i] = 0;
          }
        }

        // do point associated arrays
        for (vtkIdType ai = 0; ai < input->GetPointData()->GetNumberOfArrays(); ai++)
        {
          inArray = input->GetPointData()->GetArray(ai);
          outArray = output->GetPointData()->GetArray(ai);
          if (outArray == nullptr)
          {
            outArray = inArray->NewInstance();
            outArray->SetName(inArray->GetName());
            outArray->SetNumberOfComponents(inArray->GetNumberOfComponents());
            outArray->SetNumberOfTuples(vtkStructuredData::GetNumberOfPoints(outExt));
            output->GetPointData()->AddArray(outArray);
            outArray->Delete();
          }

          numComp = inArray->GetNumberOfComponents();
          if (numComp != outArray->GetNumberOfComponents())
          {
            vtkErrorMacro("Components of the inputs do not match");
            return 0;
          }

          // this filter expects that input is the same type as output.
          if (inArray->GetDataType() != outArray->GetDataType())
          {
            vtkErrorMacro(<< "Execute: input" << idx1 << " ScalarType (" << inArray->GetDataType()
                          << "), must match output ScalarType (" << outArray->GetDataType() << ")");
            return 0;
          }
          if (strcmp(inArray->GetName(), outArray->GetName()))
          {
            vtkErrorMacro(<< "Execute: input" << idx1 << " Name (" << inArray->GetName()
                          << "), must match output Name (" << outArray->GetName() << ")");
            return 0;
          }

          if (!Dispatcher::Execute(inArray, outArray,
                                   worker, inExt, outExt, input,
                                   validValues, ghosts, false))
          { // Fallback for unknown array types:
            worker(inArray, outArray, inExt, outExt, input, validValues,
                   ghosts, false);
          }

        }

        // do the point locations array
        inArray = input->GetPoints()->GetData();
        if (output->GetPoints() == nullptr)
        {
          vtkNew<vtkPoints> points;
          points->SetDataType(inArray->GetDataType());
          points->SetNumberOfPoints(vtkStructuredData::GetNumberOfPoints(outExt));
          output->SetPoints(points);
        }
        outArray = output->GetPoints()->GetData();

        if (!Dispatcher::Execute(inArray, outArray,
                                 worker, inExt, outExt, input,
                                 validValues, ghosts, false))
        { // Fallback for unknown array types:
          worker(inArray, outArray, inExt, outExt, input, validValues,
                 ghosts, false);
        }

        // note that we are still using validValues but only for the
        // cells now so there are less of them than points.
        if (input->GetCellData()->GetNumberOfArrays())
        { // only zero out values if we have cell arrays to compute
          validValues.resize(numCells);
          for (vtkIdType i = 0; i < numCells; i++)
          {
            validValues[i] = 0;
          }
        }
        ghosts = input->GetCellGhostArray();

        // do cell associated arrays
        for (vtkIdType ai = 0; ai < input->GetCellData()->GetNumberOfArrays(); ai++)
        {
          inArray = input->GetCellData()->GetArray(ai);
          outArray = output->GetCellData()->GetArray(ai);
          if (outArray == nullptr)
          {
            outArray = inArray->NewInstance();
            outArray->SetName(inArray->GetName());
            outArray->SetNumberOfComponents(inArray->GetNumberOfComponents());
            outArray->SetNumberOfTuples(output->GetNumberOfCells());
            output->GetCellData()->AddArray(outArray);
            outArray->Delete();
          }

          numComp = inArray->GetNumberOfComponents();
          if (numComp != outArray->GetNumberOfComponents())
          {
            vtkErrorMacro("Components of the inputs do not match");
            return 0;
          }

          // this filter expects that input is the same type as output.
          if (inArray->GetDataType() != outArray->GetDataType())
          {
            vtkErrorMacro(<< "Execute: input" << idx1 << " ScalarType (" << inArray->GetDataType()
                          << "), must match output ScalarType (" << outArray->GetDataType() << ")");
            return 0;
          }
          if (strcmp(inArray->GetName(), outArray->GetName()))
          {
            vtkErrorMacro(<< "Execute: input" << idx1 << " Name (" << inArray->GetName()
                          << "), must match output Name (" << outArray->GetName() << ")");
            return 0;
          }

          if (!Dispatcher::Execute(inArray, outArray,
                                   worker, inExt, outExt, input,
                                   validValues, ghosts, true))
          { // Fallback for unknown array types:
            worker(inArray, outArray, inExt, outExt, input, validValues,
                   ghosts, true);
          }
        }
      }
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkStructuredGridAppend::FillInputPortInformation(int i, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return this->Superclass::FillInputPortInformation(i, info);
}

//----------------------------------------------------------------------------
void vtkStructuredGridAppend::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
