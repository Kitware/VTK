/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetToDataObjectFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataSetToDataObjectFilter.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredPoints.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkDataSetToDataObjectFilter);

//----------------------------------------------------------------------------
// Instantiate object.
vtkDataSetToDataObjectFilter::vtkDataSetToDataObjectFilter()
{
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
int vtkDataSetToDataObjectFilter::RequestData(
  vtkInformation *,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataObject *output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkFieldData *fd=vtkFieldData::New();
  vtkPoints *pts;
  vtkDataArray *da;
  int i;
  
  vtkDebugMacro(<<"Generating field data from data set");

  if ( this->Geometry)
    {
    if ( input->GetDataObjectType() == VTK_POLY_DATA )
      {
      pts = static_cast<vtkPolyData *>(input)->GetPoints();
      if (pts)
        {
        da = pts->GetData();
        da->SetName("Points");
        fd->AddArray( da );
        }
      }

    else if ( input->GetDataObjectType() == VTK_STRUCTURED_POINTS )
      {
      vtkStructuredPoints *spts=static_cast<vtkStructuredPoints *>(input);

      vtkFloatArray *origin=vtkFloatArray::New();
      origin->SetNumberOfValues(3);
      double org[3];
      spts->GetOrigin(org);
      origin->SetValue(0, org[0]);
      origin->SetValue(1, org[1]);
      origin->SetValue(2, org[2]);
      origin->SetName("Origin");
      fd->AddArray(origin);
      origin->Delete();

      vtkFloatArray *spacing=vtkFloatArray::New();
      spacing->SetNumberOfValues(3);
      double sp[3];
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
      pts = static_cast<vtkStructuredGrid *>(input)->GetPoints();
      if (pts)
        {
        da = pts->GetData();
        da->SetName("Points");
        fd->AddArray( da );
        }
      }
    
    else if ( input->GetDataObjectType() == VTK_RECTILINEAR_GRID )
      {
      vtkRectilinearGrid *rgrid=static_cast<vtkRectilinearGrid *>(input);
      da = rgrid->GetXCoordinates();
      if (da != NULL)
        {
        da->SetName("XCoordinates");
        fd->AddArray( da );
        }
      da = rgrid->GetYCoordinates();
      if (da != NULL)
        {
        da->SetName("YCoordinates");
        fd->AddArray( da );
        }
      da = rgrid->GetZCoordinates();
      if (da != NULL)
        {
        da->SetName("ZCoordinates");
        fd->AddArray( da );
        }
      }
    
    else if ( input->GetDataObjectType() == VTK_UNSTRUCTURED_GRID )
      {
      pts = static_cast<vtkUnstructuredGrid *>(input)->GetPoints();
      if (pts)
        {
        da = pts->GetData();
        da->SetName("Points");
        fd->AddArray( da );
        }
      }

    else
      {
      vtkErrorMacro(<<"Unsupported dataset type!");
      fd->Delete();
      return 1;
      }
    }
  
  if (this->Topology)
    {
    if ( input->GetDataObjectType() == VTK_POLY_DATA )
      {
      vtkPolyData *pd=static_cast<vtkPolyData *>(input);
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
      static_cast<vtkStructuredPoints *>(input)->GetDimensions(dims);
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
      static_cast<vtkStructuredGrid *>(input)->GetDimensions(dims);
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
      static_cast<vtkRectilinearGrid *>(input)->GetDimensions(dims);
      dimensions->SetValue(0, dims[0]);
      dimensions->SetValue(1, dims[1]);
      dimensions->SetValue(2, dims[2]);
      dimensions->SetName("Dimensions");
      fd->AddArray( dimensions );
      dimensions->Delete();
      }
    
    else if ( input->GetDataObjectType() == VTK_UNSTRUCTURED_GRID )
      {
      vtkCellArray *ca=static_cast<vtkUnstructuredGrid *>(input)->GetCells();
      if ( ca != NULL && ca->GetNumberOfCells() > 0 )
        {
        ca->GetData()->SetName("Cells");
        fd->AddArray( ca->GetData() );

        vtkIdType numCells=input->GetNumberOfCells();
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
      fd->Delete();
      return 1;
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

  output->SetFieldData(fd);
  fd->Delete();
  return 1;
}

//----------------------------------------------------------------------------
int vtkDataSetToDataObjectFilter::RequestUpdateExtent(
  vtkInformation *,
  vtkInformationVector **inputVector,
  vtkInformationVector *)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
              0);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

  return 1;
}

//----------------------------------------------------------------------------
int vtkDataSetToDataObjectFilter::FillInputPortInformation(int port,
                                                           vtkInformation* info)
{
  if(!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkDataSetToDataObjectFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Geometry: " << (this->Geometry ? "On\n" : "Off\n");
  os << indent << "Topology: " << (this->Topology ? "On\n" : "Off\n");
  os << indent << "Field Data: " << (this->FieldData ? "On\n" : "Off\n");
  os << indent << "Point Data: " << (this->PointData ? "On\n" : "Off\n");
  os << indent << "Cell Data: " << (this->CellData ? "On\n" : "Off\n");
}
