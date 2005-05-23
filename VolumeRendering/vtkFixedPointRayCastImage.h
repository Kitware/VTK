/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFixedPointRayCastImage.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkFixedPointRayCastImage - helper class for a ray cast image
// .SECTION Description
// This is a helper class for storing the ray cast image including the
// underlying data and the size of the image. This class is not intended
// to be used directly - just as an internal class in the 
// vtkFixedPointVolumeRayCastMapper so that multiple mappers can share
// the same image. Perhaps this class could be generalized
// in the future to be used for other ray cast methods other than the
// fixed point method.

// .SECTION see also
// vtkFixedPointVolumeRayCastMapper

#ifndef __vtkFixedPointRayCastImage_h
#define __vtkFixedPointRayCastImage_h

#include "vtkObject.h"

class VTK_VOLUMERENDERING_EXPORT vtkFixedPointRayCastImage : public vtkObject
{
public:
  static vtkFixedPointRayCastImage *New();
  vtkTypeRevisionMacro(vtkFixedPointRayCastImage,vtkObject);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the internal storage for the image. It is a pointer to
  // unsigned short with four components (RGBA) per pixel. This
  // memory is allocated when the AllocateImage method is called.
  unsigned short *GetImage() {return this->Image;}
  
  // Description:
  // Set / Get the ImageViewportSize. This is the size of the
  // whole viewport in pixels.
  vtkSetVector2Macro( ImageViewportSize, int );
  vtkGetVectorMacro(  ImageViewportSize, int, 2 );

  // Description:
  // Set / Get the ImageMemorySize. This is the size in pixels
  // of the Image ivar. This will be a power of two in order
  // to ensure that the texture can be rendered by graphics
  // hardware that requires power of two textures.
  vtkSetVector2Macro( ImageMemorySize, int );
  vtkGetVectorMacro(  ImageMemorySize, int, 2 );
  
  // Description:
  // Set / Get the size of the image we are actually using. As
  // long as the memory size is big enough, but not too big,
  // we won't bother deleting and re-allocated, we'll just 
  // continue to use the memory size we have. This size will
  // always be equal to or less than the ImageMemorySize.
  vtkSetVector2Macro( ImageInUseSize, int );
  vtkGetVectorMacro(  ImageInUseSize, int, 2 );
  
  
  // Description:
  // Set / Get the origin of the image. This is the starting 
  // pixel within the whole viewport that our Image starts on.
  // That is, we could be generating just a subregion of the
  // whole viewport due to the fact that our volume occupies 
  // only a portion of the viewport. The Image pixels will
  // start from this location.
  vtkSetVector2Macro( ImageOrigin, int );
  vtkGetVectorMacro(  ImageOrigin, int, 2 );

  // Description:
  // Call this method once the ImageMemorySize has been set
  // the allocate the image. If an image already exists,
  // it will be deleted first.
  void AllocateImage();
  
  // Description:
  // Clear the image to (0,0,0,0) for each pixel.
  void ClearImage();
  
protected:
  vtkFixedPointRayCastImage();
  ~vtkFixedPointRayCastImage();
  
  // This is how big the image would be if it covered the entire viewport
  int             ImageViewportSize[2];
  
  // This is how big the allocated memory for image is. This may be bigger
  // or smaller than ImageFullSize - it will be bigger if necessary to 
  // ensure a power of 2, it will be smaller if the volume only covers a
  // small region of the viewport
  int             ImageMemorySize[2];
  
  // This is the size of subregion in ImageSize image that we are using for
  // the current image. Since ImageSize is a power of 2, there is likely
  // wasted space in it. This number will be used for things such as clearing
  // the image if necessary.
  int             ImageInUseSize[2];
  
  // This is the location in ImageFullSize image where our ImageSize image
  // is located.
  int             ImageOrigin[2];
  
  // This is the allocated image
  unsigned short  *Image;
  
private:
  vtkFixedPointRayCastImage(const vtkFixedPointRayCastImage&);  // Not implemented.
  void operator=(const vtkFixedPointRayCastImage&);  // Not implemented.
};

#endif


