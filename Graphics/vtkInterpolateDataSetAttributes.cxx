/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInterpolateDataSetAttributes.cxx
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
#include "vtkInterpolateDataSetAttributes.h"
#include "vtkPolyData.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredPoints.h"
#include "vtkUnstructuredGrid.h"
#include "vtkRectilinearGrid.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkInterpolateDataSetAttributes, "1.22");
vtkStandardNewMacro(vtkInterpolateDataSetAttributes);

// Create object with no input or output.
vtkInterpolateDataSetAttributes::vtkInterpolateDataSetAttributes()
{
  this->InputList = vtkDataSetCollection::New();

  this->T = 0.0;
}

vtkInterpolateDataSetAttributes::~vtkInterpolateDataSetAttributes()
{
  this->InputList->Delete();
  this->InputList = NULL;
}

//----------------------------------------------------------------------------
// Adds an input to the first null position in the input list.
// Expands the list memory if necessary
void vtkInterpolateDataSetAttributes::AddInput(vtkDataSet *input)
{
  if (this->NumberOfInputs == 0)
    {
    this->SetInput(input);
    }
  else
    {
    this->vtkProcessObject::AddInput(input);
    }
}

vtkDataSetCollection *vtkInterpolateDataSetAttributes::GetInputList()
{
  int i;
  this->InputList->RemoveAllItems();
  
  for (i = 0; i < this->NumberOfInputs; i++)
    {
    if (this->Inputs[i])
      {
      this->InputList->AddItem((vtkDataSet *)this->Inputs[i]);
      }
    }
  return this->InputList;
}

// Interpolate the data
void vtkInterpolateDataSetAttributes::Execute()
{
  vtkDataSetCollection *inputList = this->GetInputList();
  vtkIdType numPts, numCells, i;
  int numInputs = inputList->GetNumberOfItems();
  int lowDS, highDS;
  vtkDataSet *ds, *ds2;
  vtkDataSet *output = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  vtkCellData *outputCD = output->GetCellData();
  vtkPointData *inputPD, *input2PD;
  vtkCellData *inputCD, *input2CD;
  float t;
  

  if ( inputList->GetNumberOfItems() < 2 )
    {
    vtkErrorMacro(<< "Need at least two inputs to interpolate!");
    return;
    }  
  
  vtkDebugMacro(<<"Interpolating data...");

  // Check input and determine between which data sets the interpolation 
  // is to occur.
  if ( this->T > (float)numInputs )
    {
    vtkErrorMacro(<<"Bad interpolation parameter");
    return;
    }

  lowDS = (int) this->T;
  if ( lowDS >= (numInputs-1) )
    {
    lowDS = numInputs - 2;
    }

  highDS = lowDS + 1;
  t = this->T - (float)lowDS;
  if (t > 1.0)
    {
    t =1.0;
    }
  
  ds = inputList->GetItem(lowDS);
  ds2 = inputList->GetItem(highDS);

  numPts = ds->GetNumberOfPoints();
  numCells = ds->GetNumberOfCells();
  
  if ( numPts != ds2->GetNumberOfPoints() ||
       numCells != ds2->GetNumberOfCells() )
    {
    vtkErrorMacro(<<"Data sets not consistent!");
    return;
    }
  
  output->CopyStructure(ds);
  inputPD = ds->GetPointData();
  inputCD = ds->GetCellData();
  input2PD = ds2->GetPointData();
  input2CD = ds2->GetCellData();
    
  // Allocate the data set attributes
  outputPD->CopyAllOff();
  if ( inputPD->GetScalars() && input2PD->GetScalars() )
    {
    outputPD->CopyScalarsOn();
    }
  if ( inputPD->GetVectors() && input2PD->GetVectors() )
    {
    outputPD->CopyVectorsOn();
    }
  if ( inputPD->GetNormals() && input2PD->GetNormals() )
    {
    outputPD->CopyNormalsOn();
    }
  if ( inputPD->GetTCoords() && input2PD->GetTCoords() )
    {
    outputPD->CopyTCoordsOn();
    }
  if ( inputPD->GetTensors() && input2PD->GetTensors() )
    {
    outputPD->CopyTensorsOn();
    }
  // *TODO* Fix
  // if ( inputPD->GetFieldData() && input2PD->GetFieldData() )
  //{
  // outputPD->CopyFieldDataOn();
  //}
  outputPD->InterpolateAllocate(inputPD);

  outputCD->CopyAllOff();
  if ( inputCD->GetScalars() && input2CD->GetScalars() )
    {
    outputCD->CopyScalarsOn();
    }
  if ( inputCD->GetVectors() && input2CD->GetVectors() )
    {
    outputCD->CopyVectorsOn();
    }
  if ( inputCD->GetNormals() && input2CD->GetNormals() )
    {
    outputCD->CopyNormalsOn();
    }
  if ( inputCD->GetTCoords() && input2CD->GetTCoords() )
    {
    outputCD->CopyTCoordsOn();
    }
  if ( inputCD->GetTensors() && input2CD->GetTensors() )
    {
    outputCD->CopyTensorsOn();
    }
  // *TODO* Fix
  //if ( inputCD->GetFieldData() && input2CD->GetFieldData() )
  //{
  // outputCD->CopyFieldDataOn();
  //  }
  outputCD->InterpolateAllocate(inputCD);

 
  // Interpolate point data. We'll assume that it takes 50% of the time
  for ( i=0; i < numPts; i++ )
    {
    if ( ! (i % 10000) ) 
      {
      this->UpdateProgress ((float)i/numPts * 0.50);
      if (this->GetAbortExecute())
        {
        break;
        }
      }

    outputPD->InterpolateTime(inputPD, input2PD, i, t);
    }
  
  // Interpolate cell data. We'll assume that it takes 50% of the time
  for ( i=0; i < numCells; i++ )
    {
    if ( ! (i % 10000) ) 
      {
      this->UpdateProgress (0.5 + (float)i/numCells * 0.50);
      if (this->GetAbortExecute())
        {
        break;
        }
      }

    outputCD->InterpolateTime(inputCD, input2CD, i, t);
    }
}


void vtkInterpolateDataSetAttributes::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "T: " << this->T << endl;
}

