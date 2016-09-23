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
/**
 * @class   vtkCubicLine
 * @brief   cell represents a cubic , isoparametric 1D line
 *
 * vtkCubicLine is a concrete implementation of vtkNonLinearCell to represent a 1D Cubic line.
 * The Cubic Line is the 4 nodes isoparametric parabolic line . The
 * interpolation is the standard finite element, cubic isoparametric
 * shape function. The cell includes two mid-edge nodes. The ordering of the
 * four points defining the cell is point ids (0,1,2,3) where id #2 and #3 are the
 * mid-edge nodes. Please note that the parametric coordinates lie between -1 and 1
 * in accordance with most standard documentations.
 * @par Thanks:
 * <verbatim>
 * This file has been developed by Oxalya - www.oxalya.com
 * Copyright (c) EDF - www.edf.fr
 * </verbatim>
*/

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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * See the vtkCell API for descriptions of these methods.
   */
  int GetCellType() VTK_OVERRIDE {return VTK_CUBIC_LINE;};
  int GetCellDimension() VTK_OVERRIDE {return 1;};
  int GetNumberOfEdges() VTK_OVERRIDE {return 0;};
  int GetNumberOfFaces() VTK_OVERRIDE {return 0;};
  vtkCell *GetEdge(int) VTK_OVERRIDE {return 0;};
  vtkCell *GetFace(int) VTK_OVERRIDE {return 0;};
  int CellBoundary(int subId, double pcoords[3], vtkIdList *pts) VTK_OVERRIDE;
  void Contour(double value, vtkDataArray *cellScalars,
               vtkIncrementalPointLocator *locator, vtkCellArray *verts,
               vtkCellArray *lines, vtkCellArray *polys,
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd) VTK_OVERRIDE;
  int EvaluatePosition(double x[3], double* closestPoint,
                       int& subId, double pcoords[3],
                       double& dist2, double *weights) VTK_OVERRIDE;
  void EvaluateLocation(int& subId, double pcoords[3], double x[3],
                        double *weights) VTK_OVERRIDE;
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts) VTK_OVERRIDE;
  void Derivatives(int subId, double pcoords[3], double *values,
                   int dim, double *derivs) VTK_OVERRIDE;
  double *GetParametricCoords() VTK_OVERRIDE;
  //@}

  /**
   * Return the distance of the parametric coordinate provided to the
   * cell. If inside the cell, a distance of zero is returned.
   */
  double GetParametricDistance(double pcoords[3]) VTK_OVERRIDE;

  /**
   * Clip this line using scalar value provided. Like contouring, except
   * that it cuts the line to produce other lines.
   */
  void Clip(double value, vtkDataArray *cellScalars,
            vtkIncrementalPointLocator *locator, vtkCellArray *lines,
            vtkPointData *inPd, vtkPointData *outPd,
            vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
            int insideOut) VTK_OVERRIDE;

  /**
   * Return the center of the triangle in parametric coordinates.
   */
  int GetParametricCenter(double pcoords[3]) VTK_OVERRIDE;

  /**
   * Line-line intersection. Intersection has to occur within [0,1] parametric
   * coordinates and with specified tolerance.
   */
  int IntersectWithLine(double p1[3], double p2[3], double tol, double& t,
                        double x[3], double pcoords[3], int& subId) VTK_OVERRIDE;



  /**
   * @deprecated Replaced by vtkCubicLine::InterpolateFunctions as of VTK 5.2
   */
  static void InterpolationFunctions(double pcoords[3], double weights[4]);
  /**
   * @deprecated Replaced by vtkCubicLine::InterpolateDerivs as of VTK 5.2
   */
  static void InterpolationDerivs(double pcoords[3], double derivs[4]);
  //@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(double pcoords[3], double weights[4]) VTK_OVERRIDE
  {
    vtkCubicLine::InterpolationFunctions(pcoords,weights);
  }
  void InterpolateDerivs(double pcoords[3], double derivs[4]) VTK_OVERRIDE
  {
    vtkCubicLine::InterpolationDerivs(pcoords,derivs);
  }
  //@}

protected:
  vtkCubicLine();
  ~vtkCubicLine() VTK_OVERRIDE;

  vtkLine *Line;
  vtkDoubleArray *Scalars; //used to avoid New/Delete in contouring/clipping

private:
  vtkCubicLine(const vtkCubicLine&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCubicLine&) VTK_DELETE_FUNCTION;
};

//----------------------------------------------------------------------------
inline int vtkCubicLine::GetParametricCenter(double pcoords[3])
{

  pcoords[0]=pcoords[1] = pcoords[2] = 0.0;
  return 0;
}

#endif



