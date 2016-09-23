/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPOutlineFilterInternals.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPOutlineFilterInternals.h"

#include "vtkAMRInformation.h"
#include "vtkAppendPolyData.h"
#include "vtkBoundingBox.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataObjectTree.h"
#include "vtkGraph.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineCornerSource.h"
#include "vtkOutlineSource.h"
#include "vtkOverlappingAMR.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkUniformGrid.h"

// ----------------------------------------------------------------------------
class AddBoundsListOperator : public vtkCommunicator::Operation
{
  // Description:
  // Performs a "B.AddBounds(A)" operation.
  void Function(const void *A, void *B, vtkIdType length, int datatype) VTK_OVERRIDE
  {
    (void)datatype;
    assert((datatype == VTK_DOUBLE) && (length%6==0));
    assert("pre: A vector is NULL" && (A != NULL) );
    assert("pre: B vector is NULL" && (B != NULL) );
    vtkBoundingBox box;
    const double *aPtr = reinterpret_cast<const double*>(A);
    double *bPtr       = reinterpret_cast<double*>(B);
    for(vtkIdType idx=0; idx < length; idx+=6 )
    {
      box.SetBounds(&bPtr[ idx ]);
      box.AddBounds(&aPtr[ idx ]);
      box.GetBounds(&bPtr[ idx ]);
    }
  }

  // Description:
  // Sets Commutative to true for this operation
  int Commutative() VTK_OVERRIDE { return 1; }
};

// ----------------------------------------------------------------------------
vtkPOutlineFilterInternals::vtkPOutlineFilterInternals()
{
  this->Controller = 0;
  this->IsCornerSource = false;
  this->CornerFactor = 0.2;
}

// ----------------------------------------------------------------------------
vtkPOutlineFilterInternals::~vtkPOutlineFilterInternals()
{
  this->Controller = 0;
}

// ----------------------------------------------------------------------------
void vtkPOutlineFilterInternals::SetController(vtkMultiProcessController* controller)
{
  this->Controller = controller;
}

// ----------------------------------------------------------------------------
void vtkPOutlineFilterInternals::SetCornerFactor(double cornerFactor)
{
  this->CornerFactor = cornerFactor;
}

// ----------------------------------------------------------------------------
void vtkPOutlineFilterInternals::SetIsCornerSource(bool value)
{
  this->IsCornerSource = value;
}

// ----------------------------------------------------------------------------
int vtkPOutlineFilterInternals::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0], 0);
  vtkPolyData* output = vtkPolyData::GetData(outputVector, 0);

  if (input == NULL || output == NULL)
  {
    vtkGenericWarningMacro("Missing input or output.");
    return 0;
  }

  if (this->Controller == NULL)
  {
    vtkGenericWarningMacro("Missing Controller.");
    return 0;
  }

  vtkOverlappingAMR* oamr = vtkOverlappingAMR::SafeDownCast(input);
  if (oamr)
  {
    return this->RequestData(oamr, output);
  }

  vtkUniformGridAMR* amr = vtkUniformGridAMR::SafeDownCast(input);
  if (amr)
  {
    return this->RequestData(amr, output);
  }

  vtkDataObjectTree* cd = vtkDataObjectTree::SafeDownCast(input);
  if (cd)
  {
    return this->RequestData(cd, output);
  }

  vtkDataSet* ds = vtkDataSet::SafeDownCast(input);
  if (ds)
  {
    return this->RequestData(ds, output);
  }

  vtkGraph *graph = vtkGraph::SafeDownCast(input);
  if (graph)
  {
    return this->RequestData(graph, output);
  }
  return 0;
}

// ----------------------------------------------------------------------------
void vtkPOutlineFilterInternals::CollectCompositeBounds(vtkDataObject* input)
{
  vtkDataSet* ds = vtkDataSet::SafeDownCast(input);
  vtkCompositeDataSet* compInput = vtkCompositeDataSet::SafeDownCast(input);
  if (ds != NULL)
  {
    double bounds[6];
    ds->GetBounds(bounds);
    this->BoundsList.push_back(vtkBoundingBox(bounds));
  }
  else if (compInput != NULL)
  {
    vtkCompositeDataIterator* iter = compInput->NewIterator();
    iter->SkipEmptyNodesOff();
    iter->GoToFirstItem();
    for (; !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      this->CollectCompositeBounds(iter->GetCurrentDataObject());
    }
    iter->Delete();
  }
  else
  {
    double bounds[6];
    vtkMath::UninitializeBounds(bounds);
    this->BoundsList.push_back(bounds);
  }
}

