/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractMapper.h
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
// .NAME vtkAbstractMapper - abstract class specifies interface to map data
// .SECTION Description
// vtkAbstractMapper is an abstract class to specify interface between data and 
// graphics primitives or software rendering techniques. Subclasses of 
// vtkAbstractMapper can be used for rendering 2D data, geometry, or volumetric
// data.
//
// .SECTION See Also
// vtkAbstractMapper3D vtkMapper vtkPolyDataMapper vtkVolumeMapper

#ifndef __vtkAbstractMapper_h
#define __vtkAbstractMapper_h

#include "vtkProcessObject.h"
class vtkWindow;

class VTK_EXPORT vtkAbstractMapper : public vtkProcessObject
{
public:
  vtkTypeMacro(vtkAbstractMapper,vtkProcessObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Release any graphics resources that are being consumed by this mapper.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *) {};

  // Description:
  // Get the time required to draw the geometry last time it was rendered
  vtkGetMacro( TimeToDraw, float );

protected:
  vtkAbstractMapper();
  ~vtkAbstractMapper() {};
  vtkAbstractMapper(const vtkAbstractMapper&) {};
  void operator=(const vtkAbstractMapper&) {};

  float TimeToDraw;
  vtkWindow *LastWindow;   // Window used for the previous render
};

#endif


