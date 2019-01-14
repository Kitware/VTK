/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPassThroughEdgeStrategy.h

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
 * @class   vtkPassThroughEdgeStrategy
 * @brief   passes edge routing information through
 *
 *
 * Simply passes existing edge layout information from the input to the
 * output without making changes.
*/

#ifndef vtkPassThroughEdgeStrategy_h
#define vtkPassThroughEdgeStrategy_h

#include "vtkInfovisLayoutModule.h" // For export macro
#include "vtkEdgeLayoutStrategy.h"

class VTKINFOVISLAYOUT_EXPORT vtkPassThroughEdgeStrategy : public vtkEdgeLayoutStrategy
{
public:
  static vtkPassThroughEdgeStrategy* New();
  vtkTypeMacro(vtkPassThroughEdgeStrategy,vtkEdgeLayoutStrategy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * This is the layout method where the graph that was
   * set in SetGraph() is laid out.
   */
  void Layout() override;

protected:
  vtkPassThroughEdgeStrategy();
  ~vtkPassThroughEdgeStrategy() override;

private:
  vtkPassThroughEdgeStrategy(const vtkPassThroughEdgeStrategy&) = delete;
  void operator=(const vtkPassThroughEdgeStrategy&) = delete;
};

#endif

