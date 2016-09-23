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
 * @class   vtkContextClip
 * @brief   all children of this item are clipped
 * by the specified area.
 *
 *
 * This class can be used to clip the rendering of an item inside a rectangular
 * area.
*/

#ifndef vtkContextClip_h
#define vtkContextClip_h

#include "vtkRenderingContext2DModule.h" // For export macro
#include "vtkAbstractContextItem.h"
#include "vtkSmartPointer.h" // Needed for SP ivars.

class VTKRENDERINGCONTEXT2D_EXPORT vtkContextClip : public vtkAbstractContextItem
{
public:
  vtkTypeMacro(vtkContextClip, vtkAbstractContextItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  /**
   * Creates a vtkContextClip object.
   */
  static vtkContextClip *New();

  /**
   * Perform any updates to the item that may be necessary before rendering.
   * The scene should take care of calling this on all items before their
   * Paint function is invoked.
   */
  virtual void Update();

  /**
   * Paint event for the item, called whenever the item needs to be drawn.
   */
  virtual bool Paint(vtkContext2D *painter);

  /**
   * Set the origin, width and height of the clipping rectangle. These are in
   * pixel coordinates.
   */
  virtual void SetClip(float x, float y, float width, float height);

  /**
   * Get the clipping rectangle parameters in pixel coordinates:
   */
  virtual void GetRect(float rect[4]);
  virtual float GetX() { return Dims[0]; }
  virtual float GetY() { return Dims[1]; }
  virtual float GetWidth() { return Dims[2]; }
  virtual float GetHeight() { return Dims[3]; }

protected:
  vtkContextClip();
  ~vtkContextClip();

  float Dims[4];

private:
  vtkContextClip(const vtkContextClip &) VTK_DELETE_FUNCTION;
  void operator=(const vtkContextClip &) VTK_DELETE_FUNCTION;

};

inline void vtkContextClip::GetRect(float rect[4])
{
  rect[0] = this->Dims[0];
  rect[1] = this->Dims[1];
  rect[2] = this->Dims[2];
  rect[3] = this->Dims[3];
}

#endif //vtkContextClip_h
