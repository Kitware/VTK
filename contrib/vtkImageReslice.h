/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageReslice.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

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
// .NAME vtkImageReslice - Reslices a volume along the axes specified.
// .SECTION Description
// vtkImageReslice will reslice a volume along the axes specified by
// the reslicing transformation matrix.  The extent, origin, and sampling
// density of the output data can also be set.
// .SECTION Caveats
// Interpolation cannot (yet) be used on a 2D image because the
// trilinear interpolation requires a cube of eight input voxels for 
// each output voxel.
// .SECTION see also
// vtkImageFilter, vtkTransform


#ifndef __vtkImageReslice_h
#define __vtkImageReslice_h


#include "vtkImageFilter.h"
#include "vtkTransform.h"

class vtkMatrix4x4;

class VTK_EXPORT vtkImageReslice : public vtkImageFilter
{
public:
  vtkImageReslice();
  static vtkImageReslice *New() {return new vtkImageReslice;};
  const char *GetClassName() {return "vtkImageReslice";};

  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set reslicing transform (describes the set of axis along which to reslice)
  vtkSetObjectMacro(ResliceTransform,vtkTransform);
  vtkGetObjectMacro(ResliceTransform,vtkTransform);

  // Description:
  // Turn on trilinear interpolation (default: nearest neighbor)
  vtkSetMacro(Interpolate,int);
  vtkGetMacro(Interpolate,int);
  vtkBooleanMacro(Interpolate,int);

  // Description:
  // Allow user to set background 'color'
  vtkSetMacro(BackgroundLevel, double);
  vtkGetMacro(BackgroundLevel, double);

  // Description:
  // Spacing, origin, and extent of output data
  vtkSetVector3Macro(OutputSpacing, float);
  vtkGetVector3Macro(OutputSpacing, float);
  vtkSetVector3Macro(OutputOrigin, float);
  vtkGetVector3Macro(OutputOrigin, float);
  vtkSetVectorMacro(OutputExtent, int, 6);
  vtkGetVectorMacro(OutputExtent, int, 6);

  void ComputeIndexMatrix(vtkMatrix4x4 *matrix);
  int FindExtent(int& r1, int& r2, double *point, float *xAxis,
		      int *inMin, int *inMax, int interpolate);
protected:
  vtkTransform *ResliceTransform;
  int Interpolate;
  float OutputOrigin[3];
  float OutputSpacing[3];
  int OutputExtent[6];
  double BackgroundLevel;
  void ExecuteImageInformation();
  void ComputeRequiredInputUpdateExtent(int inExt[6], int outExt[6]);
  
  // Description:
  // This method is passed a input and output region, and executes the filter
  // algorithm to fill the output from the input.
  // It just executes a switch statement to call the correct function for
  // the regions data types.
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData, 
		       int ext[6], int id);
};

#endif





