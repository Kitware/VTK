/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoTerrain2D.cxx

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

#include "vtkGeoTerrain2D.h"

#include "vtkActor.h"
#include "vtkAssembly.h"
#include "vtkBox.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkClipPolyData.h"
#include "vtkCollection.h"
#include "vtkDoubleArray.h"
#include "vtkGeoAlignedImageRepresentation.h"
#include "vtkGeoGraticule.h"
#include "vtkGeoImageNode.h"
#include "vtkGeoProjection.h"
#include "vtkGeoSource.h"
#include "vtkGeoTerrainNode.h"
#include "vtkGeoTransform.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPainter.h"
#include "vtkPainterPolyDataMapper.h"
#include "vtkPlane.h"
#include "vtkPlanes.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProp3DCollection.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTimerLog.h"
#include "vtkTransformFilter.h"
#include "vtkXMLPolyDataWriter.h"

#include <vtksys/stl/stack>
#include <vtksys/stl/utility>

vtkStandardNewMacro(vtkGeoTerrain2D);
vtkCxxRevisionMacro(vtkGeoTerrain2D, "1.1");
vtkCxxSetObjectMacro(vtkGeoTerrain2D, GeoSource, vtkGeoSource);
//----------------------------------------------------------------------------
vtkGeoTerrain2D::vtkGeoTerrain2D()
{
  this->GeoSource = 0;
  this->Root = vtkGeoTerrainNode::New();
  this->LocationTolerance = 50.0;
  this->TextureTolerance = 1.0;
}

