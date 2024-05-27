// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

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

#include "vtkAbstractContextItem.h"
#include "vtkRenderingContext2DModule.h" // For export macro
#include "vtkSmartPointer.h"             // Needed for SP ivars.
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGCONTEXT2D_EXPORT VTK_MARSHALAUTO vtkContextClip : public vtkAbstractContextItem
{
public:
  vtkTypeMacro(vtkContextClip, vtkAbstractContextItem);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Creates a vtkContextClip object.
   */
  static vtkContextClip* New();

  /**
   * Perform any updates to the item that may be necessary before rendering.
   * The scene should take care of calling this on all items before their
   * Paint function is invoked.
   */
  void Update() override;

  /**
   * Paint event for the item, called whenever the item needs to be drawn.
   */
  bool Paint(vtkContext2D* painter) override;

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
  ~vtkContextClip() override;

  float Dims[4];

private:
  vtkContextClip(const vtkContextClip&) = delete;
  void operator=(const vtkContextClip&) = delete;
};

inline void vtkContextClip::GetRect(float rect[4])
{
  rect[0] = this->Dims[0];
  rect[1] = this->Dims[1];
  rect[2] = this->Dims[2];
  rect[3] = this->Dims[3];
}

VTK_ABI_NAMESPACE_END
#endif // vtkContextClip_h
