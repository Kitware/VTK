/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageData.h
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
// .NAME vtkImageData - Similar to structured points.
// .SECTION Description
// vtkImageData is the basic image data structure specific to the  image
// pipeline.  The axes cannot be reordered as in vtkImageRegion.  All
// the ivars are accessed as if the Axes were set to (0,1,2,3,4).
// The extent represetns the actual dimensions of the underlying memory.
//  The actual memory is stored in objects (vtkPointData,vtkScalars,vtkArray)
// used in the visualization pipline, so transfer of data is efficient.
// Cuurently, only the only contents of PointData used are the Scalars,
// and vtkColorScalars are excluded.  The underlying memory can be any data 
// type.


#ifndef __vtkImageData_h
#define __vtkImageData_h


#include "vtkReferenceCount.h"
#include "vtkImageSetGet.h"
#include "vtkPointData.h"


// It would be nice to get rid of this hard coding of dimensionality.
// Could each region/filter have its own dimensionality? (Dimensionality).
#define VTK_IMAGE_DIMENSIONS 5
#define VTK_IMAGE_EXTENT_DIMENSIONS 10


// These types are returned by GetScalarType to indicate pixel type.
#define VTK_VOID            0
#define VTK_FLOAT           1
#define VTK_INT             2
#define VTK_SHORT           3
#define VTK_UNSIGNED_SHORT  4
#define VTK_UNSIGNED_CHAR   5

// A macro to get the name of a type
#define vtkImageScalarTypeNameMacro(type) \
(((type) == VTK_VOID) ? "void" : \
(((type) == VTK_FLOAT) ? "float" : \
(((type) == VTK_INT) ? "int" : \
(((type) == VTK_SHORT) ? "short" : \
(((type) == VTK_UNSIGNED_SHORT) ? "unsigned short" : \
(((type) == VTK_UNSIGNED_CHAR) ? "unsigned char" : \
"Undefined"))))))


// These definitions give semantics to the abstract implementation of axes.
#define VTK_IMAGE_X_AXIS 0
#define VTK_IMAGE_Y_AXIS 1
#define VTK_IMAGE_Z_AXIS 2
#define VTK_IMAGE_TIME_AXIS 3
#define VTK_IMAGE_COMPONENT_AXIS 4
#define VTK_IMAGE_USER_DEFINED_AXIS 5


// A macro to get the name of an axis
#define vtkImageAxisNameMacro(axis) \
(((axis) == VTK_IMAGE_X_AXIS) ? "X" : \
(((axis) == VTK_IMAGE_Y_AXIS) ? "Y" : \
(((axis) == VTK_IMAGE_Z_AXIS) ? "Z" : \
(((axis) == VTK_IMAGE_TIME_AXIS) ? "Time" : \
(((axis) == VTK_IMAGE_COMPONENT_AXIS) ? "Component" : \
(((axis) == VTK_IMAGE_USER_DEFINED_AXIS) ? "UserDefined" : \
"Undefined"))))))



class VTK_EXPORT vtkImageData : public vtkReferenceCount
{
public:
  vtkImageData();
  static vtkImageData *New() {return new vtkImageData;};
  const char *GetClassName() {return "vtkImageData";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the Order/Axes (which axes loop first) data.  
  // Data along "axes[0]" are co-located in memory (i.e. inner most loop).
  // Axes must be set BEFORE the extent is set, and before the data has 
  // been allocated.  
  void SetAxes(int num, int *axes);
  vtkImageSetMacro(Axes, int);
  void GetAxes(int num, int *axes);
  vtkImageGetMacro(Axes, int);
  int *GetAxes(){return this->Axes;}
  
  void SetScalarType(int type);
  void SetScalarTypeToFloat(){this->SetScalarType(VTK_FLOAT);}
  void SetScalarTypeToInt(){this->SetScalarType(VTK_INT);}
  void SetScalarTypeToShort(){this->SetScalarType(VTK_SHORT);}
  void SetScalarTypeToUnsignedShort(){this->SetScalarType(VTK_UNSIGNED_SHORT);}
  void SetScalarTypeToUnsignedChar(){this->SetScalarType(VTK_UNSIGNED_CHAR);}
  // Description:
  // Get the scalar type of the underlying data.
  vtkGetMacro(ScalarType,int);

  // Description:
  // Different ways to set the extent of the data array.  The extent
  // should be set before the "Scalars" are set or allocated.
  // The Extent is stored relative to the axes, so the "Axes"
  // has to be set before the extent.
  // Extent of unspecified axes are set to 0,0.
  void SetExtent(int dim, int *extent);
  vtkImageSetExtentMacro(Extent);
  // Description:
  // Different ways to get the extent of the data array.
  void GetExtent(int dim, int *extent);
  vtkImageGetExtentMacro(Extent);

  void Translate(int vector[VTK_IMAGE_DIMENSIONS]);
  
  // Description:
  // Different ways to get the increments for moving around the data.
  void GetIncrements(int dim, int *incs);
  vtkImageGetMacro(Increments, int);
  int *GetIncrements(){return this->Increments;}

  int AreScalarsAllocated();
  void MakeScalarsWritable();
  void SetScalars(vtkScalars *scalars);
  void *GetScalarPointer(int coordinates[VTK_IMAGE_DIMENSIONS]);
  void *GetScalarPointer();
  vtkScalars *GetScalars(){return this->PointData.GetScalars();};
  
  // return pointer to this dataset's point data
  vtkPointData *GetPointData() {return &this->PointData;};

  // Description:
  // A flag that determines wheter to print the data or not.
  vtkSetMacro(PrintScalars,int);
  vtkGetMacro(PrintScalars,int);
  vtkBooleanMacro(PrintScalars,int);
  

protected:
  vtkPointData PointData;
  // The organization of memory. Axes[0] is the inner loop.
  int Axes[VTK_IMAGE_DIMENSIONS];
  // The underlying data is stored in one of: float, int ...
  int ScalarType;
  // Extent stored absolutely for each axis (as if Axes = {0, 1, 2, 3, 4}).
  int Extent[VTK_IMAGE_EXTENT_DIMENSIONS];
  // Increments  stored absolutely for each axis (as if Axes = {0, 1, 2, 3, 4})
  int Increments[VTK_IMAGE_DIMENSIONS];
  // The total number of pixels in the array.
  int Volume;

  int ScalarsAllocated;
  // For debuging
  int PrintScalars;   
  
  void ComputeIncrements();
  int AllocateScalars();
};



// Avoid including these in vtkImageData.cc .
#include "vtkScalars.h"
#include "vtkFloatScalars.h"
#include "vtkIntScalars.h"
#include "vtkShortScalars.h"
#include "vtkUnsignedShortScalars.h"
#include "vtkUnsignedCharScalars.h"


#endif


