/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericProbeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGenericProbeFilter.h"

#include "vtkCell.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkGenericDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkGenericCellIterator.h"
#include "vtkGenericAdaptorCell.h"
#include "vtkGenericAttribute.h"
#include "vtkGenericAttributeCollection.h"

vtkCxxRevisionMacro(vtkGenericProbeFilter, "1.3");
vtkStandardNewMacro(vtkGenericProbeFilter);

//----------------------------------------------------------------------------
vtkGenericProbeFilter::vtkGenericProbeFilter()
{
  this->ValidPoints = vtkIdTypeArray::New();
}

//----------------------------------------------------------------------------
vtkGenericProbeFilter::~vtkGenericProbeFilter()
{
  this->ValidPoints->Delete();
  this->ValidPoints = NULL;
}


//----------------------------------------------------------------------------
void vtkGenericProbeFilter::SetSource(vtkGenericDataSet *input)
{
  this->vtkProcessObject::SetNthInput(1, input);
}

//----------------------------------------------------------------------------
vtkGenericDataSet *vtkGenericProbeFilter::GetSource()
{
  if (this->NumberOfInputs < 2)
    {
    return NULL;
    }
  
  return (vtkGenericDataSet *)(this->Inputs[1]);
}


//----------------------------------------------------------------------------
void vtkGenericProbeFilter::Execute()
{
  vtkIdType ptId, numPts;
  double x[3], tol2;
//  vtkCell *cell;
  vtkPointData *outPD;
  int subId;
  vtkGenericDataSet *source = this->GetSource();
  vtkDataSet *input = this->GetInput();
  vtkDataSet *output= this->GetOutput();
  double pcoords[3], *weights;
  double fastweights[256];

  vtkDebugMacro(<<"Probing data");

  if (source == NULL)
    {
    vtkErrorMacro (<< "Source is NULL.");
    return;
    }

//  pd = source->GetPointData();
  //pd = NULL;
  //int size = input->GetNumberOfPoints();
  
  // lets use a stack allocated array if possible for performance reasons
  int mcs = 255; //source->GetMaxCellSize();  //FIXME
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
  //outPD->InterpolateAllocate(pd, size, size);
  vtkDoubleArray *foobar = vtkDoubleArray::New();
  outPD->SetScalars( foobar );

  // Use tolerance as a function of size of source data
  //
//  tol2 = source->GetLength();
  tol2 = 1000;  //FIXME
  tol2 = tol2 ? tol2*tol2 / 1000.0 : 0.001;

  // Loop over all input points, interpolating source data
  //
  int abort=0;
  // Need to use source to create a cellIt since this class is virtual
  vtkGenericCellIterator *cellIt = source->NewCellIterator();

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
    vtkIdType foo = source->FindCell(x,cellIt,tol2,subId,pcoords);
    if (foo >= 0)
      {
      // Interpolate the point data
      vtkGenericAdaptorCell *cellProbe = cellIt->GetCell();
      double s[3]; // FIXME: should be double *s=new double[source->GetAttributes()->GetNumberOfComponents()]
      //cellProbe->EvaluateShapeFunction(x,s);
      //source->GetAttributes()->EvaluateTuple(cellProbe, x,s);
      cellProbe->InterpolateTuple(source->GetAttributes(), x,s);
      foobar->InsertTuple( ptId, s);
      this->ValidPoints->InsertNextValue(ptId);
      }
    else
      {
      outPD->NullPoint(ptId);
      }
    }
  cellIt->Delete();

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
void vtkGenericProbeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkGenericDataSet *source = this->GetSource();

  this->Superclass::PrintSelf(os,indent);

  os << indent << "Source: " << source << "\n";
  os << indent << "ValidPoints: " << this->ValidPoints << "\n";
}
