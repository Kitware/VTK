/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergeFilter.cxx
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
#include "vtkMergeFilter.h"
#include "vtkPolyData.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredPoints.h"
#include "vtkUnstructuredGrid.h"
#include "vtkRectilinearGrid.h"

// Create object with no input or output.
vtkMergeFilter::vtkMergeFilter()
{
}

vtkMergeFilter::~vtkMergeFilter()
{
}

void vtkMergeFilter::SetScalars(vtkDataSet *input)
{
  this->vtkProcessObject::SetInput(1, input);
}
vtkDataSet *vtkMergeFilter::GetScalars()
{
  if (this->NumberOfInputs < 2)
    {
    return NULL;
    }
  return (vtkDataSet *)(this->Inputs[1]);
}

void vtkMergeFilter::SetVectors(vtkDataSet *input)
{
  this->vtkProcessObject::SetInput(2, input);
}
vtkDataSet *vtkMergeFilter::GetVectors()
{
  if (this->NumberOfInputs < 3)
    {
    return NULL;
    }
  return (vtkDataSet *)(this->Inputs[3]);
}

void vtkMergeFilter::SetNormals(vtkDataSet *input)
{
  this->vtkProcessObject::SetInput(3, input);
}
vtkDataSet *vtkMergeFilter::GetNormals()
{
  if (this->NumberOfInputs < 4)
    {
    return NULL;
    }
  return (vtkDataSet *)(this->Inputs[3]);
}

void vtkMergeFilter::SetTCoords(vtkDataSet *input)
{
  this->vtkProcessObject::SetInput(4, input);
}
vtkDataSet *vtkMergeFilter::GetTCoords()
{
  if (this->NumberOfInputs < 5)
    {
    return NULL;
    }
  return (vtkDataSet *)(this->Inputs[4]);
}

void vtkMergeFilter::SetTensors(vtkDataSet *input)
{
  this->vtkProcessObject::SetInput(5, input);
}
vtkDataSet *vtkMergeFilter::GetTensors()
{
  if (this->NumberOfInputs < 6)
    {
    return NULL;
    }
  return (vtkDataSet *)(this->Inputs[5]);
}

void vtkMergeFilter::SetFieldData(vtkDataSet *input)
{
  this->vtkProcessObject::SetInput(6, input);
}
vtkDataSet *vtkMergeFilter::GetFieldData()
{
  if (this->NumberOfInputs < 7)
    {
    return NULL;
    }
  return (vtkDataSet *)(this->Inputs[6]);
}