//----------------------------------------------------------------------------
vtkGeoTerrain2D::~vtkGeoTerrain2D()
{
  this->SetGeoSource(0);
  if (this->Root)
    {
    this->Root->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkGeoTerrain2D::SetSource(vtkGeoSource* source)
{
  if (this->GeoSource != source)
    {
    this->SetGeoSource(source);
    if (this->GeoSource)
      {
      this->Initialize();
      }
    }
}

//----------------------------------------------------------------------------
void vtkGeoTerrain2D::Initialize()
{
  if (!this->GeoSource)
    {
    vtkErrorMacro(<< "Must set source before initializing.");
    return;
    }

  // Start by fetching the root.
  this->GeoSource->FetchRoot(this->Root);
}

//----------------------------------------------------------------------------
void vtkGeoTerrain2D::AddActors(
  vtkRenderer* ren,
  vtkAssembly* assembly,
  vtkCollection* imageReps)
{
  // Determine the 2D camera bounds
  double cameraBounds[4];
  vtkCamera* cam = ren->GetActiveCamera();
  double scale = cam->GetParallelScale();
  double* pos = cam->GetPosition();
  int* size = ren->GetSize();
  double pixelSize = 2.0 * scale / size[1];
  cameraBounds[0] = pos[0] - size[0]*pixelSize/2.0;
  cameraBounds[1] = pos[0] + size[0]*pixelSize/2.0;
  cameraBounds[2] = pos[1] - size[1]*pixelSize/2.0;
  cameraBounds[3] = pos[1] + size[1]*pixelSize/2.0;

  // Extract the image representations from the collection.
  vtkGeoAlignedImageRepresentation* textureTree1 = 0;
  if (imageReps->GetNumberOfItems() >= 1)
    {
    textureTree1 = vtkGeoAlignedImageRepresentation::SafeDownCast(
      imageReps->GetItemAsObject(0));
    }
  vtkGeoAlignedImageRepresentation* textureTree2 = 0;
  if (imageReps->GetNumberOfItems() >= 2)
    {
    textureTree2 = vtkGeoAlignedImageRepresentation::SafeDownCast(
      imageReps->GetItemAsObject(1));
    }

  bool wireframe = false;
  bool colorTiles = true;
  bool colorByTextureLevel = false;

  vtkProp3DCollection* props = assembly->GetParts();
  //vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  //timer->StartTimer();

  // Remove actors at the beginning of the actor list until there are at most
  // 100 actors.
  while (props->GetNumberOfItems() > 100)
    {
    assembly->RemovePart(vtkActor::SafeDownCast(props->GetItemAsObject(0)));
    }

  // First turn off visibility of all actors
  for (int p = 0; p < props->GetNumberOfItems(); ++p)
    {
    vtkActor* actor = vtkActor::SafeDownCast(props->GetItemAsObject(p));
    actor->VisibilityOff();
    }

  // Use stack rather than recursion
  vtksys_stl::stack<vtkGeoTerrainNode*> s;
  s.push(this->Root);

  vtkGeoTerrainNode* child = NULL;
  vtkCollection* coll = NULL;

  double bounds[4];
  double llbounds[4];
  double range[2];
  while (!s.empty())
    {
    vtkGeoTerrainNode* cur = s.top();
    s.pop();
    if (cur->GetModel()->GetNumberOfCells() == 0)
      {
      continue;
      }

    cur->GetProjectionBounds(bounds);
    if (bounds[1] < cameraBounds[0] || bounds[0] > cameraBounds[1] ||
        bounds[3] < cameraBounds[2] || bounds[2] > cameraBounds[3])
      {
      continue;
      }
    range[0] = bounds[1] - bounds[0];
    range[1] = bounds[3] - bounds[2];

    // Determine the maximum allowable location error
    double maxLocationError = this->LocationTolerance*pixelSize;
    bool locationErrorOk = cur->GetError() < maxLocationError;

    // Determine the maximum allowable patch size
    double maxPatchSize = 300.0*this->TextureTolerance*pixelSize;
    double patchX = bounds[1] - bounds[0];
    double patchY = bounds[3] - bounds[2];
    double patchSize = (patchX > patchY) ? patchX : patchY;
    bool textureErrorOk = patchSize < maxPatchSize;

    child = cur->GetChild(0);

    if ((!child && (!locationErrorOk || !textureErrorOk)) || cur->GetStatus() == vtkGeoTreeNode::PROCESSING)
      {
      coll = this->GeoSource->GetRequestedNodes(cur);
      // Load children
      if (coll != NULL && coll->GetNumberOfItems() == 4)
        {
        cur->CreateChildren();
        for (int c = 0; c < 4; ++c)
          {
          child = vtkGeoTerrainNode::SafeDownCast(coll->GetItemAsObject(c));
          cur->SetChild(child, c);
          }
        cur->SetStatus(vtkGeoTreeNode::NONE);
        }
      else if(cur->GetStatus() == vtkGeoTreeNode::NONE)
        {
        cur->SetStatus(vtkGeoTreeNode::PROCESSING);
        this->GeoSource->RequestChildren(cur);
        }
      }

    if (!cur->GetChild(0) || (locationErrorOk && textureErrorOk))
      {
      // Find the best texture for this geometry
      llbounds[0] = cur->GetLongitudeRange()[0];
      llbounds[1] = cur->GetLongitudeRange()[1];
      llbounds[2] = cur->GetLatitudeRange()[0];
      llbounds[3] = cur->GetLatitudeRange()[1];
      vtkGeoImageNode* textureNode1 = textureTree1->GetBestImageForBounds(llbounds);
      if (!textureNode1)
        {
        vtkWarningMacro(<< "could not find node for bounds: " << llbounds[0] << "," << llbounds[1] << "," << llbounds[2] << "," << llbounds[3]);
        }
      vtkGeoImageNode* textureNode2 = 0;
      if (textureTree2)
        {
        textureNode2 = textureTree2->GetBestImageForBounds(llbounds);
        }


      // See if we already have an actor for this geometry
      vtkActor* existingActor = 0;
      for (int p = 0; p < props->GetNumberOfItems(); ++p)
        {
        vtkActor* actor = vtkActor::SafeDownCast(props->GetItemAsObject(p));
        if (actor && actor->GetMapper()->GetInputDataObject(0, 0) == cur->GetModel() &&
            (!textureNode1 || actor->GetProperty()->GetTexture(vtkProperty::VTK_TEXTURE_UNIT_0) == textureNode1->GetTexture()) &&
            (!textureNode2 || actor->GetProperty()->GetTexture(vtkProperty::VTK_TEXTURE_UNIT_1) == textureNode2->GetTexture()))
          {
          existingActor = actor;
          existingActor->VisibilityOn();

          // Move the actor to the end of the list so it is less likely removed.
          actor->Register(this);
          assembly->RemovePart(actor);
          assembly->AddPart(actor);
          actor->Delete();
          break;
          }
        }
      if (existingActor)
        {
        continue;
        }

      // Add the data to the view
      vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
      vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
      mapper->SetInput(cur->GetModel());
      mapper->ScalarVisibilityOff();
      actor->SetMapper(mapper);
      actor->SetPosition(0.0, 0.0, -0.1);

      if (textureNode1)
        {
#if 1
        // Multi texturing
        mapper->MapDataArrayToMultiTextureAttribute(vtkProperty::VTK_TEXTURE_UNIT_0,
          "LatLong", vtkDataObject::FIELD_ASSOCIATION_POINTS);
        textureNode1->GetTexture()->SetBlendingMode(vtkTexture::VTK_TEXTURE_BLENDING_MODE_REPLACE);
        actor->GetProperty()->SetTexture(vtkProperty::VTK_TEXTURE_UNIT_0, textureNode1->GetTexture());

        if (textureNode2)
          {
          mapper->MapDataArrayToMultiTextureAttribute(vtkProperty::VTK_TEXTURE_UNIT_1,
            "LatLong", vtkDataObject::FIELD_ASSOCIATION_POINTS);
          textureNode2->GetTexture()->SetBlendingMode(vtkTexture::VTK_TEXTURE_BLENDING_MODE_ADD);
          actor->GetProperty()->SetTexture(vtkProperty::VTK_TEXTURE_UNIT_1, textureNode2->GetTexture());
          }
#else
        // Single texturing
        cur->GetModel()->GetPointData()->SetActiveTCoords("LatLong");
        actor->SetTexture(textureNode1->GetTexture());
#endif

        if (colorTiles)
          {
          int level = cur->GetLevel();
          if (colorByTextureLevel)
            {
            level = textureNode1->GetLevel();
            }
          if (level == 0)
            {
            actor->GetProperty()->SetColor(1.0, 0.4, 0.4);
            }
          else if (level == 1)
            {
            actor->GetProperty()->SetColor(1.0, 1.0, 0.4);
            }
          else if (level == 2)
            {
            actor->GetProperty()->SetColor(0.4, 1.0, 0.4);
            }
          else if (level == 3)
            {
            actor->GetProperty()->SetColor(0.4, 0.4, 1.0);
            }
          else if (level == 4)
            {
            actor->GetProperty()->SetColor(1.0, 0.4, 1.0);
            }
          }
        if (wireframe)
          {
          actor->GetProperty()->SetRepresentationToWireframe();
          }
        assembly->AddPart(actor);
        }
      continue;
      }
    s.push(cur->GetChild(0));
    s.push(cur->GetChild(1));
    s.push(cur->GetChild(2));
    s.push(cur->GetChild(3));
    }

  //timer->StopTimer();
  //cerr << "AddActors time: " << timer->GetElapsedTime() << endl;
}

//----------------------------------------------------------------------------
void vtkGeoTerrain2D::PrintSelf(ostream & os, vtkIndent indent)
{
  this->PrintTree(os, indent, this->Root);
}

//----------------------------------------------------------------------------
void vtkGeoTerrain2D::SaveDatabase(const char* path, int depth)
{
  if (!this->Root)
    {
    this->Initialize();
    }
  vtksys_stl::stack< vtkSmartPointer<vtkGeoTerrainNode> > s;
  s.push(this->Root);
  while (!s.empty())
    {
    vtkSmartPointer<vtkGeoTerrainNode> node = s.top();
    s.pop();

    // Write out file.
    vtkSmartPointer<vtkPolyData> storedData = vtkSmartPointer<vtkPolyData>::New();
    storedData->ShallowCopy(node->GetModel());
    vtkSmartPointer<vtkXMLPolyDataWriter> writer = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
    char fn[512];
    sprintf(fn, "%s/tile_%d_%ld.vtp", path, node->GetLevel(), node->GetId());
    writer->SetFileName(fn);
    writer->SetInput(storedData);
    writer->Write();

    if (node->GetLevel() == depth)
      {
      continue;
      }

    // Recurse over children.
    for (int i = 0; i < 4; ++i)
      {
      vtkSmartPointer<vtkGeoTerrainNode> child =
        vtkSmartPointer<vtkGeoTerrainNode>::New();
      if (this->GeoSource->FetchChild(node, i, child))
        {
        s.push(child);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkGeoTerrain2D::PrintTree(ostream & os, vtkIndent indent, vtkGeoTerrainNode* parent)
{
  os << indent << "Error: " << parent->GetError() << endl;
  os << indent << "Level: " << parent->GetLevel() << endl;
  os << indent << "LatitudeRange: " << parent->GetLatitudeRange()[0]
    << "," << parent->GetLatitudeRange()[1] << endl;
  os << indent << "LongitudeRange: " << parent->GetLongitudeRange()[0]
    << "," << parent->GetLongitudeRange()[1] << endl;
  os << indent << "ProjectionBounds: " << parent->GetProjectionBounds()[0]
    << "," << parent->GetProjectionBounds()[1]
    << "," << parent->GetProjectionBounds()[2]
    << "," << parent->GetProjectionBounds()[3] << endl;
  os << indent << "Number of cells: " << parent->GetModel()->GetNumberOfCells() << endl;
  if (parent->GetChild(0) == 0)
    {
    return;
    }
  for (int i = 0; i < 4; ++i)
    {
    this->PrintTree(os, indent.GetNextIndent(), parent->GetChild(i));
    }
}

