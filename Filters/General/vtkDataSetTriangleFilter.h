/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetTriangleFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDataSetTriangleFilter
 * @brief   triangulate any type of dataset
 *
 * vtkDataSetTriangleFilter generates n-dimensional simplices from any input
 * dataset. That is, 3D cells are converted to tetrahedral meshes, 2D cells
 * to triangles, and so on. The triangulation is guaranteed to be compatible.
 *
 * This filter uses simple 1D and 2D triangulation techniques for cells
 * that are of topological dimension 2 or less. For 3D cells--due to the
 * issue of face compatibility across quadrilateral faces (which way to
 * orient the diagonal?)--a fancier ordered Delaunay triangulation is used.
 * This approach produces templates on the fly for triangulating the
 * cells. The templates are then used to do the actual triangulation.
 *
 * @sa
 * vtkOrderedTriangulator vtkTriangleFilter
*/

#ifndef vtkDataSetTriangleFilter_h
#define vtkDataSetTriangleFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

class vtkOrderedTriangulator;

class VTKFILTERSGENERAL_EXPORT vtkDataSetTriangleFilter : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkDataSetTriangleFilter *New();
  vtkTypeMacro(vtkDataSetTriangleFilter,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * When On this filter will cull all 1D and 2D cells from the output.
   * The default is Off.
   */
  vtkSetMacro(TetrahedraOnly, vtkTypeBool);
  vtkGetMacro(TetrahedraOnly, vtkTypeBool);
  vtkBooleanMacro(TetrahedraOnly, vtkTypeBool);
  //@}

protected:
  vtkDataSetTriangleFilter();
  ~vtkDataSetTriangleFilter() override;

  // Usual data generation method
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

  int FillInputPortInformation(int port, vtkInformation *info) override;

  // Used to triangulate 3D cells
  vtkOrderedTriangulator *Triangulator;

  // Different execute methods depending on whether input is structured or not
  void StructuredExecute(vtkDataSet *, vtkUnstructuredGrid *);
  void UnstructuredExecute(vtkDataSet *, vtkUnstructuredGrid *);

  vtkTypeBool TetrahedraOnly;

private:
  vtkDataSetTriangleFilter(const vtkDataSetTriangleFilter&) = delete;
  void operator=(const vtkDataSetTriangleFilter&) = delete;
};

#endif


