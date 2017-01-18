/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScatterPlotMatrix.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkIncrementalForceLayout
 * @brief   incremental force-directed layout.
 *
 *
 * Performs an incremental force-directed layout of a graph. Set the graph
 * then iteratively execute UpdatePositions() to update the vertex positions.
 * Note that this directly modifies the vertex locations in the graph.
 *
 * This layout is modeled after D3's force layout described at
 * https://github.com/mbostock/d3/wiki/Force-Layout
*/

#ifndef vtkIncrementalForceLayout_h
#define vtkIncrementalForceLayout_h

#include "vtkInfovisLayoutModule.h" // For export macro
#include "vtkObject.h"

#include "vtkVector.h" // For vector ivars

class vtkGraph;

class VTKINFOVISLAYOUT_EXPORT vtkIncrementalForceLayout : public vtkObject
{
public:
  vtkTypeMacro(vtkIncrementalForceLayout, vtkObject);
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;
  static vtkIncrementalForceLayout* New();

  //@{
  /**
   * Set the graph to be positioned.
   */
  virtual void SetGraph(vtkGraph* g);
  vtkGetObjectMacro(Graph, vtkGraph);
  //@}

  //@{
  /**
   * Set the id of the vertex that will not move during the simulation.
   * Set to -1 to allow all the vertices to move.
   */
  virtual void SetFixed(vtkIdType fixed);
  vtkGetMacro(Fixed, vtkIdType);
  //@}

  //@{
  /**
   * Set the level of activity in the simulation. Default is 0.1.
   */
  vtkSetMacro(Alpha, float);
  vtkGetMacro(Alpha, float);
  //@}

  //@{
  /**
   * Set the Barnes-Hut threshold for the simulation. Higher values
   * will speed the simulation at the expense of some accuracy.
   * Default is 0.8.
   */
  vtkSetMacro(Theta, float);
  vtkGetMacro(Theta, float);
  //@}

  //@{
  /**
   * Set the charge of each vertex. Higher negative values will repel vertices
   * from each other more strongly. Default is -30.
   */
  vtkSetMacro(Charge, float);
  vtkGetMacro(Charge, float);
  //@}

  //@{
  /**
   * Set the rigitity of links in the simulation. Default is 2.
   */
  vtkSetMacro(Strength, float);
  vtkGetMacro(Strength, float);
  //@}

  //@{
  /**
   * Set the resting distance of each link in scene units, which is equal to
   * pixels when there is no scene scaling. Default is 20.
   */
  vtkSetMacro(Distance, float);
  vtkGetMacro(Distance, float);
  //@}

  //@{
  /**
   * Set the amount of gravitational pull toward the gravity point.
   * Default is 0.01.
   */
  vtkSetMacro(Gravity, float);
  vtkGetMacro(Gravity, float);
  //@}

  //@{
  /**
   * Set the multiplier for scaling down velocity in the simulation, where values closer to 1
   * are more frictionless. Default is 0.95.
   */
  vtkSetMacro(Friction, float);
  vtkGetMacro(Friction, float);
  //@}

  /**
   * Set the gravity point where all vertices will migrate. Generally this
   * should be set to the location in the center of the scene.
   * Default location is (200, 200).
   */
  virtual void SetGravityPoint(const vtkVector2f &point)
    { this->GravityPoint = point; }
  virtual vtkVector2f GetGravityPoint()
    { return this->GravityPoint; }

  /**
   * Perform one iteration of the force-directed layout.
   */
  void UpdatePositions();

protected:
  vtkIncrementalForceLayout();
  ~vtkIncrementalForceLayout() VTK_OVERRIDE;

  vtkGraph* Graph;
  class Implementation;
  Implementation* Impl;
  vtkIdType Fixed;
  vtkVector2f GravityPoint;
  float Alpha;
  float Theta;
  float Charge;
  float Strength;
  float Distance;
  float Gravity;
  float Friction;

private:
  vtkIncrementalForceLayout(const vtkIncrementalForceLayout &) VTK_DELETE_FUNCTION;
  void operator=(const vtkIncrementalForceLayout &) VTK_DELETE_FUNCTION;
};
#endif
