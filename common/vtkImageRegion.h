/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageRegion.h
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
// .NAME vtkImageRegion - Piece of image.
// .SECTION Description
// vtkImageRegion holds a piece of an image. 
// The actual data for the image is stored in the
// vtkImageData object.  The vtkImageRegion can represent a portion
// of its vtkImageData, hiding the actual extent of the vtkImageData.
// It can also transparently reorder the axes of the data with out
// coping the data.  The Increments are used to march around the data using
// pointer arithmetic, without knowing the actual dimensions of the
// underlying data.  Both the increments and extent of the region
// are relative to the regions axes.  They get reshuffled when the 
// axes are changed.
//   A region is usually manipulated by the image pipeline behind
// the scenes, but it can also be used as an input to a filter for more
// explicit operaation.


#ifndef __vtkImageRegion_h
#define __vtkImageRegion_h


#include "vtkImageSetGet.h"
#include "vtkImageData.h"
#include "vtkImageSource.h"


class VTK_EXPORT vtkImageRegion : public vtkImageSource
{
public:
  vtkImageRegion();
  ~vtkImageRegion();
  static vtkImageRegion *New() {return new vtkImageRegion;};
  const char *GetClassName() {return "vtkImageRegion";};
  void PrintSelf(ostream& os, vtkIndent indent);


  // Description:
  // Different methods for setting and getting the axes.  
  // The only function of the Axes instance variable is to specify 
  // a context for the other filter instance variables.  
  // The affect of this variable is local to this region, and does not
  // change the behavior of any filter that operates on this region.
  void SetAxes(int dim, int *axes);
  vtkImageSetMacro(Axes, int);
  void GetAxes(int dim, int *axes);  
  vtkImageGetMacro(Axes, int);
  int *GetAxes() {return this->Axes;};

  // Description:
  // Different methods for setting the extent.
  // The 2d and 1d functions do not modify extent of the higher dimensions.
  void SetExtent(int dim, int *extent);
  vtkImageSetExtentMacro(Extent);
  void SetAxisExtent(int axis, int min, int max);
  void GetAxisExtent(int axis, int &min, int &max);

  // Description:
  // Different methods for getting the extent.
  void GetExtent(int dim, int *extent);
  vtkImageGetExtentMacro(Extent);
  
  // Description:
  // Set/Get the data type of this region.
  void SetScalarTypeToFloat(){this->SetScalarType(VTK_FLOAT);}
  void SetScalarTypeToInt(){this->SetScalarType(VTK_INT);}
  void SetScalarTypeToShort(){this->SetScalarType(VTK_SHORT);}
  void SetScalarTypeToUnsignedShort(){this->SetScalarType(VTK_UNSIGNED_SHORT);}
  void SetScalarTypeToUnsignedChar(){this->SetScalarType(VTK_UNSIGNED_CHAR);}
  void SetScalarType(int type);
  vtkGetMacro(ScalarType,int);
  
  // Stuff to use region as an vtkImageSource.
  void Update(); 
  void UpdateImageInformation();
  unsigned long GetPipelineMTime();

  
  //--------------------------------------------------------------------------
  // Used by caches
  void SetData(vtkImageData *data);
  vtkImageData *GetData();
  void ReleaseData();
  void MakeData();

  //--------------------------------------------------------------------------
  // Used by filters.
  
  // Description:
  // "Increments" allows the user to efficiently 
  // march through the memory using pointer arithmetic, while keeping the
  // actual dimensions of the memory array hidden.  
  // Increments do not include size of data type, so should be used after
  // pointers have been casted to their actual type.
  void GetIncrements(int dim, int *increments);
  vtkImageGetMacro(Increments, int);
  int *GetIncrements() {return this->Increments;};
  void GetAxesIncrements(int num, int *axes, int *incs);
  void GetAxisIncrements(int axis, int &inc);
  //vtkImageGetAxesMacro(Increments, int);

  // Description:
  // Returns a pointer for reading the scalar point data.
  // Coordinates are relative to image origin.
  // Extent mins are used for unspecified coordinates.
  void *GetScalarPointer(int dim, int *coordinates);
  vtkImageGetPointerMacro(Scalar, void);

