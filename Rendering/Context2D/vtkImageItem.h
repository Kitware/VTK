// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkImageItem
 * @brief   a vtkContextItem that draws a supplied image in the
 * scene.
 *
 *
 * This vtkContextItem draws the supplied image in the scene.
 */

#ifndef vtkImageItem_h
#define vtkImageItem_h

#include "vtkContextItem.h"
#include "vtkRenderingContext2DModule.h" // For export macro
#include "vtkSmartPointer.h"             // For SP ivars.
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkImageData;

class VTKRENDERINGCONTEXT2D_EXPORT VTK_MARSHALAUTO vtkImageItem : public vtkContextItem
{
public:
  vtkTypeMacro(vtkImageItem, vtkContextItem);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkImageItem* New();

  /**
   * Paint event for the item.
   */
  bool Paint(vtkContext2D* painter) override;

  /**
   * Set the image of the item.
   */
  void SetImage(vtkImageData* image);

  ///@{
  /**
   * Get the image of the item.
   */
  vtkGetObjectMacro(Image, vtkImageData);
  ///@}

  ///@{
  /**
   * Set the position of the bottom corner of the image.
   */
  vtkSetVector2Macro(Position, float);
  ///@}

  ///@{
  /**
   * Get the position of the bottom corner of the image.
   */
  vtkGetVector2Macro(Position, float);
  ///@}

protected:
  vtkImageItem();
  ~vtkImageItem() override;

  float Position[2];

  vtkImageData* Image;

private:
  vtkImageItem(const vtkImageItem&) = delete;
  void operator=(const vtkImageItem&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkImageItem_h
