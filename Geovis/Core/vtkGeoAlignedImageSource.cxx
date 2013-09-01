/*=============================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoAlignedImageSource.cxx

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

#include "vtkGeoAlignedImageSource.h"

#include "vtkCommand.h"
#include "vtkGeoImageNode.h"
#include "vtkImageData.h"
#include "vtkImageShrink3D.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkTexture.h"
#include "vtkTimerLog.h"
#include "vtkTransform.h"

#include <cassert>

vtkStandardNewMacro(vtkGeoAlignedImageSource);
vtkCxxSetObjectMacro(vtkGeoAlignedImageSource, Image, vtkImageData);

class vtkGeoAlignedImageSource::vtkProgressObserver : public vtkCommand
{
public:
  static vtkProgressObserver* New()
    { return new vtkProgressObserver(); }

  virtual void Execute(vtkObject *, unsigned long eventId,
    void *callData)
    {
    if (eventId == vtkCommand::ProgressEvent)
      {
      double progress = *reinterpret_cast<double*>(callData);
      progress = this->Offset + this->Scale * progress;
      if (this->Target)
        {
        this->Target->InvokeEvent(vtkCommand::ProgressEvent, &progress);
        }
      }
    }

  void SetTarget(vtkObject* t)
    {
    this->Target = t;
    }

  double Offset;
  double Scale;

private:
  vtkProgressObserver()
    {
    this->Target = 0;
    this->Offset = 0.0;
    this->Scale = 1.0;
    }
  vtkObject* Target;
};

//----------------------------------------------------------------------------
vtkGeoAlignedImageSource::vtkGeoAlignedImageSource()
{
  this->Image = 0;
  this->LevelImages = vtkMultiBlockDataSet::New();
  this->LatitudeRange[0] = -90;
  this->LatitudeRange[1] = 90;
  this->LongitudeRange[0] = -180;
  this->LongitudeRange[1] = 180;
  this->ProgressObserver = vtkProgressObserver::New();
  this->ProgressObserver->SetTarget(this);
  this->PowerOfTwoSize = true;
  this->Overlap = 0.0;
}

//-----------------------------------------------------------------------------
vtkGeoAlignedImageSource::~vtkGeoAlignedImageSource()
{
  this->SetImage(0);
  this->LevelImages->Delete();

  this->ProgressObserver->SetTarget(0);
  this->ProgressObserver->Delete();
  this->ProgressObserver = 0;
}

//-----------------------------------------------------------------------------
void vtkGeoAlignedImageSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Image: " << (this->Image ? "" : "(null)") << endl;
  if (this->Image)
    {
    this->Image->PrintSelf(os, indent.GetNextIndent());
    }
  os << indent << "LatitudeRange: " << this->LatitudeRange[0] << "," << this->LatitudeRange[1] << endl;
  os << indent << "LongitudeRange: " << this->LongitudeRange[0] << "," << this->LongitudeRange[1] << endl;
  os << indent << "PowerOfTwoSize: " << (this->PowerOfTwoSize ? "On" : "Off") << endl;
  os << indent << "Overlap: " << this->Overlap << endl;
}

//-----------------------------------------------------------------------------
bool vtkGeoAlignedImageSource::FetchRoot(vtkGeoTreeNode* r)
{
  vtkGeoImageNode* root = 0;
  if (!(root = vtkGeoImageNode::SafeDownCast(r)))
    {
    vtkErrorMacro(<< "Node must be an image node for this source.");
    return false;
    }
  int imageDims[3];
  this->Image->GetDimensions(imageDims);

  // I am ignoring the geometry of the image, and assuming the scalars
  // are cell data.  The normal shrink should not shift the image by half
  // a pixel.  I believe texture maps will preserve the image bounds.
  vtkSmartPointer<vtkImageShrink3D> shrink = vtkSmartPointer<vtkImageShrink3D>::New();
  shrink->SetShrinkFactors(2,2,1);
  shrink->AveragingOn();
  shrink->AddObserver(vtkCommand::ProgressEvent, this->ProgressObserver);

  // We count the number of times vtkImageShrink3D will be executed so that we
  // can report progress correctly.
  int numLevels = 0;
  while (imageDims[0] > 300 || imageDims[1] > 300)
    {
    imageDims[0] = static_cast<int>(floor(imageDims[0] /
        static_cast<double>(shrink->GetShrinkFactors()[0])));
    imageDims[1] = static_cast<int>(floor(imageDims[1] /
        static_cast<double>(shrink->GetShrinkFactors()[1])));
    numLevels++;
    }
  this->Image->GetDimensions(imageDims);

  // Nothing says that the images cannot overlap and be larger than
  // the terain pathces.  Nothing says that the images have to be
  // a same size for all nodes either.

  // The easiest this to do to get multiple resolutions is to reduce
  // the image size before traversing.  This way we can avoid issues
  // with the bottom up approach.  Specifically, we do not need
  // to combine tiles, or worry about seams from smoothing.

  // This is not the best termination condition, but it will do.
  // This should also work for images that do not cover the whole globe.
  vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
  image->ShallowCopy(this->Image);
  vtkSmartPointer<vtkImageData> fullImage = vtkSmartPointer<vtkImageData>::New();
  fullImage->ShallowCopy(this->Image);
  vtkSmartPointer<vtkMultiBlockDataSet> tempBlocks = vtkSmartPointer<vtkMultiBlockDataSet>::New();
  tempBlocks->SetBlock(0, fullImage);
  for (unsigned int curIter=0; imageDims[0] > 300 || imageDims[1] > 300; ++curIter)
    {
    this->ProgressObserver->Offset = curIter * 1.0/numLevels;
    this->ProgressObserver->Scale = 1.0/numLevels;

    // Shrink image for the next level.
    shrink->SetInputData(image);
    shrink->Update();
    image->ShallowCopy(shrink->GetOutput());
    shrink->SetInputData(0);
    image->GetDimensions(imageDims);

    // Store the image for the level.
    vtkSmartPointer<vtkImageData> block = vtkSmartPointer<vtkImageData>::New();
    block->ShallowCopy(shrink->GetOutput());
    block->SetOrigin(-180, -90, 0);
    block->SetSpacing(180, 90, 0);
    tempBlocks->SetBlock(curIter+1, block);
    }

  // Reverse the coarsened images so they are in order by level.
  for (unsigned int block = 0; block < tempBlocks->GetNumberOfBlocks(); ++block)
    {
    this->LevelImages->SetBlock(tempBlocks->GetNumberOfBlocks() - 1 - block,
      tempBlocks->GetBlock(block));
    }
  vtkSmartPointer<vtkTexture> texture = vtkSmartPointer<vtkTexture>::New();
  texture->SetInputData(this->LevelImages->GetBlock(0));
  vtkSmartPointer<vtkTransform> texTrans = vtkSmartPointer<vtkTransform>::New();
  // Start with (lat,lon)
  texTrans->PostMultiply();
  texTrans->RotateZ(90.0); // (-lon,lat)
  texTrans->Scale(-1.0, 1.0, 1.0); // (lon,lat)
  texTrans->Translate(-(-180.0), -(-90.0), 0.0); // to origin
  texTrans->Scale(1.0/360.0, 1.0/180.0, 1.0); // to [0,1]
  texture->SetTransform(texTrans);
  texture->InterpolateOn();
  texture->RepeatOff();
  texture->EdgeClampOn();

  root->SetLevel(-1);
  root->SetLatitudeRange(-270, 90);
  root->SetLongitudeRange(-180, 180);
  root->SetTexture(texture);
  return true;
}

//------------------------------------------------------------------------------
bool vtkGeoAlignedImageSource::FetchChild(vtkGeoTreeNode* p, int index, vtkGeoTreeNode* c)
{
  vtkGeoImageNode* parent = 0;
  if (!(parent = vtkGeoImageNode::SafeDownCast(p)))
    {
    vtkErrorMacro(<< "Node must be an image node for this source.");
    return false;
    }
  vtkGeoImageNode* child = 0;
  if (!(child = vtkGeoImageNode::SafeDownCast(c)))
    {
    vtkErrorMacro(<< "Node must be an image node for this source.");
    return false;
    }
  int level = parent->GetLevel() + 1;
  unsigned int blockLevel = level + 1;
  if (blockLevel >= this->LevelImages->GetNumberOfBlocks())
    {
    vtkDebugMacro(<< "Reached max number of blocks (" << this->LevelImages->GetNumberOfBlocks() << ")");
    return false;
    }

  double lonRange[2];
  double latRange[2];
  double center[2];
  parent->GetLongitudeRange(lonRange);
  parent->GetLatitudeRange(latRange);
  center[0] = (lonRange[1] + lonRange[0])/2.0;
  center[1] = (latRange[1] + latRange[0])/2.0;

  child->SetLevel(level);
  if (index / 2)
    {
    child->SetLatitudeRange(center[1], latRange[1]);
    }
  else
    {
    child->SetLatitudeRange(latRange[0], center[1]);
    }
  if (index % 2)
    {
    child->SetLongitudeRange(center[0], lonRange[1]);
    }
  else
    {
    child->SetLongitudeRange(lonRange[0], center[0]);
    }

  int id = 0;
  if (level == 0)
    {
    // Special case: in the first level, the western hemisphere has id 0, and
    // the eastern hemisphere has id 1. This is to be compatible with the old
    // tile database format.
    if (index == 2)
      {
      id = 0;
      }
    else if (index == 3)
      {
      id = 1;
      }
    else if (index == 0)
      {
      vtkSmartPointer<vtkImageData> dummyImageWest = vtkSmartPointer<vtkImageData>::New();
      dummyImageWest->SetOrigin(-180.0, -270.0, 0.0);
      dummyImageWest->SetSpacing(0.0, -90.0, 0.0);
      child->GetTexture()->SetInputData(dummyImageWest);
      child->SetLatitudeRange(-270, -90);
      child->SetLongitudeRange(-180, 0);
      child->SetId(2);
      return true;
      }
    else if (index == 1)
      {
      vtkSmartPointer<vtkImageData> dummyImageEast = vtkSmartPointer<vtkImageData>::New();
      dummyImageEast->SetOrigin(0.0, -270.0, 0.0);
      dummyImageEast->SetSpacing(180.0, -90.0, 0.0);
      child->GetTexture()->SetInputData(dummyImageEast);
      child->SetLatitudeRange(-270, -90);
      child->SetLongitudeRange(0, 180);
      child->SetId(3);
      return true;
      }
    }
  else
    {
    id = parent->GetId() | (index << (2*level - 1));
    }
  child->SetId(id);

  // Crop and save the image.
  // Overwrite an image if it already exists.
  this->CropImageForNode(child, vtkImageData::SafeDownCast(this->LevelImages->GetBlock(blockLevel)));
  return true;
}

//------------------------------------------------------------------------------
void vtkGeoAlignedImageSource::CropImageForNode(vtkGeoImageNode* node, vtkImageData* image)
{
  int ext[6];
  int wholeExt[6];

  // I am keeping this all external to the imageData object because
  // I consider pixels are cells not points.
  image->GetExtent(ext);
  image->GetExtent(wholeExt);
  double origin[2];
  double spacing[2];
  spacing[0] = (this->LongitudeRange[1]-this->LongitudeRange[0])/static_cast<double>(ext[1]-ext[0]+1);
  spacing[1] = (this->LatitudeRange[1]-this->LatitudeRange[0])/static_cast<double>(ext[3]-ext[2]+1);
  origin[0] = this->LongitudeRange[0] - ext[0]*spacing[0];
  origin[1] = this->LatitudeRange[0] - ext[2]*spacing[1];

  // Compute the minimum extent that covers the terrain patch.
  double overlapDist[2];
  overlapDist[0] = this->Overlap*(node->GetLongitudeRange()[1]-node->GetLongitudeRange()[0]);
  overlapDist[1] = this->Overlap*(node->GetLatitudeRange()[1]-node->GetLatitudeRange()[0]);
  ext[0] = static_cast<int>(floor((node->GetLongitudeRange()[0]-overlapDist[0]-origin[0])/spacing[0]));
  ext[1] = static_cast<int>(ceil((node->GetLongitudeRange()[1]+overlapDist[0]-origin[0])/spacing[0]));
  ext[2] = static_cast<int>(floor((node->GetLatitudeRange()[0]-overlapDist[1]-origin[1])/spacing[1]));
  ext[3] = static_cast<int>(ceil((node->GetLatitudeRange()[1]+overlapDist[1]-origin[1])/spacing[1]));

  int dims[2];
  if (this->PowerOfTwoSize)
    {
    dims[0] = this->PowerOfTwo(ext[1]-ext[0]+1);
    dims[1] = this->PowerOfTwo(ext[3]-ext[2]+1);
    ext[1] = ext[0] + dims[0] - 1;
    ext[3] = ext[2] + dims[1] - 1;
    }
  else
    {
    dims[0] = ext[1]-ext[0]+1;
    dims[1] = ext[3]-ext[2]+1;
    }

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

  vtkSmartPointer<vtkImageData> cropped = vtkSmartPointer<vtkImageData>::New();
  cropped->ShallowCopy(image);
  cropped->Crop(ext);

  // Now set the longitude and latitude range based on the actual image size.
  double lonRange[2];
  double latRange[2];
  lonRange[0] = origin[0] + ext[0]*spacing[0];
  lonRange[1] = origin[0] + (ext[1]+1)*spacing[0];
  latRange[0] = origin[1] + ext[2]*spacing[1];
  latRange[1] = origin[1] + (ext[3]+1)*spacing[1];
  cropped->SetOrigin(lonRange[0], latRange[0], 0);
  cropped->SetSpacing(lonRange[1], latRange[1], 0);
  //assert(lonRange[1] >= lonRange[0]);
  //assert(latRange[1] >= latRange[0]);

  vtkSmartPointer<vtkTexture> tex = vtkSmartPointer<vtkTexture>::New();
  vtkSmartPointer<vtkTransform> texTrans = vtkSmartPointer<vtkTransform>::New();
  // Start with (lat,lon)
  texTrans->PostMultiply();
  texTrans->RotateZ(90.0); // (-lon,lat)
  texTrans->Scale(-1.0, 1.0, 1.0); // (lon,lat)
  texTrans->Translate(-lonRange[0], -latRange[0], 0.0); // to origin
  texTrans->Scale(1.0/(lonRange[1] - lonRange[0]), 1.0/(latRange[1] - latRange[0]), 1.0); // to [0,1]
  tex->SetTransform(texTrans);
  tex->SetInputData(cropped);
  tex->InterpolateOn();
  tex->RepeatOff();
  tex->EdgeClampOn();

  node->SetTexture(tex);
}

//-----------------------------------------------------------------------------
int vtkGeoAlignedImageSource::PowerOfTwo(int val)
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

