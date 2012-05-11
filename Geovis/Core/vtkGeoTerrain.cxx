/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoTerrain.cxx

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

#include "vtkGeoTerrain.h"

#include "vtkActor.h"
#include "vtkAssembly.h"
#include "vtkBox.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkClipPolyData.h"
#include "vtkCollection.h"
#include "vtkDoubleArray.h"
#include "vtkExtractSelectedFrustum.h"
#include "vtkGeoAlignedImageRepresentation.h"
#include "vtkGeoCamera.h"
#include "vtkGeoImageNode.h"
#include "vtkGeoInteractorStyle.h"
#include "vtkGeoTreeNodeCache.h"
#include "vtkGeoSource.h"
#include "vtkGeoTerrainNode.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLHardwareSupport.h"
#include "vtkOpenGLRenderWindow.h"
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
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkTimerLog.h"
#include "vtkTransformFilter.h"
#include "vtkXMLPolyDataWriter.h"

#include <vtksys/stl/stack>
#include <vtksys/stl/utility>

vtkStandardNewMacro(vtkGeoTerrain);
vtkCxxSetObjectMacro(vtkGeoTerrain, GeoSource, vtkGeoSource);
vtkCxxSetObjectMacro(vtkGeoTerrain, GeoCamera, vtkGeoCamera);
//----------------------------------------------------------------------------
vtkGeoTerrain::vtkGeoTerrain()
{
  this->GeoSource = 0;
  this->Root = vtkGeoTerrainNode::New();
  this->Origin[0] = 0.0;
  this->Origin[1] = 0.0;
  this->Origin[2] = 0.0;
  this->Extractor = vtkExtractSelectedFrustum::New();
  this->GeoCamera = 0;
  this->MaxLevel = 20;
  this->Cache = vtkGeoTreeNodeCache::New();
}