// ----------------------------------------------------------------------------
int vtkPOutlineFilterInternals::RequestData(
  vtkDataObjectTree* input, vtkPolyData* output)
{
  // Check Output and Input

  // Collect local bounds.
  this->CollectCompositeBounds(input);

  // Make an array of bounds from collected bounds
  std::vector<double> boundsList;
  boundsList.resize(6*this->BoundsList.size());

  for(size_t i =0; i < this->BoundsList.size(); ++i)
  {
    this->BoundsList[i].GetBounds(&boundsList[i*6]);
  }

  // Collect global bounds and copy into the array
  if (this->Controller && this->Controller->GetNumberOfProcesses () >1)
  {
    AddBoundsListOperator operation;
    double* temp = new double[6*this->BoundsList.size()];
    this->Controller->Reduce(&boundsList[0],
                             temp,
                             6*this->BoundsList.size(),
                             &operation,
                             0);
    memcpy(&boundsList[0], temp, 6*this->BoundsList.size()*sizeof(double));
    delete [] temp;

    if (this->Controller->GetLocalProcessId() > 0)
    {
      // only root node will produce the output.
      return 1;
    }
  }

  // Make output with collected bounds
  vtkNew<vtkAppendPolyData> appender;
  for (size_t i=0; i < 6*this->BoundsList.size (); i+=6)
  {
    vtkBoundingBox box(&boundsList[i]);
    if (box.IsValid())
    {
      if(this->IsCornerSource)
      {
        vtkNew<vtkOutlineCornerSource> corner;
        corner->SetBounds(&boundsList[i]);
        corner->SetCornerFactor(this->CornerFactor);
        corner->Update();
        appender->AddInputData(corner->GetOutput());
      }
      else
      {
        vtkNew<vtkOutlineSource> corner;
        corner->SetBounds(&boundsList[i]);
        corner->Update();
        appender->AddInputData(corner->GetOutput());
      }
    }
  }

  if (appender->GetNumberOfInputConnections(0) > 1)
  {
    appender->Update();
    output->ShallowCopy(appender->GetOutput());
  }
  return 1;
}

// ----------------------------------------------------------------------------
int vtkPOutlineFilterInternals::RequestData(
  vtkOverlappingAMR* input, vtkPolyData* output)
{
  // For Overlapping AMR, we have meta-data on all processes for the complete
  // AMR structure. Hence root node can build the outlines using that
  // meta-data, itself.
  int procid = this->Controller->GetLocalProcessId();
  if (procid != 0)
  {
    // we only generate output on the root node.
    return 1;
  }

  vtkNew<vtkAppendPolyData> appender;
  for (unsigned int level=0; level < input->GetNumberOfLevels(); ++level )
  {
    unsigned int num_datasets = input->GetNumberOfDataSets(level);
    for (unsigned int dataIdx=0; dataIdx < num_datasets; ++dataIdx)
    {
      double bounds[6];
      input->GetAMRInfo()->GetBounds(level, dataIdx, bounds);

      // Check if the bounds recieved are not default bounding box
      if (vtkBoundingBox::IsValid(bounds))
      {
        if (this->IsCornerSource)
        {
          vtkNew<vtkOutlineCornerSource> corner;
          corner->SetBounds(bounds);
          corner->SetCornerFactor(this->CornerFactor);
          corner->Update();
          appender->AddInputData(corner->GetOutput());
        }
        else
        {
          vtkNew<vtkOutlineSource> corner;
          corner->SetBounds(bounds);
          corner->Update();
          appender->AddInputData(corner->GetOutput());
        }
      }
    }
  }
  if (appender->GetNumberOfInputConnections(0) > 1)
  {
    appender->Update();
    output->ShallowCopy(appender->GetOutput());
  }
  return 1;
}

