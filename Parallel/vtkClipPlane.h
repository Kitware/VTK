/*=========================================================================

  Program:   ParaView
  Module:    vtkClipPlane.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

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
// .NAME vtkClipPlane - SImple version
// .SECTION Description
// vtkClipPlane is a simple version of the super class.
// The need for this should go away once all attributes are put in field.


#ifndef __vtkClipPlane_h
#define __vtkClipPlane_h

#include "vtkClipDataSet.h"
#include "vtkPlane.h"


class VTK_PARALLEL_EXPORT vtkClipPlane : public vtkClipDataSet
{
public:
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeMacro(vtkClipPlane,vtkClipDataSet);
  static vtkClipPlane *New();

  // Description:
  // Direct acces to the plane parameters
  vtkSetVector3Macro(Origin, float);
  vtkGetVector3Macro(Origin, float);
  vtkSetVector3Macro(Normal, float);
  vtkGetVector3Macro(Normal, float);

  // Description:
  // Offset of the plane from the origin.
  // Units are respect to normal.
  vtkSetMacro(Offset, float);
  vtkGetMacro(Offset, float);

protected:
  vtkClipPlane();
  ~vtkClipPlane();
  vtkClipPlane(const vtkClipPlane&) {};
  void operator=(const vtkClipPlane&) {};

  void Execute(); //generate output data

  float Normal[3];
  float Origin[3];
  float Offset;
  
  vtkPlane *PlaneFunction;
  
};

#endif