//----------------------------------------------------------------------------
vtkGeoTerrain::~vtkGeoTerrain()
{
  this->SetGeoSource(0);
  this->SetGeoCamera(0);
  if (this->Root)
    {
    this->Root->Delete();
    }
  if (this->Extractor)
    {
    this->Extractor->Delete();
    }
  if (this->Cache)
    {
    this->Cache->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkGeoTerrain::SetSource(vtkGeoSource* source)
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
void vtkGeoTerrain::Initialize()
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
void vtkGeoTerrain::InitializeNodeAnalysis(vtkRenderer* ren)
{
  vtkGeoInteractorStyle* style = vtkGeoInteractorStyle::SafeDownCast(
    ren->GetRenderWindow()->GetInteractor()->GetInteractorStyle());
  if (!style)
    {
    vtkErrorMacro("vtkGeoTerrain requires vtkGeoInteractorStyle in order to get geo camera.");
    return;
    }
  vtkGeoCamera* camera = style->GetGeoCamera();
  int* rendererSize = ren->GetSize();
  camera->InitializeNodeAnalysis(rendererSize);
  this->SetGeoCamera(camera);

  // Setup the frustum extractor for finding
  // node intersections with the view frustum.
  double frustumPlanes[24];
  double aspect = ren->GetTiledAspectRatio();
  camera->GetVTKCamera()->GetFrustumPlanes( aspect, frustumPlanes );
  vtkSmartPointer<vtkPlanes> frustum = vtkSmartPointer<vtkPlanes>::New();
  frustum->SetFrustumPlanes(frustumPlanes);
  this->Extractor->SetFrustum(frustum);
}

//----------------------------------------------------------------------------
bool vtkGeoTerrain::NodeInViewport(vtkGeoTerrainNode* cur)
{
  // Determine if node is within viewport
  double bbox[6];
  cur->GetModel()->GetBounds(bbox);
  for (int i = 0; i < 6; ++i)
    {
    bbox[i] = bbox[i] - this->GeoCamera->GetOrigin()[i/2];
    }
  int boundsTest = this->Extractor->OverallBoundsTest(bbox);
  return (boundsTest != 0);
}

//-----------------------------------------------------------------------------
// Returns 0 if there should be no change, -1 if the node resolution is too
// high, and +1 if the nodes resolution is too low.
int vtkGeoTerrain::EvaluateNode(vtkGeoTerrainNode* node)
{
  double sphereViewSize;

  if (!this->GeoCamera)
    {
    return 0;
    }

  // Size of the sphere in view area units (0 -> 1)
  sphereViewSize = this->GeoCamera->GetNodeCoverage(node);

  // Arbitrary thresholds
  if (sphereViewSize > 0.2)
    {
    return 1;
    }
  if (sphereViewSize < 0.05)
    {
    return -1;
    }
  // Do not change the node.
  return 0;
}

//----------------------------------------------------------------------------
void vtkGeoTerrain::AddActors(
  vtkRenderer* ren,
  vtkAssembly* assembly,
  vtkCollection* imageReps)
{
  // This method requires that the render window graphics context
  // has been created.
  ren->GetRenderWindow()->MakeCurrent();
  if (!ren->GetRenderWindow()->IsCurrent())
    {
    return;
    }

  this->InitializeNodeAnalysis(ren);

  // See if we have multiTexturing
  vtkOpenGLHardwareSupport * hardware =
    vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow())->GetHardwareSupport();

  bool multiTexturing = hardware->GetSupportsMultiTexturing();
  int textureUnits = 0;
  if(multiTexturing)
    {
    textureUnits = hardware->GetNumberOfFixedTextureUnits();
    }

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

  int visibleActors = 0;

  vtkProp3DCollection* props = assembly->GetParts();
  vtkDebugMacro("Number Of Props: " << props->GetNumberOfItems());
  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  timer->StartTimer();

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

  double llbounds[4];
  while (!s.empty())
    {
    vtkGeoTerrainNode* cur = s.top();
    s.pop();
    if (!cur->HasData() || cur->GetModel()->GetNumberOfCells() == 0)
      {
      continue;
      }

    if (!this->NodeInViewport(cur))
      {
      // Totally outside, so prune node and subtree
      continue;
      }

    // Mark this node as "visited" so it will be less likely to
    // be deleted.
    this->Cache->SendToFront(cur);

    // Determine whether to traverse this node's children
    int refine = this->EvaluateNode(cur);

    child = cur->GetChild(0);
    if (((!child || !child->HasData()) &&
         cur->GetLevel() < this->MaxLevel &&
         refine == 1) || cur->GetStatus() == vtkGeoTreeNode::PROCESSING)
      {
      coll = this->GeoSource->GetRequestedNodes(cur);
      // Load children
      if (coll != NULL && coll->GetNumberOfItems() == 4)
        {
        for (int c = 0; c < 4; ++c)
          {
          child = vtkGeoTerrainNode::SafeDownCast(coll->GetItemAsObject(c));
          vtkGeoTerrainNode* oldChild = cur->GetChild(c);
          if (oldChild)
            {
            this->Cache->RemoveNode(oldChild);
            }
          this->Cache->SendToFront(child);
          cur->SetChild(child, c);
          child->SetParent(cur);
          }
        cur->SetStatus(vtkGeoTreeNode::NONE);
        }
      else if(cur->GetStatus() == vtkGeoTreeNode::NONE)
        {
        cur->SetStatus(vtkGeoTreeNode::PROCESSING);
        vtkGeoTerrainNode * temp = vtkGeoTerrainNode::New();
        temp->DeepCopy(cur);
        this->GeoSource->RequestChildren(temp);
        }
      if (coll)
        {
        coll->Delete();
        }
      }

    if (!cur->GetChild(0) || !cur->GetChild(0)->HasData() || refine != 1)
      {

      // Find the best texture for this geometry
      llbounds[0] = cur->GetLongitudeRange()[0];
      llbounds[1] = cur->GetLongitudeRange()[1];
      llbounds[2] = cur->GetLatitudeRange()[0];
      llbounds[3] = cur->GetLatitudeRange()[1];
      vtkGeoImageNode* textureNode1 = textureTree1->GetBestImageForBounds(llbounds);
      if (!textureNode1)
        {
        vtkWarningMacro(<< "could not find node for bounds: "
                        << llbounds[0] << ","
                        << llbounds[1] << ","
                        << llbounds[2] << ","
                        << llbounds[3]);
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
        bool sameTexture = false;
        if (multiTexturing)
          {
          sameTexture = (
              !textureNode1 ||
              actor->GetProperty()->GetNumberOfTextures() < 1 ||
              actor->GetProperty()->GetTexture(vtkProperty::VTK_TEXTURE_UNIT_0) == textureNode1->GetTexture()
            ) && (
              !textureNode2 ||
              actor->GetProperty()->GetNumberOfTextures() < 2 ||
              actor->GetProperty()->GetTexture(vtkProperty::VTK_TEXTURE_UNIT_1) == textureNode2->GetTexture()
            );
          }
        else
          {
          sameTexture = !textureNode1 || actor->GetTexture() == textureNode1->GetTexture();
          }
        if (actor && actor->GetMapper()->GetInputDataObject(0, 0) == cur->GetModel() && sameTexture)
          {
          existingActor = actor;
          existingActor->VisibilityOn();
          visibleActors++;

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
      mapper->SetInputData(cur->GetModel());
      mapper->ScalarVisibilityOff();
      actor->SetMapper(mapper);
      actor->SetPosition(-this->Origin[0], -this->Origin[1], -this->Origin[2] - 0.1);
      visibleActors++;

      if (textureNode1)
        {
        // Some implementations will report they support multi-texturing but have only 1
        // texture unit. Seriously!!
        if (multiTexturing && textureUnits > 1)
          {
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
          }
        else
          {
          if(multiTexturing)
            {
            textureNode1->GetTexture()->SetBlendingMode(
              vtkTexture::VTK_TEXTURE_BLENDING_MODE_REPLACE);
            }
          // Single texturing
          cur->GetModel()->GetPointData()->SetActiveTCoords("LatLong");
          actor->SetTexture(textureNode1->GetTexture());
          }
        actor->GetProperty()->SetAmbient(1);
        assembly->AddPart(actor);
        }
      continue;
      }
    // Workaround for the isse where if refinement does not happen for some reason
    // then we don't see a tile as its visibility is turned off.
    else
      {
      llbounds[0] = cur->GetLongitudeRange()[0];
      llbounds[1] = cur->GetLongitudeRange()[1];
      llbounds[2] = cur->GetLatitudeRange()[0];
      llbounds[3] = cur->GetLatitudeRange()[1];
      vtkGeoImageNode* textureNode1 = textureTree1->GetBestImageForBounds(llbounds);
      vtkGeoImageNode* textureNode2 = 0;
      // See if we already have an actor for this geometry
      vtkActor* existingActor = 0;
      for (int p = 0; p < props->GetNumberOfItems(); ++p)
        {
        vtkActor* actor = vtkActor::SafeDownCast(props->GetItemAsObject(p));
        bool sameTexture = false;
        if (multiTexturing)
          {
          sameTexture = (
              !textureNode1 ||
              actor->GetProperty()->GetNumberOfTextures() < 1 ||
              actor->GetProperty()->GetTexture(vtkProperty::VTK_TEXTURE_UNIT_0) == textureNode1->GetTexture()
            ) && (
              !textureNode2 ||
              actor->GetProperty()->GetNumberOfTextures() < 2 ||
              actor->GetProperty()->GetTexture(vtkProperty::VTK_TEXTURE_UNIT_1) == textureNode2->GetTexture()
            );
          }
        else
          {
          sameTexture = !textureNode1 || actor->GetTexture() == textureNode1->GetTexture();
          }
        if (actor && actor->GetMapper()->GetInputDataObject(0, 0) == cur->GetModel() && sameTexture)
          {
          existingActor = actor;
          existingActor->VisibilityOn();
          visibleActors++;

          // Move the actor to the end of the list so it is less likely removed.
          actor->Register(this);
          assembly->RemovePart(actor);
          assembly->AddPart(actor);
          actor->Delete();
          break;
          }
        }
      }

    s.push(cur->GetChild(0));
    s.push(cur->GetChild(1));
    s.push(cur->GetChild(2));
    s.push(cur->GetChild(3));
    }

  timer->StopTimer();
  vtkDebugMacro("Visible Actors: " << visibleActors);
  vtkDebugMacro("AddActors time: " << timer->GetElapsedTime());
}

//----------------------------------------------------------------------------
void vtkGeoTerrain::PrintSelf(ostream & os, vtkIndent indent)
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "GeoSource: " << this->GeoSource << "\n";
  os << indent << "Origin: (" << this->Origin[0] << ", "
     << this->Origin[1] << ", " << this->Origin[2] << ")\n";
  os << indent << "MaxLevel: " << this->MaxLevel << "\n";
  this->PrintTree(os, indent, this->Root); // Root
}

//----------------------------------------------------------------------------
void vtkGeoTerrain::SaveDatabase(const char* path, int depth)
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
    writer->SetInputData(storedData);
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
void vtkGeoTerrain::PrintTree(ostream & os, vtkIndent indent, vtkGeoTerrainNode* parent)
{
  os << indent << "Error: " << parent->GetError() << endl;
  os << indent << "Level: " << parent->GetLevel() << "  "
               << "Id: " << parent->GetId() << endl;
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

