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
// .NAME vtkImageRegion - Generic piece of an image used in pipeline.
// .SECTION Description
// vtkImageRegion holds a 3d piece of an image and is the object filters
// deal with directly.  The actual data for the image is stored in the
// vtkImageData object.  The vtkImageRegion can represent only a portion 
// of its vtkImageData, hiding the actual dimensions of the vtkImageData.

#ifndef __vtkImageRegion_h
#define __vtkImageRegion_h


#include "vtkObject.hh"
#include "vtkImageData.hh"

class vtkImageRegion : public vtkObject 
{
public:
  vtkImageRegion();
  ~vtkImageRegion();
  char *GetClassName() {return "vtkImageRegion";};

  // Description:
  // Set/Get the portion, offset and size, of the vtkImageRegion represented
  // by this object.  The portion must be contained the vtkImageData.
  // No error checking is performed!
  vtkSetVector3Macro(Size,int);
  vtkGetVector3Macro(Size,int);
  vtkSetVector3Macro(Offset,int);
  vtkGetVector3Macro(Offset,int);

  int Allocate();
  void SetData(vtkImageData *data);
  // Description:
  // You can get the data object to share with another vtkImageRegion.
  vtkGetObjectMacro(Data,vtkImageData);

  float *GetPointer(int coordinates[3]);
  void GetInc(int &inc0, int &inc1, int &inc2);
  int *GetInc();

protected:
  vtkImageData *Data;   // Data is stored in this object.
  int Size[3];          // The size of the tile (can be smaller than data)
  int Offset[3];        // The offset relative to image origin.
};

#endif


