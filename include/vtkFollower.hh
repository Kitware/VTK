/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFollower.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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

#ifndef __vtkFollower_hh
#define __vtkFollower_hh

#include "vtkActor.hh"
class vtkCamera;

class vtkFollower : public vtkActor
{
 public:
  vtkFollower();
  ~vtkFollower();
  char *GetClassName() {return "vtkFollower";};
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void GetMatrix(vtkMatrix4x4& m);

  // Description:
  // Set/Get the camera to follow. If this is not set, then the follower
  // won't know who to follow.
  vtkSetObjectMacro(Camera,vtkCamera);
  vtkGetObjectMacro(Camera,vtkCamera);

protected:
  vtkCamera *Camera; 
};

#endif

