/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageData.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
lorscThe following terms apply to all files associated with the software unless
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
// The extent represents the actual dimensions of the underlying memory.
// The actual memory is stored in objects (vtkPointData,vtkScalars,vtkArray)
// used in the visualization pipline, so transfer of data is efficient.
// Currently, the only contents of PointData used are the Scalars.
// The underlying memory can be any data type.

#ifndef __vtkImageData_h
#define __vtkImageData_h

#include "vtkStructuredPoints.h"

  
class VTK_EXPORT vtkImageData : public vtkStructuredPoints
{
public:
  vtkImageData();
  static vtkImageData *New() {return new vtkImageData;};
  const char *GetClassName() {return "vtkImageData";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Different ways to set the extent of the data array.  The extent
  // should be set before the "Scalars" are set or allocated.
  // The Extent is stored  in the order (X, Y, Z).
  void SetExtent(int *extent);
  void SetExtent(int x1, int x2, int y1, int y2, int z1, int z2);
  vtkGetVectorMacro(Extent,int,6);
  void GetExtent(int &x1, int &x2, int &y1, int &y2, int &z1, int &z2);
  
  // Description:
  // Set the data scalar type of the regions created by this cache.
  vtkSetMacro(ScalarType,int);
  vtkGetMacro(ScalarType,int);

  // Description:
  // Set the size of the scalar type in bytes.
  int GetScalarSize();

  // Description:
  // Different ways to get the increments for moving around the data.
  vtkGetVector3Macro(Increments,int);
  
  // Description:
  // Different ways to get the increments for moving around the data.
  // They are store (Component, X, Y, Z). This method returns
  // increments that are suited for continuous incrementing of the
  // pointer in a Z, Y, X, C nested for loop.
  void GetContinuousIncrements(int extent[6], int &incX, int &incY,
			       int &incZ);
  
  // Description:
  // Access the native pointer for the scalar data
  void *GetScalarPointerForExtent(int coordinates[6]);
  void *GetScalarPointer(int coordinates[3]);
  void *GetScalarPointer(int x, int y, int z);
  void *GetScalarPointer();

  // Description:
  // Allocate the vtkScalars object associated with this object.
  void AllocateScalars();
  
  // Description:
  // Set/Get the number of scalar components
  void SetNumberOfScalarComponents(int num);
  vtkGetMacro(NumberOfScalarComponents,int);
  
  void CopyAndCastFrom(vtkImageData *inData, int extent[6]);
  
protected:
  int Extent[6];
  int Increments[3];
  int ScalarType;
  int NumberOfScalarComponents;
  
  void ComputeIncrements();
};

#endif


