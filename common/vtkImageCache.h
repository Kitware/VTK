/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCache.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkImageCache - Caches are used by vtkImageCachedSource.
// .SECTION Description
// vtkImageCache is the super class of all filter caches.  
// If the cached source descides to generate in pieces, the caches 
// collects all of the pieces into a single vtkImageRegion object.
// The cache can also save vtkImageData objects between UpdateRegion
// messages, to avoid regeneration of data.  Since regions
// can be any size or location, caching strategies can be
// numerous and complex.  Some predefined generic subclasses have been 
// defined, but specific applications may need to implement subclasses
// with their own stratgies tailored to the pattern of regions generated.


#ifndef __vtkImageCache_h
#define __vtkImageCache_h
#include "vtkImageSource.h"
#include "vtkImageCachedSource.h"
#include "vtkImageData.h"
class vtkImageRegion;

class VTK_EXPORT vtkImageCache : public vtkImageSource
{
public:
  vtkImageCache();
  ~vtkImageCache();
  char *GetClassName() {return "vtkImageCache";};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  unsigned long int GetPipelineMTime();
  void UpdateImageInformation(vtkImageRegion *region);
  void Update(vtkImageRegion *region);
  vtkImageRegion *UpdateRegion();
  void Update();
  void UpdateImageInformation();
  
  // Description:
  // Set/Get the source associated with this cache
  vtkSetObjectMacro(Source,vtkImageCachedSource);
  vtkGetObjectMacro(Source,vtkImageCachedSource);

  // Description:
  // Turn the save data option on or off
  void SetReleaseDataFlag(int value);
  vtkGetMacro(ReleaseDataFlag,int);
  vtkBooleanMacro(ReleaseDataFlag,int);

  // Description:
  // Turn on/off flag to control whether every object releases its data
  // after being used by a filter.
  void SetGlobalReleaseDataFlag(int val);
  void GlobalReleaseDataFlagOn() {this->SetGlobalReleaseDataFlag(1);};
  void GlobalReleaseDataFlagOff() {this->SetGlobalReleaseDataFlag(0);};
  int  GetGlobalReleaseDataFlag(); 
  
  // Description:
  // Subclass implements this method to delete any cached data.
  virtual void ReleaseData() = 0;

  // Description:
  // This method saves a region in the cache for later reference.
  virtual void CacheRegion(vtkImageRegion *region) = 0;
  
  // Description:
  // Set/Get the data scalar type of the regions created by this cache.
  void SetScalarTypeToFloat(){this->SetScalarType(VTK_FLOAT);}
  void SetScalarTypeToInt(){this->SetScalarType(VTK_INT);}
  void SetScalarTypeToShort(){this->SetScalarType(VTK_SHORT);}
  void SetScalarTypeToUnsignedShort(){this->SetScalarType(VTK_UNSIGNED_SHORT);}
  void SetScalarTypeToUnsignedChar(){this->SetScalarType(VTK_UNSIGNED_CHAR);}
  vtkSetMacro(ScalarType,int);
  vtkGetMacro(ScalarType,int);

  // Description:
  // Set/Get the memory order of regions stored and returned by this cache.
  // The data along the first axis is colocated in memory.  NOTE: THIS WILL 
  // NOT AFFECT THE BEHAVIOR OF THE PIPELINE, ONLY THE SPEED OF PROCESSING.
  // Unless you are trying to tune your application, you will not need this 
  // method.  The underlying memory order is abstracted and hidden by
  // vtkImageRegion.
  void SetMemoryOrder(int num, int *axes);
  vtkImageSetMacro(MemoryOrder, int);
  void GetMemoryOrder(int num, int *axes);
  vtkImageGetMacro(MemoryOrder, int);

  // Description:
  // These methods allow direct access to the cached image information.
  void GetSpacing(int num, float *spacing);
  vtkImageGetMacro(Spacing, float);
  float *GetSpacing() {return this->Spacing;}
  void GetOrigin(int num, float *origin);
  vtkImageGetMacro(Origin, float);
  float *GetOrigin() {return this->Origin;}
  void GetImageExtent(int num, int *extent);
  vtkImageGetExtentMacro(ImageExtent);
  // Description:
  // These duplicate the above and also provide compatability 
  // with vtkImageStructuredPoints.  Note: The result of these calls
  // depends on the coordinate system!  Note:  These methods provide 
  // image information, not the data in the cache.
  void GetDimensions(int num, int *dimensions);
  vtkImageGetMacro(Dimensions, int);
  int *GetDimensions() {return this->Dimensions;}
  void GetCenter(int num, float *center);
  vtkImageGetMacro(Center, float);
  float *GetCenter() {return this->Center;}
  void GetBounds(int num, float *bounds);
  vtkGetVector3Macro(Bounds, float);

  void SetSpacing(int num, float *spacing);
  vtkImageSetMacro(Spacing, float);
  void SetOrigin(int num, float *origin);
  vtkImageSetMacro(Origin, float);

  vtkImageToStructuredPoints *GetImageToStructuredPoints();

protected:
  vtkImageCachedSource *Source;
  vtkImageToStructuredPoints *ImageToStructuredPoints;

  // Spacing, Dimensions, ... are accessed in this fixed (0, 1, 2, 3, 4)
  // coordinate system.  Should we allow the user to change the caches axes?
  int Axes[VTK_IMAGE_DIMENSIONS];
  
  //  to tell the cache to save data or not.
  int ReleaseDataFlag;

  // Cache the ImageExtent, to avoid recomputing the ImageExtent on each pass.
  vtkTimeStamp ImageInformationTime;
  float Spacing[VTK_IMAGE_DIMENSIONS];
  float Origin[VTK_IMAGE_DIMENSIONS];
  int ImageExtent[VTK_IMAGE_EXTENT_DIMENSIONS];
  // This is for vtkStructuredPoints compatability.  
  // These variables are redundant.
  int Dimensions[VTK_IMAGE_DIMENSIONS];
  float Center[VTK_IMAGE_DIMENSIONS];
  float Bounds[VTK_IMAGE_EXTENT_DIMENSIONS];

  // The cache manipulates (and returns) regions with this data type.
  int ScalarType;

  // The cache returns Regions with this underlying memoyy order.
  int MemoryOrder[VTK_IMAGE_DIMENSIONS];
  
  void GenerateUnCachedRegionData(vtkImageRegion *region);
  // Description:
  // This method is used by a subclass to first look to cached 
  // data.  It can also return null if the method
  // fails for any reason.
  virtual void GenerateCachedRegionData(vtkImageRegion *region) = 0;
  
};

#endif


