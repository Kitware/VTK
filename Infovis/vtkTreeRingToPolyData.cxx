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
#define VTK_CREATE(type, name)                                  \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

vtkCxxRevisionMacro(vtkTreeRingToPolyData, "1.9");
vtkStandardNewMacro(vtkTreeRingToPolyData);

vtkTreeRingToPolyData::vtkTreeRingToPolyData()
{
  this->SetSectorsArrayName("sectors");
  this->ShrinkPercentage = 0.0;
}

vtkTreeRingToPolyData::~vtkTreeRingToPolyData()
{
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
  vtkDataArray* coordArray = this->GetInputArrayToProcess(0, inputTree);
  if (!coordArray)
    {
    vtkErrorMacro("Sectors array not found.");
    return 0;
    }
  VTK_CREATE(vtkAppendPolyData, append);
  
  int i;
  vtkIdType rootId = inputTree->GetRoot();
  for( i = 0; i < inputTree->GetNumberOfVertices(); i++)
    {
    // Grab coords from the input
    double coords[4];
    if( i == rootId )
      {
      //don't draw the root node...
      coords[0] = 0.0;
      coords[1] = 0.0;
      coords[2] = 1.0;
      coords[3] = 1.0;
      }
    else
      {
      coordArray->GetTuple(i,coords);
      }
    
    VTK_CREATE(vtkSectorSource, sector);
    double radial_length = coords[3] - coords[2];
    
    //calculate the amount of change in the arcs based on the shrink 
    // percentage of the arc_length
    double conversion = vtkMath::Pi()/180.;
    double arc_length = (conversion*(coords[1] - coords[0])*coords[3]);
    double radial_shrink = radial_length*this->ShrinkPercentage;
//    double arc_length_shrink = ((radial_length*this->ShrinkPercentage) < (arc_length*this->ShrinkPercentage)) ? (radial_length*this->ShrinkPercentage) : (arc_length*this->ShrinkPercentage);
    double arc_length_shrink;
    if( radial_shrink > 0.25*arc_length )
      {
      arc_length_shrink = 0.25*arc_length;
      }
    else
      {
      arc_length_shrink = radial_shrink;
      }
    
    double arc_length_new = arc_length - arc_length_shrink;
    double angle_change = ((arc_length_new/coords[3])/conversion);
    double delta_change_each = 0.5*((coords[1]-coords[0]) - angle_change);
    
    sector->SetInnerRadius(coords[2] + (0.5*(radial_length*this->ShrinkPercentage)));
    sector->SetOuterRadius(coords[3] - (0.5*(radial_length*this->ShrinkPercentage)));
    
    if( coords[1] - coords[0] == 360. )
      {
      sector->SetStartAngle( coords[0] );
      sector->SetEndAngle( coords[1] );
      }
    else
      {
      sector->SetStartAngle(coords[0] + delta_change_each);
      sector->SetEndAngle(coords[1] - delta_change_each);
      }
    
    int resolution = (int)((coords[1] - coords[0])/1);
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
  
  // Pass the input vertex data to the output cell data :)
  vtkDataSetAttributes* const input_vertex_data = inputTree->GetVertexData();
  vtkDataSetAttributes* const output_cell_data = outputPoly->GetCellData();
  output_cell_data->PassData(input_vertex_data);
#if 0
  int copy_counter = 0;
  output_cell_data->CopyAllocate(input_vertex_data);
  for( i = 0; i < inputTree->GetNumberOfVertices(); i++)
    {
    if( i == rootId )
      {
      //No cell data for the root node, so don't copy it...
      continue;
      }
    
    //copy data from -> to
    output_cell_data->CopyData(input_vertex_data, i, copy_counter++);
    }
#endif
  
  return 1;
}

void vtkTreeRingToPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "ShrinkPercentage: " << this->ShrinkPercentage << endl;
}
