/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPropPicker.h
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
// .NAME vtkPropPicker - pick an actor/prop using graphics hardware
// .SECTION Description
// vtkPropPicker is used to pick an actor/prop given a selection
// point (in display coordinates) and a renderer. This class uses
// graphics hardware/rendering system to pick rapidly (as compared
// to using ray casting as does vtkCellPicker and vtkPointPicker).
// This class determines the actor/prop and pick position in world
// coordinates; point and cell ids are not determined.

// .SECTION See Also 
// vtkPicker vtkWorldPointPicker vtkCellPicker vtkPointPicker 

#ifndef __vtkPropPicker_h
#define __vtkPropPicker_h

#include "vtkAbstractPropPicker.h"

class vtkProp;
class vtkWorldPointPicker;

class VTK_RENDERING_EXPORT vtkPropPicker : public vtkAbstractPropPicker
{
public:
  static vtkPropPicker *New();

  vtkTypeMacro(vtkPropPicker,vtkAbstractPropPicker);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Perform the pick and set the PickedProp ivar. If something is picked, a
  // 1 is returned, otherwise 0 is returned.  Use the GetProp() method
  // to get the instance of vtkProp that was picked.  Props are picked from
  // the renderers list of pickable Props.
  int PickProp(float selectionX, float selectionY, vtkRenderer *renderer);  

  // Description:
  // Perform a pick from the user-provided list of vtkProps and not from the
  // list of vtkProps that the render maintains.
  int PickProp(float selectionX, float selectionY, vtkRenderer *renderer, 
	       vtkPropCollection* pickfrom);  

  // Description:
  // Overide superclasses' Pick() method.
  int Pick(float selectionX, float selectionY, float selectionZ, 
           vtkRenderer *renderer);  
  int Pick(float selectionPt[3], vtkRenderer *renderer)
    { return this->Pick( selectionPt[0], 
			 selectionPt[1], selectionPt[2], renderer); };  

protected:
  vtkPropPicker();
  ~vtkPropPicker();
  vtkPropPicker(const vtkPropPicker&);
  void operator=(const vtkPropPicker&);

  void Initialize();
  
  vtkPropCollection* PickFromProps;
  
  // Used to get x-y-z pick position
  vtkWorldPointPicker *WorldPointPicker;
};

#endif


