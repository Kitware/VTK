/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStateSpace.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkStateSpace - StateSpace for CLAW to search.
// .SECTION Description
// vtkStateSpace has topological and collision methods that defines
// a space. For now, the maximum dimensionality of state space is three.


#ifndef __vtkStateSpace_h
#define __vtkStateSpace_h

#include "vtkObject.h"
#include "vtkClaw.h"

class VTK_EXPORT vtkStateSpace : public vtkObject
{
public:
  vtkStateSpace();
  ~vtkStateSpace();
  char *GetClassName() {return "vtkStateSpace";};

  // Description:
  // The planner can call this method to report the creation of a new sphere.
  virtual void SphereCallBack(Sphere *sphere){sphere = sphere;};
  // Description:
  // The planner can call this method to report the recording of a collision.
  virtual void CollisionCallBack(float *state){state = state;};
  // Description:
  // The planner can call this method to report the end of a sample period.
  virtual void SampleCallBack(vtkClaw *planner){planner = planner;};
    
  
  // Description:
  // Returns the number of independent state variables.
  // Determines how many directions the GetChildState will take.
  virtual int GetDegreesOfFreedom() = 0;

  // Description:
  // Returns the number of elements in the state vector.  It is used
  // by claw to determine how much memory to allocate for each state.
  virtual int GetStateDimensionality() = 0;

  // Description:
  // Allocates memory to hold a state.
  virtual float *NewState() = 0;
  
  // Description:
  // Returns  a floating point value form 0 to 1 that represents
  // the pseudo probablility that a state will be in the final path.
  // It is used to implement guide paths.
  virtual float BoundsTest(float *state) = 0;

  // Description:
  // This method computes max distance between two points.
  virtual float Distance(float *s0, float *s1) = 0;

  // Description:
  // This method determines collision space from free space.
  // It is assumed that this is an expensive operation.
  virtual int Collide(float *state) = 0;

  // Description:
  // This method should return the state half way between two states.
  // It is used to break a link into smaller steps.
  virtual void GetMiddleState(float *s0, float *s1, float *middle) = 0;
  
  // Description:
  // This method should return a new (child) state from a parent state.
  // The child state should be "distance" along "axis".
  virtual void GetChildState(float *state, int axis, float distance, 
			     float *child) = 0;
  
  
protected:

};

#endif















