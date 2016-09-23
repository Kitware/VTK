/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphLayout.h

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
 * @class   vtkGraphLayout
 * @brief   layout a graph in 2 or 3 dimensions
 *
 *
 * This class is a shell for many graph layout strategies which may be set
 * using the SetLayoutStrategy() function.  The layout strategies do the
 * actual work.
 *
 * .SECION Thanks
 * Thanks to Brian Wylie from Sandia National Laboratories for adding incremental
 * layout capabilities.
*/

#ifndef vtkGraphLayout_h
#define vtkGraphLayout_h

#include "vtkInfovisLayoutModule.h" // For export macro
#include "vtkGraphAlgorithm.h"

class vtkAbstractTransform;
class vtkEventForwarderCommand;
class vtkGraphLayoutStrategy;

class VTKINFOVISLAYOUT_EXPORT vtkGraphLayout : public vtkGraphAlgorithm
{
public:
  static vtkGraphLayout *New();
  vtkTypeMacro(vtkGraphLayout, vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * The layout strategy to use during graph layout.
   */
  void SetLayoutStrategy(vtkGraphLayoutStrategy *strategy);
  vtkGetObjectMacro(LayoutStrategy, vtkGraphLayoutStrategy);
  //@}

  /**
   * Ask the layout algorithm if the layout is complete
   */
  virtual int IsLayoutComplete();

  /**
   * Get the modification time of the layout algorithm.
   */
  virtual vtkMTimeType GetMTime();

  //@{
  /**
   * Set the ZRange for the output data.
   * If the initial layout is planar (i.e. all z coordinates are zero),
   * the coordinates will be evenly spaced from 0.0 to ZRange.
   * The default is zero, which has no effect.
   */
  vtkGetMacro(ZRange, double);
  vtkSetMacro(ZRange, double);
  //@}

  //@{
  /**
   * Transform the graph vertices after the layout.
   */
  vtkGetObjectMacro(Transform, vtkAbstractTransform);
  virtual void SetTransform(vtkAbstractTransform* t);
  //@}

  //@{
  /**
   * Whether to use the specified transform after layout.
   */
  vtkSetMacro(UseTransform, bool);
  vtkGetMacro(UseTransform, bool);
  vtkBooleanMacro(UseTransform, bool);
  //@}

protected:
  vtkGraphLayout();
  ~vtkGraphLayout();

  vtkGraphLayoutStrategy* LayoutStrategy;

  /**
   * This intercepts events from the strategy object and re-emits them
   * as if they came from the layout engine itself.
   */
  vtkEventForwarderCommand *EventForwarder;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:

  vtkGraph* LastInput;
  vtkGraph* InternalGraph;
  vtkMTimeType LastInputMTime;
  bool StrategyChanged;
  double ZRange;
  vtkAbstractTransform* Transform;
  bool UseTransform;

  vtkGraphLayout(const vtkGraphLayout&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGraphLayout&) VTK_DELETE_FUNCTION;
};

#endif
