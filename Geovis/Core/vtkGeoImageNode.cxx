/*=============================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoImageNode.cxx

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
#include "vtkGeoImageNode.h"
#include "vtkJPEGWriter.h"
#include "vtkTexture.h"
#include "vtkXMLImageDataReader.h"
#include "vtkXMLImageDataWriter.h"

vtkStandardNewMacro(vtkGeoImageNode);


//----------------------------------------------------------------------------
vtkGeoImageNode::vtkGeoImageNode()
{
  this->Image = vtkSmartPointer<vtkImageData>::New();
  this->Texture = vtkSmartPointer<vtkTexture>::New();
}

//-----------------------------------------------------------------------------
vtkGeoImageNode::~vtkGeoImageNode()
{
}

//-----------------------------------------------------------------------------
void vtkGeoImageNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//-----------------------------------------------------------------------------
void vtkGeoImageNode::SetImage(vtkImageData* image)
{
  this->Image = image;
}

//-----------------------------------------------------------------------------
vtkImageData* vtkGeoImageNode::GetImage()
{
  return this->Image;
}

//-----------------------------------------------------------------------------
void vtkGeoImageNode::SetTexture(vtkTexture* texture)
{
  this->Texture = texture;
}

//-----------------------------------------------------------------------------
vtkTexture* vtkGeoImageNode::GetTexture()
{
  return this->Texture;
}

//-----------------------------------------------------------------------------
vtkGeoImageNode* vtkGeoImageNode::GetChild(int idx)
{
  if (idx < 0 || idx > 3)
    {
    vtkErrorMacro("Index out of range.");
    return 0;
    }
  return vtkGeoImageNode::SafeDownCast(this->Children[idx]);
}


//-----------------------------------------------------------------------------
vtkGeoImageNode* vtkGeoImageNode::GetParent()
{
  return vtkGeoImageNode::SafeDownCast(this->Parent);
}

//-----------------------------------------------------------------------------
// We have to get a power of 2 for dimensions of the image.  VTK
// resamples every time a tile is selected anr changed otherwise.
//
// We have two choises for dealing with images.
//
// 1: Treat pixels like cell data.
//   This makes subsampling easy.  Simply use vtkImageShrink3D.
//   Tile images do not overlap.
//   Difficult:
//   Texture mapping is point data.  TCoords have to be extended half a pixel.
//   vtkImageData is point data.  Have to handle meta data extenal to object.
//   Interpolated texture map will have seams between tiles.
// 2: Treat pixels like point data.
//   We would need a new shrink filter that uses a 3x3 kernel.
//   Tiles would have to duplicate a row of xels with neighbors.
//   This would make dividing image more difficult with natural pixel boundaries
//   every 255 pixels (instead of 256).

void vtkGeoImageNode::CropImageForTile(
  vtkImageData* image,
  double* imageLonLatExt,
  const char* prefix)
{
  int ext[6];
  int wholeExt[6];

  // I am keeping this all external to the imageData object because
  // I consider pixels are cells not points.
  image->GetExtent(ext);
  image->GetExtent(wholeExt);
  double origin[2];
  double spacing[2];
  spacing[0] = (imageLonLatExt[1]-imageLonLatExt[0])/(ext[1]-ext[0]+1);
  spacing[1] = (imageLonLatExt[3]-imageLonLatExt[2])/(ext[3]-ext[2]+1);
  origin[0] = imageLonLatExt[0] - ext[0]*spacing[0];
  origin[1] = imageLonLatExt[2] - ext[2]*spacing[1];

  // Compute the minimum extent that covers the terrain patch.
  ext[0] = static_cast<int>(floor((this->LongitudeRange[0]-origin[0])/spacing[0]));
  ext[1] = static_cast<int>(ceil((this->LongitudeRange[1]-origin[0])/spacing[0]));
  ext[2] = static_cast<int>(floor((this->LatitudeRange[0]-origin[1])/spacing[1]));
  ext[3] = static_cast<int>(ceil((this->LatitudeRange[1]-origin[1])/spacing[1]));

  int dims[2];
  dims[0] = this->PowerOfTwo(ext[1]-ext[0]+1);
  dims[1] = this->PowerOfTwo(ext[3]-ext[2]+1);
  ext[1] = ext[0] + dims[0] - 1;
  ext[3] = ext[2] + dims[1] - 1;
  if (ext[1] > wholeExt[1])
    {
    ext[1] = wholeExt[1];
    }
  if (ext[3] > wholeExt[3])
    {
    ext[3] = wholeExt[3];
    }
  ext[0] = ext[1] - dims[0] + 1;
  ext[2] = ext[3] - dims[1] + 1;
  if (ext[0] < wholeExt[0])
    {
    ext[0] = wholeExt[0];
    }
  if (ext[2] < wholeExt[2])
    {
    ext[2] = wholeExt[2];
    }

  if (this->Image == 0)
    {
    this->Image = vtkSmartPointer<vtkImageData>::New();
    }
  this->Image->ShallowCopy(image);
  this->Image->Crop(ext);

  // Now set the longitude and latitude range based on the actual image size.
  this->LongitudeRange[0] = origin[0] + ext[0]*spacing[0];
  this->LongitudeRange[1] = origin[0] + (ext[1]+1)*spacing[0];
  this->LatitudeRange[0] = origin[1] + ext[2]*spacing[1];
  this->LatitudeRange[1] = origin[1] + (ext[3]+1)*spacing[1];

  // Save out the image to verify we are processing properly
  if (prefix)
    {
    vtkImageData* storedImage = vtkImageData::New();
    storedImage->ShallowCopy(this->Image);
    storedImage->SetOrigin(this->LongitudeRange[0], this->LatitudeRange[0], 0);
    storedImage->SetSpacing(this->LongitudeRange[1], this->LatitudeRange[1], 0);
    vtkXMLImageDataWriter* writer = vtkXMLImageDataWriter::New();
    char fn[512];
    sprintf(fn, "%s/tile_%d_%ld.vti", prefix, this->Level, this->Id);
    writer->SetFileName(fn);
    writer->SetInputData(storedImage);
    writer->Write();
    writer->Delete();
    storedImage->Delete();
    }
}

//-----------------------------------------------------------------------------
void vtkGeoImageNode::LoadAnImage(const char* prefix)
{
  vtkXMLImageDataReader* reader = vtkXMLImageDataReader::New();
  char fn[512];
  sprintf(fn, "%s/tile_%d_%ld.vti", prefix, this->Level, this->Id);
  reader->SetFileName(fn);
  reader->Update();
  vtkImageData* image = reader->GetOutput();
  this->Image = image;
  this->LongitudeRange[0] = this->Image->GetOrigin()[0];
  this->LatitudeRange[0] = this->Image->GetOrigin()[1];
  this->LongitudeRange[1] = this->Image->GetSpacing()[0];
  this->LatitudeRange[1] = this->Image->GetSpacing()[1];
  reader->Delete();
}

//-----------------------------------------------------------------------------
int vtkGeoImageNode::PowerOfTwo(int val)
{
  // Pick the next highest power of two.
  int tmp;
  bool nextHigherFlag = false;
  tmp = 1;
  while (val)
    {
    if ((val & 1) && val > 1)
      {
      nextHigherFlag = true;
      }
    val = val >> 1;
    tmp = tmp << 1;
    }

  if ( ! nextHigherFlag)
    {
    tmp = tmp >> 1;
    }
  return tmp;
}

//-----------------------------------------------------------------------------
void vtkGeoImageNode::ShallowCopy(vtkGeoTreeNode *src)
{
  vtkGeoImageNode *imageNode = vtkGeoImageNode::SafeDownCast(src);

  if(imageNode != NULL)
    {
    this->Image = imageNode->Image;
    this->Texture = imageNode->Texture;
    }
  this->Superclass::ShallowCopy(src);
}

//-----------------------------------------------------------------------------
void vtkGeoImageNode::DeepCopy(vtkGeoTreeNode *src)
{
  vtkGeoImageNode *imageNode = vtkGeoImageNode::SafeDownCast(src);

  if(imageNode != NULL)
    {
    vtkImageData * image = vtkImageData::New();
    image->DeepCopy(imageNode->Image);
    this->SetImage(image);
    image->Delete();
    image = NULL;

    this->Texture = imageNode->Texture;
    }
  this->Superclass::DeepCopy(src);
}

//-----------------------------------------------------------------------------
bool vtkGeoImageNode::HasData()
{
  return (this->Image != 0);
}

//-----------------------------------------------------------------------------
void vtkGeoImageNode::DeleteData()
{
  this->Image = 0;
  this->Texture = 0;
}
