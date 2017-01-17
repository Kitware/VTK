/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeMapView.h

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
 * @class   vtkTreeMapView
 * @brief   Displays a tree as a tree map.
 *
 *
 * vtkTreeMapView shows a vtkTree in a tree map, where each vertex in the
 * tree is represented by a box.  Child boxes are contained within the
 * parent box, and may be colored and sized by various parameters.
*/

#ifndef vtkTreeMapView_h
#define vtkTreeMapView_h

#include "vtkViewsInfovisModule.h" // For export macro
#include "vtkTreeAreaView.h"

class vtkBoxLayoutStrategy;
class vtkSliceAndDiceLayoutStrategy;
class vtkSquarifyLayoutStrategy;

class VTKVIEWSINFOVIS_EXPORT vtkTreeMapView : public vtkTreeAreaView
{
public:
  static vtkTreeMapView *New();
  vtkTypeMacro(vtkTreeMapView, vtkTreeAreaView);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Sets the treemap layout strategy
   */
  void SetLayoutStrategy(vtkAreaLayoutStrategy* s) VTK_OVERRIDE;
  virtual void SetLayoutStrategy(const char* name);
  virtual void SetLayoutStrategyToBox();
  virtual void SetLayoutStrategyToSliceAndDice();
  virtual void SetLayoutStrategyToSquarify();
  //@}

  //@{
  /**
   * The sizes of the fonts used for labeling.
   */
  virtual void SetFontSizeRange(
    const int maxSize, const int minSize, const int delta=4);
  virtual void GetFontSizeRange(int range[3]);
  //@}

protected:
  vtkTreeMapView();
  ~vtkTreeMapView() VTK_OVERRIDE;

  vtkSmartPointer<vtkBoxLayoutStrategy> BoxLayout;
  vtkSmartPointer<vtkSliceAndDiceLayoutStrategy> SliceAndDiceLayout;
  vtkSmartPointer<vtkSquarifyLayoutStrategy> SquarifyLayout;

private:
  vtkTreeMapView(const vtkTreeMapView&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTreeMapView&) VTK_DELETE_FUNCTION;
};

#endif
