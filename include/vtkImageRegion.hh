/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageRegion.hh
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


#include "vtkImageSource.hh"
#include "vtkImageData.hh"


// These definitions give semantics to the abstract implementation of axes.
#define VTK_IMAGE_X_AXIS 0
#define VTK_IMAGE_Y_AXIS 1
#define VTK_IMAGE_Z_AXIS 2
#define VTK_IMAGE_TIME_AXIS 3






class vtkImageRegion : public vtkImageSource 
{
public:
  vtkImageRegion();
  ~vtkImageRegion();
  char *GetClassName() {return "vtkImageRegion";};

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
  void GetIncrements4d(int &inc0, int &inc1, int &inc2, int &inc3);
  void GetIncrements3d(int &inc0, int &inc1, int &inc2);
  void GetIncrements2d(int &inc0, int &inc1);
  void GetIncrements1d(int &inc0);
  int *GetIncrements4d() {return this->GetIncrements();};
  int *GetIncrements3d() {return this->GetIncrements();};
  int *GetIncrements2d() {return this->GetIncrements();};
  int *GetIncrements1d() {return this->GetIncrements();};

  // Description:
  // Set the current volume, image or line.  Used to disambiguate the
  // 3d, 2d and 1d coordinates.
  vtkSetMacro(DefaultCoordinate3,int);
  vtkGetMacro(DefaultCoordinate3,int);
  vtkSetMacro(DefaultCoordinate2,int);
  vtkGetMacro(DefaultCoordinate2,int);
  vtkSetMacro(DefaultCoordinate1,int);
  vtkGetMacro(DefaultCoordinate1,int);
  vtkSetMacro(DefaultCoordinate0,int);
  vtkGetMacro(DefaultCoordinate0,int);
  
  // Description:
  // Returns a pointer relative to the current volume, image or line.
  void *GetVoidPointer4d(int coordinates[4]);
  void *GetVoidPointer3d(int coordinates[3]);
  void *GetVoidPointer2d(int coordinates[2]);
  void *GetVoidPointer1d(int coordinates[1]);
  // Description:
  // Returns pointer at origin of current volume, image or line.
  void *GetVoidPointer4d();
  void *GetVoidPointer3d();
  void *GetVoidPointer2d();
  void *GetVoidPointer1d();
  
  // Description:
  // Different methods for setting the bounds.
  // The 2d and 1d functions do not modify bounds of the higher dimensions.
  void SetBounds4d(int min0, int max0, int min1, int max1, 
		   int min2, int max2, int min3, int max3);
  void SetBounds3d(int min0, int max0, int min1, int max1, int min2, int max2);
  void SetBounds2d(int min0, int max0, int min1, int max1);
  void SetBounds1d(int min0, int max0);

  void SetBounds4d(int *bounds) {this->SetBounds(bounds,4);};
  void SetBounds3d(int *bounds) {this->SetBounds(bounds,3);};
  void SetBounds2d(int *bounds) {this->SetBounds(bounds,2);};
  void SetBounds1d(int *bounds) {this->SetBounds(bounds,1);};

  // Description:
  // Different methods for getting the bounds.
  void GetBounds4d(int &min0, int &max0, int &min1, int &max1,
		   int &min2, int &max2, int &min3, int &max3);
  void GetBounds3d(int &min0, int &max0, int &min1, int &max1,
		   int &min2, int &max2);
  void GetBounds2d(int &min0, int &max0, int &min1, int &max1);
  void GetBounds1d(int &min0, int &max0);

  void GetBounds4d(int *bounds) {this->GetBounds(bounds, 4);};
  void GetBounds3d(int *bounds) {this->GetBounds(bounds, 3);};
  void GetBounds2d(int *bounds) {this->GetBounds(bounds, 2);};
  void GetBounds1d(int *bounds) {this->GetBounds(bounds, 1);};
  
  int *GetBounds4d() { return this->Bounds;};
  int *GetBounds3d() { return this->Bounds;};
  int *GetBounds2d() { return this->Bounds;};
  int *GetBounds1d() { return this->Bounds;};
  
  int *GetAbsoluteBounds() {return this->AbsoluteBounds;};
  
  
  // Description:
  // Different methods for setting the ImageBounds.
  // The 2d and 1d functions do not modify ImageBounds of the higher dimensions.
  void SetImageBounds4d(int min0, int max0, int min1, int max1, 
			int min2, int max2, int min3, int max3);
  void SetImageBounds3d(int min0,int max0, int min1,int max1, 
			int min2,int max2);
  void SetImageBounds2d(int min0, int max0, int min1, int max1);
  void SetImageBounds1d(int min0, int max0);

