/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCastToConcrete.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkCastToConcrete.h"

// Description:
// Construct object.
vtkCastToConcrete::vtkCastToConcrete()
{
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

vtkCastToConcrete::~vtkCastToConcrete()
{
  this->PolyData->Delete();
  this->StructuredPoints->Delete();
  this->StructuredGrid->Delete();
  this->UnstructuredGrid->Delete();
  this->RectilinearGrid->Delete();
  this->Output = NULL;
}

// Description:
// Special method just passes Update through pipeline.
void vtkCastToConcrete::Update()
{
  // make sure input is available
  if ( !this->Input )
    {
    vtkErrorMacro(<< "No input...can't execute!");
    return;
    }

  // prevent chasing our tail
  if (this->Updating) return;

  this->Updating = 1;
  this->Input->Update();
  this->Updating = 0;

  if (this->Input->GetMTime() > this->ExecuteTime ||
  this->GetMTime() > this->ExecuteTime )
    {
    if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
    this->Execute();
    this->ExecuteTime.Modified();
    if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
    }
}
  
void vtkCastToConcrete::Execute()
{
  vtkDebugMacro(<<"Casting to concrete type...");

  if ( ! strcmp(this->Input->GetDataType(),"vtkPolyData") )
    {
    this->PolyData->CopyStructure(this->Input);
    this->PolyData->GetPointData()->PassData(this->Input->GetPointData());
    }

  else if ( ! strcmp(this->Input->GetDataType(),"vtkStructuredPoints") )
    {
    this->StructuredPoints->CopyStructure(this->Input);
    this->StructuredPoints->GetPointData()->PassData(this->Input->GetPointData());
    }

  else if ( ! strcmp(this->Input->GetDataType(),"vtkStructuredGrid") )
    {
    this->StructuredGrid->CopyStructure(this->Input);
    this->StructuredGrid->GetPointData()->PassData(this->Input->GetPointData());
    }

  else if ( ! strcmp(this->Input->GetDataType(),"vtkUnstructuredGrid") )
    {
    this->UnstructuredGrid->CopyStructure(this->Input);
    this->UnstructuredGrid->GetPointData()->PassData(this->Input->GetPointData());
    }

  else if ( ! strcmp(this->Input->GetDataType(),"vtkRectilinearGrid") )
    {
    this->RectilinearGrid->CopyStructure(this->Input);
    this->RectilinearGrid->GetPointData()->PassData(this->Input->GetPointData());
    }

  else
    {
    this->Output = this->Input;
    }
}

// Description:
// Get the output of this filter. If output is NULL then input hasn't been set
// which is necessary for abstract objects.
vtkDataSet *vtkCastToConcrete::GetOutput()
{
  if ( this->Input == NULL )
    {
    vtkErrorMacro(<<"Filter requires input to be set before output can be retrieved");
    }
  return this->Input;
}

// Description:
// Get the output of this filter as type vtkPolyData. Performs run-time
// checking on type. Returns NULL if wrong type.
vtkPolyData *vtkCastToConcrete::GetPolyDataOutput()
{
  if ( this->Input == NULL )
    {
    vtkErrorMacro(<<"Filter requires input to be set before output can be retrieved");
    }
  else
    {
    if ( strcmp(this->Input->GetDataType(),"vtkPolyData") )
      {
      vtkErrorMacro(<<"Cannot cast to type requested");
      return NULL;
      }
    }

  return this->PolyData;
}

// Description:
// Get the output of this filter as type vtkStructuredPoints. Performs run-time
// checking on type. Returns NULL if wrong type.
vtkStructuredPoints *vtkCastToConcrete::GetStructuredPointsOutput()
{
  if ( this->Input == NULL )
    {
    vtkErrorMacro(<<"Filter requires input to be set before output can be retrieved");
    }
  else
    {
    if ( strcmp(this->Input->GetDataType(),"vtkStructuredPoints") )
      {
      vtkErrorMacro(<<"Cannot cast to type requested");
      return NULL;
      }
    }

  return this->StructuredPoints;
}

// Description:
// Get the output of this filter as type vtkStructuredGrid. Performs run-time
// checking on type. Returns NULL if wrong type.
vtkStructuredGrid *vtkCastToConcrete::GetStructuredGridOutput()
{
  if ( this->Input == NULL )
    {
    vtkErrorMacro(<<"Filter requires input to be set before output can be retrieved");
    }
  else
    {
    if ( strcmp(this->Input->GetDataType(),"vtkStructuredGrid") )
      {
      vtkErrorMacro(<<"Cannot cast to type requested");
      return NULL;
      }
    }

  return this->StructuredGrid;
}

// Description:
// Get the output of this filter as type vtkUnstructuredGrid. Performs run-time
// checking on type. Returns NULL if wrong type.
vtkUnstructuredGrid *vtkCastToConcrete::GetUnstructuredGridOutput()
{
  if ( this->Input == NULL )
    {
    vtkErrorMacro(<<"Filter requires input to be set before output can be retrieved");
    }
  else
    {
    if ( strcmp(this->Input->GetDataType(),"vtkUnstructuredGrid") )
      {
      vtkErrorMacro(<<"Cannot cast to type requested");
      return NULL;
      }
    }

  return this->UnstructuredGrid;
}

// Description:
// Get the output of this filter as type vtkUnstructuredGrid. Performs run-time
// checking on type. Returns NULL if wrong type.
vtkRectilinearGrid *vtkCastToConcrete::GetRectilinearGridOutput()
{
  if ( this->Input == NULL )
    {
    vtkErrorMacro(<<"Filter requires input to be set before output can be retrieved");
    }
  else
    {
    if ( strcmp(this->Input->GetDataType(),"vtkRectilinearGrid") )
      {
      vtkErrorMacro(<<"Cannot cast to type requested");
      return NULL;
      }
    }

  return this->RectilinearGrid;
}

