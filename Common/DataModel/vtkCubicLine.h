/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCubicLine.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCubicLine - cell represents a cubic , isoparametric 1D line
// .SECTION Description
// vtkCubicLine is a concrete implementation of vtkNonLinearCell to represent a 1D Cubic line.
// The Cubic Line is the 4 nodes isoparametric parabolic line . The
// interpolation is the standard finite element, cubic isoparametric
// shape function. The cell includes two mid-edge nodes. The ordering of the
// four points defining the cell is point ids (0,1,2,3) where id #2 and #3 are the
// mid-edge nodes. Please note that the parametric coordinates lie between -1 and 1
// in accordance with most standard documentations.
// .SECTION Thanks
// <verbatim>
// This file has been developed by Oxalya - www.oxalya.com
// Copyright (c) EDF - www.edf.fr
// </verbatim>


#ifndef vtkCubicLine_h
#define vtkCubicLine_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkNonLinearCell.h"

class vtkLine;
class vtkDoubleArray;

class VTKCOMMONDATAMODEL_EXPORT vtkCubicLine : public vtkNonLinearCell
{
public:
  static vtkCubicLine *New();
  vtkTypeMacro(vtkCubicLine,vtkNonLinearCell);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // See the vtkCell API for descriptions of these methods.
  int GetCellType() {return VTK_CUBIC_LINE;};
  int GetCellDimension() {return 1;};
  int GetNumberOfEdges() {return 0;};
  int GetNumberOfFaces() {return 0;};
  vtkCell *GetEdge(int) {return 0;};
  vtkCell *GetFace(int) {return 0;};
  int CellBoundary(int subId, double pcoords[3], vtkIdList *pts);
  void Contour(double value, vtkDataArray *cellScalars,
               vtkIncrementalPointLocator *locator, vtkCellArray *verts,
               vtkCellArray *lines, vtkCellArray *polys,
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd);
  int EvaluatePosition(double x[3], double* closestPoint,
                       int& subId, double pcoords[3],
                       double& dist2, double *weights);
  void EvaluateLocation(int& subId, double pcoords[3], double x[3],
                        double *weights);
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts);
  void Derivatives(int subId, double pcoords[3], double *values,
                   int dim, double *derivs);
  virtual double *GetParametricCoords();

  // Description:
  // Return the distance of the parametric coordinate provided to the
  // cell. If inside the cell, a distance of zero is returned.
  double GetParametricDistance(double pcoords[3]);

  // Description:
  // Clip this line using scalar value provided. Like contouring, except
  // that it cuts the line to produce other lines.
  void Clip(double value, vtkDataArray *cellScalars,
            vtkIncrementalPointLocator *locator, vtkCellArray *lines,
            vtkPointData *inPd, vtkPointData *outPd,
            vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
            int insideOut);

  // Description:
  // Return the center of the triangle in parametric coordinates.
  int GetParametricCenter(double pcoords[3]);

  // Description:
  // Line-line intersection. Intersection has to occur within [0,1] parametric
  // coordinates and with specified tolerance.
  int IntersectWithLine(double p1[3], double p2[3], double tol, double& t,
                        double x[3], double pcoords[3], int& subId);



  // Description:
  // @deprecated Replaced by vtkCubicLine::InterpolateFunctions as of VTK 5.2
  static void InterpolationFunctions(double pcoords[3], double weights[4]);
  // Description:
  // @deprecated Replaced by vtkCubicLine::InterpolateDerivs as of VTK 5.2
  static void InterpolationDerivs(double pcoords[3], double derivs[4]);
  // Description:
  // Compute the interpolation functions/derivatives
  // (aka shape functions/derivatives)
  virtual void InterpolateFunctions(double pcoords[3], double weights[4])
    {
    vtkCubicLine::InterpolationFunctions(pcoords,weights);
    }
  virtual void InterpolateDerivs(double pcoords[3], double derivs[4])
    {
    vtkCubicLine::InterpolationDerivs(pcoords,derivs);
    }

protected:
  vtkCubicLine();
  ~vtkCubicLine();

  vtkLine *Line;
  vtkDoubleArray *Scalars; //used to avoid New/Delete in contouring/clipping

private:
  vtkCubicLine(const vtkCubicLine&);  // Not implemented.
  void operator=(const vtkCubicLine&);  // Not implemented.
};

//----------------------------------------------------------------------------
inline int vtkCubicLine::GetParametricCenter(double pcoords[3])
{

  pcoords[0]=pcoords[1] = pcoords[2] = 0.0;
  return 0;
}

#endif



