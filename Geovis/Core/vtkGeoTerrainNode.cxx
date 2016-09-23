/*=============================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoTerrainNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=============================================================================*/

/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/


#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkMath.h"
#include "vtkGeoMath.h"
#include "vtkGeoTerrainNode.h"

vtkStandardNewMacro(vtkGeoTerrainNode);


//----------------------------------------------------------------------------
vtkGeoTerrainNode::vtkGeoTerrainNode()
{
  this->Model = vtkSmartPointer<vtkPolyData>::New();
  this->BoundingSphereRadius = 0.0;

  for (int idx = 0; idx < 3; ++idx)
  {
    this->BoundingSphereCenter[idx] = 0.0;
    this->CornerNormal00[idx] = 0.0;
    this->CornerNormal01[idx] = 0.0;
    this->CornerNormal10[idx] = 0.0;
    this->CornerNormal11[idx] = 0.0;
  }
  this->ProjectionBounds[0] = 0.0;
  this->ProjectionBounds[1] = 0.0;
  this->ProjectionBounds[2] = 0.0;
  this->ProjectionBounds[3] = 0.0;
  this->GraticuleLevel = 0;
  this->Error = 0.0;
  this->Coverage = 0.0;
}

//-----------------------------------------------------------------------------
vtkGeoTerrainNode::~vtkGeoTerrainNode()
{
}

//-----------------------------------------------------------------------------
void vtkGeoTerrainNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "BoundingSphereCenter: "
    << this->BoundingSphereCenter[0] << ", "
    << this->BoundingSphereCenter[1] << ", "
    << this->BoundingSphereCenter[2] << "\n";
  os << indent << "BoundingSphereRadius: "
    << this->BoundingSphereRadius << "\n";

  os << indent << "CornerNormal00: "
    << this->CornerNormal00[0] << ", "
    << this->CornerNormal00[1] << ", "
    << this->CornerNormal00[2] << "\n";
  os << indent << "CornerNormal01: "
    << this->CornerNormal01[0] << ", "
    << this->CornerNormal01[1] << ", "
    << this->CornerNormal01[2] << "\n";
  os << indent << "CornerNormal10: "
    << this->CornerNormal10[0] << ", "
    << this->CornerNormal10[1] << ", "
    << this->CornerNormal10[2] << "\n";
  os << indent << "CornerNormal11: "
    << this->CornerNormal11[0] << ", "
    << this->CornerNormal11[1] << ", "
    << this->CornerNormal11[2] << "\n";
  os << indent << "ProjectionBounds: "
    << this->ProjectionBounds[0] << ", "
    << this->ProjectionBounds[1] << ", "
    << this->ProjectionBounds[2] << ", "
    << this->ProjectionBounds[3] << "\n";
  os << indent << "GraticuleLevel: " << this->GraticuleLevel << endl;
  os << indent << "Error: " << this->Error << endl;
  os << indent << "Coverage: " << this->Coverage << endl;
}

//-----------------------------------------------------------------------------
void vtkGeoTerrainNode::SetModel(vtkPolyData* model)
{
  this->Model = model;
}

//-----------------------------------------------------------------------------
vtkPolyData* vtkGeoTerrainNode::GetModel()
{
  return this->Model;
}

//-----------------------------------------------------------------------------
double vtkGeoTerrainNode::GetAltitude(double, double)
{
  return 0;
}

