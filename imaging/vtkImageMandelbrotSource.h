/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMandelbrotSource.h
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
// .NAME vtkImageMandelbrotSource - Mandelbrot image.
// .SECTION Description
// vtkImageMandelbrotSource creates an unsigned char image of the Mandelbrot
// set.  The values in the image are the number of iterations it takes for
// the magnitude of the value to get over 2.  The equation repeated is
// z = z^2 + C (z and C are complex).  Initial value of z is zero, and the 
// real value of C is mapped onto the x axis, and the imaginary value of C
// is mapped onto the Y Axis.  I was thinking of extending this source
// to generate Julia Sets (initial value of Z varies).  This would be 4
// possible parameters to vary, but there are no more 4d images :(
// The third dimension (z axis) is the imaginary value of the initial value.

#ifndef __vtkImageMandelbrotSource_h
#define __vtkImageMandelbrotSource_h

#include "vtkImageSource.h"

class VTK_EXPORT vtkImageMandelbrotSource : public vtkImageSource
{
public:
  vtkImageMandelbrotSource();
  ~vtkImageMandelbrotSource();
  static vtkImageMandelbrotSource *New() {return new vtkImageMandelbrotSource;};
  const char *GetClassName() {return "vtkImageMandelbrotSource";};
  void PrintSelf(ostream& os, vtkIndent indent);   
  
  // Description:
  // Mandelbrot mode vs Julia set mode.
  vtkSetMacro(JuliaSet, int);
  vtkGetMacro(JuliaSet, int);
  vtkBooleanMacro(JuliaSet, int);

  // Description:
  // Set/Get the extent of the whole output image.
  void SetWholeExtent(int extent[6]);
  void SetWholeExtent(int minX, int maxX, int minY, int maxY, 
			    int minZ, int maxZ);
  void GetWholeExtent(int extent[6]);
  int *GetWholeExtent() {return this->WholeExtent;}
  
  // Description:
  // Position of pixel 0, 0 ,0
  vtkSetVector3Macro(Origin, double);
  vtkGetVector3Macro(Origin, double);
   
  // Description:
  // Set/Get the spacing used to specify the scale of the image.
  vtkSetMacro(Spacing, double);
  vtkGetMacro(Spacing, double);

  // Description:
  // The maximum number of cycles run to see if the value goes over 2
  vtkSetClampMacro(MaximumNumberOfIterations, unsigned short, 1, 5000);
  vtkGetMacro(MaximumNumberOfIterations, unsigned short);

  // Description:
  // Convienence for Viewer.  Pan relative to spacing.
  // Zoom constant factor.
  void Zoom(double factor);
  void Pan(double x, double y, double z);


protected:
  int WholeExtent[6];
  double Origin[3];
  double Spacing;
  unsigned short MaximumNumberOfIterations;

  // flag to change to julia set
  int JuliaSet;
  
  void Execute(vtkImageData *outData);
  void ExecuteInformation();
  unsigned short EvaluateSet(double x, double y, double z);
};


#endif


