/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHexahedron.h
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
// .NAME vtkHexahedron - a cell that represents a 3D rectangular hexahedron
// .SECTION Description
// vtkHexahedron is a concrete implementation of vtkCell to represent a 3D
// rectangular hexahedron (e.g., "brick" topology).

#ifndef __vtkHexahedron_h
#define __vtkHexahedron_h

#include "vtkCell.h"

class vtkHexahedron : public vtkCell
{
public:
  vtkHexahedron() {};
  vtkHexahedron(const vtkHexahedron& h);
  char *GetClassName() {return "vtkHexahedron";};

  // cell methods
  vtkCell *MakeObject() {return new vtkHexahedron(*this);};
  int GetCellType() {return VTK_HEXAHEDRON;};
  int GetCellDimension() {return 3;};
  int GetNumberOfEdges() {return 12;};
  int GetNumberOfFaces() {return 6;};
  vtkCell *GetEdge(int edgeId);
  vtkCell *GetFace(int faceId);

  int CellBoundary(int subId, float pcoords[3], vtkIdList& pts);
  void Contour(float value, vtkFloatScalars *cellScalars, 
               vtkPointLocator *locator, vtkCellArray *verts, 
               vtkCellArray *lines, vtkCellArray *polys, vtkFloatScalars *s);
  int EvaluatePosition(float x[3], float closestPoint[3],
                       int& subId, float pcoords[3],
                       float& dist2, float *weights);
  void EvaluateLocation(int& subId, float pcoords[3], float x[3],
                        float *weights);
  int IntersectWithLine(float p1[3], float p2[3], float tol, float& t,
                        float x[3], float pcoords[3], int& subId);
  int Triangulate(int index, vtkFloatPoints &pts);
  void Derivatives(int subId, float pcoords[3], float *values, 
                   int dim, float *derivs);

  // Hexahedron specific
  void InterpolationFunctions(float pcoords[3], float weights[8]);
  void InterpolationDerivs(float pcoords[3], float derivs[24]);
  void JacobianInverse(float pcoords[3], double **inverse, float derivs[24]);

};

#endif


