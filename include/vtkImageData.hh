/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageData.hh
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


#include "vtkRefCount.hh"
#include "vtkScalars.hh"


// The current maximum dimensionality of "images" is 4 (volume and time).
// These definitions will help (a little) if this needs to be changed.
#define VTK_IMAGE_DIMENSIONS 5
#define VTK_IMAGE_BOUNDS_DIMENSIONS 10


// These types are returned by GetType to indicate pixel type.
#define VTK_IMAGE_VOID            0
#define VTK_IMAGE_FLOAT           1
#define VTK_IMAGE_INT             2
#define VTK_IMAGE_SHORT           3
#define VTK_IMAGE_UNSIGNED_SHORT  4
#define VTK_IMAGE_UNSIGNED_CHAR   5

// A macro to get the name of a type
#define vtkImageDataTypeNameMacro(type) \
(((type) == VTK_IMAGE_VOID) ? "void" : \
(((type) == VTK_IMAGE_FLOAT) ? "float" : \
(((type) == VTK_IMAGE_INT) ? "int" : \
(((type) == VTK_IMAGE_SHORT) ? "short" : \
(((type) == VTK_IMAGE_UNSIGNED_SHORT) ? "unsigned short" : \
(((type) == VTK_IMAGE_UNSIGNED_CHAR) ? "unsigned char" : \
"Undefined"))))))



class vtkImageData : public vtkRefCount
{
public:
  vtkImageData();
  ~vtkImageData();
  char *GetClassName() {return "vtkImageData";};
  void PrintSelf(ostream& os, vtkIndent indent);

  int GetReferenceCount();

  // Description:
  // Get the vtkScalars that contain the actual data.
  vtkGetObjectMacro(Scalars,vtkScalars);
  
  // Description:
  // Get the bounds of the data array.
  vtkSetVectorMacro(Bounds,int,10);
  vtkGetVectorMacro(Bounds,int,10);

  void SetBounds(int min0, int max0, int min1, int max1, 
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
  void *GetVoidPointer(int coordinates[VTK_IMAGE_DIMENSIONS]);
  void *GetVoidPointer();
  
protected:
  vtkScalars *Scalars;  // Store the data in native VTK format.
  int Type;             // What type of data is in this object.
  int Bounds[VTK_IMAGE_BOUNDS_DIMENSIONS]; // bounds of data.
  int Increments[VTK_IMAGE_DIMENSIONS];    // Values used to move around data.
  int Allocated;
};



// Avoid including these in vtkImageData.cc .
#include "vtkFloatScalars.hh"
#include "vtkIntScalars.hh"
#include "vtkShortScalars.hh"
#include "vtkUnsignedShortScalars.hh"
#include "vtkUnsignedCharScalars.hh"


#endif


