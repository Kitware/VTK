/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFollower.h
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
// .NAME vtkFollower - a subclass of actor that always faces the camera
// .SECTION Description
// vtkFollower is a subclass of vtkActor that always follows its specified 
// camera. More specifically it will not change its position or scale,
// but it will continually update its orientation so that it is right side
// up and facing the camera. This is typically used for text labels in a
// scene. All of the adjustments that can be made to an actor also will
// take effect with a follower.  So, if you change the orientation of the
// follower by 90 degrees, then it will follow the camera, but be off by 
// 90 degrees.

// .SECTION see also
// vtkActor vtkCamera

#ifndef __vtkFollower_h
#define __vtkFollower_h

#include "vtkActor.h"
#include "vtkCamera.h"

class VTK_RENDERING_EXPORT vtkFollower : public vtkActor
{
 public:
  vtkTypeMacro(vtkFollower,vtkActor);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Creates a follower with no camera set
  static vtkFollower *New();

  // Description:
  // This causes the actor to be rendered. It in turn will render the actor's
  // property, texture map and then mapper. If a property hasn't been
  // assigned, then the actor will create one automatically. 
  virtual int RenderOpaqueGeometry(vtkViewport *viewport);
  virtual int RenderTranslucentGeometry(vtkViewport *viewport);
  virtual void Render(vtkRenderer *ren);
  
  // Description:
  // Copy the follower's composite 4x4 matrix into the matrix provided.
  virtual void GetMatrix(vtkMatrix4x4 *m);
  virtual void GetMatrix(double m[16])
    {this->GetMatrix(this->Matrix); vtkMatrix4x4::DeepCopy(m,this->Matrix);};
  virtual vtkMatrix4x4* GetMatrix()
    {return this->vtkActor::GetMatrix();}

  // Description:
  // Set/Get the camera to follow. If this is not set, then the follower
  // won't know who to follow.
  vtkSetObjectMacro(Camera,vtkCamera);
  vtkGetObjectMacro(Camera,vtkCamera);

  // Description:
  // Shallow copy of a follower. Overloads the virtual vtkProp method.
  void ShallowCopy(vtkProp *prop);

protected:
  vtkFollower();
  ~vtkFollower();

  vtkCamera *Camera; 
  vtkActor  *Device;
private:
  // hide the two parameter Render() method from the user and the compiler.
  virtual void Render(vtkRenderer *, vtkMapper *) {};
private:
  vtkFollower(const vtkFollower&);  // Not implemented.
  void operator=(const vtkFollower&);  // Not implemented.
};

#endif



