// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkEdgeLayout
 * @brief   layout graph edges
 *
 *
 * This class is a shell for many edge layout strategies which may be set
 * using the SetLayoutStrategy() function.  The layout strategies do the
 * actual work.
 */

#ifndef vtkEdgeLayout_h
#define vtkEdgeLayout_h

#include "vtkGraphAlgorithm.h"
#include "vtkInfovisLayoutModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkEdgeLayoutStrategy;
class vtkEventForwarderCommand;

class VTKINFOVISLAYOUT_EXPORT vtkEdgeLayout : public vtkGraphAlgorithm
{
public:
  static vtkEdgeLayout* New();
  vtkTypeMacro(vtkEdgeLayout, vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * The layout strategy to use during graph layout.
   */
  void SetLayoutStrategy(vtkEdgeLayoutStrategy* strategy);
  vtkGetObjectMacro(LayoutStrategy, vtkEdgeLayoutStrategy);
  ///@}

  /**
   * Get the modification time of the layout algorithm.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkEdgeLayout();
  ~vtkEdgeLayout() override;

  vtkEdgeLayoutStrategy* LayoutStrategy;

  ///@{
  /**
   * This intercepts events from the strategy object and re-emits them
   * as if they came from the layout engine itself.
   */
  vtkEventForwarderCommand* EventForwarder;
  unsigned long ObserverTag;
  ///@}

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkGraph* InternalGraph;

  vtkEdgeLayout(const vtkEdgeLayout&) = delete;
  void operator=(const vtkEdgeLayout&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
