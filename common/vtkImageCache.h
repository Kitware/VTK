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
// .NAME vtkImageCache - Caches are used by vtkImageSource.
// .SECTION Description
// vtkImageCache is the super class of all image caches.  
// If the cached source descides to generate in pieces, the caches 
// collects all of the pieces into a single vtkImageRegion object.
// The cache can also save vtkImageData objects between UpdateRegion
// messages, to avoid regeneration of data.  Since regions
// can be any size or location, caching strategies can be
// numerous and complex.  Applications can create caches that fit
// their special needs.
// The caches access methods do not make any internal checks to make
// sure the data is up to date.  Update has to be called explicitly.
// to ensure the information retrieved is valid.


#ifndef __vtkImageCache_h
#define __vtkImageCache_h
class vtkImageToStructuredPoints;
#include "vtkReferenceCount.h"
#include "vtkImageSource.h"
#include "vtkImageData.h"

class VTK_EXPORT vtkImageCache : public vtkReferenceCount
{
public:
  vtkImageCache();
  ~vtkImageCache();
  const char *GetClassName() {return "vtkImageCache";};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // This method sets the instance variable "UpdateExtent" which specifies
  // the extent of the image that will be updated. Extent is specified and
  // a (min,max) value for each axis (in the order X, Y, Z).
  // All the "Components" of vectors and scalars are generated all the time.
  // If "UpdateExtent" has not been set by the first update, it defaults
  // to the "WholeExtent".  If the UpdateExtent is larger than the 
  // "WholeExtent" then "UpdateExtent" will be reduced, 
  // and a waring message will occur.
  void SetUpdateExtent(int extent[6]);
  void SetUpdateExtent(int xMin, int xMax,
		       int yMin, int yMax, int zMin, int zMax);
  void SetAxisUpdateExtent(int axis, int min, int max);
  void SetUpdateExtentToWholeExtent();
  
  void GetUpdateExtent(int extent[6]);
  int *GetUpdateExtent() {return this->UpdateExtent;}
  void GetUpdateExtent(int &xMin, int &xMax,
		       int &yMin, int &yMax, int &zMin, int &zMax);
  void GetAxisUpdateExtent(int axis, int &min, int &max);
  void ClipUpdateExtentWithWholeExtent();
  
  virtual void Update() = 0;
  virtual vtkImageData *UpdateAndReturnData() = 0;
  virtual void ReleaseData() {};

  // supplied by subclass: just return the data object associated with
  // the UpdateExtent. (Law)
  // I need this for vtkImageIterateFilter Ken.
  virtual vtkImageData *GetData() = 0;  
  
  // Description:
  // Return flag indicating whether data should be released after use  
  // by a filter.
  int ShouldIReleaseData();

  virtual void UpdateImageInformation();
  virtual unsigned long GetPipelineMTime();
  
  // Description:
  // These methods give access to the cached image information.
  // "UpdateImageInformation", or "Update" should be called before 
  //  the get methods are called..  The set methods are used by
  // the source to update the values.
  // Description:
  vtkSetVector3Macro(Spacing,float);
  vtkGetVectorMacro(Spacing,float,3);
  vtkSetVector3Macro(Origin,float);
  vtkGetVectorMacro(Origin,float,3);
  void SetWholeExtent(int extent[6]);
  void SetWholeExtent(int xMin, int xMax,
		      int yMin, int yMax, int zMin, int zMax);
  void GetWholeExtent(int extent[6]);
  int *GetWholeExtent() {return this->WholeExtent;}
  void GetWholeExtent(int &xMin, int &xMax,
		      int &yMin, int &yMax, int &zMin, int &zMax);
  
  // Description:
  // These duplicate the above and also provide compatability 
  // with vtkImageStructuredPoints.  Note: The result of these calls
  // depends on the coordinate system!  Note:  These methods provide 
  // image information, not the data in the cache.
  void GetDimensions(int dimensions[3]);
  void GetDimensions(int &x, int &y, int &z);
  
  void GetCenter(float center[3]);
  void GetCenter(float &x, float &y, float &z);

  void GetBounds(float bounds[6]);
  void GetBounds(float &xMin, float &xMax, float &yMin, float &yMax,
		 float &zMin, float &zMax);
  float *GetBounds() {this->GetBounds (this->Bounds); return this->Bounds;}
  
  // Description:
  // Set/Get the source associated with this cache
  vtkSetObjectMacro(Source,vtkImageSource);
  vtkGetObjectMacro(Source,vtkImageSource);

  long GetUpdateExtentMemorySize();

  vtkImageToStructuredPoints *GetImageToStructuredPoints();
  
  // Description:
  // Set the data scalar type of the regions created by this cache.
  vtkSetMacro(ScalarType,int);
  vtkGetMacro(ScalarType,int);
  
  // Description:
  // Set/Get the number of scalar components
  vtkSetMacro(NumberOfScalarComponents,int);
  vtkGetMacro(NumberOfScalarComponents,int);

  // Description:
  // Set/Get memory limit.  Make this smaller to stream.
  vtkSetMacro(MemoryLimit,long);
  vtkGetMacro(MemoryLimit,long);

  // Description:
  // Set/Get the DataReleased ivar.
  vtkSetMacro(DataReleased,int);
  vtkGetMacro(DataReleased,int);

  // Description:
  // Turn on/off flag to control whether this object's data is released
  // after being used by a filter.
  vtkSetMacro(ReleaseDataFlag,int);
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
  // Needed because not all objects are reference counted.
  void UnRegister(vtkObject* o);
  
protected:
  long MemoryLimit;
  int UpdateExtent[6];
  vtkTimeStamp ExecuteTime;
  int ScalarType;
  int NumberOfScalarComponents;
  
  vtkImageSource *Source;
  vtkImageToStructuredPoints *ImageToStructuredPoints;
  int ReleaseDataFlag;
  int DataReleased;

  // ImageInformation
  float Spacing[3];
  float Origin[3];
  int WholeExtent[6];
  float Bounds[6];

  // This is for vtkStructuredPoints compatability.  
  // These variables are redundant.
  unsigned long PipelineMTime;
};

#endif


