/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEdgeLayoutStrategy.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
/**
 * @class   vtkEdgeLayoutStrategy
 * @brief   abstract superclass for all edge layout strategies
 *
 *
 * All edge layouts should subclass from this class.  vtkEdgeLayoutStrategy
 * works as a plug-in to the vtkEdgeLayout algorithm.
*/

#ifndef vtkEdgeLayoutStrategy_h
#define vtkEdgeLayoutStrategy_h

#include "vtkInfovisLayoutModule.h" // For export macro
#include "vtkObject.h"

class vtkGraph;

class VTKINFOVISLAYOUT_EXPORT vtkEdgeLayoutStrategy : public vtkObject
{
public:
  vtkTypeMacro(vtkEdgeLayoutStrategy,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Setting the graph for the layout strategy
   */
  virtual void SetGraph(vtkGraph *graph);

  /**
   * This method allows the layout strategy to
   * do initialization of data structures
   * or whatever else it might want to do.
   */
  virtual void Initialize() {}

  /**
   * This is the layout method where the graph that was
   * set in SetGraph() is laid out.
   */
  virtual void Layout()=0;

  //@{
  /**
   * Set/Get the field to use for the edge weights.
   */
  vtkSetStringMacro(EdgeWeightArrayName);
  vtkGetStringMacro(EdgeWeightArrayName);
  //@}

protected:
  vtkEdgeLayoutStrategy();
  ~vtkEdgeLayoutStrategy();

  vtkGraph *Graph;
  char     *EdgeWeightArrayName;

private:

  vtkEdgeLayoutStrategy(const vtkEdgeLayoutStrategy&) VTK_DELETE_FUNCTION;
  void operator=(const vtkEdgeLayoutStrategy&) VTK_DELETE_FUNCTION;
};

#endif

