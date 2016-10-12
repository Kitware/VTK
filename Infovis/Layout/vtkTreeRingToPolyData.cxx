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
#include "vtkCommand.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkTimerLog.h"
#include "vtkTree.h"
#include "vtkStripper.h"
#include "vtkSectorSource.h"
#include "vtkAppendPolyData.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name)                                  \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

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

  if( inputTree->GetNumberOfVertices() == 0 )
  {
    return 1;
  }

  // Now set the point coordinates, normals, and insert the cell
  vtkDataArray* coordArray = this->GetInputArrayToProcess(0, inputTree);
  if (!coordArray)
  {
    vtkErrorMacro("Sectors array not found.");
    return 0;
  }

  double pt1x[3] = {0.0, 0.0, 0.0};
  double pt2x[3] = {0.0, 0.0, 0.0};
  double ang = 0.0;
  double cos_ang = 0.0;
  double sin_ang = 0.0;
  vtkIdType pt1 = 0;
  vtkIdType pt2 = 0;
  vtkIdType rootId = inputTree->GetRoot();
  VTK_CREATE(vtkCellArray, strips);
  VTK_CREATE(vtkPoints, pts);
  double progress = 0.0;
  this->InvokeEvent(vtkCommand::ProgressEvent, &progress);

  for( int i = 0; i < inputTree->GetNumberOfVertices(); i++ )
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

    double radial_length = coords[3] - coords[2];

    // Calculate the amount of change in the arcs based on the shrink
    // percentage of the arc_length
    double conversion = vtkMath::Pi()/180.;
    double arc_length = (conversion*(coords[1] - coords[0])*coords[3]);
    double radial_shrink = radial_length*this->ShrinkPercentage;
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

    double inner_radius = coords[2] + (0.5*(radial_length*this->ShrinkPercentage));
    double outer_radius = coords[3] - (0.5*(radial_length*this->ShrinkPercentage));
    double start_angle;
    double end_angle;
    if( coords[1] - coords[0] == 360. )
    {
      start_angle = coords[0];
      end_angle = coords[1];
    }
    else
    {
      start_angle = coords[0] + delta_change_each;
      end_angle = coords[1] - delta_change_each;
    }

    int num_angles = static_cast<int>(end_angle - start_angle);
    if ( num_angles < 1 )
    {
      num_angles = 1;
    }
    int num_points = 2*num_angles + 2;
    strips->InsertNextCell(num_points);
    for ( int j = 0; j < num_angles; ++j )
    {
      ang = start_angle + j;
      cos_ang = cos(vtkMath::RadiansFromDegrees(ang));
      sin_ang = sin(vtkMath::RadiansFromDegrees(ang));
      pt1x[0] = cos_ang*inner_radius;
      pt1x[1] = sin_ang*inner_radius;
      pt2x[0] = cos_ang*outer_radius;
      pt2x[1] = sin_ang*outer_radius;
      pt1 = pts->InsertNextPoint(pt1x);
      pt2 = pts->InsertNextPoint(pt2x);
      strips->InsertCellPoint(pt1);
      strips->InsertCellPoint(pt2);
    }
    ang = end_angle;
    cos_ang = cos(vtkMath::RadiansFromDegrees(ang));
    sin_ang = sin(vtkMath::RadiansFromDegrees(ang));
    pt1x[0] = cos_ang*inner_radius;
    pt1x[1] = sin_ang*inner_radius;
    pt2x[0] = cos_ang*outer_radius;
    pt2x[1] = sin_ang*outer_radius;
    pt1 = pts->InsertNextPoint(pt1x);
    pt2 = pts->InsertNextPoint(pt2x);
    strips->InsertCellPoint(pt1);
    strips->InsertCellPoint(pt2);

    if ( i%1000 == 0 )
    {
      progress = static_cast<double>(i) / inputTree->GetNumberOfVertices() * 0.8;
      this->InvokeEvent(vtkCommand::ProgressEvent, &progress);
    }
  }

  outputPoly->SetPoints(pts);
  outputPoly->SetStrips(strips);

  // Pass the input vertex data to the output cell data :)
  vtkDataSetAttributes* const input_vertex_data = inputTree->GetVertexData();
  vtkDataSetAttributes* const output_cell_data = outputPoly->GetCellData();
  output_cell_data->PassData(input_vertex_data);

  return 1;
}

void vtkTreeRingToPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "ShrinkPercentage: " << this->ShrinkPercentage << endl;
}
