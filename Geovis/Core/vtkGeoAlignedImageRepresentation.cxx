/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoAlignedImageRepresentation.cxx

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

#include "vtkGeoAlignedImageRepresentation.h"

#include "vtkCollection.h"
#include "vtkGeoImageNode.h"
#include "vtkGeoSource.h"
#include "vtkGeoTreeNodeCache.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"
#include "vtkTexture.h"
#include "vtkTransform.h"
#include "vtkXMLImageDataReader.h"
#include "vtkXMLImageDataWriter.h"

#include <sstream>
#include <stack>
#include <utility>

static std::pair<vtkGeoImageNode*, double>
vtkGeoAlignedImageRepresentationFind(vtkGeoSource* source, vtkGeoImageNode* p, double* bounds, vtkGeoTreeNodeCache* nodeList)
{
  if (!p->HasData())
  {
    return std::make_pair(static_cast<vtkGeoImageNode*>(0), 0.0);
  }
  double lb[3];
  double ub[3];
  p->GetTexture()->GetImageDataInput(0)->GetOrigin(lb);
  p->GetTexture()->GetImageDataInput(0)->GetSpacing(ub);
  double bcenter[2] = {(bounds[0] + bounds[1])/2.0, (bounds[2] + bounds[3])/2.0};
  double ncenter[2] = {(lb[0] + ub[0])/2.0, (lb[1] + ub[1])/2.0};
  double vec[2] = {bcenter[0] - ncenter[0], bcenter[1] - ncenter[1]};
  double dist2 = vec[0]*vec[0] + vec[1]*vec[1];
  if (lb[0] <= bounds[0] &&
      ub[0] >= bounds[1] &&
      lb[1] <= bounds[2] &&
      ub[1] >= bounds[3])
  {
    nodeList->SendToFront(p);
    std::pair<vtkGeoImageNode*, double> minDist(static_cast<vtkGeoImageNode *>(NULL), VTK_DOUBLE_MAX);

    vtkGeoImageNode* child = p->GetChild(0);
    vtkCollection* coll = NULL;

    if (!child || !child->HasData() || p->GetStatus() == vtkGeoTreeNode::PROCESSING)
    {
      // TODO: This multiplier should be configurable
      if ((ub[0] - lb[0]) > 2.0*(bounds[1] - bounds[0]))
      {
        // Populate the children
        coll = source->GetRequestedNodes(p);
        if (coll && coll->GetNumberOfItems() == 4)
        {
          if (!child)
          {
            p->CreateChildren();
          }
          for (int c = 0; c < 4; ++c)
          {
            vtkGeoImageNode* node = vtkGeoImageNode::SafeDownCast(coll->GetItemAsObject(c));
            if (node)
            {
              p->GetChild(c)->SetImage(node->GetImage());
              p->GetChild(c)->SetTexture(node->GetTexture());
              p->GetChild(c)->SetId(node->GetId());
              p->GetChild(c)->SetLevel(node->GetLevel());
              nodeList->SendToFront(p->GetChild(c));
            }
          }
          p->SetStatus(vtkGeoTreeNode::NONE);
        }
        if (coll)
        {
          coll->Delete();
        }
        else if(p->GetStatus() == vtkGeoTreeNode::NONE)
        {
          p->SetStatus(vtkGeoTreeNode::PROCESSING);
          vtkGeoImageNode * temp = vtkGeoImageNode::New();
          temp->DeepCopy(p);
          source->RequestChildren(temp);
          //source->RequestChildren(p);
        }
      }
    }

    if (p->GetChild(0))
    {
      for (int i = 0; i < 4; ++i)
      {
        std::pair<vtkGeoImageNode*, double> subsearch =
          vtkGeoAlignedImageRepresentationFind(source, p->GetChild(i), bounds, nodeList);
        if (subsearch.first && subsearch.second < minDist.second)
        {
          minDist = subsearch;
        }
      }
    }
    if (minDist.first)
    {
      return minDist;
    }
    return std::make_pair(p, dist2);
  }
  else
  {
    return std::make_pair(static_cast<vtkGeoImageNode*>(0), 0.0);
  }
}

