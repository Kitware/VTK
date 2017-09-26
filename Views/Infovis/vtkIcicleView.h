/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIcicleView.h

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
 * @class   vtkIcicleView
 * @brief   Displays a tree in a stacked "icicle" view
 *
 *
 * vtkIcicleView shows a vtkTree in horizontal layers
 * where each vertex in the tree is represented by a bar.
 * Child sectors are below (or above) parent sectors, and may be
 * colored and sized by various parameters.
*/

#ifndef vtkIcicleView_h
#define vtkIcicleView_h

#include "vtkViewsInfovisModule.h" // For export macro
#include "vtkTreeAreaView.h"

class VTKVIEWSINFOVIS_EXPORT vtkIcicleView : public vtkTreeAreaView
{
public:
  static vtkIcicleView *New();
  vtkTypeMacro(vtkIcicleView, vtkTreeAreaView);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Sets whether the stacks go from top to bottom or bottom to top.
   */
  virtual void SetTopToBottom(bool value);
  virtual bool GetTopToBottom();
  vtkBooleanMacro(TopToBottom, bool);
  //@}

  //@{
  /**
   * Set the width of the root node
   */
  virtual void SetRootWidth(double width);
  virtual double GetRootWidth();
  //@}

  //@{
  /**
   * Set the thickness of each layer
   */
  virtual void SetLayerThickness(double thickness);
  virtual double GetLayerThickness();
  //@}

  //@{
  /**
   * Turn on/off gradient coloring.
   */
  virtual void SetUseGradientColoring(bool value);
  virtual bool GetUseGradientColoring();
  vtkBooleanMacro(UseGradientColoring, bool);
  //@}

protected:
  vtkIcicleView();
  ~vtkIcicleView() override;

private:
  vtkIcicleView(const vtkIcicleView&) = delete;
  void operator=(const vtkIcicleView&) = delete;
};

#endif
