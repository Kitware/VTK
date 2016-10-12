/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphLayoutFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkGraphLayoutFilter
 * @brief   nice layout of undirected graphs in 3D
 *
 * vtkGraphLayoutFilter will reposition a network of nodes, connected by
 * lines or polylines, into a more pleasing arrangement. The class
 * implements a simple force-directed placement algorithm
 * (Fruchterman & Reingold "Graph Drawing by Force-directed Placement"
 * Software-Practice and Experience 21(11) 1991).
 *
 * The input to the filter is a vtkPolyData representing the undirected
 * graphs. A graph is represented by a set of polylines and/or lines.
 * The output is also a vtkPolyData, where the point positions have been
 * modified. To use the filter, specify whether you wish the layout to
 * occur in 2D or 3D; the bounds in which the graph should lie (note that you
 * can just use automatic bounds computation); and modify the cool down
 * rate (controls the final process of simulated annealing).
*/

#ifndef vtkGraphLayoutFilter_h
#define vtkGraphLayoutFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSGENERAL_EXPORT vtkGraphLayoutFilter : public vtkPolyDataAlgorithm
{
public:
  static vtkGraphLayoutFilter *New();

  vtkTypeMacro(vtkGraphLayoutFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Set / get the region in space in which to place the final graph.
   * The GraphBounds only affects the results if AutomaticBoundsComputation
   * is off.
   */
  vtkSetVector6Macro(GraphBounds,double);
  vtkGetVectorMacro(GraphBounds,double,6);
  //@}

  //@{
  /**
   * Turn on/off automatic graph bounds calculation. If this
   * boolean is off, then the manually specified GraphBounds is used.
   * If on, then the input's bounds us used as the graph bounds.
   */
  vtkSetMacro(AutomaticBoundsComputation, int);
  vtkGetMacro(AutomaticBoundsComputation, int);
  vtkBooleanMacro(AutomaticBoundsComputation, int);
  //@}

  //@{
  /**
   * Set/Get the maximum number of iterations to be used.
   * The higher this number, the more iterations through the algorithm
   * is possible, and thus, the more the graph gets modified.
   */
  vtkSetClampMacro(MaxNumberOfIterations, int, 0, VTK_INT_MAX);
  vtkGetMacro(MaxNumberOfIterations, int);
  //@}

  //@{
  /**
   * Set/Get the Cool-down rate.
   * The higher this number is, the longer it will take to "cool-down",
   * and thus, the more the graph will be modified.
   */
  vtkSetClampMacro(CoolDownRate, double, 0.01, VTK_DOUBLE_MAX);
  vtkGetMacro(CoolDownRate, double);
  //@}

  // Turn on/off layout of graph in three dimensions. If off, graph
  // layout occurs in two dimensions. By default, three dimensional
  // layout is on.
  vtkSetMacro(ThreeDimensionalLayout, int);
  vtkGetMacro(ThreeDimensionalLayout, int);
  vtkBooleanMacro(ThreeDimensionalLayout, int);

protected:
  vtkGraphLayoutFilter();
  ~vtkGraphLayoutFilter() VTK_OVERRIDE {}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;

  double GraphBounds[6];
  int   AutomaticBoundsComputation;  //Boolean controls automatic bounds calc.
  int   MaxNumberOfIterations;  //Maximum number of iterations.
  double CoolDownRate;  //Cool-down rate.  Note:  Higher # = Slower rate.
  int   ThreeDimensionalLayout;  //Boolean for a third dimension.
private:
  vtkGraphLayoutFilter(const vtkGraphLayoutFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGraphLayoutFilter&) VTK_DELETE_FUNCTION;
};

#endif
