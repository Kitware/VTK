/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInterpolateDataSetAttributes.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkInterpolateDataSetAttributes.h"
#include "vtkPolyData.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredPoints.h"
#include "vtkUnstructuredGrid.h"
#include "vtkRectilinearGrid.h"

// Create object with no input or output.
vtkInterpolateDataSetAttributes::vtkInterpolateDataSetAttributes()
{
  this->InputList = vtkDataSetCollection::New();

  this->T = 0.0;

  this->PolyData = vtkPolyData::New();
  this->PolyData->SetSource(this);
  
  this->StructuredPoints = vtkStructuredPoints::New();
  this->StructuredPoints->SetSource(this);
  
  this->StructuredGrid = vtkStructuredGrid::New();
  this->StructuredGrid->SetSource(this);
  
  this->UnstructuredGrid = vtkUnstructuredGrid::New();
  this->UnstructuredGrid->SetSource(this);
  
  this->RectilinearGrid = vtkRectilinearGrid::New();
  this->RectilinearGrid->SetSource(this);
}

vtkInterpolateDataSetAttributes::~vtkInterpolateDataSetAttributes()
{
  this->InputList->Delete();
  this->InputList = NULL;

  this->PolyData->Delete();
  this->StructuredPoints->Delete();
  this->StructuredGrid->Delete();
  this->UnstructuredGrid->Delete();
  this->RectilinearGrid->Delete();

  // Output should only be one of the above. We set it to NULL
  // so that we don't free it twice
  this->Output = NULL;
}

// Add a dataset to the list of data to interpolate.
void vtkInterpolateDataSetAttributes::AddInput(vtkDataSet *ds)
{
  if ( ds != NULL && ! this->InputList->IsItemPresent(ds) )
    {
    vtkDebugMacro(<<" setting Input to " << (void *)ds);
    this->Modified();
    this->InputList->AddItem(ds);

    if ( ds->GetDataSetType() == VTK_POLY_DATA )
      {
      this->Output = this->PolyData;
      }

    else if ( ds->GetDataSetType() == VTK_STRUCTURED_POINTS )
      {
      this->Output = this->StructuredPoints;
      }

    else if ( ds->GetDataSetType() == VTK_STRUCTURED_GRID )
      {
      this->Output = this->StructuredGrid;
      }

    else if ( ds->GetDataSetType() == VTK_UNSTRUCTURED_GRID )
      {
      this->Output = this->UnstructuredGrid;
      }

    else if ( ds->GetDataSetType() == VTK_RECTILINEAR_GRID )
      {
      this->Output = this->RectilinearGrid;
      }

    else
      {
      vtkErrorMacro(<<"Mismatch in data type");
      }
    }
}

// Remove a dataset from the list of data to interpolate.
void vtkInterpolateDataSetAttributes::RemoveInput(vtkDataSet *ds)
{
  if ( this->InputList->IsItemPresent(ds) )
    {
    this->Modified();
    this->InputList->RemoveItem(ds);
    }
}

