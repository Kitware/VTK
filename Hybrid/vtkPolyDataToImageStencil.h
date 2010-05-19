/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataToImageStencil.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

Copyright (c) 2008 Atamai, Inc.

Use, modification and redistribution of the software, in source or
binary forms, are permitted provided that the following terms and
conditions are met:

1) Redistribution of the source code, in verbatim or modified
   form, must retain the above copyright notice, this license,
   the following disclaimer, and any notices that refer to this
   license and/or the following disclaimer.  

2) Redistribution in binary form must include the above copyright
   notice, a copy of this license and the following disclaimer
   in the documentation or with other materials provided with the
   distribution.

3) Modified copies of the source code must be clearly marked as such,
   and must not be misrepresented as verbatim copies of the source code.

THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE SOFTWARE "AS IS"
WITHOUT EXPRESSED OR IMPLIED WARRANTY INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE.  IN NO EVENT SHALL ANY COPYRIGHT HOLDER OR OTHER PARTY WHO MAY
MODIFY AND/OR REDISTRIBUTE THE SOFTWARE UNDER THE TERMS OF THIS LICENSE
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, LOSS OF DATA OR DATA BECOMING INACCURATE
OR LOSS OF PROFIT OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF
THE USE OR INABILITY TO USE THE SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGES.

=========================================================================*/
// .NAME vtkPolyDataToImageStencil - use polydata to mask an image
// .SECTION Description
// The vtkPolyDataToImageStencil class will convert a surface mesh
// into an image stencil that can be used to mask an image with
// vtkImageStencil, or used to calculate statistics within the
// enclosed region with vtkImageAccumulate.
// .SECTION Caveats
// The input polydata must contain polygons (or other 2D cells) that
// form a 3D surface that encloses a finite volume. Polyline contours
// are ignored.
// .SECTION See Also
// vtkImageStencil vtkImageAccumulate vtkImageBlend vtkImageReslice

#ifndef __vtkPolyDataToImageStencil_h
#define __vtkPolyDataToImageStencil_h

#include "vtkImageStencilSource.h"

class vtkMergePoints;
class vtkDataSet;
class vtkPolyData;
class vtkImageData;

class VTK_HYBRID_EXPORT vtkPolyDataToImageStencil :
  public vtkImageStencilSource
{
public:
  static vtkPolyDataToImageStencil* New();
  vtkTypeMacro(vtkPolyDataToImageStencil, vtkImageStencilSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the implicit function to convert into a stencil.
  virtual void SetInput(vtkPolyData*);
  vtkPolyData *GetInput();
  
  // Description:
  // Set a vtkImageData that has the Spacing, Origin, and
  // WholeExtent that will be used for the stencil.  This
  // input should be set to the image that you wish to
  // apply the stencil to.  If you use this method, then
  // any values set with the SetOutputSpacing, SetOutputOrigin,
  // and SetOutputWholeExtent methods will be ignored.
  virtual void SetInformationInput(vtkImageData*);
  vtkGetObjectMacro(InformationInput, vtkImageData);

  // Description:
  // Set the Origin to be used for the stencil.  It should be
  // set to the Origin of the image you intend to apply the
  // stencil to. The default value is (0,0,0).
  vtkSetVector3Macro(OutputOrigin, double);
  vtkGetVector3Macro(OutputOrigin, double);

  // Description:
  // Set the Spacing to be used for the stencil. It should be
  // set to the Spacing of the image you intend to apply the
  // stencil to. The default value is (1,1,1)
  vtkSetVector3Macro(OutputSpacing, double);
  vtkGetVector3Macro(OutputSpacing, double);

  // Description:
  // Set the whole extent for the stencil (anything outside
  // this extent will be considered to be "outside" the stencil).
  // If this is not set, then the stencil will always use
  // the requested UpdateExtent as the stencil extent.
  vtkSetVector6Macro(OutputWholeExtent, int);
  vtkGetVector6Macro(OutputWholeExtent, int);  

  // Description:
  // The tolerance to apply in when determining whether a voxel
  // is inside the stencil, given as a fraction of a voxel.
  // Only used in X and Y, not in Z.
  vtkSetClampMacro(Tolerance, double, 0.0, 1.0);
  vtkGetMacro(Tolerance, double);

protected:
  vtkPolyDataToImageStencil();
  ~vtkPolyDataToImageStencil();

  void ThreadedExecute(vtkImageStencilData *output,
                       int extent[6], int threadId);

  static void DataSetCutter(vtkDataSet *input, vtkPolyData *output,
                            double z, vtkMergePoints *locator);
  
  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **,
                                 vtkInformationVector *);
  virtual int FillInputPortInformation(int, vtkInformation*);

  // Description:
  // Set in subclasses where the primary input is not a vtkImageData.
  vtkImageData *InformationInput;

  // Description:
  // Set in subclasses where the primary input is not a vtkImageData.
  int OutputWholeExtent[6];
  double OutputOrigin[3];
  double OutputSpacing[3];

  // Description:
  // The tolerance distance for favoring the inside of the stencil
  double Tolerance;

private:
  vtkPolyDataToImageStencil(const vtkPolyDataToImageStencil&);  // Not implemented.
  void operator=(const vtkPolyDataToImageStencil&);  // Not implemented.
};

#endif
