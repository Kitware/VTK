/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageStateSpace.h
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
// .NAME vtkImageStateSpace - ImageStateSpace for CLAW to search.
// .SECTION Description
// vtkImageStateSpace has topological and collision methods that defines
// a space. For now, the maximum dimensionality of state space is three.


#ifndef __vtkImageStateSpace_h
#define __vtkImageStateSpace_h

#include "vtkStateSpace.h"
#include "vtkImageRegion.h"
#include "vtkImagePaint.h"
#include "vtkImageViewer.h"
#include "vtkPolyData.h"
#include "vtkClaw.h"


class VTK_EXPORT vtkImageStateSpace : public vtkStateSpace
{
public:
  vtkImageStateSpace();
  ~vtkImageStateSpace();
  char *GetClassName() {return "vtkImageStateSpace";};

  // Description:
  // For debugging.  Maybe it should know the planner anyway.
  vtkSetObjectMacro(Planner, vtkClaw);
  
  // Description:
  // This StateSpace only deals with volumes, 
  // so there are 3 degrees of freedom.
  int GetDegreesOfFreedom();

  // Description:
  // This state space is general for any dimensionality
  vtkSetMacro(StateDimensionality, int);
  vtkGetMacro(StateDimensionality, int);

  // Description:
  // Allocates a new state.
  float *NewState() {return new float[this->GetStateDimensionality()];};
  
  // Description:
  // Returns  0.0 if state is out of bounds
  float BoundsTest(float *state);

  // Description:
  // This function computes max distance between two points.
  // Manhatten distance. 
  float Distance(float *s0, float *s1);

  // Description:
  // This function determines collision space from free space
  int Collide(float *state);

  void GetMiddleState(float *s0, float *s1, float *middle);
  
  void GetChildState(float *state, int axis, float distance, float *child);
  
  // Description:
  // Set/Get the image which defines the space.
  vtkSetObjectMacro(Region,vtkImageRegion);
  vtkGetObjectMacro(Region,vtkImageRegion);
  
  // Description:
  // Set/Get the value which defines collision space.
  vtkSetMacro(CollisionValue,int);
  vtkGetMacro(CollisionValue,int);

  // Description:
  // Get the viewer to give planning feedback.
  vtkImageViewer *GetViewer();
  vtkImagePaint *GetCanvas();

  void DrawPath(vtkClaw *claw);
  
  vtkPolyData *GetPathPolyData(vtkClaw *planner);
  vtkPolyData *GetSpherePolyData(vtkClaw *planner);
  vtkPolyData *GetCollisionPolyData(vtkClaw *planner);
  
  // Description:
  // The planner can call this method to report the end of a sample period 
  void SampleCallBack(vtkClaw *planner);

  void CollisionCallBack(float *collision);

  
  
protected:
  vtkClaw *Planner;
  int StateDimensionality;
  vtkImageRegion *Region;
  int CollisionValue;
  vtkImagePaint *Canvas;
  vtkImageViewer *Viewer;

  // Description:
  // This function make sure the circular parameter is in range -rad -> rad.
  // Same point, smaller(smallest) absolute parameter values.
  void Wrap(float *state);
  
  void CheckCanvas();
  void ClearCanvas();
  
};

#endif















