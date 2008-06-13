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
#include "vtkImageShrink3D.h"
#include "vtkJPEGReader.h"
#include "vtkObjectFactory.h"
#include "vtkTimerLog.h"

vtkCxxRevisionMacro(vtkGeoAlignedImageSource, "1.2");
vtkStandardNewMacro(vtkGeoAlignedImageSource);


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
  this->UseTileDatabase = false;
  this->TileDatabaseLocation = 0;
  this->TileDatabaseDepth = 0;
  this->WesternHemisphere = vtkSmartPointer<vtkGeoImageNode>::New();
  this->WesternHemisphere->SetLongitudeRange(-180.0, 0.0);
  this->WesternHemisphere->SetLatitudeRange(-90.0, 90.0);
  this->WesternHemisphere->SetId(0);
  this->EasternHemisphere = vtkSmartPointer<vtkGeoImageNode>::New();
  this->EasternHemisphere->SetLongitudeRange(0.0, 180.0);
  this->EasternHemisphere->SetLatitudeRange(-90.0, 90.0);
  this->EasternHemisphere->SetId(1);

  this->ProgressObserver = vtkProgressObserver::New();
  this->ProgressObserver->SetTarget(this);
}

//-----------------------------------------------------------------------------
vtkGeoAlignedImageSource::~vtkGeoAlignedImageSource() 
{ 
  this->ProgressObserver->SetTarget(0);
  this->ProgressObserver->Delete();
  this->ProgressObserver = 0;

  this->SetTileDatabaseLocation(0);
}

//-----------------------------------------------------------------------------
void vtkGeoAlignedImageSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//-----------------------------------------------------------------------------
// This assumes a jpeg image for now.
void vtkGeoAlignedImageSource::LoadAnImage(
  const char* fileName, 
  double imageLonLatExtent[4],
  const char* dbLocation)
{
  vtkSmartPointer<vtkJPEGReader> reader 
    = vtkSmartPointer<vtkJPEGReader>::New();

  if ( ! reader->CanReadFile(fileName))
    {
    vtkErrorMacro("Cannot read file " << fileName);
    return;
    }
  
  reader->SetFileName(fileName);
  reader->Update();
  vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
  image->ShallowCopy(reader->GetOutput());
  this->LoadAnImage(image, imageLonLatExtent, dbLocation);
}

//-----------------------------------------------------------------------------
void vtkGeoAlignedImageSource::LoadAnImage(
  vtkImageData* inImage,
  double imageLonLatExtent[4],
  const char* dbLocation)
{
  vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
  image->ShallowCopy(inImage);
  int imageDims[3];
  image->GetDimensions(imageDims);

  // I am ignoring the geometry of the image, and assuming the scalars
  // are cell data.  The normal shrink should not shift the image by half
  // a pixel.  I believe texture maps will preserve the image bounds.
  vtkSmartPointer<vtkImageShrink3D> shrink = vtkSmartPointer<vtkImageShrink3D>::New();
  shrink->SetShrinkFactors(2,2,1);
  shrink->AveragingOn();
  shrink->AddObserver(vtkCommand::ProgressEvent, this->ProgressObserver);

  // We count the number of times vtkImageShrink3D will be executed so that we
  // can report progress correctly.
  int numIterations = 0;
  while (imageDims[0] > 300 || imageDims[1] > 300)
    {
    imageDims[0] = static_cast<int>(floor(imageDims[0] / 
        static_cast<double>(shrink->GetShrinkFactors()[0])));
    imageDims[1] = static_cast<int>(floor(imageDims[1] / 
        static_cast<double>(shrink->GetShrinkFactors()[1])));
    numIterations++;
    }
  image->GetDimensions(imageDims);

  // Nothing says that the images cannot overlap and be larger than
  // the terain pathces.  Nothing says that the images have to be
  // a same size for all nodes either.

  // The easiest this to do to get multiple resolutions is to reduce
  // the image size before traversing.  This way we can avoid issues
  // with the bottom up approach.  Specifically, we do not need
  // to combine tiles, or worry about seams from smoothing.
  
  // This is not the best termination condition, but it will do.
  // This should also work for images that do not cover the whole globe.
  for (int curIter=0; imageDims[0] > 300 || imageDims[1] > 300; ++curIter)
    {
    this->ProgressObserver->Offset = curIter * 1.0/numIterations;
    this->ProgressObserver->Scale = 1.0/numIterations;

    // Crop and set images for leaves (by recursing).
    // This creates the intermediate nodes (without images)
    // if necessary.
    this->AddImageToTree(this->WesternHemisphere, image,
      imageLonLatExtent, dbLocation);
    this->AddImageToTree(this->EasternHemisphere, image,
      imageLonLatExtent, dbLocation);
    // Shrink image for the next level.
    shrink->SetInput(image);
    shrink->Update();
    image->ShallowCopy(shrink->GetOutput());
    shrink->SetInput(0);
    image->GetDimensions(imageDims);
    }
}

