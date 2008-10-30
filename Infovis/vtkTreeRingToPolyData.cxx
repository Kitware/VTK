/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeRingToPolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "vtkTreeRingToPolyData.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkTree.h"
#include "vtkStripper.h"
#include "vtkSectorSource.h"
#include "vtkAppendPolyData.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

vtkCxxRevisionMacro(vtkTreeRingToPolyData, "1.3");
vtkStandardNewMacro(vtkTreeRingToPolyData);

vtkTreeRingToPolyData::vtkTreeRingToPolyData()
{
  this->SectorsFieldName = 0;
  this->SetSectorsFieldName("sectors");
  this->ShrinkPercentage = 0.0;
}

vtkTreeRingToPolyData::~vtkTreeRingToPolyData()
{
  this->SetSectorsFieldName(0);
}

int vtkTreeRingToPolyData::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTree");
  return 1;
}

int vtkTreeRingToPolyData::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkTree *inputTree = vtkTree::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *outputPoly = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Now set the point coordinates, normals, and insert the cell
  vtkDataArray *coordArray = inputTree->GetVertexData()->GetArray(this->SectorsFieldName);
  VTK_CREATE(vtkAppendPolyData, append);
  
  for (int i = 0; i < inputTree->GetNumberOfVertices(); i++)
  {
    if(i==0)
    {
        //don't draw the root node...
      continue;
    }
    
    // Grab coords from the input
    double coords[4];
    coordArray->GetTuple(i,coords);

    VTK_CREATE(vtkSectorSource, sector);
    double radial_length = coords[1] - coords[0];
    
      //calculate the amount of change in the arcs based on the shrink 
      // percentage of the arc_length
    double conversion = vtkMath::Pi()/180.;
    double arc_length_new = (conversion*(coords[3] - coords[2])*coords[1]) - this->ShrinkPercentage;
    double angle_change = ((arc_length_new/coords[1])/conversion);
    double delta_change_each = 0.5*((coords[3]-coords[2]) - angle_change);
    
    sector->SetInnerRadius(coords[0] + (0.5*(radial_length*this->ShrinkPercentage)));
    sector->SetOuterRadius(coords[1] - (0.5*(radial_length*this->ShrinkPercentage)));
    sector->SetStartAngle(coords[2] + delta_change_each);
    sector->SetEndAngle(coords[3] - delta_change_each);

//FIXME-jfsheph - need to determine the desired resolution....
    int resolution = (int)((coords[3] - coords[2])/1);
    if( resolution < 1 )
        resolution = 1;
    sector->SetCircumferentialResolution(resolution);
    sector->Update();
    
    VTK_CREATE(vtkStripper, strip);
    strip->SetInput(sector->GetOutput());
    
    append->AddInput(strip->GetOutput());
  }
  
  append->Update();
  outputPoly->ShallowCopy(append->GetOutput());
  
    // Pass the input point data to the output cell data :)
  vtkDataSetAttributes* const input_vertex_data = inputTree->GetVertexData();
  vtkDataSetAttributes* const output_cell_data = outputPoly->GetCellData();
  output_cell_data->CopyAllocate(input_vertex_data);
  for (int i = 0; i < inputTree->GetNumberOfVertices(); i++)
  {
    if( i == 0 )
    {
        //No cell data for the root node, so don't copy it...
      continue;
    }
    
      //copy data from -> to
    output_cell_data->CopyData(input_vertex_data, i, i-1);
  }
  
  return 1;
}

void vtkTreeRingToPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "SectorsFieldName: " << (this->SectorsFieldName ? this->SectorsFieldName : "(none)") << endl;
  os << indent << "ShrinkPercentage: " << this->ShrinkPercentage << endl;
}
