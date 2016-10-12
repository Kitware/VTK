/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkContextItem
 * @brief   base class for items that are part of a vtkContextScene.
 *
 *
 * Derive from this class to create custom items that can be added to a
 * vtkContextScene.
*/

#ifndef vtkContextItem_h
#define vtkContextItem_h

#include "vtkRenderingContext2DModule.h" // For export macro
#include "vtkAbstractContextItem.h"

class VTKRENDERINGCONTEXT2D_EXPORT vtkContextItem : public vtkAbstractContextItem
{
public:
  vtkTypeMacro(vtkContextItem, vtkAbstractContextItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  //@{
  /**
   * Get the opacity of the item.
   */
  vtkGetMacro(Opacity, double);
  //@}

  //@{
  /**
   * Set the opacity of the item.
   * 1.0 by default.
   */
  vtkSetMacro(Opacity, double);
  //@}

protected:
  vtkContextItem();
  ~vtkContextItem();

  double Opacity;

private:
  vtkContextItem(const vtkContextItem &) VTK_DELETE_FUNCTION;
  void operator=(const vtkContextItem &) VTK_DELETE_FUNCTION;

};

#endif //vtkContextItem_h
