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
  this->Geometry = NULL;
  this->Scalars = NULL;
  this->Vectors = NULL;
  this->Normals = NULL;
  this->TCoords = NULL;
  this->Tensors = NULL;
  this->FieldData = NULL;

  this->Output = NULL;

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

vtkMergeFilter::~vtkMergeFilter()
{
  this->PolyData->Delete();
  this->StructuredPoints->Delete();
  this->StructuredGrid->Delete();
  this->UnstructuredGrid->Delete();
  this->RectilinearGrid->Delete();
  // Output should only be one of the above. We set it to NULL
  // so that we don't free it twice
  this->Output = NULL;
}

void vtkMergeFilter::SetGeometry(vtkDataSet *input)
{
  if ( this->Geometry != input )
    {
    vtkDebugMacro(<<" setting Geometry to " << (void *)input);
    this->Geometry = input;
    this->Modified();
    
    if ( input->GetDataSetType() == VTK_POLY_DATA )
      {
      this->Output = this->PolyData;
      }

    else if ( input->GetDataSetType() == VTK_STRUCTURED_POINTS )
      {
      this->Output = this->StructuredPoints;
      }

    else if ( input->GetDataSetType() == VTK_STRUCTURED_GRID )
      {
      this->Output = this->StructuredGrid;
      }

    else if ( input->GetDataSetType() == VTK_UNSTRUCTURED_GRID )
      {
      this->Output = this->UnstructuredGrid;
      }

    else if ( input->GetDataSetType() == VTK_RECTILINEAR_GRID )
      {
      this->Output = this->RectilinearGrid;
      }

    else
      {
      vtkErrorMacro(<<"Mismatch in data type");
      }

    this->Modified();
    }
}

void vtkMergeFilter::Update()
{
  unsigned long int mtime=0, dsMtime;

  // make sure geometry is defined
  if ( this->Geometry == NULL )
    {
    vtkErrorMacro(<< "No geometry input...can't execute!");
    return;
    }

  // prevent chasing our tail
  if (this->Updating) return;

  this->Updating = 1;
  this->Geometry->Update();
  mtime = this->Geometry->GetMTime();
  
  if ( this->Scalars ) 
    {
    this->Scalars->Update();
    dsMtime = this->Scalars->GetMTime();
    if ( dsMtime > mtime ) mtime = dsMtime;
    }
  if ( this->Vectors )
    {
    this->Vectors->Update();
    dsMtime = this->Vectors->GetMTime();
    if ( dsMtime > mtime ) mtime = dsMtime;
    }
  if ( this->Normals )
    {
    this->Normals->Update();
    dsMtime = this->Normals->GetMTime();
    if ( dsMtime > mtime ) mtime = dsMtime;
    }
  if ( this->TCoords )
    {
    this->TCoords->Update();
    dsMtime = this->TCoords->GetMTime();
    if ( dsMtime > mtime ) mtime = dsMtime;
    }
  if ( this->Tensors )
    {
    this->Tensors->Update();
    dsMtime = this->Tensors->GetMTime();
    if ( dsMtime > mtime ) mtime = dsMtime;
    }
  if ( this->FieldData )
    {
    this->FieldData->Update();
    dsMtime = this->FieldData->GetMTime();
    if ( dsMtime > mtime ) mtime = dsMtime;
    }
  this->Updating = 0;

  if ( mtime > this->ExecuteTime || this->GetMTime() > this->ExecuteTime )
    {
    if ( this->Geometry->GetDataReleased() ) this->Geometry->ForceUpdate();
    if ( this->Scalars && this->Scalars->GetDataReleased() ) 
      this->Scalars->ForceUpdate();
    if ( this->Vectors && this->Vectors->GetDataReleased() ) 
      this->Vectors->ForceUpdate();
    if ( this->Normals && this->Normals->GetDataReleased() ) 
      this->Normals->ForceUpdate();
    if ( this->TCoords && this->TCoords->GetDataReleased() ) 
      this->TCoords->ForceUpdate();
    if ( this->Tensors && this->Tensors->GetDataReleased() ) 
      this->Tensors->ForceUpdate();
    if ( this->FieldData && this->FieldData->GetDataReleased() ) 
      this->FieldData->ForceUpdate();

    if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
    this->Output->Initialize(); //clear output
    // reset AbortExecute flag and Progress
    this->AbortExecute = 0;
    this->Progress = 0.0;
    this->Execute();
    this->ExecuteTime.Modified();
    if ( !this->AbortExecute ) this->UpdateProgress(1.0);
    this->SetDataReleased(0);
    if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
    }
  
  if ( this->Geometry->ShouldIReleaseData() ) this->Geometry->ReleaseData();

  if ( this->Scalars && this->Scalars->ShouldIReleaseData() ) 
    this->Scalars->ReleaseData();

  if ( this->Vectors && this->Vectors->ShouldIReleaseData() ) 
    this->Vectors->ReleaseData();

  if ( this->Normals && this->Normals->ShouldIReleaseData() ) 
    this->Normals->ReleaseData();

  if ( this->TCoords && this->TCoords->ShouldIReleaseData() ) 
    this->TCoords->ReleaseData();

  if ( this->Tensors && this->Tensors->ShouldIReleaseData() ) 
    this->Tensors->ReleaseData();

  if ( this->FieldData && this->FieldData->ShouldIReleaseData() ) 
    this->FieldData->ReleaseData();
}

