/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageRegion.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

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
// .NAME vtkImageRegion - Piece of image. Pixel type defaults to float.
// .SECTION Description
// vtkImageRegion holds a piece of an image. 
// The actual data for the image is stored in the
// vtkImageData object.  The vtkImageRegion can represent only a portion 
// of its vtkImageData, hiding the actual dimensions of the vtkImageData.
// It can also transparently reorder the axes of the data with out
// coping the data.
//   A region can now be used as an input to a filter, but the relative
// coordinates of the region are ignored.


#ifndef __vtkImageRegion_h
#define __vtkImageRegion_h


#include "vtkImageSource.h"
#include "vtkImageData.h"


// These macro are for creating the many convenience functions used 
// for accessing instance variables.  They could simplify this class.
#define vtkImageRegionSetMacro(name,type) \
void Set##name (type *_tmp) { this->Set##name (_tmp, 5);} \
void Set##name##5d(type *_tmp) { this->Set##name (_tmp, 5);} \
void Set##name##4d(type *_tmp) { this->Set##name (_tmp, 4);} \
void Set##name##3d(type *_tmp) { this->Set##name (_tmp, 3);} \
void Set##name##2d(type *_tmp) { this->Set##name (_tmp, 2);} \
void Set##name##1d(type *_tmp) { this->Set##name (_tmp, 1);} \
void Set##name##5d(type _name0,type _name1,type _name2, type _name3,type _name4) \
{ \
  type _tmp[5]; \
  _tmp[0] = _name0; _tmp[1] = _name1; _tmp[2] = _name2; \
  _tmp[3] = _name3; _tmp[4] = _name4; \
  this->Set##name (_tmp,5); \
} \
void Set##name##4d(type _name0,type _name1,type _name2, type _name3) \
{ \
  type _tmp[4]; \
  _tmp[0] = _name0; _tmp[1] = _name1; \
  _tmp[2] = _name2; _tmp[3] = _name3; \
  this->Set##name (_tmp,4); \
} \
void Set##name##3d(type _name0,type _name1,type _name2) \
{ \
  type _tmp[3]; \
  _tmp[0] = _name0; _tmp[1] = _name1; _tmp[2] = _name2; \
  this->Set##name (_tmp,3); \
} \
void Set##name##2d(type _name0,type _name1) \
{ \
  type _tmp[2]; \
  _tmp[0] = _name0; _tmp[1] = _name1; \
  this->Set##name (_tmp,2); \
} \
void Set##name##1d(type _name0) \
{ \
  type _tmp[1]; \
  _tmp[0] = _name0; \
  this->Set##name (_tmp,1); \
} 
#define vtkImageRegionGetMacro(name,type) \
type *Get##name () { return this->##name ;}  \
type *Get##name##5d() { return this->##name ;} \
type *Get##name##4d() { return this->##name ;} \
type *Get##name##3d() { return this->##name ;} \
type *Get##name##2d() { return this->##name ;} \
type *Get##name##1d() { return this->##name ;} \
void Get##name (type *_tmp) { this->Get##name (_tmp, 5);} \
void Get##name##5d(type *_tmp) { this->Get##name (_tmp, 5);} \
void Get##name##4d(type *_tmp) { this->Get##name (_tmp, 4);} \
void Get##name##3d(type *_tmp) { this->Get##name (_tmp, 3);} \
void Get##name##2d(type *_tmp) { this->Get##name (_tmp, 2);} \
void Get##name##1d(type *_tmp) { this->Get##name (_tmp, 1);} \
void Get##name##5d(type &_name0,type &_name1,type &_name2, \
		   type &_name3,type &_name4) \
{ \
  type _tmp[5]; \
  this->Get##name (_tmp,5); \
  _name0 = _tmp[0]; _name1 = _tmp[1]; _name2 = _tmp[2]; \
  _name3 = _tmp[3]; _name4 = _tmp[4]; \
} \
void Get##name##4d(type &_name0,type &_name1,type &_name2,type &_name3) \
{ \
  type _tmp[4]; \
  this->Get##name (_tmp,4); \
  _name0 = _tmp[0]; _name1 = _tmp[1]; _name2 = _tmp[2]; _name3 = _tmp[3]; \
} \
void Get##name##3d(type &_name0,type &_name1,type &_name2) \
{ \
  type _tmp[3]; \
  this->Get##name (_tmp,3); \
  _name0 = _tmp[0]; _name1 = _tmp[1]; _name2 = _tmp[2]; \
} \
void Get##name##2d(type &_name0,type &_name1) \
{ \
  type _tmp[2]; \
  this->Get##name (_tmp,2); \
  _name0 = _tmp[0]; _name1 = _tmp[1]; \
} \
void Get##name##1d(type &_name0) \
{ \
  type _tmp[1]; \
  this->Get##name (_tmp,1); \
  _name0 = _tmp[0]; \
} 
#define vtkImageRegionSetBoundsMacro(name) \
void Set##name (int *_tmp) { this->Set##name (_tmp, 5);} \
void Set##name##5d(int *_tmp) { this->Set##name (_tmp, 5);} \
void Set##name##4d(int *_tmp) { this->Set##name (_tmp, 4);} \
void Set##name##3d(int *_tmp) { this->Set##name (_tmp, 3);} \
void Set##name##2d(int *_tmp) { this->Set##name (_tmp, 2);} \
void Set##name##1d(int *_tmp) { this->Set##name (_tmp, 1);} \
void Set##name##5d(int _min0,int _max0,int _min1,int _max1, \
 		   int _min2,int _max2,int _min3,int _max3, \
		   int _min4,int _max4) \
{ \
  int _tmp[10]; \
  _tmp[0] = _min0; _tmp[1] = _max0; \
  _tmp[2] = _min1; _tmp[3] = _max1; \
  _tmp[4] = _min2; _tmp[5] = _max2; \
  _tmp[6] = _min3; _tmp[7] = _max3; \
  _tmp[8] = _min4; _tmp[9] = _max4; \
  this->Set##name (_tmp,5); \
} \
void Set##name##4d(int _min0,int _max0,int _min1,int _max1, \
		   int _min2,int _max2,int _min3,int _max3) \
{ \
  int _tmp[8]; \
  _tmp[0] = _min0; _tmp[1] = _max0; \
  _tmp[2] = _min1; _tmp[3] = _max1; \
  _tmp[4] = _min2; _tmp[5] = _max2; \
  _tmp[6] = _min3; _tmp[7] = _max3; \
  this->Set##name (_tmp,4); \
} \
void Set##name##3d(int _min0,int _max0,int _min1,int _max1, \
		   int _min2,int _max2) \
{ \
  int _tmp[6]; \
  _tmp[0] = _min0; _tmp[1] = _max0; \
  _tmp[2] = _min1; _tmp[3] = _max1; \
  _tmp[4] = _min2; _tmp[5] = _max2; \
  this->Set##name (_tmp,3); \
} \
void Set##name##2d(int _min0,int _max0,int _min1,int _max1) \
{ \
  int _tmp[4]; \
  _tmp[0] = _min0; _tmp[1] = _max0; \
  _tmp[2] = _min1; _tmp[3] = _max1; \
  this->Set##name (_tmp,2); \
} \
void Set##name##1d(int _min0,int _max0) \
{ \
  int _tmp[2]; \
  _tmp[0] = _min0; _tmp[1] = _max0; \
  this->Set##name (_tmp,1); \
} 
#define vtkImageRegionGetBoundsMacro(name) \
int *Get##name () { return this->##name ;}  \
int *Get##name##5d() { return this->##name ;} \
int *Get##name##4d() { return this->##name ;} \
int *Get##name##3d() { return this->##name ;} \
int *Get##name##2d() { return this->##name ;} \
int *Get##name##1d() { return this->##name ;} \
void Get##name (int *_tmp) { this->Get##name (_tmp, 5);} \
void Get##name##5d(int *_tmp) { this->Get##name (_tmp, 5);} \
void Get##name##4d(int *_tmp) { this->Get##name (_tmp, 4);} \
void Get##name##3d(int *_tmp) { this->Get##name (_tmp, 3);} \
void Get##name##2d(int *_tmp) { this->Get##name (_tmp, 2);} \
void Get##name##1d(int *_tmp) { this->Get##name (_tmp, 1);} \
void Get##name##5d(int &_min0,int &_max0,int &_min1,int &_max1, \
		   int &_min2,int &_max2,int &_min3,int &_max3, \
		   int &_min4,int &_max4) \
{ \
  int _tmp[10]; \
  this->Get##name (_tmp,5); \
  _min0 = _tmp[0]; _max0 = _tmp[1]; \
  _min1 = _tmp[2]; _max1 = _tmp[3]; \
  _min2 = _tmp[4]; _max2 = _tmp[5]; \
  _min3 = _tmp[6]; _max3 = _tmp[7]; \
  _min4 = _tmp[8]; _max4 = _tmp[9]; \
} \
void Get##name##4d(int &_min0,int &_max0,int &_min1,int &_max1, \
		   int &_min2,int &_max2,int &_min3,int &_max3) \
{ \
  int _tmp[8]; \
  this->Get##name##(_tmp,4); \
  _min0 = _tmp[0]; _max0 = _tmp[1]; \
  _min1 = _tmp[2]; _max1 = _tmp[3]; \
  _min2 = _tmp[4]; _max2 = _tmp[5]; \
  _min3 = _tmp[6]; _max3 = _tmp[7]; \
} \
void Get##name##3d(int &_min0,int &_max0,int &_min1,int &_max1, \
		   int &_min2,int &_max2) \
{ \
  int _tmp[6]; \
  this->Get##name (_tmp,3); \
  _min0 = _tmp[0]; _max0 = _tmp[1]; \
  _min1 = _tmp[2]; _max1 = _tmp[3]; \
  _min2 = _tmp[4]; _max2 = _tmp[5]; \
} \
void Get##name##2d(int &_min0,int &_max0,int &_min1,int &_max1) \
{ \
  int _tmp[4]; \
  this->Get##name (_tmp,2); \
  _min0 = _tmp[0]; _max0 = _tmp[1]; \
  _min1 = _tmp[2]; _max1 = _tmp[3]; \
} \
void Get##name##1d(int &_min0,int &_max0) \
{ \
  int _tmp[2]; \
  this->Get##name (_tmp,1); \
  _min0 = _tmp[0]; _max0 = _tmp[1]; \
} 