//-----------------------------------------------------------------------------
void vtkGeoAlignedImageSource::LoadTiles(const char* loc, vtkGeoImageNode* n)
{
  if (!n)
    {
    this->UseTileDatabase = true;
    this->SetTileDatabaseLocation(loc);

    // Find the max depth of the database.
    this->TileDatabaseDepth = 0;
    vtksys_ios::ifstream in;
    char tileFile[100];
    sprintf(tileFile, "%s/tile_%d_0.vti", loc, this->TileDatabaseDepth);
    in.open(tileFile, ifstream::in);
    while (!in.fail())
      {
      in.close();
      this->TileDatabaseDepth++;
      sprintf(tileFile, "%s/tile_%d_0.vti", loc, this->TileDatabaseDepth);
      in.open(tileFile, ifstream::in);
      }
    in.close();
    this->TileDatabaseDepth--;

    this->LoadTiles(loc, this->WesternHemisphere);
    this->LoadTiles(loc, this->EasternHemisphere);
    return;
    }
  n->LoadAnImage(loc);
  // For now, just load the top level.
  // Load other levels on demand.
#if 0
  if (n->GetLevel() < this->TileDatabaseDepth)
    {
    n->CreateChildren();
    for (int c = 0; c < 4; ++c)
      {
      this->LoadTiles(loc, n->GetChild(c));
      }
    }
#endif
}

//------------------------------------------------------------------------------
void vtkGeoAlignedImageSource::AddImageToTree(
  vtkGeoImageNode* branch,
  vtkImageData* image,
  double imageLonLatExt[4],
  const char* dbLocation) 
{
  double* longitudeRange = branch->GetLongitudeRange();
  double* latitudeRange = branch->GetLatitudeRange();

  // The image must cover the terrain or we can not use it for this node.
  if (imageLonLatExt[0] > longitudeRange[0] ||
      imageLonLatExt[1] < longitudeRange[1] ||
      imageLonLatExt[2] > latitudeRange[0] ||
      imageLonLatExt[3] < latitudeRange[1])
    { // Other options are to write on top of an existing image.
    return;
    }
  // Have we reached a leaf?
  int dims[3];
  image->GetDimensions(dims);
  // Compute the dimensions of the tile for this node.
  dims[0] = (int)((double)(dims[0])
                    * (longitudeRange[1]-longitudeRange[0])
                    / (imageLonLatExt[1]-imageLonLatExt[0]));
  dims[1] = (int)((double)(dims[1])
                    * (latitudeRange[1]-latitudeRange[0])
                    / (imageLonLatExt[3]-imageLonLatExt[2]));
  if (dims[0] < 300 && dims[1] < 300)
    { // The image is small enough to be a leaf.
    // Crop and save the image.
    // Overwrite an image if it already exists.
    branch->CropImageForTile(image,imageLonLatExt, dbLocation);
    return;
    }

  // Recurse to children.
  branch->CreateChildren();
  this->AddImageToTree(branch->GetChild(0), image, imageLonLatExt, dbLocation);
  this->AddImageToTree(branch->GetChild(1), image, imageLonLatExt, dbLocation);
  this->AddImageToTree(branch->GetChild(2), image, imageLonLatExt, dbLocation);
  this->AddImageToTree(branch->GetChild(3), image, imageLonLatExt, dbLocation);
}