  // Description:
  // Returns a pointer for writing into the scalar point data.
  // Coordinates are relative to image origin.
  // Extent mins are used for unspecified coordinates.
  void *GetScalarWritePointer(int dim, int *coordinates)
  {this->MakeScalarsWritable();return this->GetScalarPointer(dim,coordinates);}
  vtkImageGetPointerMacro(ScalarWrite, void);

  
  //--------------------------------------------------------------------------
  // Image information
  
  // Description:
  // Different methods for setting the WholeExtent.
  // The 2d and 1d functions do not modify WholeExtent of the higher
  // dimensions.  The image extent is the largest region that can be 
  // requested from the pipeline.
  void SetWholeExtent(int dim, int *extent);
  vtkImageSetExtentMacro(WholeExtent);
  void SetAxisWholeExtent(int axis, int min, int max);

  // Description:
  // Different methods for getting the WholeExtent.
  void GetWholeExtent(int dim, int *extent);
  vtkImageGetExtentMacro(WholeExtent);

  // Description:
  // Different methods for setting the data spacing.
  // The 2d and 1d functions do not modify the data spacing of the higher
  // dimensions.
  void SetSpacing(int dim, float *ratio);
  vtkImageSetMacro(Spacing, float);

  // Description:
  // Different methods for getting the data spacing.
  void GetSpacing(int dim, float *ratio);
  vtkImageGetMacro(Spacing, float);
  float *GetSpacing() {return this->Spacing;};

  // Description:
  // Different methods for setting the Origin.
  void SetOrigin(int dim, float *origin);
  vtkImageSetMacro(Origin, float);

  // Description:
  // Different methods for getting the Origin.
  void GetOrigin(int dim, float *origin);
  vtkImageGetMacro(Origin, float);
  float *GetOrigin() {return this->Origin;};
  

  //--------------------------------------------------------------------------
  // Convenience methods
  void CopyRegionData(vtkImageRegion *region);
  void Fill(float value);
  
  int GetExtentMemorySize();
  
  // Description:
  // This method returns 1 if this bounding box has zero volume.
  int IsEmpty() {return (Extent[1] < Extent[0] 
			 || Extent[3] < Extent[2] 
			 || Extent[5] < Extent[4] 
			 || Extent[7] < Extent[6]
			 || Extent[9] < Extent[8]);};

  // Description:
  // This method returns 1 if the region has associated data.
  int AreScalarsAllocated()
  {return this->Data && this->Data->AreScalarsAllocated();};

  void AllocateScalars();
  
  // These methods are used by vtkImageRegions and vtkImageData to
  // reorder their axes and corresponding ivars.
  static void ChangeVectorCoordinateSystem(int *vectIn, int *axesIn,
					   int *vectOut, int *axesOut);
  static void ChangeVectorCoordinateSystem(float *vectIn, int *axesIn,
					   float *vectOut, int *axesOut);
  static void ChangeExtentCoordinateSystem(int *extentIn, int *axesIn,
					   int *extentOut, int *axesOut);
  static void CompleteUnspecifiedAxes(int num, int *axesIn, int *axesOut);
  
  // Description:
  // These methods make sure that the data has only one reference count.
  void MakeDataWritable();
  void MakeScalarsWritable()
  {this->MakeDataWritable(); this->Data->MakeScalarsWritable();};

protected:
  vtkImageData *Data;   // Data is stored in this object.
  int ScalarType;         // Remember the pixel type of this region.

  // Defines the relative coordinate system
  int Axes[VTK_IMAGE_DIMENSIONS]; // Coordinate system of this region.

  // Extent reordered to match Axes (relative coordinate system).
  int Extent[VTK_IMAGE_EXTENT_DIMENSIONS];         // Min/Max for each axis.
  // Increments in relative coordinate system
  int Increments[VTK_IMAGE_DIMENSIONS];

  // Possibly make a new object to hold global information like WholeExtent.
  int WholeExtent[VTK_IMAGE_EXTENT_DIMENSIONS];
  float Spacing[VTK_IMAGE_DIMENSIONS];
  float Origin[VTK_IMAGE_DIMENSIONS];

};



#endif


