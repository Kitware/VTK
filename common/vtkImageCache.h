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
class vtkImageSource;
class vtkImageRegion;
#include "vtkImageData.h"
#include "vtkObject.h"

class VTK_EXPORT vtkImageCache : public vtkObject
{
public:
  vtkImageCache();
  ~vtkImageCache();
  static vtkImageCache *New() {return new vtkImageCache;};
  const char *GetClassName() {return "vtkImageCache";};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // This method sets the instance variable "UpdateExtent" which specifies
  // the extent of the image that will be updated. Extent is specified and
  // a (min,max) value for each axis (in the order X, Y, Z, Time).
  // All the "Components" of vectors and scalars are generated all the time.
  // If "UpdateExtent" has not been set by the first update, it defaults
  // to the "WholeExtent".  If the UpdateExtent is larger than the 
  // "WholeExtent" then "UpdateExtent" will be reduced, 
  // and a waring message will occur.
  void SetUpdateExtent(int extent[8]);
  void SetUpdateExtent(int xMin, int xMax, int yMin, int yMax,
		       int zMin, int zMax, int tMin, int tMax);
  void SetAxesUpdateExtent(int num, int *axes, int *extent);
  vtkImageSetExtentAxesMacro(UpdateExtent, int);
  void SetUpdateExtentToWholeExtent();
  
  void GetUpdateExtent(int extent[8]);
  int *GetUpdateExtent() {return this->UpdateExtent;}
  void GetUpdateExtent(int &xMin, int &xMax, int &yMin, int &yMax,
		       int &zMin, int &zMax, int &tMin, int &tMax);
  void GetAxesUpdateExtent(int num, int *axes, int *extent);
  vtkImageGetExtentAxesMacro(UpdateExtent, int);
  void ClipUpdateExtentWithWholeExtent();
  
  virtual void Update();
  virtual void UpdateImageInformation();
  virtual unsigned long GetPipelineMTime();
  
  vtkImageRegion *GetScalarRegion();
  vtkImageRegion *GetVectorRegion();

  void SetWholeUpdateExtent(int *extent);
    
  // Description:
  // These methods give access to the cached image information.
  // "UpdateImageInformation", or "Update" should be called before 
  //  the get methods are called..  The set methods are used by
  // the source to update the values.
  void SetSpacing(float spacing[4]);
  void SetSpacing(float x, float y, float z, float t);
  void SetAxesSpacing(int num, int *axes, float *spacing);
  vtkImageSetAxesMacro(Spacing,float);
  void GetSpacing(float spacing[4]);
  float *GetSpacing() {return this->Spacing;}
  void GetSpacing(float &x, float &y, float &z, float &time);
  void GetAxesSpacing(int num, int *axes, float *spacing);
  vtkImageGetAxesMacro(Spacing,float);
  void SetOrigin(float origin[4]);
  void SetOrigin(float x, float y, float z, float t);
  void SetAxesOrigin(int num, int *axes, float *origin);
  vtkImageSetAxesMacro(Origin, float);
  void GetOrigin(float origin[4]);
  float *GetOrigin() {return this->Origin;}
  void GetOrigin(float &x, float &y, float &z, float &time);
  void GetAxesOrigin(int num, int *axes, float *origin);
  vtkImageGetAxesMacro(Origin, float);
  void SetWholeExtent(int extent[8]);
  void SetWholeExtent(int xMin, int xMax, int yMin, int yMax,
		      int zMin, int zMax, int tMin, int tMax);
  void SetAxesWholeExtent(int num, int *axes, int *extent);
  vtkImageSetExtentAxesMacro(WholeExtent,int);
  void GetWholeExtent(int extent[8]);
  int *GetWholeExtent() {return this->WholeExtent;}
  void GetWholeExtent(int &xMin, int &xMax, int &yMin, int &yMax,
		      int &zMin, int &zMax, int &tMin, int &tMax);
  void GetAxesWholeExtent(int num, int *axis, int *extent);
  vtkImageGetExtentAxesMacro(WholeExtent,int);
  
  // Description:
  // These duplicate the above and also provide compatability 
  // with vtkImageStructuredPoints.  Note: The result of these calls
  // depends on the coordinate system!  Note:  These methods provide 
  // image information, not the data in the cache.
  void GetDimensions(int dimensions[4]);
  int *GetDimensions() {return this->Dimensions;}
  void GetDimensions(int &x, int &y, int &z, int &time);
  void GetAxesDimensions(int num, int *axes, int *dimensions);
  vtkImageGetAxesMacro(Dimensions,int);
  