//-----------------------------------------------------------------------------
// This might not work if Longitude range is over 180, but that is OK.
//  Highest level is half the globe.
void vtkGeoTerrainNode::UpdateBoundingSphere()
{
  // Compute the corners.
  double* c00 = this->CornerNormal00;
  double* c01 = this->CornerNormal01;
  double* c10 = this->CornerNormal10;
  double* c11 = this->CornerNormal11;
  double longitudeLatitudeAltitude[3];

  longitudeLatitudeAltitude[0] = this->LongitudeRange[0];
  longitudeLatitudeAltitude[1] = this->LatitudeRange[0];
  longitudeLatitudeAltitude[2]
    = this->GetAltitude(this->LongitudeRange[0], this->LatitudeRange[0]);
  vtkGeoMath::LongLatAltToRect(longitudeLatitudeAltitude, c00);

  longitudeLatitudeAltitude[0] = this->LongitudeRange[1];
  longitudeLatitudeAltitude[1] = this->LatitudeRange[0];
  longitudeLatitudeAltitude[2]
    = this->GetAltitude(this->LongitudeRange[1], this->LatitudeRange[0]);
  vtkGeoMath::LongLatAltToRect(longitudeLatitudeAltitude, c01);

  longitudeLatitudeAltitude[0] = this->LongitudeRange[0];
  longitudeLatitudeAltitude[1] = this->LatitudeRange[1];
  longitudeLatitudeAltitude[2]
    = this->GetAltitude(this->LongitudeRange[0], this->LatitudeRange[1]);
  vtkGeoMath::LongLatAltToRect(longitudeLatitudeAltitude, c10);

  longitudeLatitudeAltitude[0] = this->LongitudeRange[1];
  longitudeLatitudeAltitude[1] = this->LatitudeRange[1];
  longitudeLatitudeAltitude[2]
    = this->GetAltitude(this->LongitudeRange[1], this->LatitudeRange[1]);
  vtkGeoMath::LongLatAltToRect(longitudeLatitudeAltitude, c11);

  // Average the points to get a center for a bounding sphere.
  this->BoundingSphereCenter[0] = 0.25 * (c00[0]+c01[0]+c10[0]+c11[0]);
  this->BoundingSphereCenter[1] = 0.25 * (c00[1]+c01[1]+c10[1]+c11[1]);
  this->BoundingSphereCenter[2] = 0.25 * (c00[2]+c01[2]+c10[2]+c11[2]);

  double distance;
  distance = vtkGeoMath::DistanceSquared(this->BoundingSphereCenter,c00);
  this->BoundingSphereRadius = distance;
  distance = vtkGeoMath::DistanceSquared(this->BoundingSphereCenter,c01);
  if (this->BoundingSphereRadius < distance)
  {
    this->BoundingSphereRadius = distance;
  }
  distance = vtkGeoMath::DistanceSquared(this->BoundingSphereCenter,c10);
  if (this->BoundingSphereRadius < distance)
  {
    this->BoundingSphereRadius = distance;
  }
  distance = vtkGeoMath::DistanceSquared(this->BoundingSphereCenter,c11);
  if (this->BoundingSphereRadius < distance)
  {
    this->BoundingSphereRadius = distance;
  }
  this->BoundingSphereRadius = sqrt(this->BoundingSphereRadius);

  // Change corners to normals (c00 points to normal ivar)
  vtkMath::Normalize(c00);
  vtkMath::Normalize(c01);
  vtkMath::Normalize(c10);
  vtkMath::Normalize(c11);
}


//-----------------------------------------------------------------------------
vtkGeoTerrainNode* vtkGeoTerrainNode::GetChild(int idx)
{
  if (idx < 0 || idx > 3)
  {
    vtkErrorMacro("Index out of range.");
    return 0;
  }
  return vtkGeoTerrainNode::SafeDownCast(this->Children[idx]);
}


//-----------------------------------------------------------------------------
vtkGeoTerrainNode* vtkGeoTerrainNode::GetParent()
{
  return vtkGeoTerrainNode::SafeDownCast(this->Parent);
}