vtkStandardNewMacro(vtkGeoAlignedImageRepresentation);
vtkCxxSetObjectMacro(vtkGeoAlignedImageRepresentation, GeoSource, vtkGeoSource);
//----------------------------------------------------------------------------
vtkGeoAlignedImageRepresentation::vtkGeoAlignedImageRepresentation()
{
  this->GeoSource = 0;
  this->Root = vtkGeoImageNode::New();
  this->Cache = vtkGeoTreeNodeCache::New();
  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
vtkGeoAlignedImageRepresentation::~vtkGeoAlignedImageRepresentation()
{
  this->SetGeoSource(0);
  if (this->Root)
  {
    this->Root->Delete();
  }
  if (this->Cache)
  {
    this->Cache->Delete();
  }
}

//----------------------------------------------------------------------------
void vtkGeoAlignedImageRepresentation::SetSource(vtkGeoSource* source)
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
void vtkGeoAlignedImageRepresentation::Initialize()
{
  if (!this->GeoSource)
  {
    vtkErrorMacro(<< "You must set the source before initialization.");
    return;
  }
  this->GeoSource->FetchRoot(this->Root);
}

//----------------------------------------------------------------------------
void vtkGeoAlignedImageRepresentation::SaveDatabase(const char* path)
{
  if (!this->Root)
  {
    this->Initialize();
  }
  std::stack< vtkSmartPointer<vtkGeoImageNode> > s;
  s.push(this->Root);
  while (!s.empty())
  {
    vtkSmartPointer<vtkGeoImageNode> node = s.top();
    s.pop();

    // Write out file.
    vtkSmartPointer<vtkImageData> storedImage = vtkSmartPointer<vtkImageData>::New();
    storedImage->ShallowCopy(node->GetTexture()->GetInput());
    vtkSmartPointer<vtkXMLImageDataWriter> writer = vtkSmartPointer<vtkXMLImageDataWriter>::New();
    char fn[512];
    snprintf(fn, sizeof(fn), "%s/tile_%d_%ld.vti", path, node->GetLevel(), node->GetId());
    writer->SetFileName(fn);
    writer->SetInputData(storedImage);
    writer->Write();

    // Recurse over children.
    for (int i = 0; i < 4; ++i)
    {
      vtkSmartPointer<vtkGeoImageNode> child =
        vtkSmartPointer<vtkGeoImageNode>::New();
      if (this->GeoSource->FetchChild(node, i, child))
      {
        // Skip nodes outside range of the world.
        if (child->GetLatitudeRange()[1] > -90.0)
        {
          s.push(child);
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
vtkGeoImageNode* vtkGeoAlignedImageRepresentation::GetBestImageForBounds(double bounds[4])
{
  std::pair<vtkGeoImageNode*, double> res =
    vtkGeoAlignedImageRepresentationFind(this->GeoSource, this->Root, bounds, this->Cache);
  return res.first;
}

//----------------------------------------------------------------------------
void vtkGeoAlignedImageRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->PrintTree(os, indent, this->Root);
}

//----------------------------------------------------------------------------
void vtkGeoAlignedImageRepresentation::PrintTree(ostream& os, vtkIndent indent, vtkGeoImageNode* root)
{
  os << indent << "Id: " << root->GetId() << endl;
  os << indent << "LatitudeRange: " << root->GetLatitudeRange()[0] << ", " << root->GetLatitudeRange()[1] << endl;
  os << indent << "LongitudeRange: " << root->GetLongitudeRange()[0] << ", " << root->GetLongitudeRange()[1] << endl;
  os << indent << "Level: " << root->GetLevel() << endl;
  if (root->GetChild(0))
  {
    for (int i = 0; i < 4; ++i)
    {
      this->PrintTree(os, indent.GetNextIndent(), root->GetChild(i));
    }
  }
}
