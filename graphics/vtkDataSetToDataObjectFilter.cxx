/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetToDataObjectFilter.cxx
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
#include "vtkDataSetToDataObjectFilter.h"
#include "vtkPolyData.h"
#include "vtkStructuredPoints.h"
#include "vtkStructuredGrid.h"
#include "vtkRectilinearGrid.h"
#include "vtkUnstructuredGrid.h"

// Instantiate object.
vtkDataSetToDataObjectFilter::vtkDataSetToDataObjectFilter()
{
  this->Output = vtkDataObject::New();
  this->Output->SetSource(this);

  this->Geometry = 1;
  this->Topology = 1;
  this->PointData = 1;
  this->CellData = 1;
  this->FieldData = 1;
}

void vtkDataSetToDataObjectFilter::Execute()
{
  vtkDataSet *input = (vtkDataSet *)this->GetInput();
  vtkDataObject *output = (vtkDataObject *)this->GetOutput();
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
    if ( input->GetDataSetType() == VTK_POLY_DATA )
      {
      da = ((vtkPolyData *)input)->GetPoints()->GetData();
      fd->SetArray(arrayNum, da);
      fd->SetArrayName(arrayNum++, "Points");
      }

    else if ( input->GetDataSetType() == VTK_STRUCTURED_POINTS )
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
    
    else if ( input->GetDataSetType() == VTK_STRUCTURED_GRID )
      {
      da = ((vtkStructuredGrid *)input)->GetPoints()->GetData();
      fd->SetArray(arrayNum, da);
      fd->SetArrayName(arrayNum++, "Points");
      }
    
    else if ( input->GetDataSetType() == VTK_RECTILINEAR_GRID )
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
    
    else if ( input->GetDataSetType() == VTK_UNSTRUCTURED_GRID )
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
    if ( input->GetDataSetType() == VTK_POLY_DATA )
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

    else if ( input->GetDataSetType() == VTK_STRUCTURED_POINTS )
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
    
    else if ( input->GetDataSetType() == VTK_STRUCTURED_GRID )
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
    
    else if ( input->GetDataSetType() == VTK_RECTILINEAR_GRID )
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
    
    else if ( input->GetDataSetType() == VTK_UNSTRUCTURED_GRID )
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

    if ( scalars=inPD->GetScalars() )
      {
      fd->SetArray(arrayNum, scalars->GetData());
      fd->SetArrayName(arrayNum++, "PointScalars");
      }

    if ( vectors=inPD->GetVectors() )
      {
      fd->SetArray(arrayNum, vectors->GetData());
      fd->SetArrayName(arrayNum++, "PointVectors");
      }

    if ( tensors=inPD->GetTensors() )
      {
      fd->SetArray(arrayNum, tensors->GetData());
      fd->SetArrayName(arrayNum++, "PointTensors");
      }

    if ( normals=inPD->GetNormals() )
      {
      fd->SetArray(arrayNum, normals->GetData());
      fd->SetArrayName(arrayNum++, "PointNormals");
      }

    if ( tcoords=inPD->GetTCoords() )
      {
      fd->SetArray(arrayNum, tcoords->GetData());
      fd->SetArrayName(arrayNum++, "PointTCoords");
      }

    if ( fieldData=inPD->GetFieldData() )
      {
      vtkDataArray *da;
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
    
    if ( scalars=inCD->GetScalars() )
      {
      fd->SetArray(arrayNum, scalars->GetData());
      fd->SetArrayName(arrayNum++, "CellScalars");
      }

    if ( vectors=inCD->GetVectors() )
      {
      fd->SetArray(arrayNum, vectors->GetData());
      fd->SetArrayName(arrayNum++, "CellVectors");
      }

    if ( tensors=inCD->GetTensors() )
      {
      fd->SetArray(arrayNum, tensors->GetData());
      fd->SetArrayName(arrayNum++, "CellTensors");
      }

    if ( normals=inCD->GetNormals() )
      {
      fd->SetArray(arrayNum, normals->GetData());
      fd->SetArrayName(arrayNum++, "CellNormals");
      }

    if ( tcoords=inCD->GetTCoords() )
      {
      fd->SetArray(arrayNum, tcoords->GetData());
      fd->SetArrayName(arrayNum++, "CellTCoords");
      }

    if ( fieldData=inCD->GetFieldData() )
      {
      vtkDataArray *da;
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
  
  this->Output->SetFieldData(fd);
}

void vtkDataSetToDataObjectFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetFilter::PrintSelf(os,indent);

  os << indent << "Geometry: " << (this->Geometry ? "On\n" : "Off\n");
  os << indent << "Topology: " << (this->Topology ? "On\n" : "Off\n");
  os << indent << "Field Data: " << (this->FieldData ? "On\n" : "Off\n");
  os << indent << "Point Data: " << (this->PointData ? "On\n" : "Off\n");
  os << indent << "Cell Data: " << (this->CellData ? "On\n" : "Off\n");
}