  void SetImageBounds4d(int *bounds) {this->SetImageBounds(bounds,4);};
  void SetImageBounds3d(int *bounds) {this->SetImageBounds(bounds,3);};
  void SetImageBounds2d(int *bounds) {this->SetImageBounds(bounds,2);};
  void SetImageBounds1d(int *bounds) {this->SetImageBounds(bounds,1);};

  // Description:
  // Different methods for getting the ImageBounds.
  void GetImageBounds4d(int &min0, int &max0, int &min1, int &max1,
			int &min2, int &max2, int &min3, int &max3);
  void GetImageBounds3d(int &min0, int &max0, int &min1, int &max1,
			int &min2, int &max2);
  void GetImageBounds2d(int &min0, int &max0, int &min1, int &max1);
  void GetImageBounds1d(int &min0, int &max0);

  void GetImageBounds4d(int *bounds) {this->GetImageBounds(bounds, 4);};
  void GetImageBounds3d(int *bounds) {this->GetImageBounds(bounds, 3);};
  void GetImageBounds2d(int *bounds) {this->GetImageBounds(bounds, 2);};
  void GetImageBounds1d(int *bounds) {this->GetImageBounds(bounds, 1);};
  
  int *GetImageBounds4d() { return this->ImageBounds;};
  int *GetImageBounds3d() { return this->ImageBounds;};
  int *GetImageBounds2d() { return this->ImageBounds;};
  int *GetImageBounds1d() { return this->ImageBounds;};
  
  

  
  // Description:
  // Different methods for setting the axes.
  void SetAxes4d(int *axes);
  void SetAxes4d(int axis0, int axis1, int axis2, int axis3);

  // Description:
  // Different methods for getting the axes.
  void GetAxes4d(int &axis0, int &axis1, int &axis2, int &axis3);
  void GetAxes3d(int &axis0, int &axis1, int &axis2);
  void GetAxes2d(int &axis0, int &axis1);
  void GetAxes1d(int &axis0);

  void GetAxes4d(int *axes) {this->GetAxes(axes, 4);};
  void GetAxes3d(int *axes) {this->GetAxes(axes, 3);};
  void GetAxes2d(int *axes) {this->GetAxes(axes, 2);};
  void GetAxes1d(int *axes) {this->GetAxes(axes, 1);};
  
  int *GetAxes4d() { return this->Axes;};
  int *GetAxes3d() { return this->Axes;};
  int *GetAxes2d() { return this->Axes;};
  int *GetAxes1d() { return this->Axes;};

  // Description:
  // This method returns the number of pixels enclosed in this bounding box.
  int GetVolume(){return ((Bounds[1]-Bounds[0]+1) 
			  * (Bounds[3]-Bounds[2]+1)
			  * (Bounds[5]-Bounds[4]+1)
			  * (Bounds[7]-Bounds[6]+1));};
  
  // Description:
  // This method returns 1 if this bounding box has zero volume.
  int IsEmpty() {return (Bounds[1] < Bounds[0] 
			 || Bounds[3] < Bounds[2] 
			 || Bounds[5] < Bounds[4] 
			 || Bounds[7] < Bounds[6]);};

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
  
  void Allocate();
  
protected:
  vtkImageData *Data;   // Data is stored in this object.
  int DataType;         // Remember the pixel type of this region.
  int DefaultCoordinate3;          // The current volume
  int DefaultCoordinate2;          // The current image
  int DefaultCoordinate1;          // The current line
  int DefaultCoordinate0;          // The current pixel

  // Defines the relative coordinate system
  int Axes[VTK_IMAGE_DIMENSIONS]; // Reorder the axis of the Data.

  // Absolute Coordinates: Is not affected by Axes instance variable.
  int AbsoluteBounds[VTK_IMAGE_BOUNDS_DIMENSIONS]; // Min/Max for each axis.
  
  // Bounds reordered to match Axes (relative coordinate system).
  int Bounds[VTK_IMAGE_BOUNDS_DIMENSIONS];         // Min/Max for each axis.
  // Increments in relative coordinate system
  int Increments[VTK_IMAGE_DIMENSIONS];

  // Possibly make a new object to hold global information like ImageBounds.
  int ImageBounds[VTK_IMAGE_BOUNDS_DIMENSIONS];
  int AbsoluteImageBounds[VTK_IMAGE_BOUNDS_DIMENSIONS];
  

  // Helper methods.
  void ResetDefaultCoordinates(int dim);
  void SetBounds(int *bounds, int dim);
  void GetBounds(int *bounds, int dim);
  void SetImageBounds(int *bounds, int dim);
  void GetImageBounds(int *bounds, int dim);
  void GetAxes(int *axes, int dim);  
  int *GetIncrements();
  void UpdateAbsoluteBounds();
  void UpdateAbsoluteImageBounds();
  void ShuffleRelativeToAbsolute4d(int *relative, int *absolute);
  void ShuffleAbsoluteToRelative4d(int *absolute, int *relative);
};






















#endif


