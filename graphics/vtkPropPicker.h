/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPropPicker.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


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

#include "vtkWorldPointPicker.h"
class vtkProp;

class VTK_EXPORT vtkPropPicker : public vtkWorldPointPicker
{
public:
  static vtkPropPicker *New();

  const char *GetClassName() {return "vtkPropPicker";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Perform the pick and set the PickedProp ivar.   If something is
  // picked, a 1 is returned, otherwise 0 is returned.  Use the GetPickedProp
  // method to get the prop picked.  Props are picked from the renerers list
  // of pickable Props.
  int PickProp(float selectionX, float selectionY, vtkRenderer *renderer);  
  // Description:
  // Perform a pick from the collection contents and not the cotents of the renderer
  int PickProp(float selectionX, float selectionY, vtkRenderer *renderer, 
	       vtkPropCollection* pickfrom);  
  // Description:
  // Overide parent Pick function
  int Pick(float selectionX, float selectionY, float selectionZ, 
           vtkRenderer *renderer);  
  vtkGetObjectMacro(PickedProp, vtkProp);
protected:
  vtkPropPicker();
  ~vtkPropPicker() {};
  vtkPropPicker(const vtkPropPicker&) {};
  void operator=(vtkPropPicker&) {};

  void Initialize();
  
  vtkProp* PickedProp;
  vtkPropCollection* PickFromProps;
};

#endif