  void GetCenter(float center[4]);
  float *GetCenter() {return this->Center;}
  void GetCenter(float &x, float &y, float &z, float &time);
  void GetAxesCenter(int num, int *axes, float *center);
  vtkImageGetAxesMacro(Center,float);

  void GetBounds(float bounds[8]);
  float *GetBounds() {return this->Bounds;}
  void GetBounds(float &xMin, float &xMax, float &yMin, float &yMax,
		 float &zMin, float &zMax, float &tMin, float &tMax);
  void GetAxesBounds(int num, int *axes, float *bounds);
  vtkImageGetExtentAxesMacro(Bounds,float);
  
  // Description:
  // Set/Get the source associated with this cache
  vtkSetObjectMacro(Source,vtkImageSource);
  vtkGetObjectMacro(Source,vtkImageSource);

  // Description:
  // Turn the caching of data on or off.  When this flag is off,
  // The cache releases the data imediately after the region is returned
  // by the methods "GetScalarRegion", and "GetVectorRegion".
  // The default value of "ReleaseDataFlag" is on.
  void SetReleaseDataFlag(int value);
  vtkGetMacro(ReleaseDataFlag,int);
  vtkBooleanMacro(ReleaseDataFlag,int);

  // Description:
  // Turn on/off flag to control whether every object releases its data
  // after being used by a filter.  These methods do nothing for now.
  void SetGlobalReleaseDataFlag(int val);
  void GlobalReleaseDataFlagOn() {this->SetGlobalReleaseDataFlag(1);};
  void GlobalReleaseDataFlagOff() {this->SetGlobalReleaseDataFlag(0);};
  int  GetGlobalReleaseDataFlag(); 
  
  virtual void ReleaseData();

  // Description:
  // Return flag indicating whether data should be released after use  
  // by a filter.
  int ShouldIReleaseData();  
  
  // Description:
  // Keep track of when data was released for pipeline execution test.
  vtkSetMacro(DataReleased,int);
  vtkGetMacro(DataReleased,int); 
  
  // Description:
  // Set/Get the data scalar type of the regions created by this cache.
  void SetScalarTypeToFloat(){this->SetScalarType(VTK_FLOAT);}
  void SetScalarTypeToInt(){this->SetScalarType(VTK_INT);}
  void SetScalarTypeToShort(){this->SetScalarType(VTK_SHORT);}
  void SetScalarTypeToUnsignedShort(){this->SetScalarType(VTK_UNSIGNED_SHORT);}
  void SetScalarTypeToUnsignedChar(){this->SetScalarType(VTK_UNSIGNED_CHAR);}
  vtkSetMacro(ScalarType,int);
  vtkGetMacro(ScalarType,int);

  long GetUpdateExtentMemorySize();

  // Description:
  // The number of scalar components refers to the number of scalars
  // per pixel to store color information.
  vtkSetMacro(NumberOfScalarComponents,int);
  vtkGetMacro(NumberOfScalarComponents,int);
  
  // Description:
  // Here for Bypass functionality
  vtkSetReferenceCountedObjectMacro(ScalarData, vtkImageData);
  vtkGetObjectMacro(ScalarData, vtkImageData);

  vtkImageToStructuredPoints *GetImageToStructuredPoints();
  
  
protected:
  int UpdateExtent[8];
  // Save stuff to figure whether we need to execute.
  int ExecuteExtent[8];
  vtkTimeStamp ExecuteTime;
  vtkTimeStamp ExecuteImageInformationTime;
  
  vtkImageSource *Source;
  vtkImageToStructuredPoints *ImageToStructuredPoints;
  int ReleaseDataFlag;
  int DataReleased;

  // ImageInformation
  float Spacing[4];
  float Origin[4];
  int WholeExtent[8];
  int NumberOfScalarComponents;
  int NumberOfVectorComponents;
  // This is for vtkStructuredPoints compatability.  
  // These variables are redundant.
  int Dimensions[4];
  float Center[4];
  float Bounds[8];
  unsigned long PipelineMTime;
  // The cache manipulates (and returns) regions with this data type.
  int ScalarType;
  // Cached data (may have different extents)
  vtkImageData *ScalarData;
  vtkImageData *VectorData;
  
  void UpdateImageInformation(unsigned long pipelineMTime);
  void ComputeBounds();
  vtkTimeStamp ComputeBoundsTime;
};

#endif


