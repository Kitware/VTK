/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetMapper.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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
// .NAME vtkDataSetMapper - map vtkDataSet and derived classes to graphics primitives
// .SECTION Description
// vtkDataSetMapper is a mapper to map data sets (i.e., vtkDataSet and 
// all derived classes) to graphics primitives. The mapping procedure
// is as follows: all 0D, 1D, and 2D cells are converted into points,
// lines, and polygons/triangle strips and then mapped to the graphics 
// system. The 2D faces of 3D cells are mapped only if they are used by 
// only one cell, i.e., on the boundary of the data set.

#ifndef __vtkDataSetMapper_h
#define __vtkDataSetMapper_h

#include "vtkDataSetSurfaceFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkImageData.h"


class VTK_RENDERING_EXPORT vtkDataSetMapper : public vtkMapper 
{
public:
  static vtkDataSetMapper *New();
  vtkTypeMacro(vtkDataSetMapper,vtkMapper);
  void PrintSelf(ostream& os, vtkIndent indent);
  void Render(vtkRenderer *ren, vtkActor *act);

  // Description:
  // Get the internal poly data mapper used to map data set to graphics system.
  vtkGetObjectMacro(PolyDataMapper, vtkPolyDataMapper);

  // Description:
  // Release any graphics resources that are being consumed by this mapper.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // Get the mtime also considering the lookup table.
  unsigned long GetMTime();

  // Description:
  // Set the Input of this mapper.
  void SetInput(vtkDataSet *input);
  vtkDataSet *GetInput();
  
protected:
  vtkDataSetMapper();
  ~vtkDataSetMapper();

  vtkDataSetSurfaceFilter *GeometryExtractor;
  vtkPolyDataMapper *PolyDataMapper;
private:
  vtkDataSetMapper(const vtkDataSetMapper&);  // Not implemented.
  void operator=(const vtkDataSetMapper&);  // Not implemented.
};

#endif


