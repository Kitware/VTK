/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProbeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProbeFilter.h"

#include "vtkCell.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkCxxRevisionMacro(vtkProbeFilter, "1.82.12.1");
vtkStandardNewMacro(vtkProbeFilter);

//----------------------------------------------------------------------------
vtkProbeFilter::vtkProbeFilter()
{
  this->SpatialMatch = 0;
  this->ValidPoints = vtkIdTypeArray::New();
  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
vtkProbeFilter::~vtkProbeFilter()
{
  this->ValidPoints->Delete();
  this->ValidPoints = NULL;
}

//----------------------------------------------------------------------------
void vtkProbeFilter::SetSourceConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}
 
//----------------------------------------------------------------------------
void vtkProbeFilter::SetSource(vtkDataObject *input)
{
  this->SetInput(1, input);
}

//----------------------------------------------------------------------------
vtkDataSet *vtkProbeFilter::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1)
    {
    return NULL;
    }
  
  return vtkDataSet::SafeDownCast(
    this->GetExecutive()->GetInputData(1, 0));
}

//----------------------------------------------------------------------------
int vtkProbeFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and ouptut
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *source = vtkDataSet::SafeDownCast(
    sourceInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType ptId, numPts;
  double x[3], tol2;
  vtkCell *cell;
  vtkPointData *pd, *outPD;
  int subId;
  double pcoords[3], *weights;
  double fastweights[256];

  vtkDebugMacro(<<"Probing data");

  pd = source->GetPointData();
  int size = input->GetNumberOfPoints();
  
  // lets use a stack allocated array if possible for performance reasons
  int mcs = source->GetMaxCellSize();
  if (mcs<=256)
    {
    weights = fastweights;
    }
  else
    {
    weights = new double[mcs];
    }

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  numPts = input->GetNumberOfPoints();
  this->ValidPoints->Allocate(numPts);

  // Allocate storage for output PointData
  //
  outPD = output->GetPointData();
  outPD->InterpolateAllocate(pd, size, size);

  // Use tolerance as a function of size of source data
  //
  tol2 = source->GetLength();
  tol2 = tol2 ? tol2*tol2 / 1000.0 : 0.001;

  // Loop over all input points, interpolating source data
  //
  int abort=0;
  vtkIdType progressInterval=numPts/20 + 1;
  for (ptId=0; ptId < numPts && !abort; ptId++)
    {
    if ( !(ptId % progressInterval) )
      {
      this->UpdateProgress((double)ptId/numPts);
      abort = GetAbortExecute();
      }

    // Get the xyz coordinate of the point in the input dataset
    input->GetPoint(ptId, x);

    // Find the cell that contains xyz and get it
    cell = source->FindAndGetCell(x,NULL,-1,tol2,subId,pcoords,weights);
    if (cell)
      {
      // Interpolate the point data
      outPD->InterpolatePoint(pd,ptId,cell->PointIds,weights);
      this->ValidPoints->InsertNextValue(ptId);
      }
    else
      {
      outPD->NullPoint(ptId);
      }
    }
  // BUG FIX: JB.
  // Output gets setup from input, but when output is imagedata, scalartype
  // depends on source scalartype not input scalartype
  if (output->IsA("vtkImageData"))
    {
    vtkImageData *out = (vtkImageData*)output;
    vtkDataArray *s = outPD->GetScalars();
    out->SetScalarType(s->GetDataType());
    out->SetNumberOfScalarComponents(s->GetNumberOfComponents());
    }
  if (mcs>256)
    {
    delete [] weights;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkProbeFilter::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()),
               6);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
               inInfo->Get(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES()));

  // Special case for ParaView.
  if (this->SpatialMatch == 2)
    {
    outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
                 sourceInfo->Get(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES()));
    }
  
  if (this->SpatialMatch == 1)
    {
    int m1 = inInfo->Get(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES());
    int m2 = sourceInfo->Get(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES());
    if (m1 < 0 && m2 < 0)
      {
      outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
                   -1);
      }
    else
      {
      if (m1 < -1)
        {
        m1 = VTK_LARGE_INTEGER;
        }
      if (m2 < -1)
        {
        m2 = VTK_LARGE_INTEGER;
        }
      if (m2 < m1)
        {
        m1 = m2;
        }
      outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
                   m1);
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkProbeFilter::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  int usePiece = 0;

  // What ever happend to CopyUpdateExtent in vtkDataObject?
  // Copying both piece and extent could be bad.  Setting the piece
  // of a structured data set will affect the extent.
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  if (output &&
      (!strcmp(output->GetClassName(), "vtkUnstructuredGrid") ||
       !strcmp(output->GetClassName(), "vtkPolyData")))
    {
    usePiece = 1;
    }
  
  inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);
  
  if ( ! this->SpatialMatch)
    {
    sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
    sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
                    1);
    sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
                    0);
    }
  else if (this->SpatialMatch == 1)
    {
    if (usePiece)
      {
      // Request an extra ghost level because the probe
      // gets external values with computation prescision problems.
      // I think the probe should be changed to have an epsilon ...
      sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
                      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
      sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
                      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
      sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
                      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS())+1);
      }
    else
      {
      sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
                      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()),
                      6);
      }
    }
  
  if (usePiece)
    {
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
                outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
                outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
                outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
    }
  else
    {
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
                outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()),
                6);
    }
  
  // Use the whole input in all processes, and use the requested update
  // extent of the output to divide up the source.
  if (this->SpatialMatch == 2)
    {
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
    sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
                    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
    sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
                    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
    sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
                    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkProbeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSet *source = this->GetSource();

  this->Superclass::PrintSelf(os,indent);
  os << indent << "Source: " << source << "\n";
  if (this->SpatialMatch)
    {
    os << indent << "SpatialMatchOn\n";
    }
  else
    {
    os << indent << "SpatialMatchOff\n";
    }
  os << indent << "ValidPoints: " << this->ValidPoints << "\n";
}