// Merge it all together
void vtkMergeFilter::Execute()
{
  int numPts, numScalars=0, numVectors=0, numNormals=0, numTCoords=0;
  int numTensors=0, numTuples=0;
  vtkPointData *pd;
  vtkScalars *scalars = NULL;
  vtkVectors *vectors = NULL;
  vtkNormals *normals = NULL;
  vtkTCoords *tcoords = NULL;
  vtkTensors *tensors = NULL;
  vtkFieldData *f = NULL;
  vtkDataSet *output = (vtkDataSet *)this->Output;
  vtkPointData *outputPD = output->GetPointData();
  
  vtkDebugMacro(<<"Merging data!");

  // geometry needs to be copied
  output->CopyStructure(this->Geometry);
  if ( (numPts = this->Geometry->GetNumberOfPoints()) < 1 )
    {
    vtkWarningMacro(<<"Nothing to merge!");
    }
  
  if ( this->Scalars ) 
    {
    pd = this->Scalars->GetPointData();
    scalars = pd->GetScalars();
    if ( scalars != NULL ) numScalars= scalars->GetNumberOfScalars();
    }

  if ( this->Vectors ) 
    {
    pd = this->Vectors->GetPointData();
    vectors = pd->GetVectors();
    if ( vectors != NULL ) numVectors= vectors->GetNumberOfVectors();
    }

  if ( this->Normals ) 
    {
    pd = this->Normals->GetPointData();
    normals = pd->GetNormals();
    if ( normals != NULL ) numNormals= normals->GetNumberOfNormals();
    }

  if ( this->TCoords ) 
    {
    pd = this->TCoords->GetPointData();
    tcoords = pd->GetTCoords();
    if ( tcoords != NULL ) numTCoords= tcoords->GetNumberOfTCoords();
    }

  if ( this->Tensors ) 
    {
    pd = this->Tensors->GetPointData();
    tensors = pd->GetTensors();
    if ( tensors != NULL ) numTensors = tensors->GetNumberOfTensors();
    }

  if ( this->FieldData ) 
    {
    pd = this->FieldData->GetPointData();
    f = pd->GetFieldData();
    if ( f != NULL ) numTuples = f->GetNumberOfTuples();
    }

  // merge data only if it is consistent
  if ( numPts == numScalars )
    outputPD->SetScalars(scalars);

  if ( numPts == numVectors )
    outputPD->SetVectors(vectors);
    
  if ( numPts == numNormals )
    outputPD->SetNormals(normals);

  if ( numPts == numTCoords )
    outputPD->SetTCoords(tcoords);

  if ( numPts == numTensors )
    outputPD->SetTensors(tensors);

  if ( numPts == numTuples )
    outputPD->SetFieldData(f);
}

// Get the output as vtkPolyData.
vtkPolyData *vtkMergeFilter::GetPolyDataOutput() 
{
  return this->PolyData;
}

// Get the output as vtkStructuredPoints.
vtkStructuredPoints *vtkMergeFilter::GetStructuredPointsOutput() 
{
  return this->StructuredPoints;
}

// Get the output as vtkStructuredGrid.
vtkStructuredGrid *vtkMergeFilter::GetStructuredGridOutput()
{
  return this->StructuredGrid;
}

// Get the output as vtkUnstructuredGrid.
vtkUnstructuredGrid *vtkMergeFilter::GetUnstructuredGridOutput()
{
  return this->UnstructuredGrid;
}

// Get the output as vtkRectilinearGrid. 
vtkRectilinearGrid *vtkMergeFilter::GetRectilinearGridOutput()
{
  return this->RectilinearGrid;
}

void vtkMergeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkFilter::PrintSelf(os,indent);

  if ( this->Geometry )
    {
    os << indent << "Geometry: (" << this->Geometry << ")\n";
    os << indent << "Geometry type: " << this->Geometry->GetClassName() << "\n";
    }
  else
    os << indent << "Geometry: (none)\n";

  if ( this->Scalars )
    os << indent << "Scalars: (" << this->Scalars << ")\n";
  else
    os << indent << "Scalars: (none)\n";

  if ( this->Vectors )
    os << indent << "Vectors: (" << this->Vectors << ")\n";
  else
    os << indent << "Vectors: (none)\n";

  if ( this->Normals )
    os << indent << "Normals: (" << this->Normals << ")\n";
  else
    os << indent << "Normals: (none)\n";

  if ( this->TCoords )
    os << indent << "TCoords: (" << this->TCoords << ")\n";
  else
    os << indent << "TCoords: (none)\n";

  if ( this->Tensors )
    os << indent << "Tensors: (" << this->Tensors << ")\n";
  else
    os << indent << "Tensors: (none)\n";

  if ( this->FieldData )
    os << indent << "Field Data: (" << this->FieldData << ")\n";
  else
    os << indent << "Field Data: (none)\n";
}