//-----------------------------------------------------------------------------
void vtkGeoTerrainNode::ShallowCopy(vtkGeoTreeNode *src)
{
  vtkGeoTerrainNode *terrainNode = vtkGeoTerrainNode::SafeDownCast(src);

  if(terrainNode != NULL)
  {
    vtkPolyData * polyData = vtkPolyData::New();
    polyData->ShallowCopy(terrainNode->Model);
    this->SetModel(polyData);
    polyData->Delete();
    polyData = NULL;
    //this->Model = terrainNode->Model;
    this->BoundingSphereRadius = terrainNode->BoundingSphereRadius;
    this->BoundingSphereCenter[0] = terrainNode->BoundingSphereCenter[0];
    this->BoundingSphereCenter[1] = terrainNode->BoundingSphereCenter[1];

    this->CornerNormal00[0] = terrainNode->CornerNormal00[0];
    this->CornerNormal00[1] = terrainNode->CornerNormal00[1];
    this->CornerNormal00[2] = terrainNode->CornerNormal00[2];

    this->CornerNormal01[0] = terrainNode->CornerNormal01[0];
    this->CornerNormal01[1] = terrainNode->CornerNormal01[1];
    this->CornerNormal01[2] = terrainNode->CornerNormal01[2];

    this->CornerNormal10[0] = terrainNode->CornerNormal10[0];
    this->CornerNormal10[1] = terrainNode->CornerNormal10[1];
    this->CornerNormal10[2] = terrainNode->CornerNormal10[2];

    this->CornerNormal11[0] = terrainNode->CornerNormal11[0];
    this->CornerNormal11[1] = terrainNode->CornerNormal11[1];
    this->CornerNormal11[2] = terrainNode->CornerNormal11[2];

    this->ProjectionBounds[0] = terrainNode->ProjectionBounds[0];
    this->ProjectionBounds[1] = terrainNode->ProjectionBounds[1];
    this->ProjectionBounds[2] = terrainNode->ProjectionBounds[2];
    this->ProjectionBounds[3] = terrainNode->ProjectionBounds[3];

    this->GraticuleLevel = terrainNode->GraticuleLevel;
    this->Error = terrainNode->Error;
    this->Coverage = terrainNode->Coverage;
  }
  this->Superclass::ShallowCopy(src);
}

//-----------------------------------------------------------------------------
void vtkGeoTerrainNode::DeepCopy(vtkGeoTreeNode *src)
{
  vtkGeoTerrainNode *terrainNode = vtkGeoTerrainNode::SafeDownCast(src);

  if(terrainNode != NULL)
  {
    vtkPolyData * polyData = vtkPolyData::New();
    polyData->DeepCopy(terrainNode->Model);
    this->SetModel(polyData);
    polyData->Delete();
    polyData = NULL;

    //this->Model = terrainNode->Model;
    this->BoundingSphereRadius = terrainNode->BoundingSphereRadius;
    this->BoundingSphereCenter[0] = terrainNode->BoundingSphereCenter[0];
    this->BoundingSphereCenter[1] = terrainNode->BoundingSphereCenter[1];

    this->CornerNormal00[0] = terrainNode->CornerNormal00[0];
    this->CornerNormal00[1] = terrainNode->CornerNormal00[1];
    this->CornerNormal00[2] = terrainNode->CornerNormal00[2];

    this->CornerNormal01[0] = terrainNode->CornerNormal01[0];
    this->CornerNormal01[1] = terrainNode->CornerNormal01[1];
    this->CornerNormal01[2] = terrainNode->CornerNormal01[2];

    this->CornerNormal10[0] = terrainNode->CornerNormal10[0];
    this->CornerNormal10[1] = terrainNode->CornerNormal10[1];
    this->CornerNormal10[2] = terrainNode->CornerNormal10[2];

    this->CornerNormal11[0] = terrainNode->CornerNormal11[0];
    this->CornerNormal11[1] = terrainNode->CornerNormal11[1];
    this->CornerNormal11[2] = terrainNode->CornerNormal11[2];

    this->ProjectionBounds[0] = terrainNode->ProjectionBounds[0];
    this->ProjectionBounds[1] = terrainNode->ProjectionBounds[1];
    this->ProjectionBounds[2] = terrainNode->ProjectionBounds[2];
    this->ProjectionBounds[3] = terrainNode->ProjectionBounds[3];

    this->GraticuleLevel = terrainNode->GraticuleLevel;
    this->Error = terrainNode->Error;
    this->Coverage = terrainNode->Coverage;
  }

  this->Superclass::DeepCopy(src);
}

//-----------------------------------------------------------------------------
bool vtkGeoTerrainNode::HasData()
{
  return (this->Model != 0);
}

//-----------------------------------------------------------------------------
void vtkGeoTerrainNode::DeleteData()
{
  this->Model = 0;
}
