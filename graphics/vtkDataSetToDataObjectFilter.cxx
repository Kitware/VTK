/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetToDataObjectFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkDataSetToDataObjectFilter.h"
#include "vtkPolyData.h"
#include "vtkStructuredPoints.h"
#include "vtkStructuredGrid.h"
#include "vtkRectilinearGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"


//------------------------------------------------------------------------------
vtkDataSetToDataObjectFilter* vtkDataSetToDataObjectFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkDataSetToDataObjectFilter");
  if(ret)
    {
    return (vtkDataSetToDataObjectFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkDataSetToDataObjectFilter;
}


//----------------------------------------------------------------------------
// Instantiate object.
vtkDataSetToDataObjectFilter::vtkDataSetToDataObjectFilter()
{
  this->NumberOfRequiredInputs = 1;
  this->Geometry = 1;
  this->Topology = 1;
  this->PointData = 1;
  this->CellData = 1;
  this->FieldData = 1;
}

vtkDataSetToDataObjectFilter::~vtkDataSetToDataObjectFilter()
{
}

//----------------------------------------------------------------------------
void vtkDataSetToDataObjectFilter::Execute()
{
  vtkDataSet *input = this->GetInput();
  vtkFieldData *fd=vtkFieldData::New();
  vtkDataArray *da;
  int i;
  
  vtkDebugMacro(<<"Generating field data from data set");

  if ( this->Geometry)
    {
    if ( input->GetDataObjectType() == VTK_POLY_DATA )
      {
      da = ((vtkPolyData *)input)->GetPoints()->GetData();
      da->SetName("Points");
      fd->AddArray( da );
      }

    else if ( input->GetDataObjectType() == VTK_STRUCTURED_POINTS )
      {
      vtkStructuredPoints *spts=(vtkStructuredPoints *)input;

      vtkFloatArray *origin=vtkFloatArray::New();
      origin->SetNumberOfValues(3);
      float org[3];
      spts->GetOrigin(org);
      origin->SetValue(0, org[0]);
      origin->SetValue(1, org[1]);
      origin->SetValue(2, org[2]);
      origin->SetName("Origin");
      fd->AddArray(origin);
      origin->Delete();

      vtkFloatArray *spacing=vtkFloatArray::New();
      spacing->SetNumberOfValues(3);
      float sp[3];
      spts->GetSpacing(sp);
      spacing->SetValue(0, sp[0]);
      spacing->SetValue(1, sp[1]);
      spacing->SetValue(2, sp[2]);
      spacing->SetName("Spacing");
      fd->AddArray(spacing);
      spacing->Delete();
      }
    
    else if ( input->GetDataObjectType() == VTK_STRUCTURED_GRID )
      {
      da = ((vtkStructuredGrid *)input)->GetPoints()->GetData();
      da->SetName("Points");
      fd->AddArray(da);
      }
    
    else if ( input->GetDataObjectType() == VTK_RECTILINEAR_GRID )
      {
      vtkRectilinearGrid *rgrid=(vtkRectilinearGrid *)input;
      da = rgrid->GetXCoordinates();
      da->SetName("XCoordinates");
      fd->AddArray( da );
      da = rgrid->GetYCoordinates();
      da->SetName("YCoordinates");
      fd->AddArray( da );
      da = rgrid->GetZCoordinates();
      da->SetName("ZCoordinates");
      fd->AddArray( da );
      }
    
    else if ( input->GetDataObjectType() == VTK_UNSTRUCTURED_GRID )
      {
      da = ((vtkUnstructuredGrid *)input)->GetPoints()->GetData();
      da->SetName("Points");
      fd->AddArray( da );
      }

    else
      {
      vtkErrorMacro(<<"Unsupported dataset type!");
      return;
      }
    }
  
  if (this->Topology)
    {
    if ( input->GetDataObjectType() == VTK_POLY_DATA )
      {
      vtkPolyData *pd=(vtkPolyData *)input;
      vtkCellArray *ca;
      if ( pd->GetVerts()->GetNumberOfCells() > 0 )
        {
        ca = pd->GetVerts();
	ca->GetData()->SetName("Verts");
        fd->AddArray( ca->GetData() );
        }
      if ( pd->GetLines()->GetNumberOfCells() > 0 )
        {
        ca = pd->GetLines();
	ca->GetData()->SetName("Lines");
        fd->AddArray( ca->GetData() );
        }
      if ( pd->GetPolys()->GetNumberOfCells() > 0 )
        {
        ca = pd->GetPolys();
	ca->GetData()->SetName("Polys");
        fd->AddArray( ca->GetData() );
        }
      if ( pd->GetStrips()->GetNumberOfCells() > 0 )
        {
        ca = pd->GetStrips();
	ca->GetData()->SetName("Strips");
        fd->AddArray( ca->GetData() );
        }
      }

    else if ( input->GetDataObjectType() == VTK_STRUCTURED_POINTS )
      {
      vtkIntArray *dimensions=vtkIntArray::New();
      dimensions->SetNumberOfValues(3);
      int dims[3];
      ((vtkStructuredPoints *)input)->GetDimensions(dims);
      dimensions->SetValue(0, dims[0]);
      dimensions->SetValue(1, dims[1]);
      dimensions->SetValue(2, dims[2]);
      dimensions->SetName("Dimensions");
      fd->AddArray( dimensions );
      dimensions->Delete();
      }
    
    else if ( input->GetDataObjectType() == VTK_STRUCTURED_GRID )
      {
      vtkIntArray *dimensions=vtkIntArray::New();
      dimensions->SetNumberOfValues(3);
      int dims[3];
      ((vtkStructuredGrid *)input)->GetDimensions(dims);
      dimensions->SetValue(0, dims[0]);
      dimensions->SetValue(1, dims[1]);
      dimensions->SetValue(2, dims[2]);
      dimensions->SetName("Dimensions");
      fd->AddArray( dimensions );
      dimensions->Delete();
      }
    
    else if ( input->GetDataObjectType() == VTK_RECTILINEAR_GRID )
      {
      vtkIntArray *dimensions=vtkIntArray::New();
      dimensions->SetNumberOfValues(3);
      int dims[3];
      ((vtkRectilinearGrid *)input)->GetDimensions(dims);
      dimensions->SetValue(0, dims[0]);
      dimensions->SetValue(1, dims[1]);
      dimensions->SetValue(2, dims[2]);
      dimensions->SetName("Dimensions");
      fd->AddArray( dimensions );
      dimensions->Delete();
      }
    
    else if ( input->GetDataObjectType() == VTK_UNSTRUCTURED_GRID )
      {
      vtkCellArray *ca=((vtkUnstructuredGrid *)input)->GetCells();
      if ( ca != NULL && ca->GetNumberOfCells() > 0 )
        {
	ca->GetData()->SetName("Cells");
        fd->AddArray( ca->GetData() );

        int numCells=input->GetNumberOfCells();
        vtkIntArray *types=vtkIntArray::New();
        types->SetNumberOfValues(numCells);
        for (i=0; i<numCells; i++)
          {
          types->SetValue(i, input->GetCellType(i));
          }
	types->SetName("CellTypes");
        fd->AddArray( types ); 
	types->Delete();
        }
      }

    else
      {
      vtkErrorMacro(<<"Unsupported dataset type!");
      return;
      }
    }
  
  vtkFieldData* fieldData;

  if (this->FieldData)
    {
    fieldData = input->GetFieldData();
    
    for (i=0; i<fieldData->GetNumberOfArrays(); i++)
      {
      fd->AddArray(fieldData->GetArray(i));
      }
    }

  if (this->PointData)
    {
    fieldData = input->GetPointData();
    
    for (i=0; i<fieldData->GetNumberOfArrays(); i++)
      {
      fd->AddArray(fieldData->GetArray(i));
      }
    }

  if (this->CellData)
    {
    fieldData = input->GetCellData();
    
    for (i=0; i<fieldData->GetNumberOfArrays(); i++)
      {
      fd->AddArray(fieldData->GetArray(i));
      }
    }

  this->GetOutput()->SetFieldData(fd);
  fd->Delete();
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkDataSetToDataObjectFilter::SetInput(vtkDataSet *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkDataSet *vtkDataSetToDataObjectFilter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkDataSet *)(this->Inputs[0]);
}

//----------------------------------------------------------------------------
void vtkDataSetToDataObjectFilter::ComputeInputUpdateExtents(
					    vtkDataObject *vtkNotUsed(output))
{
  if (this->GetInput())
    {
    // what should we do here?
    this->GetInput()->SetUpdateExtent(0, 1, 0);
    this->GetInput()->RequestExactExtentOn();
    }
}


//----------------------------------------------------------------------------
void vtkDataSetToDataObjectFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataObjectSource::PrintSelf(os,indent);

  os << indent << "Geometry: " << (this->Geometry ? "On\n" : "Off\n");
  os << indent << "Topology: " << (this->Topology ? "On\n" : "Off\n");
  os << indent << "Field Data: " << (this->FieldData ? "On\n" : "Off\n");
  os << indent << "Point Data: " << (this->PointData ? "On\n" : "Off\n");
  os << indent << "Cell Data: " << (this->CellData ? "On\n" : "Off\n");
}

