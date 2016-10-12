/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoFileImageSource.cxx

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

#include "vtkGeoFileImageSource.h"

#include "vtkDoubleArray.h"
#include "vtkGeoImageNode.h"
#include "vtkImageData.h"
#include "vtkImageGridSource.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"
#include "vtkTexture.h"
#include "vtkTransform.h"
#include "vtkXMLImageDataReader.h"

#include <fstream>
#include <sstream>
#include <utility>

vtkStandardNewMacro(vtkGeoFileImageSource);
//----------------------------------------------------------------------------
vtkGeoFileImageSource::vtkGeoFileImageSource()
{
  this->Path = 0;
}

//----------------------------------------------------------------------------
vtkGeoFileImageSource::~vtkGeoFileImageSource()
{
  this->SetPath(0);
}

//----------------------------------------------------------------------------
bool vtkGeoFileImageSource::FetchRoot(vtkGeoTreeNode* r)
{
  vtkGeoImageNode* root = 0;
  if (!(root = vtkGeoImageNode::SafeDownCast(r)))
  {
    vtkErrorMacro(<< "Can only fetch image nodes from this source.");
    return false;
  }

  root->SetLatitudeRange(-270, 90);
  root->SetLongitudeRange(-180, 180);
  this->ReadImage(-1, 0, root);
  return true;
}

//----------------------------------------------------------------------------
bool vtkGeoFileImageSource::FetchChild(vtkGeoTreeNode* p, int index, vtkGeoTreeNode* c)
{
  vtkGeoImageNode* parent = 0;
  if (!(parent = vtkGeoImageNode::SafeDownCast(p)))
  {
    vtkErrorMacro(<< "Can only fetch image nodes from this source.");
    return false;
  }
  vtkGeoImageNode* child = 0;
  if (!(child = vtkGeoImageNode::SafeDownCast(c)))
  {
    vtkErrorMacro(<< "Can only fetch image nodes from this source.");
    return false;
  }

  if (parent->GetLevel() == -1)
  {
    // Child 0 is the dummy western hemisphere, child 1 is the dummy eastern hemisphere
    // Child 2 is the western hemisphere, child 3 is the eastern hemisphere
    if (index == 0)
    {
      vtkSmartPointer<vtkImageData> dummyImageWest = vtkSmartPointer<vtkImageData>::New();
      dummyImageWest->SetOrigin(-180.0, -270.0, 0.0);
      dummyImageWest->SetSpacing(0.0, -90.0, 0.0);
      child->GetTexture()->SetInputData(dummyImageWest);
      child->SetLatitudeRange(-270, -90);
      child->SetLongitudeRange(-180, 0);
    }
    else if (index == 1)
    {
      vtkSmartPointer<vtkImageData> dummyImageEast = vtkSmartPointer<vtkImageData>::New();
      dummyImageEast->SetOrigin(0.0, -270.0, 0.0);
      dummyImageEast->SetSpacing(180.0, -90.0, 0.0);
      child->GetTexture()->SetInputData(dummyImageEast);
      child->SetLatitudeRange(-270, -90);
      child->SetLongitudeRange(0, 180);
    }
    else if (index == 2)
    {
      this->ReadImage(0, 0, child);
    }
    else
    {
      this->ReadImage(0, 1, child);
    }
    return true;
  }

  int level = parent->GetLevel() + 1;
  int id = parent->GetId() | (index << (2*level-1));
  return this->ReadImage(level, id, child);
}

//----------------------------------------------------------------------------
bool vtkGeoFileImageSource::ReadImage(int level, int id, vtkGeoImageNode* node)
{
  // Load the image
  node->SetId(id);
  node->SetLevel(level);
  vtkSmartPointer<vtkXMLImageDataReader> reader = vtkSmartPointer<vtkXMLImageDataReader>::New();
  std::stringstream ss;
  ss.str("");
  ss << this->Path << "/tile_" << level << "_" << id << ".vti";

  // Check if the file exists
  std::ifstream in;
  in.open(ss.str().c_str(), std::ifstream::in);
  if (in.fail())
  {
    // Make a dummy image
    in.close();
    vtkSmartPointer<vtkImageData> dummy = vtkSmartPointer<vtkImageData>::New();
    dummy->SetDimensions(1, 1, 1);
    vtkSmartPointer<vtkDoubleArray> scalar = vtkSmartPointer<vtkDoubleArray>::New();
    scalar->InsertNextValue(0.0);
    dummy->GetPointData()->SetScalars(scalar);
    dummy->SetOrigin(node->GetLongitudeRange()[0], node->GetLatitudeRange()[0], 0.0);
    dummy->SetSpacing(node->GetLongitudeRange()[1], node->GetLatitudeRange()[1], 0.0);
    node->GetTexture()->SetInputData(dummy);
    return false;
  }
  in.close();

  // Read the file
  reader->SetFileName(ss.str().c_str());
  reader->Update();
  vtkImageData* image = reader->GetOutput();

  // Retrieve the image bounds using origin and spacing
  double x1[3];
  double x2[3];
  image->GetOrigin(x1);
  image->GetSpacing(x2);
  double lonRange[2] = {x1[0], x2[0]};
  double latRange[2] = {x1[1], x2[1]};
  node->SetLatitudeRange(latRange);
  node->SetLongitudeRange(lonRange);

  // Make the texture with the correct transform
  vtkSmartPointer<vtkTexture> texture = vtkSmartPointer<vtkTexture>::New();
  vtkSmartPointer<vtkTransform> texTrans = vtkSmartPointer<vtkTransform>::New();

  // Start with (lat,lon)
  texTrans->PostMultiply();
  texTrans->RotateZ(90.0); // (-lon,lat)
  texTrans->Scale(-1.0, 1.0, 1.0); // (lon,lat)
  texTrans->Translate(-lonRange[0], -latRange[0], 0.0); // to origin
  texTrans->Scale(1.0/(lonRange[1] - lonRange[0]), 1.0/(latRange[1] - latRange[0]), 1.0); // to [0,1]

  texture->SetInputConnection(reader->GetOutputPort());
  texture->SetTransform(texTrans);
  texture->RepeatOff();
  texture->InterpolateOn();
  texture->EdgeClampOn();
  node->SetTexture(texture);
  return true;
}

//-----------------------------------------------------------------------------
void vtkGeoFileImageSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Path: " << (this->Path ? this->Path : "(none)") << endl;
}
