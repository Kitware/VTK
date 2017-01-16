/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLoopSubdivisionFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkLoopSubdivisionFilter
 * @brief   generate a subdivision surface using the Loop Scheme
 *
 * vtkLoopSubdivisionFilter is an approximating subdivision scheme that
 * creates four new triangles for each triangle in the mesh. The user can
 * specify the NumberOfSubdivisions. Loop's subdivision scheme is
 * described in: Loop, C., "Smooth Subdivision surfaces based on
 * triangles,", Masters Thesis, University of Utah, August 1987.
 * For a nice summary of the technique see, Hoppe, H., et. al,
 * "Piecewise Smooth Surface Reconstruction,:, Proceedings of Siggraph 94
 * (Orlando, Florida, July 24-29, 1994). In COmputer Graphics
 * Proceedings, Annual COnference Series, 1994, ACM SIGGRAPH,
 * pp. 295-302.
 * <P>
 * The filter only operates on triangles. Users should use the
 * vtkTriangleFilter to triangulate meshes that contain polygons or
 * triangle strips.
 * <P>
 * The filter approximates point data using the same scheme. New
 * triangles create at a subdivision step will have the cell data of
 * their parent cell.
 *
 * @par Thanks:
 * This work was supported by PHS Research Grant No. 1 P41 RR13218-01
 * from the National Center for Research Resources.
 *
 * @sa
 * vtkApproximatingSubdivisionFilter
*/

#ifndef vtkLoopSubdivisionFilter_h
#define vtkLoopSubdivisionFilter_h

#include "vtkFiltersModelingModule.h" // For export macro
#include "vtkApproximatingSubdivisionFilter.h"

class vtkPolyData;
class vtkIntArray;
class vtkPoints;
class vtkIdList;

class VTKFILTERSMODELING_EXPORT vtkLoopSubdivisionFilter : public vtkApproximatingSubdivisionFilter
{
public:
  //@{
  /**
   * Construct object with NumberOfSubdivisions set to 1.
   */
  static vtkLoopSubdivisionFilter *New();
  vtkTypeMacro(vtkLoopSubdivisionFilter,vtkApproximatingSubdivisionFilter);
  //@}

protected:
  vtkLoopSubdivisionFilter () {}
  ~vtkLoopSubdivisionFilter () VTK_OVERRIDE {}

  int GenerateSubdivisionPoints (vtkPolyData *inputDS, vtkIntArray *edgeData,
                                 vtkPoints *outputPts,
                                 vtkPointData *outputPD) VTK_OVERRIDE;
  int  GenerateEvenStencil (vtkIdType p1, vtkPolyData *polys,
                            vtkIdList *stencilIds, double *weights);
  void GenerateOddStencil (vtkIdType p1, vtkIdType p2, vtkPolyData *polys,
                           vtkIdList *stencilIds, double *weights);

  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;

private:
  vtkLoopSubdivisionFilter(const vtkLoopSubdivisionFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkLoopSubdivisionFilter&) VTK_DELETE_FUNCTION;
};

#endif
// VTK-HeaderTest-Exclude: vtkLoopSubdivisionFilter.h
