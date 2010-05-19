/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoFileTerrainSource.cxx

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

#include "vtkGeoFileTerrainSource.h"

#include "vtkGeoTerrainNode.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"
#include "vtkXMLPolyDataReader.h"

#include <vtksys/ios/fstream>
#include <vtksys/ios/sstream>
#include <vtksys/stl/utility>

vtkStandardNewMacro(vtkGeoFileTerrainSource);
//----------------------------------------------------------------------------
vtkGeoFileTerrainSource::vtkGeoFileTerrainSource()
{
  this->Path = 0;
}

//----------------------------------------------------------------------------
vtkGeoFileTerrainSource::~vtkGeoFileTerrainSource()
{
  this->SetPath(0);
}

//----------------------------------------------------------------------------
bool vtkGeoFileTerrainSource::FetchRoot(vtkGeoTreeNode* r)
{
  vtkGeoTerrainNode* root = 0;
  if (!(root = vtkGeoTerrainNode::SafeDownCast(r)))
    {
    vtkErrorMacro(<< "Can only fetch terrain nodes from this source.");
    return false;
    }

  this->ReadModel(0, 0, root);
  return true;
}

//----------------------------------------------------------------------------
bool vtkGeoFileTerrainSource::FetchChild(vtkGeoTreeNode* p, int index, vtkGeoTreeNode* c)
{
  vtkGeoTerrainNode* parent = 0;
  if (!(parent = vtkGeoTerrainNode::SafeDownCast(p)))
    {
    vtkErrorMacro(<< "Can only fetch terrain nodes from this source.");
    return false;
    }
  vtkGeoTerrainNode* child = 0;
  if (!(child = vtkGeoTerrainNode::SafeDownCast(c)))
    {
    vtkErrorMacro(<< "Can only fetch terrain nodes from this source.");
    return false;
    }

  int level = parent->GetLevel() + 1;
  int id = parent->GetId() | (index << (2*level-2));
  return this->ReadModel(level, id, child);
}

//----------------------------------------------------------------------------
bool vtkGeoFileTerrainSource::ReadModel(int level, int id, vtkGeoTerrainNode* node)
{
  // Load the image
  node->SetId(id);
  node->SetLevel(level);
  vtkSmartPointer<vtkXMLPolyDataReader> reader = vtkSmartPointer<vtkXMLPolyDataReader>::New();
  vtksys_ios::stringstream ss;
  ss.str("");
  ss << this->Path << "/tile_" << level << "_" << id << ".vtp";

  // Check if the file exists
  vtksys_ios::ifstream in;
  in.open(ss.str().c_str(), vtksys_ios::ifstream::in);
  if (in.fail())
    {
    // Make a dummy polydata
    in.close();
    vtkSmartPointer<vtkPolyData> dummy = vtkSmartPointer<vtkPolyData>::New();
    node->SetModel(dummy);
    return false;
    }
  in.close();

  // Read the file
  reader->SetFileName(ss.str().c_str());
  reader->Update();
  vtkPolyData* model = reader->GetOutput();
  node->SetModel(model);

  double lonRange[2] = {0.0, 0.0};
  double latRange[2] = {0.0, 0.0};
  double xRange[2] = {0.0, 0.0};
  double yRange[2] = {0.0, 0.0};
  if (model->GetNumberOfPoints() > 0)
    {
    model->GetPointData()->GetArray("LatLong")->GetRange(latRange, 0);
    model->GetPointData()->GetArray("LatLong")->GetRange(lonRange, 1);
    model->GetPoints()->GetData()->GetRange(xRange, 0);
    model->GetPoints()->GetData()->GetRange(yRange, 1);
    }
  node->SetLatitudeRange(latRange);
  node->SetLongitudeRange(lonRange);
  node->SetProjectionBounds(xRange[0], xRange[1], yRange[0], yRange[1]);
  node->UpdateBoundingSphere();

  return true;
}

//-----------------------------------------------------------------------------
void vtkGeoFileTerrainSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Path: " << (this->Path ? this->Path : "(none)") << endl;
}