void vtkMergeFilter::Execute()
{
  int numPts, numScalars=0, numVectors=0, numNormals=0, numTCoords=0;
  int numTensors=0, numTuples=0;
  int numCells, numCellScalars=0, numCellVectors=0, numCellNormals=0;
  int numCellTCoords=0, numCellTensors=0, numCellTuples=0;
  vtkPointData *pd;
  vtkScalars *scalars = NULL;
  vtkVectors *vectors = NULL;
  vtkNormals *normals = NULL;
  vtkTCoords *tcoords = NULL;
  vtkTensors *tensors = NULL;
  vtkFieldData *f = NULL;
  vtkCellData *cd;
  vtkScalars *cellScalars = NULL;
  vtkVectors *cellVectors = NULL;
  vtkNormals *cellNormals = NULL;
  vtkTCoords *cellTCoords = NULL;
  vtkTensors *cellTensors = NULL;
  vtkFieldData *cellf = NULL;
  vtkDataSet *output = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  vtkCellData *outputCD = output->GetCellData();
  
  vtkDebugMacro(<<"Merging data!");

  // geometry needs to be copied
  output->CopyStructure(this->GetInput());
  if ( (numPts = this->GetInput()->GetNumberOfPoints()) < 1 )
    {
    vtkWarningMacro(<<"Nothing to merge!");
    }
  numCells = this->GetInput()->GetNumberOfCells();
  
  if ( this->GetScalars() ) 
    {
    pd = this->GetScalars()->GetPointData();
    scalars = pd->GetScalars();
    if ( scalars != NULL )
      {
      numScalars = scalars->GetNumberOfScalars();
      }
    cd = this->GetScalars()->GetCellData();
    cellScalars = cd->GetScalars();
    if ( cellScalars != NULL )
      {
      numCellScalars = cellScalars->GetNumberOfScalars();
      }
    }

  if ( this->GetVectors() ) 
    {
    pd = this->GetVectors()->GetPointData();
    vectors = pd->GetVectors();
    if ( vectors != NULL )
      {
      numVectors= vectors->GetNumberOfVectors();
      }
    cd = this->GetVectors()->GetCellData();
    cellVectors = cd->GetVectors();
    if ( cellVectors != NULL )
      {
      numCellVectors = cellVectors->GetNumberOfVectors();
      }
    }

  if ( this->GetNormals() ) 
    {
    pd = this->GetNormals()->GetPointData();
    normals = pd->GetNormals();
    if ( normals != NULL )
      {
      numNormals= normals->GetNumberOfNormals();
      }
    cd = this->GetNormals()->GetCellData();
    cellNormals = cd->GetNormals();
    if ( cellNormals != NULL )
      {
      numCellNormals = cellNormals->GetNumberOfNormals();
      }
    }

  if ( this->GetTCoords() ) 
    {
    pd = this->GetTCoords()->GetPointData();
    tcoords = pd->GetTCoords();
    if ( tcoords != NULL )
      {
      numTCoords= tcoords->GetNumberOfTCoords();
      }
    cd = this->GetTCoords()->GetCellData();
    cellTCoords = cd->GetTCoords();
    if ( cellTCoords != NULL )
      {
      numCellTCoords = cellTCoords->GetNumberOfTCoords();
      }
    }

  if ( this->GetTensors() ) 
    {
    pd = this->GetTensors()->GetPointData();
    tensors = pd->GetTensors();
    if ( tensors != NULL )
      {
      numTensors = tensors->GetNumberOfTensors();
      }
    cd = this->GetTensors()->GetCellData();
    cellTensors = cd->GetTensors();
    if ( cellTensors != NULL )
      {
      numCellTensors = cellTensors->GetNumberOfTensors();
      }
    }

  if ( this->GetFieldData() ) 
    {
    pd = this->GetFieldData()->GetPointData();
    f = pd->GetFieldData();
    if ( f != NULL )
      {
      numTuples = f->GetNumberOfTuples();
      }
    cd = this->GetFieldData()->GetCellData();
    cellf = cd->GetFieldData();
    if ( cellf != NULL )
      {
      numCellTuples = cellf->GetNumberOfTuples();
      }
    }

  // merge data only if it is consistent
  if ( numPts == numScalars )
    {
    outputPD->SetScalars(scalars);
    }
  if ( numCells == numCellScalars )
    {
    outputCD->SetScalars(cellScalars);
    }

  if ( numPts == numVectors )
    {
    outputPD->SetVectors(vectors);
    }
  if ( numCells == numCellVectors )
    {
    outputCD->SetVectors(cellVectors);
    }
    
  if ( numPts == numNormals )
    {
    outputPD->SetNormals(normals);
    }
  if ( numCells == numCellNormals )
    {
    outputCD->SetNormals(cellNormals);
    }

  if ( numPts == numTCoords )
    {
    outputPD->SetTCoords(tcoords);
    }
  if ( numCells == numCellTCoords )
    {
    outputCD->SetTCoords(cellTCoords);
    }

  if ( numPts == numTensors )
    {
    outputPD->SetTensors(tensors);
    }
  if ( numCells == numCellTensors )
    {
    outputCD->SetTensors(cellTensors);
    }

  if ( numPts == numTuples )
    {
    outputPD->SetFieldData(f);
    }
  if ( numCells == numCellTuples )
    {
    outputCD->SetFieldData(cellf);
    }
}

//----------------------------------------------------------------------------
//  Trick:  Abstract data types that may or may not be the same type
// (structured/unstructured), but the points/cells match up.
// Output/Geometry may be structured while ScalarInput may be 
// unstructured (but really have same triagulation/topology as geometry).
// Just request al the input. Always generate all of the output (todo).
int vtkMergeFilter::ComputeInputUpdateExtents(vtkDataObject *data)
{
  vtkDataSet *input;
  int idx;
  
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    input = (vtkDataSet *)(this->Inputs[idx]);
    if (input)
      {
      input->SetUpdateExtent(0, 1);
      }
    }
  
  return 1;
}

void vtkMergeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

}

