/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProbeFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProbeFilter.h"
#include "vtkObjectFactory.h"
#include "vtkImageData.h"

vtkCxxRevisionMacro(vtkProbeFilter, "1.72");
vtkStandardNewMacro(vtkProbeFilter);

//----------------------------------------------------------------------------
vtkProbeFilter::vtkProbeFilter()
{
  this->SpatialMatch = 0;
  this->ValidPoints = vtkIdTypeArray::New();
}

//----------------------------------------------------------------------------
vtkProbeFilter::~vtkProbeFilter()
{
  this->ValidPoints->Delete();
  this->ValidPoints = NULL;
}


//----------------------------------------------------------------------------
void vtkProbeFilter::SetSource(vtkDataSet *input)
{
  this->vtkProcessObject::SetNthInput(1, input);
}

//----------------------------------------------------------------------------
vtkDataSet *vtkProbeFilter::GetSource()
{
  if (this->NumberOfInputs < 2)
    {
    return NULL;
    }
  
  return (vtkDataSet *)(this->Inputs[1]);
}


//----------------------------------------------------------------------------
void vtkProbeFilter::Execute()
{
  vtkIdType ptId, numPts;
  float *x, tol2;
  vtkCell *cell;
  vtkPointData *pd, *outPD;
  int subId;
  vtkDataSet *source = this->GetSource();
  vtkDataSet *input = this->GetInput();
  vtkDataSet *output= this->GetOutput();
  float pcoords[3], *weights;
  float fastweights[256];

  vtkDebugMacro(<<"Probing data");

  if (source == NULL)
    {
    vtkErrorMacro (<< "Source is NULL.");
    return;
    }

  pd = source->GetPointData();

  // lets use a stack allocated array if possible for performance reasons
  int mcs = source->GetMaxCellSize();
  if (mcs<=256)
    {
    weights = fastweights;
    }
  else
    {
    weights = new float[mcs];
    }

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  numPts = input->GetNumberOfPoints();
  this->ValidPoints->Allocate(numPts);

  // Allocate storage for output PointData
  //
  outPD = output->GetPointData();
  outPD->InterpolateAllocate(pd);

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
      this->UpdateProgress((float)ptId/numPts);
      abort = GetAbortExecute();
      }

    // Get the xyz coordinate of the point in the input dataset
    x = input->GetPoint(ptId);

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
}

//----------------------------------------------------------------------------
void vtkProbeFilter::ExecuteInformation()
{
  // Copy whole extent ...
  this->vtkSource::ExecuteInformation();

  if (this->GetInput() == NULL || this->GetSource() == NULL)
    {
    vtkErrorMacro("Missing input or source");
    return;
    }
}


//----------------------------------------------------------------------------
void vtkProbeFilter::ComputeInputUpdateExtents( vtkDataObject *output )
{
  vtkDataObject *input = this->GetInput();
  vtkDataObject *source = this->GetSource();
  int usePiece = 0;
  
  if (input == NULL || source == NULL)
    {
    vtkErrorMacro("Missing input or source.");
    return;
    }

  // What ever happend to CopyUpdateExtent in vtkDataObject?
  // Copying both piece and extent could be bad.  Setting the piece
  // of a structured data set will affect the extent.
  if (output->IsA("vtkUnstructuredGrid") || output->IsA("vtkPolyData"))
    {
    usePiece = 1;
    }
  
  input->RequestExactExtentOn();
  
  if ( ! this->SpatialMatch)
    {
    source->SetUpdateExtent(0, 1, 0);
    }
  else if (this->SpatialMatch == 1)
    {
    if (usePiece)
      {
      // Request an extra ghost level because the probe
      // gets external values with computation prescision problems.
      // I think the probe should be changed to have an epsilon ...
      source->SetUpdateExtent(output->GetUpdatePiece(), 
                              output->GetUpdateNumberOfPieces(),
                              output->GetUpdateGhostLevel()+1);
      }
    else
      {
      source->SetUpdateExtent(output->GetUpdateExtent()); 
      }
    }
  
  if (usePiece)
    {
    input->SetUpdateExtent(output->GetUpdatePiece(), 
                           output->GetUpdateNumberOfPieces(),
                           output->GetUpdateGhostLevel());
    }
  else
    {
    input->SetUpdateExtent(output->GetUpdateExtent()); 
    }
  
  // Use the whole input in all processes, and use the requested update
  // extent of the output to divide up the source.
  if (this->SpatialMatch == 2)
    {
    input->SetUpdateExtent(0, 1, 0);
    source->SetUpdateExtent(output->GetUpdatePiece(),
                            output->GetUpdateNumberOfPieces(),
                            output->GetUpdateGhostLevel());
    }
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
