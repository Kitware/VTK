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

class vtkImageData : public vtkRefCount
{
public:
  vtkImageData();
  ~vtkImageData();
  char *GetClassName() {return "vtkImageData";};

  // Description:
  // Set/Get the offset and size of the data array.
  // The size should be set before the Allocate method is called.
  vtkSetVector3Macro(Size,int);
  vtkGetVector3Macro(Size,int);
  vtkSetVector3Macro(Offset,int);
  vtkGetVector3Macro(Offset,int);

  // Description:
  // Gets the increments between columns, rows, and images.  These
  // Values are computed from the size of the data array, and allow the
  // user to step through memory using pointer arithmatic.
  vtkGetVector3Macro(Inc,int);

  int Allocated();
  int Allocate();
  float *GetPointer(int coordinates[3]);

protected:
  float *Data;       // Data is stored in this array.
  int Length;        // The number of floats in the data memory.
  int Size[3];       // The actual size of the data outImageData
  int Offset[3];     // The actual offset of the data
  int Inc[3];        // Values used to move around data.
};

#endif


