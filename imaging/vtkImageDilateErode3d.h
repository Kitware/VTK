/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDilateErode3d.h
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
// .NAME vtkImageDilateErode3d - Dilates one value and erodes another.
// .SECTION Description
// vtkImageDilateErode3d will dilate one value and erode another.
// It uses an box foot print, and only erodes/dilates on the
// boundary of the two values.


#ifndef __vtkImageDilateErode3d_h
#define __vtkImageDilateErode3d_h


#include "vtkImageSpatialFilter.h"

class vtkImageDilateErode3d : public vtkImageSpatialFilter
{
public:
  vtkImageDilateErode3d();
  char *GetClassName() {return "vtkImageDilateErode3d";};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  void SetKernelSize(int size){this->SetKernelSize(size,size,size);};
  void SetKernelSize(int size0, int size1, int size2);
  
  // Description:
  // Set/Get the value to dilate/erode
  vtkSetMacro(DilateValue, float);
  vtkGetMacro(DilateValue, float);
  vtkSetMacro(ErodeValue, float);
  vtkGetMacro(ErodeValue, float);

  // Description:
  // Get the Mask used as a footprint.
  vtkGetObjectMacro(Mask, vtkImageRegion);
  
protected:
  float DilateValue;
  float ErodeValue;
  vtkImageRegion *Mask;
    
  void ExecuteCenter3d(vtkImageRegion *inRegion, vtkImageRegion *outRegion);
  void Execute(vtkImageRegion *inRegion, vtkImageRegion *outRegion);
};

#endif



