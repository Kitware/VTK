/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRayCastFunction.h
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

// .NAME vtkVolumeRayCastFunction - a superclass for ray casting functions

// .SECTION Description
// vtkVolumeRayCastFunction is a superclass for ray casting functions that 
// can be used within a vtkVolumeRayCastMapper. This includes for example,
// vtkVolumeRayCastCompositeFunction, vtkVolumeRayCastMIPFunction, and
// vtkVolumeRayCastIsosurfaceFunction.

// .SECTION See Also
// vtkVolumeRayCastCompositeFunction vtkVolumeRayCastMIPFunction
// vtkVolumeRayCastIsosurfaceFunction vtkVolumeRayCastMapper

#ifndef __vtkVolumeRayCastFunction_h
#define __vtkVolumeRayCastFunction_h

#include "vtkObject.h"
#include "vtkRayCastStructures.h"
#include "vtkVolumeRayCastStructures.h"

class vtkRenderer;
class vtkVolume;
class vtkVolumeRayCastMapper;

class VTK_EXPORT vtkVolumeRayCastFunction : public vtkObject
{
public:
  vtkTypeMacro(vtkVolumeRayCastFunction,vtkObject);

  // Description:
  // Do the basic initialization. This includes saving the parameters
  // passed in into local variables, as well as grabbing some useful
  // info from the volume property and normal encoder. This initialize
  // routine is called once per render. It also calls the 
  // SpecificFunctionInitialize of the subclass function.
//BTX
  void FunctionInitialize( vtkRenderer *ren,
			   vtkVolume   *vol,
			   VTKRayCastVolumeInfo *volumeInfo,
			   vtkVolumeRayCastMapper *mapper );

  virtual void CastRay( VTKRayCastRayInfo *rayInfo,
			VTKRayCastVolumeInfo *volumeInfo )=0;
//ETX

  // Description:
  // Get the value below which all scalar values are considered to
  // have 0 opacity.
  virtual float GetZeroOpacityThreshold( vtkVolume *vol )=0;

protected:
  vtkVolumeRayCastFunction() {};
  ~vtkVolumeRayCastFunction() {};
  vtkVolumeRayCastFunction(const vtkVolumeRayCastFunction &);
  void operator=(const vtkVolumeRayCastFunction &);

  // Description:
  // This method gives the subclass a chance to do any special
  // initialization that it may need to do
//BTX
  virtual void SpecificFunctionInitialize( vtkRenderer *ren,
					   vtkVolume   *vol,
					   VTKRayCastVolumeInfo *volumeInfo,
					   vtkVolumeRayCastMapper *mapper )=0;
//ETX
};

#endif
