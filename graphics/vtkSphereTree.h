/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSphereTree.h
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
// .NAME vtkSphereTree - generate sphere tree to approximate a polygonal model.
// .SECTION Description
// vtkSphereTree is an experimental sphere tree that could be used for 
// collision detection.  The idea is to model the volume of the models insides,
// not just its surface.  This should allow a much closer tolerance
// with fewer spheres.

#ifndef __vtkSphereTree_h
#define __vtkSphereTree_h

#include <math.h>
#include <stdlib.h>
#include "vtkPolyDataToPolyDataFilter.h"



// A helper object.
// Each triangle has one of these "objects".
// Spheres contian the triangle, but no other verticies.
// This information needs to be stored in case new verticies are added.
typedef struct vtkSphereTreeSphere;
struct vtkSphereTreeSphere
{
  // All points of triangle are TriangleRadius distance from centroid.
  float P0[3];
  float P1[3];
  float P2[3];
  float TriangleCentroid[3];
  float TriangleNormal[3];
  float TriangleRadiusSquared;
  // Determines sphere's center (TriangleCentroid + K * TriangleNormal)
  float K;
  // Radius can be computed (Radius = sqrt(K^2 + TriangleRadiusSquared); 
  float RadiusSquared;
  float Center[3];
  // To keep the objects in a list
  vtkSphereTreeSphere *Next;
};
 


class VTK_EXPORT vtkSphereTree : public vtkPolyDataToPolyDataFilter
{
public:
  vtkSphereTree();
  static vtkSphereTree *New() {return new vtkSphereTree;};
  const char *GetClassName() {return "vtkSphereTree";};

  void Execute();

  // Description:
  // Set/Get Tolerance (The largest allowed over estimation.)
  vtkSetMacro(Tolerance, float);
  vtkGetMacro(Tolerance, float);
  
  // Description:
  // Set/Get the maximum radius of a sphere in the tree.
  vtkSetMacro(MaximumRadius, float);
  vtkGetMacro(MaximumRadius, float);
  
  // Description:
  // Determine if spheres should be on the inside or outside.
  vtkSetMacro(Inside, int);
  vtkGetMacro(Inside, int);
  vtkBooleanMacro(Inside, int);

protected:
  float Tolerance;
  float MaximumRadius;
  int Inside;
  vtkSphereTreeSphere *Spheres;
  vtkPoints *Points;
  vtkPoints *NewPoints;
  
  void TriangleExecute(float *p0, float *p1, float *p2);
  void BigTriangleExecute(float *p0, float *p1, float *p2);
  void SplitTriangle(float *p0, float *p1, float *p2);
  
  int ComputeTriangleInfo(float *p0, float *p1, float *p2,
			  float *centroid, float *normal,float &radiusSquared);
  void ComputeSegmentInfo(float *p0, float *p1, float *center,
			  float &radiusSquared);
  float ComputeNewCentroid(float *centroid, float *normal,
			   float radiusSquared, float *point);

  void AddPoint(float *point);
  vtkSphereTreeSphere *AddSphere(float *triangleCentroid, 
				 float *triangleNormal,
				 float triangleRadiusSquared,
				 float *p0, float *p1, float *p2);
  vtkSphereTreeSphere *MakeSphere(float *p0, float *p1, float *p2,
				  float *triangleCentroid, 
				  float *triangleNormal, 
				  float triangleRadiusSquared, float kMin, 
				  float *center);
  void CleanUp();
  int GetNumberOfSpheres();
  
};

#endif


