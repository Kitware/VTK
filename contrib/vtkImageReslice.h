/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageReslice.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen.

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
// vtkImageReslice will regrid a volume along the axes specified by
// the reslicing matrix.  The extent, origin, and sampling
// density of the output data can also be set.  This class is the
// swiss-army-knife of image geometry filters:  It can permute, flip,
// rotate, scale, resample, and pad image data in any combination. 
// It does the permute and resample operations at close to the 
// efficency of vtkImagePermute and vtkImageResample.  It can also
// extract oblique slices from image volumes, which no other VTK
// imaging filter can do.
// .SECTION Caveats
// This filter is very inefficient if the output X dimension is 1.
// .SECTION see also
// vtkImageToImageFilter, vtkTransform


#ifndef __vtkImageReslice_h
#define __vtkImageReslice_h


#include "vtkImageToImageFilter.h"
#include "vtkTransform.h"

class vtkMatrix4x4;

#define VTK_RESLICE_NEAREST 0
#define VTK_RESLICE_LINEAR 1
#define VTK_RESLICE_CUBIC 3

class VTK_EXPORT vtkImageReslice : public vtkImageToImageFilter
{
public:
  static vtkImageReslice *New();
  vtkTypeMacro(vtkImageReslice,vtkImageToImageFilter);

  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the axes of the mesh along which the volume will be resliced. 
  // The axes are extracted from the 4x4 matrix:  The x-axis is the 
  // first column, the y-axis is the second column, the z-axis is the  
  // third column, and the origin is the final column.  The bottom 
  // row of the matrix should always be (0,0,0,1). 
  // If you don't set the axes, the axes will default to 
  // (1,0,0), (0,1,0), (0,0,1) and their origin will be (0,0,0). 
  // Generally, this is a permutation matrix (though it need not be). 
  vtkSetObjectMacro(ResliceAxes,vtkMatrix4x4);
  vtkGetObjectMacro(ResliceAxes,vtkMatrix4x4);

  // Description:
  // Set a transform to be applied to the reslicing axes.
  // If you don't set this, it will be treated as the identity transform.
  // This is often used to obtain oblique slices from the original data,
  // or to regrid one data set to match another given a linear
  // registration transformation.
  vtkSetObjectMacro(ResliceTransform,vtkTransform);
  vtkGetObjectMacro(ResliceTransform,vtkTransform);

  // Description:
  // Turn on wrap-pad feature (default: off). 
  vtkSetMacro(Wrap,int);
  vtkGetMacro(Wrap,int);
  vtkBooleanMacro(Wrap,int);

  // Description:
  // Turn on mirror-pad feature (default: off). 
  // This will override the wrap-pad, if set.
  vtkSetMacro(Mirror,int);
  vtkGetMacro(Mirror,int);
  vtkBooleanMacro(Mirror,int);

  // Description:
  // Set interpolation mode (default: nearest neighbor). 
  vtkSetMacro(InterpolationMode,int);
  vtkGetMacro(InterpolationMode,int);
  void SetInterpolationModeToNearestNeighbor()
    { this->SetInterpolationMode(VTK_RESLICE_NEAREST); };
  void SetInterpolationModeToLinear()
    { this->SetInterpolationMode(VTK_RESLICE_LINEAR); };
  void SetInterpolationModeToCubic()
    { this->SetInterpolationMode(VTK_RESLICE_CUBIC); };
  char *GetInterpolationModeAsString();

  // Description:
  // Obsolete method, but still convenient for switching between 
  // nearest-neighbor and linear interpolation (default: off). 
  void SetInterpolate(int terp) { this->SetInterpolationMode( \
     (terp ? VTK_RESLICE_LINEAR : VTK_RESLICE_NEAREST)); };
  void InterpolateOn() { this->SetInterpolationModeToLinear(); };
  void InterpolateOff() { this->SetInterpolationModeToNearestNeighbor(); };
  int GetInterpolate() { 
    return (this->GetInterpolationMode() != VTK_RESLICE_NEAREST); };

  // Description:
  // Turn on and off optimizations (default on, turn them off only if
  // they are not stable on your architecture). 
  vtkSetMacro(Optimization,int);
  vtkGetMacro(Optimization,int);
  vtkBooleanMacro(Optimization,int);

  // Description:
  // Set the background color (for multi-component images)
  vtkSetVector4Macro(BackgroundColor, float);
  vtkGetVector4Macro(BackgroundColor, float);

  // Description:
  // Set background grey level (for single-component images)
  void SetBackgroundLevel(float v) {this->SetBackgroundColor(v,v,v,v);};
  float GetBackgroundLevel() { return this->GetBackgroundColor()[0]; };

  // Description:
  // Spacing, origin, and extent of output data. 
  // The OutputSpacing default is (1,1,1). 
  // The OutputOrigin and OutputExtent are set to cover the entire
  // transformed input extent by default.
  vtkSetVector3Macro(OutputSpacing, float);
  vtkGetVector3Macro(OutputSpacing, float);
  vtkSetVector3Macro(OutputOrigin, float);
  vtkGetVector3Macro(OutputOrigin, float);
  vtkSetVector6Macro(OutputExtent, int);
  vtkGetVector6Macro(OutputExtent, int);

  // Description:
  // When determining the modified time of the filter, 
  // check the modified time of the transform.
  unsigned long int GetMTime();

  // Description:
  // Helper functions not meant to be used outside this class. 
//BTX
  vtkMatrix4x4 *GetIndexMatrix();
  int FindExtent(int& r1, int& r2, double *point, double *xAxis,
		      int *inMin, int *inMax, int *outExt);
//ETX
protected:
  vtkImageReslice();
  ~vtkImageReslice();
  vtkImageReslice(const vtkImageReslice&) {};
  void operator=(const vtkImageReslice&) {};

  vtkMatrix4x4 *ResliceAxes;
  vtkTransform *ResliceTransform;
  vtkMatrix4x4 *IndexMatrix;
  int Wrap;
  int Mirror;
  int InterpolationMode;
  int Optimization;
  float BackgroundColor[4];
  float OutputOrigin[3];
  float OutputSpacing[3];
  int OutputExtent[6];

  void ExecuteInformation(vtkImageData *input, vtkImageData *output);
  void ExecuteInformation(){this->vtkImageToImageFilter::ExecuteInformation();};
  void ComputeInputUpdateExtent(int inExt[6], int outExt[6]);
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData, 
		       int ext[6], int id);
  void OptimizedComputeInputUpdateExtent(int inExt[6], int outExt[6]);
  void OptimizedThreadedExecute(vtkImageData *inData, vtkImageData *outData, 
				int ext[6], int id);  
};

//----------------------------------------------------------------------------
inline char *vtkImageReslice::GetInterpolationModeAsString()
{
  switch (this->InterpolationMode)
    {
    case VTK_RESLICE_NEAREST:
      return "NearestNeighbor";
    case VTK_RESLICE_LINEAR:
      return "Linear";
    case VTK_RESLICE_CUBIC:
      return "Cubic";
    default:
      return "";
    }
}  

#endif





