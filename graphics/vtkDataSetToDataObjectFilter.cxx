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

//----------------------------------------------------------------------------
void vtkDataSetToDataObjectFilter::Execute()
{
  vtkDataSet *input = this->GetInput();
  vtkScalars *scalars;
  vtkVectors *vectors;
  vtkTensors *tensors;
  vtkNormals *normals;
  vtkTCoords *tcoords;
  vtkFieldData *fieldData;
  int arrayNum=0;
  vtkFieldData *fd=vtkFieldData::New();
  vtkDataArray *da;
  
  vtkDebugMacro(<<"Generating field data from data set");

  if ( this->Geometry)
    {
    if ( input->GetDataObjectType() == VTK_POLY_DATA )
      {
      da = ((vtkPolyData *)input)->GetPoints()->GetData();
      fd->SetArray(arrayNum, da);
      fd->SetArrayName(arrayNum++, "Points");
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
      fd->SetArray(arrayNum, origin);
      fd->SetArrayName(arrayNum++, "Origin");
      origin->Delete();

      vtkFloatArray *spacing=vtkFloatArray::New();
      spacing->SetNumberOfValues(3);
      float sp[3];
      spts->GetSpacing(sp);
      spacing->SetValue(0, sp[0]);
      spacing->SetValue(1, sp[1]);
      spacing->SetValue(2, sp[2]);
      fd->SetArray(arrayNum, spacing);
      fd->SetArrayName(arrayNum++, "Spacing");
      spacing->Delete();
      }
    
    else if ( input->GetDataObjectType() == VTK_STRUCTURED_GRID )
      {
      da = ((vtkStructuredGrid *)input)->GetPoints()->GetData();
      fd->SetArray(arrayNum, da);
      fd->SetArrayName(arrayNum++, "Points");
      }
    
    else if ( input->GetDataObjectType() == VTK_RECTILINEAR_GRID )
      {
      vtkRectilinearGrid *rgrid=(vtkRectilinearGrid *)input;
      da = rgrid->GetXCoordinates()->GetData();
      fd->SetArray(arrayNum, da);
      fd->SetArrayName(arrayNum++, "XCoordinates");
      da = rgrid->GetYCoordinates()->GetData();
      fd->SetArray(arrayNum, da);
      fd->SetArrayName(arrayNum++, "YCoordinates");
      da = rgrid->GetZCoordinates()->GetData();
      fd->SetArray(arrayNum, da);
      fd->SetArrayName(arrayNum++, "ZCoordinates");
      }
    
    else if ( input->GetDataObjectType() == VTK_UNSTRUCTURED_GRID )
      {
      da = ((vtkUnstructuredGrid *)input)->GetPoints()->GetData();
      fd->SetArray(arrayNum, da);
      fd->SetArrayName(arrayNum++, "Points");
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
        fd->SetArray(arrayNum, ca->GetData());
        fd->SetArrayName(arrayNum++, "Verts");
        }
      if ( pd->GetLines()->GetNumberOfCells() > 0 )
        {
        ca = pd->GetLines();
        fd->SetArray(arrayNum, ca->GetData());
        fd->SetArrayName(arrayNum++, "Lines");
        }
      if ( pd->GetPolys()->GetNumberOfCells() > 0 )
        {
        ca = pd->GetPolys();
        fd->SetArray(arrayNum, ca->GetData());
        fd->SetArrayName(arrayNum++, "Polys");
        }
      if ( pd->GetStrips()->GetNumberOfCells() > 0 )
        {
        ca = pd->GetStrips();
        fd->SetArray(arrayNum, ca->GetData());
        fd->SetArrayName(arrayNum++, "Strips");
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
      fd->SetArray(arrayNum, dimensions);
      fd->SetArrayName(arrayNum++, "Dimensions");
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
      fd->SetArray(arrayNum, dimensions);
      fd->SetArrayName(arrayNum++, "Dimensions");
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
      fd->SetArray(arrayNum, dimensions);
      fd->SetArrayName(arrayNum++, "Dimensions");
      dimensions->Delete();
      }
    
    else if ( input->GetDataObjectType() == VTK_UNSTRUCTURED_GRID )
      {
      vtkCellArray *ca=((vtkUnstructuredGrid *)input)->GetCells();
      if ( ca != NULL && ca->GetNumberOfCells() > 0 )
        {
        fd->SetArray(arrayNum, ca->GetData());
        fd->SetArrayName(arrayNum++, "Cells");

        int numCells=input->GetNumberOfCells();
        vtkIntArray *types=vtkIntArray::New();
        types->SetNumberOfValues(numCells);
        for (int i=0; i<numCells; i++)
          {
          types->SetValue(i, input->GetCellType(i));
          }
        fd->SetArray(arrayNum, types);
        fd->SetArrayName(arrayNum++, "CellTypes");
	types->Delete();
        }
      }

    else
      {
      vtkErrorMacro(<<"Unsupported dataset type!");
      return;
      }
    }
  
  if (this->FieldData)
    {
    fieldData = input->GetFieldData();
    
    for (int i=0; i<fieldData->GetNumberOfArrays(); i++)
      {
      da = fieldData->GetArray(i);
      fd->SetArray(arrayNum, da);
      fd->SetArrayName(arrayNum++, fieldData->GetArrayName(i));
      }
    }
  
  if (this->PointData)
    {
    vtkPointData *inPD=input->GetPointData();

    if ( (scalars=inPD->GetScalars()) )
      {
      fd->SetArray(arrayNum, scalars->GetData());
      fd->SetArrayName(arrayNum++, "PointScalars");
      }

    if ( (vectors=inPD->GetVectors()) )
      {
      fd->SetArray(arrayNum, vectors->GetData());
      fd->SetArrayName(arrayNum++, "PointVectors");
      }

    if ( (tensors=inPD->GetTensors()) )
      {
      fd->SetArray(arrayNum, tensors->GetData());
      fd->SetArrayName(arrayNum++, "PointTensors");
      }

    if ( (normals=inPD->GetNormals()) )
      {
      fd->SetArray(arrayNum, normals->GetData());
      fd->SetArrayName(arrayNum++, "PointNormals");
      }

    if ( (tcoords=inPD->GetTCoords()) )
      {
      fd->SetArray(arrayNum, tcoords->GetData());
      fd->SetArrayName(arrayNum++, "PointTCoords");
      }

    if ( (fieldData=inPD->GetFieldData()) )
      {
      for (int i=0; i<fieldData->GetNumberOfArrays(); i++)
        {
        da = fieldData->GetArray(i);
        fd->SetArray(arrayNum, da);
        fd->SetArrayName(arrayNum++, fieldData->GetArrayName(i));
        }
      }
    }
  
  if (this->CellData)
    {
    vtkCellData *inCD=input->GetCellData();
    
    if ( (scalars=inCD->GetScalars()) )
      {
      fd->SetArray(arrayNum, scalars->GetData());
      fd->SetArrayName(arrayNum++, "CellScalars");
      }

    if ( (vectors=inCD->GetVectors()) )
      {
      fd->SetArray(arrayNum, vectors->GetData());
      fd->SetArrayName(arrayNum++, "CellVectors");
      }

    if ( (tensors=inCD->GetTensors()) )
      {
      fd->SetArray(arrayNum, tensors->GetData());
      fd->SetArrayName(arrayNum++, "CellTensors");
      }

    if ( (normals=inCD->GetNormals()) )
      {
      fd->SetArray(arrayNum, normals->GetData());
      fd->SetArrayName(arrayNum++, "CellNormals");
      }

    if ( (tcoords=inCD->GetTCoords()) )
      {
      fd->SetArray(arrayNum, tcoords->GetData());
      fd->SetArrayName(arrayNum++, "CellTCoords");
      }

    if ( (fieldData=inCD->GetFieldData()) )
      {
      for (int i=0; i<fieldData->GetNumberOfArrays(); i++)
        {
        da = fieldData->GetArray(i);
        fd->SetArray(arrayNum, da);
        fd->SetArrayName(arrayNum++, fieldData->GetArrayName(i));
        }
      }
    }

  vtkDebugMacro(<<"Created field data with " << fd->GetNumberOfArrays()
                <<"arrays");
  
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