// ----------------------------------------------------------------------------
int vtkPOutlineFilterInternals::RequestData(vtkUniformGridAMR* input,
  vtkPolyData* output)
{
  // All processes simply produce the outline for the non-null blocks that exist
  // on the process.
  vtkNew<vtkAppendPolyData> appender;
  unsigned int block_id=0;
  for (unsigned int level=0; level < input->GetNumberOfLevels(); ++level )
  {
    unsigned int num_datasets = input->GetNumberOfDataSets(level);
    for (unsigned int dataIdx=0; dataIdx < num_datasets; ++dataIdx, block_id++)
    {
      vtkUniformGrid *ug = input->GetDataSet( level, dataIdx );
      if(ug)
      {
        double bounds[6];
        ug->GetBounds(bounds);

        // Check if the bounds recieved are not default bounding box
        if (vtkBoundingBox::IsValid(bounds))
        {
          if (this->IsCornerSource)
          {
            vtkNew<vtkOutlineCornerSource> corner;
            corner->SetBounds(bounds);
            corner->SetCornerFactor(this->CornerFactor);
            corner->Update();
            appender->AddInputData(corner->GetOutput());
          }
          else
          {
            vtkNew<vtkOutlineSource> corner;
            corner->SetBounds(bounds);
            corner->Update();
            appender->AddInputData(corner->GetOutput());
          }
        }
      }
    }
  }
  if (appender->GetNumberOfInputConnections(0) > 1)
  {
    appender->Update();
    output->ShallowCopy(appender->GetOutput());
  }
  return 1;
}

// ----------------------------------------------------------------------------
int vtkPOutlineFilterInternals::RequestData(
  vtkDataSet* input, vtkPolyData* output)
{
  double bounds[6];
  input->GetBounds(bounds);

  if (this->Controller->GetNumberOfProcesses() > 1)
  {
    double reduced_bounds[6];
    int procid = this->Controller->GetLocalProcessId();
    AddBoundsListOperator operation;
    this->Controller->Reduce(bounds, reduced_bounds, 6, &operation, 0);
    if (procid > 0)
    {
      // Satellite node
      return 1;
    }
    memcpy(bounds, reduced_bounds, 6*sizeof(double));
  }

  if (vtkMath::AreBoundsInitialized(bounds))
  {
    // only output in process 0.
    if(this->IsCornerSource)
    {
      vtkNew<vtkOutlineCornerSource> corner;
      corner->SetBounds(bounds);
      corner->SetCornerFactor(this->CornerFactor);
      corner->Update();
      output->ShallowCopy(corner->GetOutput());
    }
    else
    {
      vtkNew<vtkOutlineSource> corner;
      corner->SetBounds(bounds);
      corner->Update();
      output->ShallowCopy(corner->GetOutput());
    }
  }

  return 1;
}

// ----------------------------------------------------------------------------
int vtkPOutlineFilterInternals::RequestData(vtkGraph *input,
                                            vtkPolyData *output)
{
  double bounds[6];
  input->GetBounds(bounds);

  if (this->Controller->GetNumberOfProcesses() > 1)
  {
    double reduced_bounds[6];
    int procid = this->Controller->GetLocalProcessId();
    AddBoundsListOperator operation;
    this->Controller->Reduce(bounds, reduced_bounds, 6, &operation, 0);
    if (procid > 0)
    {
      // Satellite node
      return 1;
    }
    memcpy(bounds, reduced_bounds, 6*sizeof(double));
  }

  if (vtkMath::AreBoundsInitialized(bounds))
  {
    // only output in process 0.
    if(this->IsCornerSource)
    {
      vtkNew<vtkOutlineCornerSource> corner;
      corner->SetBounds(bounds);
      corner->SetCornerFactor(this->CornerFactor);
      corner->Update();
      output->ShallowCopy(corner->GetOutput());
    }
    else
    {
      vtkNew<vtkOutlineSource> corner;
      corner->SetBounds(bounds);
      corner->Update();
      output->ShallowCopy(corner->GetOutput());
    }
  }

  return 1;
}
