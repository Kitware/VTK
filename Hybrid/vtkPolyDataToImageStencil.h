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

Copyright (c) 2004 Atamai, Inc.

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
// .NAME vtkPolyDataToImageStencil
// .SECTION Description
// vtkPolyDataToImageStencil will convert a vtkPolyData into an image
// that can be used with vtkImageStecil or other vtk classes that apply
// a stencil to an image.  The algorithm used is as follows:
// First a YZ projection plane is defined, where each YZ pixel has a list
// for storing X values that are associated with that YZ position.
// Each polygon is projected onto the YZ plane and the X value for each
// projected YZ point is stored. After this is done for all the polygons,
// each list of X values is sorted from lowest to greatest.  Each list
// of X values hence defines the intersection points of a ray along
// the X direction at position YZ with the polygons that form the
// polydata. These stored intersection points are used to create the
// vtkImageStencilData.
// .SECTION Caveats
// This filter does not work on 2D polydata. The polydata must form a
// closed 3D surface.
// .SECTION Thanks
// Thanks to David Gobbi and Glen Lehmann of Atamai Inc. for
// developing this class.
// .SECTION see also
// vtkPolyData vtkImageStencil vtkImplicitFunctionToImageStencil

#ifndef __vtkPolyDataToImageStencil_h
#define __vtkPolyDataToImageStencil_h

#include "vtkImageStencilSource.h"

class vtkPolyData;

class VTK_HYBRID_EXPORT vtkPolyDataToImageStencil : public vtkImageStencilSource
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
  // Set the tolerance for doing spatial searches of the polydata.
  vtkSetMacro(Tolerance, double);
  vtkGetMacro(Tolerance, double);
  
protected:
  vtkPolyDataToImageStencil();
  ~vtkPolyDataToImageStencil();

  double Tolerance;

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int, vtkInformation*);

private:
  vtkPolyDataToImageStencil(const vtkPolyDataToImageStencil&);  // Not implemented.
  void operator=(const vtkPolyDataToImageStencil&);  // Not implemented.
};

#endif
