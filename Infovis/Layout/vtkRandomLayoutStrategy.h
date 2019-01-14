/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRandomLayoutStrategy.h

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
 * @class   vtkRandomLayoutStrategy
 * @brief   randomly places vertices in 2 or 3 dimensions
 *
 *
 * Assigns points to the vertices of a graph randomly within a bounded range.
 *
 * .SECTION Thanks
 * Thanks to Brian Wylie from Sandia National Laboratories for adding incremental
 * layout capabilities.
*/

#ifndef vtkRandomLayoutStrategy_h
#define vtkRandomLayoutStrategy_h

#include "vtkInfovisLayoutModule.h" // For export macro
#include "vtkGraphLayoutStrategy.h"

class VTKINFOVISLAYOUT_EXPORT vtkRandomLayoutStrategy : public vtkGraphLayoutStrategy
{
public:
  static vtkRandomLayoutStrategy *New();

  vtkTypeMacro(vtkRandomLayoutStrategy, vtkGraphLayoutStrategy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Seed the random number generator used to compute point positions.
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
  vtkSetMacro(AutomaticBoundsComputation, vtkTypeBool);
  vtkGetMacro(AutomaticBoundsComputation, vtkTypeBool);
  vtkBooleanMacro(AutomaticBoundsComputation, vtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off layout of graph in three dimensions. If off, graph
   * layout occurs in two dimensions. By default, three dimensional
   * layout is on.
   */
  vtkSetMacro(ThreeDimensionalLayout, vtkTypeBool);
  vtkGetMacro(ThreeDimensionalLayout, vtkTypeBool);
  vtkBooleanMacro(ThreeDimensionalLayout, vtkTypeBool);
  //@}

  /**
   * Set the graph to layout.
   */
  void SetGraph(vtkGraph *graph) override;

  /**
   * Perform the random layout.
   */
  void Layout() override;

protected:
  vtkRandomLayoutStrategy();
  ~vtkRandomLayoutStrategy() override;

  int RandomSeed;
  double GraphBounds[6];
  vtkTypeBool   AutomaticBoundsComputation;
  vtkTypeBool   ThreeDimensionalLayout;  //Boolean for a third dimension.
private:

  vtkRandomLayoutStrategy(const vtkRandomLayoutStrategy&) = delete;
  void operator=(const vtkRandomLayoutStrategy&) = delete;
};

#endif

