/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataMapper.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkPolyDataMapper - map vtkPolyData to graphics primitives
// .SECTION Description
// vtkPolyDataMapper is a class that maps polygonal data (i.e., vtkPolyData)
// to graphics primitives. vtkPolyDataMapper serves as a superclass for
// device-specific poly data mappers, that actually do the mapping to the
// rendering/graphics hardware/software.

#ifndef __vtkPolyDataMapper_h
#define __vtkPolyDataMapper_h

#include "vtkMapper.h"
#include "vtkPolyData.h"
#include "vtkRenderer.h"

class VTK_EXPORT vtkPolyDataMapper : public vtkMapper 
{
public:
  static vtkPolyDataMapper *New();
  vtkTypeMacro(vtkPolyDataMapper,vtkMapper);

  // Description:
  // Implement required Render method. Just a dummy routine here.
  virtual void Render(vtkRenderer *, vtkActor *) {};

  // Description:
  // Specify the input data to map.
  void SetInput(vtkPolyData *in);
  vtkPolyData *GetInput();
  
  void Update();
  float *GetBounds();

  // Description:
  // If you want only a part of the data, specify by seting the piece.
  vtkSetMacro(Piece, int);
  vtkGetMacro(Piece, int);
  vtkSetMacro(NumberOfPieces, int);
  vtkGetMacro(NumberOfPieces, int);

protected:  
  vtkPolyDataMapper() {};
  ~vtkPolyDataMapper() {};
  vtkPolyDataMapper(const vtkPolyDataMapper&) {};
  void operator=(const vtkPolyDataMapper&) {};
  void PrintSelf(ostream& os, vtkIndent indent);

  int Piece;
  int NumberOfPieces;

};

#endif


