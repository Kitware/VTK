/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageToStructuredPoints.h
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
// .NAME vtkImageToStructuredPoints - Attaches image pipeline to VTK. 
// .SECTION Description
// vtkImageToStructuredPoints changes an image region format to
// a structured points dataset.  The Order of the axes is fixed.
// VTK_IMAGE_X_AXIS is always mapped to X axis of the structured points,
// VTK_IMAGE_COMPONENT_AXIS is always mapped to the Colors of
// ColorScalars of Vectors.  The only use of the Axes instance variable
// is for specifying an extent.


#ifndef __vtkImageToStructuredPoints_h
#define __vtkImageToStructuredPoints_h

#include "vtkStructuredPointsSource.h"
#include "vtkImageSetGet.h"
class vtkImageRegion;
class vtkImageCache;
class vtkColorScalars;

class VTK_EXPORT vtkImageToStructuredPoints : public vtkStructuredPointsSource
{
public:
  vtkImageToStructuredPoints();
  ~vtkImageToStructuredPoints();
  static vtkImageToStructuredPoints *New() 
    {return new vtkImageToStructuredPoints;};
  const char *GetClassName() {return "vtkImageToStructuredPoints";};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set/Get the input object from the image pipeline.
  vtkSetObjectMacro(Input,vtkImageCache);
  vtkGetObjectMacro(Input,vtkImageCache);

  // Set/Get the extent to translate explicitely.
  void SetExtent(int dim, int *extent);
  vtkImageSetExtentMacro(Extent);
  void GetExtent(int dim, int *extent);
  vtkImageGetExtentMacro(Extent);

  // Description:
  // Set/Get the order of the axes to split while streaming.
  void SetSplitOrder(int dim, int *axes);
  vtkImageSetMacro(SplitOrder,int);
  void GetSplitOrder(int dim, int *axes);
  vtkImageGetMacro(SplitOrder,int);
  int *GetSplitOrder() {return this->SplitOrder;};  

  // Description:
  // This object will stream to keep the input regions below this limit.
  vtkSetMacro(InputMemoryLimit,int);
  vtkGetMacro(InputMemoryLimit,int);

  // Description:
  // Which coordinate to use for the fourth dimension. (slice)
  vtkSetMacro(TimeSlice,int);
  vtkGetMacro(TimeSlice,int);
  
  void Update();
  
protected:
  vtkImageCache *Input;
  int TimeSlice;
  int Extent[6];
  int NumberOfSplitAxes;
  int SplitOrder[4];
  int InputMemoryLimit;

  void Execute();
  vtkScalars *GetScalarsFromRegion(vtkImageRegion *region);
  void InputSplitUpdate(int splitAxisIdx);
  vtkScalars *ReformatRegionData(vtkImageRegion *region, int flag);
  vtkColorScalars *CreateColorScalars(vtkScalars *scalars, int dim);
};


#endif