class vtkImageRegion : public vtkImageSource 
{
public:
  vtkImageRegion();
  ~vtkImageRegion();
  char *GetClassName() {return "vtkImageRegion";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void CopyRegionData(vtkImageRegion *region);

  // Stuff to use region as a source.
  void UpdateRegion(vtkImageRegion *region); 
  void UpdateImageInformation(vtkImageRegion *region);
  unsigned long GetPipelineMTime();

  void SetData(vtkImageData *data);
  // Description:
  // You can get the data object to share with another vtkImageRegion.
  vtkGetObjectMacro(Data,vtkImageData);
  // Description:
  // Get the data type of this region.
  vtkSetMacro(DataType,int);
  vtkGetMacro(DataType,int);
  
  // Description:
  // Returns pointer increments that can be used to step around the data.
  // Increments do not include size of data type, so should be used after
  // pointers have been converted to their actual type.
  void GetIncrements(int *increments, int dim);
  vtkImageRegionGetMacro(Increments, int);
  
  // Description:
  // Returns a pointer relative to the current volume, image or line.
  void *GetVoidPointer(int coords[5]){return this->GetVoidPointer5d(coords);};
  void *GetVoidPointer5d(int coordinates[5]);
  void *GetVoidPointer4d(int coordinates[4]);
  void *GetVoidPointer3d(int coordinates[3]);
  void *GetVoidPointer2d(int coordinates[2]);
  void *GetVoidPointer1d(int coordinates[1]);
  // Description:
  // Returns a pointer relative to the current volume, image or line.
  void *GetVoidPointer5d(int c0, int c1, int c2, int c3, int c4);
  void *GetVoidPointer4d(int c0, int c1, int c2, int c3);
  void *GetVoidPointer3d(int c0, int c1, int c2);
  void *GetVoidPointer2d(int c0, int c1);
  void *GetVoidPointer1d(int c0);
  // Description:
  // Returns pointer at origin of current volume, image or line.
  void *GetVoidPointer();
  void *GetVoidPointer5d(){return this->GetVoidPointer();};
  void *GetVoidPointer4d(){return this->GetVoidPointer();};
  void *GetVoidPointer3d(){return this->GetVoidPointer();};
  void *GetVoidPointer2d(){return this->GetVoidPointer();};
  void *GetVoidPointer1d(){return this->GetVoidPointer();};
  
  // Description:
  // Different methods for setting the bounds.
  // The 2d and 1d functions do not modify bounds of the higher dimensions.
  void SetBounds(int *bounds, int dim);
  vtkImageRegionSetBoundsMacro(Bounds);

  // Description:
  // Different methods for getting the bounds.
  void GetBounds(int *bounds, int dim);
  vtkImageRegionGetBoundsMacro(Bounds);
  
  // Description:
  // Different methods for setting the ImageBounds.
  // The 2d and 1d functions do not modify ImageBounds of the higher
  // dimensions.
  void SetImageBounds(int *bounds, int dim);
  vtkImageRegionSetBoundsMacro(ImageBounds);
  // Description:
  // Different methods for getting the ImageBounds.
  void GetImageBounds(int *bounds, int dim);
  vtkImageRegionGetBoundsMacro(ImageBounds);

  
  // Description:
  // Different methods for setting the AspectRatio.
  // The 2d and 1d functions do not modify aspect ratio of the higher
  // dimensions.
  void SetAspectRatio(float *ratio, int dim);
  vtkImageRegionSetMacro(AspectRatio, float);
  // Description:
  // Different methods for getting the Aspect Ratio.
  void GetAspectRatio(float *ratio, int dim);
  vtkImageRegionGetMacro(AspectRatio, float);
  

  // Description:
  // Different methods for setting the Origin.
  void SetOrigin(float *origin, int dim);
  vtkImageRegionSetMacro(Origin, float);
  // Description:
  // Different methods for getting the Origin.
  void GetOrigin(float *origin, int dim);
  vtkImageRegionGetMacro(Origin, float);
  

  // Description:
  // Different methods for setting the axes.
  void SetAxes(int *axes, int dim);
  vtkImageRegionSetMacro(Axes, int);

  // Description:
  // Different methods for getting the axes.
  void GetAxes(int *axes, int dim);  
  vtkImageRegionGetMacro(Axes, int);

  // Description:
  // This method returns the number of pixels enclosed in this bounding box.
  int GetVolume(){return ((Bounds[1]-Bounds[0]+1) 
			  * (Bounds[3]-Bounds[2]+1)
			  * (Bounds[5]-Bounds[4]+1)
			  * (Bounds[7]-Bounds[6]+1)
			  * (Bounds[9]-Bounds[8]+1));};
  
    // Description:
  // This method returns 1 if this bounding box has zero volume.
  int IsEmpty() {return (Bounds[1] < Bounds[0] 
			 || Bounds[3] < Bounds[2] 
			 || Bounds[5] < Bounds[4] 
			 || Bounds[7] < Bounds[6]
			 || Bounds[9] < Bounds[8]);};

  // Description:
  // This method returns 1 if the region has associated data.
  int IsAllocated(){return this->Data && this->Data->IsAllocated();};

  // Description:
  // If the image pipeline will with another package with a different
  // data structure, these functions will act as the glue.
  // Import will take a chunk of memory with its type and dimensions,
  // so you can use it as a region.  Export will give you a pointer to 
  // the data to manipulate.  Warning! when this region is deleted,
  // the data memory may be deleted also.
  void ImportMemory(void *ptr);
  void *ExportMemory();

  int GetMemorySize();
  
  void Allocate();
  void ReleaseData();
  
  void MakeWritable();
  

  //------------------------------------------------------------------
  // This should really be handled by one of the macros, 
  // but Set is not in the name.
  // Description:
  // These functions will change the "origin" of the region.
  // The bounds change, but the data does not.
  void Translate(int *vector, int dim);
  void Translate (int *vector) { this->Translate(vector, 5);};
  void Translate5d(int *vector) { this->Translate(vector, 5);};
  void Translate4d(int *vector) { this->Translate(vector, 4);};
  void Translate3d(int *vector) { this->Translate(vector, 3);};
  void Translate2d(int *vector) { this->Translate(vector, 2);};
  void Translate1d(int *vector) { this->Translate(vector, 1);};
  void Translate5d(int v0,int v1,int v2, int v3,int v4) 
  {int v[5];  v[0]=v0;v[1]=v1;v[2]=v2;v[3]=v3;v[4]=v4; this->Translate(v,5);};
  void Translate4d(int v0,int v1,int v2, int v3) 
  {int v[4];  v[0]=v0;v[1]=v1;v[2]=v2;v[3]=v3; this->Translate(v,4);};
  void Translate3d(int v0,int v1,int v2) 
  {int v[3];  v[0]=v0;v[1]=v1;v[2]=v2; this->Translate(v,3);};
  void Translate2d(int v0,int v1)
  {int v[2];  v[0]=v0;v[1]=v1; this->Translate(v,2);}; 
  void Translate1d(int v0) 
  {int v[1];  v[0]=v0; this->Translate(v,1);};
  
  
protected:
  vtkImageData *Data;   // Data is stored in this object.
  int DataType;         // Remember the pixel type of this region.

  // Defines the relative coordinate system
  int Axes[VTK_IMAGE_DIMENSIONS]; // Coordinate system of this region.

  // Bounds reordered to match Axes (relative coordinate system).
  int Bounds[VTK_IMAGE_BOUNDS_DIMENSIONS];         // Min/Max for each axis.
  // Increments in relative coordinate system
  int Increments[VTK_IMAGE_DIMENSIONS];

  // Possibly make a new object to hold global information like ImageBounds.
  int ImageBounds[VTK_IMAGE_BOUNDS_DIMENSIONS];
  float AspectRatio[VTK_IMAGE_DIMENSIONS];
  float Origin[VTK_IMAGE_DIMENSIONS];

  // Helper methods.
  void ChangeBoundsCoordinateSystem(int *boundsIn, int *axesIn,
				    int *boundsOut, int *axesOut);
};



#endif


