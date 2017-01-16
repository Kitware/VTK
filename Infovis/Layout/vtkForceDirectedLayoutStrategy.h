/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkForceDirectedLayoutStrategy.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   vtkForceDirectedLayoutStrategy
 * @brief   a force directed graph layout algorithm
 *
 *
 * Lays out a graph in 2D or 3D using a force-directed algorithm.
 * The user may specify whether to layout the graph randomly initially,
 * the bounds, the number of dimensions (2 or 3), and the cool-down rate.
 *
 * @par Thanks:
 * Thanks to Brian Wylie for adding functionality for allowing this layout
 * to be incremental.
*/

#ifndef vtkForceDirectedLayoutStrategy_h
#define vtkForceDirectedLayoutStrategy_h

#include "vtkInfovisLayoutModule.h" // For export macro
#include "vtkGraphLayoutStrategy.h"

class VTKINFOVISLAYOUT_EXPORT vtkForceDirectedLayoutStrategy : public vtkGraphLayoutStrategy
{
public:
  static vtkForceDirectedLayoutStrategy *New();

  vtkTypeMacro(vtkForceDirectedLayoutStrategy, vtkGraphLayoutStrategy);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Seed the random number generator used to jitter point positions.
   * This has a significant effect on their final positions when
   * the layout is complete.
   */
  vtkSetClampMacro(RandomSeed, int, 0, VTK_INT_MAX);
  vtkGetMacro(RandomSeed, int);
  //@}

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
   * The default is '50' for no particular reason
   */
  vtkSetClampMacro(MaxNumberOfIterations, int, 0, VTK_INT_MAX);
  vtkGetMacro(MaxNumberOfIterations, int);
  //@}

  //@{
  /**
   * Set/Get the number of iterations per layout.
   * The only use for this ivar is for the application
   * to do visualizations of the layout before it's complete.
   * The default is '50' to match the default 'MaxNumberOfIterations'
   */
  vtkSetClampMacro(IterationsPerLayout, int, 0, VTK_INT_MAX);
  vtkGetMacro(IterationsPerLayout, int);
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

  //@{
  /**
   * Turn on/off layout of graph in three dimensions. If off, graph
   * layout occurs in two dimensions. By default, three dimensional
   * layout is off.
   */
  vtkSetMacro(ThreeDimensionalLayout, int);
  vtkGetMacro(ThreeDimensionalLayout, int);
  vtkBooleanMacro(ThreeDimensionalLayout, int);
  //@}

  //@{
  /**
   * Turn on/off use of random positions within the graph bounds as initial points.
   */
  vtkSetMacro(RandomInitialPoints, int);
  vtkGetMacro(RandomInitialPoints, int);
  vtkBooleanMacro(RandomInitialPoints, int);
  //@}

  //@{
  /**
   * Set the initial temperature.  If zero (the default) , the initial temperature
   * will be computed automatically.
   */
  vtkSetClampMacro(InitialTemperature, float, 0.0, VTK_FLOAT_MAX);
  vtkGetMacro(InitialTemperature, float);
  //@}

  /**
   * This strategy sets up some data structures
   * for faster processing of each Layout() call
   */
  void Initialize() VTK_OVERRIDE;

  /**
   * This is the layout method where the graph that was
   * set in SetGraph() is laid out. The method can either
   * entirely layout the graph or iteratively lay out the
   * graph. If you have an iterative layout please implement
   * the IsLayoutComplete() method.
   */
  void Layout() VTK_OVERRIDE;

  /**
   * I'm an iterative layout so this method lets the caller
   * know if I'm done laying out the graph
   */
  int IsLayoutComplete() VTK_OVERRIDE {return this->LayoutComplete;}

protected:
  vtkForceDirectedLayoutStrategy();
  ~vtkForceDirectedLayoutStrategy() VTK_OVERRIDE;

  double GraphBounds[6];
  int   AutomaticBoundsComputation;  //Boolean controls automatic bounds calc.
  int   MaxNumberOfIterations;  //Maximum number of iterations.
  double CoolDownRate;  //Cool-down rate.  Note:  Higher # = Slower rate.
  double InitialTemperature;
  int   ThreeDimensionalLayout;  //Boolean for a third dimension.
  int RandomInitialPoints; //Boolean for having random points
private:

  // A vertex contains a position and a displacement.
  typedef struct
  {
    double x[3];
    double d[3];
  } vtkLayoutVertex;

  // An edge consists of two vertices joined together.
  // This struct acts as a "pointer" to those two vertices.
  typedef struct
  {
    int t;
    int u;
  } vtkLayoutEdge;

  int RandomSeed;
  int IterationsPerLayout;
  int TotalIterations;
  int LayoutComplete;
  double Temp;
  double optDist;
  vtkLayoutVertex *v;
  vtkLayoutEdge *e;

  vtkForceDirectedLayoutStrategy(const vtkForceDirectedLayoutStrategy&) VTK_DELETE_FUNCTION;
  void operator=(const vtkForceDirectedLayoutStrategy&) VTK_DELETE_FUNCTION;
};

#endif

