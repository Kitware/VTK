// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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

VTK_ABI_NAMESPACE_BEGIN
class vtkGraph;

class VTKINFOVISLAYOUT_EXPORT vtkEdgeLayoutStrategy : public vtkObject
{
public:
  vtkTypeMacro(vtkEdgeLayoutStrategy, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Setting the graph for the layout strategy
   */
  virtual void SetGraph(vtkGraph* graph);

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
  virtual void Layout() = 0;

  ///@{
  /**
   * Set/Get the field to use for the edge weights.
   */
  vtkSetStringMacro(EdgeWeightArrayName);
  vtkGetStringMacro(EdgeWeightArrayName);
  ///@}

protected:
  vtkEdgeLayoutStrategy();
  ~vtkEdgeLayoutStrategy() override;

  vtkGraph* Graph;
  char* EdgeWeightArrayName;

private:
  vtkEdgeLayoutStrategy(const vtkEdgeLayoutStrategy&) = delete;
  void operator=(const vtkEdgeLayoutStrategy&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
