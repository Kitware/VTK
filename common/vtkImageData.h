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
// .NAME vtkImageData - Holds an array of floats represtion a piece of the 
// image.
// .SECTION Description
// vtkImageData holds a reference counted 3d array of floats that is the basic
// data object of the Tile Image Pipeline.  It is not directly accessed, but
// is referenced through vtkImageRegion objects.  
// vtkImageCache objects are the only other class that uses vtkImageData
// objects directly.
// The memory of outImageData can be accessed 
// quickly through pointer arithmatic.

#ifndef __vtkImageData_h
#define __vtkImageData_h


#include "vtkRefCount.h"
#include "vtkScalars.h"


// The current maximum dimensionality of "images" is 4 (volume and time).
// These definitions will help (a little) if this needs to be changed.
#define VTK_IMAGE_DIMENSIONS 5
#define VTK_IMAGE_BOUNDS_DIMENSIONS 10


// These types are returned by GetType to indicate pixel type.
#define VTK_IMAGE_VOID            0
#define VTK_FLOAT           1
#define VTK_INT             2
#define VTK_SHORT           3
#define VTK_UNSIGNED_SHORT  4
#define VTK_UNSIGNED_CHAR   5

// A macro to get the name of a type
#define vtkImageDataTypeNameMacro(type) \
(((type) == VTK_IMAGE_VOID) ? "void" : \
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


// A macro to get the name of an axis
#define vtkImageAxisNameMacro(axis) \
(((axis) == VTK_IMAGE_X_AXIS) ? "X" : \
(((axis) == VTK_IMAGE_Y_AXIS) ? "Y" : \
(((axis) == VTK_IMAGE_Z_AXIS) ? "Z" : \
(((axis) == VTK_IMAGE_TIME_AXIS) ? "Time" : \
(((axis) == VTK_IMAGE_COMPONENT_AXIS) ? "Component" : \
"Undefined")))))




class vtkImageData : public vtkRefCount
{
public:
  vtkImageData();
  ~vtkImageData();
  char *GetClassName() {return "vtkImageData";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetScalars(vtkScalars *scalars);
  // Description:
  // Get the vtkScalars that contain the actual data.
  vtkGetObjectMacro(Scalars,vtkScalars);
  
  // Description:
  // Get the extent of the data array.
  vtkSetVectorMacro(Extent,int,10);
  vtkGetVectorMacro(Extent,int,10);

  void SetExtent(int min0, int max0, int min1, int max1, 
		 int min2, int max2, int min3, int max3,
		 int min4, int max4);

  // Description:
  // Set/Get the size in chars, of each pixel.
  // The pixel size should also be set before the Allocate method is called.
  void SetType(int type);
  vtkGetMacro(Type,int);
  
  // Description:
  // Gets the increments between columns, rows, and images.  These
  // Values are computed from the size of the data array, and allow the
  // user to step through memory using pointer arithmatic.
  vtkGetVectorMacro(Increments,int,5);

  int IsAllocated();
  int Allocate();
  void *GetScalarPointer(int coordinates[VTK_IMAGE_DIMENSIONS]);
  void *GetScalarPointer();
  void Translate(int vector[VTK_IMAGE_DIMENSIONS]);
  
  
  // Description:
  // Set/Get the coordinate system of the data (data order)
  void SetAxes(int *axes);
  vtkGetVectorMacro(Axes,int,VTK_IMAGE_DIMENSIONS);
  
  // Description:
  // A flag that determines wheter to print the data or not.
  vtkSetMacro(PrintScalars,int);
  vtkGetMacro(PrintScalars,int);
  vtkBooleanMacro(PrintScalars,int);
  
  void CopyData(vtkImageData *data);
  void CopyData(vtkImageData *data, int *extent);
  

protected:
  int PrintScalars;
  vtkScalars *Scalars;  // Store the data in native VTK format.
  int Axes[VTK_IMAGE_DIMENSIONS];   
  int Type;             // What type of data is in this object.
  int Extent[VTK_IMAGE_BOUNDS_DIMENSIONS]; // extent of data.
  int Increments[VTK_IMAGE_DIMENSIONS];    // Values used to move around data.
  int Allocated;
};



// Avoid including these in vtkImageData.cc .
#include "vtkFloatScalars.h"
#include "vtkIntScalars.h"
#include "vtkShortScalars.h"
#include "vtkUnsignedShortScalars.h"
#include "vtkUnsignedCharScalars.h"


#endif


