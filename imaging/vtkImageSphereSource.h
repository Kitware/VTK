/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSphereSource.h
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
// .NAME vtkImageSphereSource - Create a binary image of a sphere.
// SphereSource creates a binary image of a sphere.  It was created
// as an example of a simple source, and to test the mask filter.



#ifndef __vtkImageSphereSource_h
#define __vtkImageSphereSource_h

#include "vtkImageSource.h"

class VTK_EXPORT vtkImageSphereSource : public vtkImageSource
{
public:
  vtkImageSphereSource();
  ~vtkImageSphereSource();
  static vtkImageSphereSource *New() {return new vtkImageSphereSource;};
  const char *GetClassName() {return "vtkImageSphereSource";};
  void PrintSelf(ostream& os, vtkIndent indent);   
  
  // Description:
  // Set/Get the extent of the whole output image.
  void SetWholeExtent(int dim, int *extent);
  vtkImageSetExtentMacro(WholeExtent);
  void GetWholeExtent(int dim, int *extent);
  vtkImageGetExtentMacro(WholeExtent);
  
  // Description:
  // Set/Get the center of the sphere.
  void SetCenter(int dim, float *center);
  vtkImageSetMacro(Center, float);
  void GetCenter(int dim, float *canter);
  vtkImageGetMacro(Center, float);
  
  // Description:
  // Set/Get the radius of the sphere.
  vtkSetMacro(Radius,float);
  vtkGetMacro(Radius,float);

  // Description:
  // Set/Get the inside pixel values.
  vtkSetMacro(InValue,float);
  vtkGetMacro(InValue,float);

  // Description:
  // Set/Get the outside pixel values.
  vtkSetMacro(OutValue,float);
  vtkGetMacro(OutValue,float);
  
  void UpdateImageInformation();

protected:
  int WholeExtent[8];
  float Center[4];
  float Radius;
  float InValue;
  float OutValue;

  void Execute(vtkImageRegion *outRegion);
};


#endif