void vtkInterpolateDataSetAttributes::Update()
{
  unsigned long int mtime=0, dsMtime;
  vtkDataSet *ds;
  int dataSetType;

  // make sure input is available
  if ( this->InputList->GetNumberOfItems() < 2 )
    {
    vtkErrorMacro(<< "Need at least two inputs to interpolate!");
    return;
    }

  // prevent chasing our tail
  if (this->Updating)
    {
    return;
    }

  this->Updating = 1;
  for (mtime=0, this->InputList->InitTraversal(); 
  (ds = this->InputList->GetNextItem()); )
    {
    if ( mtime == 0 ) //first time
      {
      dataSetType = ds->GetDataSetType();
      }

    if ( dataSetType != ds->GetDataSetType() )
      {
      vtkErrorMacro(<<"All input data sets must be of the same type!");
      return;
      }
          
    ds->Update();
    dsMtime = ds->GetMTime();
    if ( dsMtime > mtime )
      {
      mtime = dsMtime;
      }
    }
  this->Updating = 0;

  if ( mtime > this->ExecuteTime || this->GetMTime() > this->ExecuteTime )
    {
    for (this->InputList->InitTraversal();(ds=this->InputList->GetNextItem());)
      {
      if ( ds->GetDataReleased() )
        {
        ds->ForceUpdate();
        }
      }

    if ( this->StartMethod )
      {
      (*this->StartMethod)(this->StartMethodArg);
      }
    this->Output->Initialize(); //clear output
    // reset AbortExecute flag and Progress
    this->AbortExecute = 0;
    this->Progress = 0.0;
    this->Execute();
    this->ExecuteTime.Modified();
    if ( !this->AbortExecute )
      {
      this->UpdateProgress(1.0);
      }
    this->SetDataReleased(0);
    if ( this->EndMethod )
      {
      (*this->EndMethod)(this->EndMethodArg);
      }
    }

  for (this->InputList->InitTraversal();(ds = this->InputList->GetNextItem());)
    {
    if ( ds->ShouldIReleaseData() )
      {
      ds->ReleaseData();
      }
    }
}

// Interpolate the data
void vtkInterpolateDataSetAttributes::Execute()
{
  int numPts, numCells, numInputs=this->InputList->GetNumberOfItems();
  int i, lowDS, highDS;
  vtkDataSet *ds, *ds2;
  vtkDataSet *output = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  vtkCellData *outputCD = output->GetCellData();
  vtkPointData *inputPD, *input2PD;
  vtkCellData *inputCD, *input2CD;
  float t;
  
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
  if (t > 1.0) t =1.0;
  
  ds = this->InputList->GetItem(lowDS);
  ds2 = this->InputList->GetItem(highDS);

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
  if ( inputPD->GetFieldData() && input2PD->GetFieldData() )
    {
    outputPD->CopyFieldDataOn();
    }
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
  if ( inputCD->GetFieldData() && input2CD->GetFieldData() )
    {
    outputCD->CopyFieldDataOn();
    }
  outputCD->InterpolateAllocate(inputCD);

  // Interpolate point data. We'll assume that it takes 50% of the time
  for ( i=0; i < numPts; i++ )
    {
    if ( ! (i % 10000) ) 
      {
      this->UpdateProgress ((float)i/numPts * 0.50);
      if (this->GetAbortExecute()) break;
      }

    outputPD->InterpolateTime(inputPD, input2PD, i, t);
    }
  
  // Interpolate cell data. We'll assume that it takes 50% of the time
  for ( i=0; i < numCells; i++ )
    {
    if ( ! (i % 10000) ) 
      {
      this->UpdateProgress (0.5 + (float)i/numCells * 0.50);
      if (this->GetAbortExecute()) break;
      }

    outputCD->InterpolateTime(inputCD, input2CD, i, t);
    }
}

// Get the output as vtkPolyData.
vtkPolyData *vtkInterpolateDataSetAttributes::GetPolyDataOutput() 
{
  return this->PolyData;
}

// Get the output as vtkStructuredPoints.
vtkStructuredPoints *vtkInterpolateDataSetAttributes::GetStructuredPointsOutput() 
{
  return this->StructuredPoints;
}

// Get the output as vtkStructuredGrid.
vtkStructuredGrid *vtkInterpolateDataSetAttributes::GetStructuredGridOutput()
{
  return this->StructuredGrid;
}

// Get the output as vtkUnstructuredGrid.
vtkUnstructuredGrid *vtkInterpolateDataSetAttributes::GetUnstructuredGridOutput()
{
  return this->UnstructuredGrid;
}

// Get the output as vtkRectilinearGrid. 
vtkRectilinearGrid *vtkInterpolateDataSetAttributes::GetRectilinearGridOutput()
{
  return this->RectilinearGrid;
}

void vtkInterpolateDataSetAttributes::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkFilter::PrintSelf(os,indent);

  os << indent << "Input Data Sets:\n";
  this->InputList->PrintSelf(os,indent.GetNextIndent());

  os << indent << "T: " << this->T << endl;
}



