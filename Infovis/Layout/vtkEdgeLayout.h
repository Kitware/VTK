/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEdgeLayout.h

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

#include "vtkInfovisLayoutModule.h" // For export macro
#include "vtkGraphAlgorithm.h"

class vtkEdgeLayoutStrategy;
class vtkEventForwarderCommand;

class VTKINFOVISLAYOUT_EXPORT vtkEdgeLayout : public vtkGraphAlgorithm
{
public:
  static vtkEdgeLayout *New();
  vtkTypeMacro(vtkEdgeLayout, vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * The layout strategy to use during graph layout.
   */
  void SetLayoutStrategy(vtkEdgeLayoutStrategy *strategy);
  vtkGetObjectMacro(LayoutStrategy, vtkEdgeLayoutStrategy);
  //@}

  /**
   * Get the modification time of the layout algorithm.
   */
  virtual vtkMTimeType GetMTime();

protected:
  vtkEdgeLayout();
  ~vtkEdgeLayout();

  vtkEdgeLayoutStrategy* LayoutStrategy;

  //@{
  /**
   * This intercepts events from the strategy object and re-emits them
   * as if they came from the layout engine itself.
   */
  vtkEventForwarderCommand *EventForwarder;
  unsigned long ObserverTag;
  //@}

  int RequestData(
    vtkInformation *,
    vtkInformationVector **,
    vtkInformationVector *);

private:

  vtkGraph *InternalGraph;

  vtkEdgeLayout(const vtkEdgeLayout&) VTK_DELETE_FUNCTION;
  void operator=(const vtkEdgeLayout&) VTK_DELETE_FUNCTION;
};

#endif
