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


#include "vtkImageSetGet.h"
#include "vtkImageSource.h"
#include "vtkImageData.h"


class VTK_EXPORT vtkImageRegion : public vtkImageSource 
{
public:
  vtkImageRegion();
  ~vtkImageRegion();
  char *GetClassName() {return "vtkImageRegion";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void CopyRegionData(vtkImageRegion *region);

  // Description:
  // This function sets all the pixels in a region to the specified value.
  void Fill(float value);
  
  // Stuff to use region as a source.
  void UpdateRegion(vtkImageRegion *region); 
  void UpdateImageInformation(vtkImageRegion *region);
  unsigned long GetPipelineMTime();

  // Description:
  // Set/Get the data object to share with other vtkImageRegions.
  void SetData(vtkImageData *data);
  vtkImageData *GetData();
  // Description:
  // Get the data type of this region.
  vtkSetMacro(ScalarType,int);
  vtkGetMacro(ScalarType,int);
  
  // Description:
  // Returns pointer increments that can be used to step around the data.
  // Increments do not include size of data type, so should be used after
  // pointers have been converted to their actual type.
  void GetIncrements(int dim, int *increments);
  vtkImageGetMacro(Increments, int);
  int *GetIncrements() {return this->Increments;};

  // Description:
  // Different methods for setting the extent.
  // The 2d and 1d functions do not modify extent of the higher dimensions.
  void SetExtent(int dim, int *extent);
  vtkImageSetExtentMacro(Extent);

  // Description:
  // Different methods for getting the extent.
  void GetExtent(int dim, int *extent);
  vtkImageGetExtentMacro(Extent);
  
  // Description:
  // Different methods for setting the ImageExtent.
  // The 2d and 1d functions do not modify ImageExtent of the higher
  // dimensions.
  void SetImageExtent(int dim, int *extent);
  vtkImageSetExtentMacro(ImageExtent);
  // Description:
  // Different methods for getting the ImageExtent.
  void GetImageExtent(int dim, int *extent);
  vtkImageGetExtentMacro(ImageExtent);

  
  // Description:
  // Different methods for setting the AspectRatio.
  // The 2d and 1d functions do not modify aspect ratio of the higher
  // dimensions.
  void SetAspectRatio(int dim, float *ratio);
  vtkImageSetMacro(AspectRatio, float);
  // Description:
  // Different methods for getting the Aspect Ratio.
  void GetAspectRatio(int dim, float *ratio);
  vtkImageGetMacro(AspectRatio, float);
  float *GetAspectRatio() {return this->AspectRatio;};
  

  // Description:
  // Different methods for setting the Origin.
  void SetOrigin(int dim, float *origin);
  vtkImageSetMacro(Origin, float);
  // Description:
  // Different methods for getting the Origin.
  void GetOrigin(int dim, float *origin);
  vtkImageGetMacro(Origin, float);
  float *GetOrigin() {return this->Origin;};
  

  // Description:
  // Different methods for setting the axes.
  void SetAxes(int dim, int *axes);
  vtkImageSetMacro(Axes, int);

  // Description:
  // Different methods for getting the axes.
  void GetAxes(int dim, int *axes);  
  vtkImageGetMacro(Axes, int);
  int *GetAxes() {return this->Axes;};

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

  // Description:
  // Returns a pointer for reading the vector point data.
  // Coordinates are relative to image origin.
  // Extent mins are used for unspecified coordinates.
  float *GetVectorPointer(int dim, int *coordinates);
  vtkImageGetPointerMacro(Vector, float);
  
  // Description:
  // Returns a pointer for writing into the vector point data.
  // Coordinates are relative to image origin.
  // Extent mins are used for unspecified coordinates.
  float *GetVectorWritePointer(int dim, int *coordinates)
  {this->MakeVectorsWritable();return this->GetVectorPointer(dim,coordinates);}
  vtkImageGetPointerMacro(VectorWrite, float);
  
  // Description:
  // This method returns the number of pixels enclosed in this bounding box.
  int GetVolume(){return ((Extent[1]-Extent[0]+1) 
			  * (Extent[3]-Extent[2]+1)
			  * (Extent[5]-Extent[4]+1)
			  * (Extent[7]-Extent[6]+1)
			  * (Extent[9]-Extent[8]+1));};
  
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

  // Description:
  // If the image pipeline will be used with another image
  // data structure, these functions will act as the glue.
  // Import will take a chunk of memory with its type and dimensions,
  // so you can use it as a region.  Export will give you a pointer to 
  // the data to manipulate.  Warning! when this region is deleted,
  // the data memory may be deleted also.
  void ImportMemory(void *ptr);
  void *ExportMemory();
  
  void AllocateScalars();
  void AllocateVectors();
  void ReleaseData();
  
  //------------------------------------------------------------------
  // This should really be handled by one of the macros, 
  // but Set is not in the name.
  // Description:
  // These functions will change the "origin" of the region.
  // The extent change, but the data does not.
  void Translate(int dim, int *vector);
  void Translate (int *vector) { this->Translate(5,vector);};
  void Translate(int v0,int v1,int v2, int v3,int v4) 
  {int v[5];  v[0]=v0;v[1]=v1;v[2]=v2;v[3]=v3;v[4]=v4; this->Translate(5,v);};
  void Translate(int v0,int v1,int v2, int v3) 
  {int v[4];  v[0]=v0;v[1]=v1;v[2]=v2;v[3]=v3; this->Translate(4,v);};
  void Translate(int v0,int v1,int v2) 
  {int v[3];  v[0]=v0;v[1]=v1;v[2]=v2; this->Translate(3,v);};
  void Translate(int v0,int v1)
  {int v[2];  v[0]=v0;v[1]=v1; this->Translate(2,v);}; 
  void Translate(int v0) 
  {int v[1];  v[0]=v0; this->Translate(1,v);};
  
  
protected:
  vtkImageData *Data;   // Data is stored in this object.
  int ScalarType;         // Remember the pixel type of this region.

  // Defines the relative coordinate system
  int Axes[VTK_IMAGE_DIMENSIONS]; // Coordinate system of this region.

  // Extent reordered to match Axes (relative coordinate system).
  int Extent[VTK_IMAGE_EXTENT_DIMENSIONS];         // Min/Max for each axis.
  // Increments in relative coordinate system
  int Increments[VTK_IMAGE_DIMENSIONS];

  // Possibly make a new object to hold global information like ImageExtent.
  int ImageExtent[VTK_IMAGE_EXTENT_DIMENSIONS];
  float AspectRatio[VTK_IMAGE_DIMENSIONS];
  float Origin[VTK_IMAGE_DIMENSIONS];

  // Helper methods.
  void MakeDataWritable();
  void MakeScalarsWritable()
  {this->MakeDataWritable(); this->Data->MakeScalarsWritable();};
  void MakeVectorsWritable()
  {this->MakeDataWritable(); this->Data->MakeVectorsWritable();};
  void ChangeExtentCoordinateSystem(int *extentIn, int *axesIn,
				    int *extentOut, int *axesOut);
};



#endif


