/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkChairDisplay.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkChairDisplay - generate isosurface(s) from volume/images
// .SECTION Description


#ifndef __vtkChairDisplay_h
#define __vtkChairDisplay_h

#include "vtkImageData.h"
#include "vtkPolyDataSource.h"

class VTK_EXPORT vtkChairDisplay : public vtkPolyDataSource
{
public:
  static vtkChairDisplay *New();
  vtkTypeMacro(vtkChairDisplay,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set/Get the source for the scalar data to contour.
  void SetInput(vtkImageData *input);
  vtkImageData *GetInput();
  
  // Description:
  // Set/Get the size of the notch.
  vtkSetMacro(XNotchSize, int);
  vtkGetMacro(XNotchSize, int);
  vtkSetMacro(YNotchSize, int);
  vtkGetMacro(YNotchSize, int);
  vtkSetMacro(ZNotchSize, int);
  vtkGetMacro(ZNotchSize, int);

  void Update();
  
  vtkStructuredPoints *GetTextureOutput() {return this->TextureOutput;};

  void GenerateTexture(vtkImageData *inData, vtkScalars *scalars,
                       int xstart, int ystart,int xsize, int ysize, int p2x);
  
protected:
  vtkChairDisplay();
  ~vtkChairDisplay();
  vtkChairDisplay(const vtkChairDisplay&);
  void operator=(const vtkChairDisplay&);

  vtkScalars *Scalars;
  vtkStructuredPoints *TextureOutput;
  
  int MaxYZSize;

  int XNotchSize;
  int YNotchSize;
  int ZNotchSize;
  
  void Execute(int recomputeTexture);
  void GeneratePolyData(int *, float *, float *, int, int,
			vtkCellArray *, vtkPoints *, vtkTCoords *);
};

#endif



