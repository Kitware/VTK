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
/**
 * @class   vtkPolyDataToImageStencil
 * @brief   use polydata to mask an image
 *
 * The vtkPolyDataToImageStencil class will convert polydata into
 * an image stencil.  The polydata can either be a closed surface
 * mesh or a series of polyline contours (one contour per slice).
 * @warning
 * If contours are provided, the contours must be aligned with the
 * Z planes.  Other contour orientations are not supported.
 * @sa
 * vtkImageStencil vtkImageAccumulate vtkImageBlend vtkImageReslice
*/

#ifndef vtkPolyDataToImageStencil_h
#define vtkPolyDataToImageStencil_h

#include "vtkImagingStencilModule.h" // For export macro
#include "vtkImageStencilSource.h"

class vtkMergePoints;
class vtkDataSet;
class vtkPolyData;

class VTKIMAGINGSTENCIL_EXPORT vtkPolyDataToImageStencil :
  public vtkImageStencilSource
{
public:
  static vtkPolyDataToImageStencil* New();
  vtkTypeMacro(vtkPolyDataToImageStencil, vtkImageStencilSource);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Specify the implicit function to convert into a stencil.
   */
  virtual void SetInputData(vtkPolyData*);
  vtkPolyData *GetInput();
  //@}

  //@{
  /**
   * The tolerance for including a voxel inside the stencil.
   * This is in fractions of a voxel, and must be between 0 and 1.
   * Tolerance is only applied in the x and y directions, not in z.
   * Setting the tolerance to zero disables all tolerance checks and
   * might result in faster performance.
   */
  vtkSetClampMacro(Tolerance, double, 0.0, 1.0);
  vtkGetMacro(Tolerance, double);
  //@}

protected:
  vtkPolyDataToImageStencil();
  ~vtkPolyDataToImageStencil() override;

  void ThreadedExecute(vtkImageStencilData *output,
                       int extent[6], int threadId);

  static void PolyDataCutter(vtkPolyData *input, vtkPolyData *output,
                             double z);

  static void PolyDataSelector(vtkPolyData *input, vtkPolyData *output,
                               double z, double thickness);

  int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *) override;

  int FillInputPortInformation(int, vtkInformation*) override;

  /**
   * The tolerance distance for favoring the inside of the stencil
   */
  double Tolerance;

private:
  vtkPolyDataToImageStencil(const vtkPolyDataToImageStencil&) = delete;
  void operator=(const vtkPolyDataToImageStencil&) = delete;
};

#endif
