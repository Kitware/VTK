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

#include "vtkAbstractContextItem.h"
#include "vtkRenderingContext2DModule.h" // For export macro

class vtkContextTransform;

class VTKRENDERINGCONTEXT2D_EXPORT vtkContextItem : public vtkAbstractContextItem
{
public:
  vtkTypeMacro(vtkContextItem, vtkAbstractContextItem);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the opacity of the item.
   */
  vtkGetMacro(Opacity, double);
  ///@}

  ///@{
  /**
   * Set the opacity of the item.
   * 1.0 by default.
   */
  vtkSetMacro(Opacity, double);
  ///@}

  /**
   * Set the transform of the item.
   */
  vtkSetMacro(Transform, vtkContextTransform*);

protected:
  vtkContextItem() = default;
  ~vtkContextItem() override = default;

  double Opacity = 1.0;
  vtkContextTransform* Transform = nullptr;

private:
  vtkContextItem(const vtkContextItem&) = delete;
  void operator=(const vtkContextItem&) = delete;
};

#endif // vtkContextItem_h
