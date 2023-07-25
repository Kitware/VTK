// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTDxInteractorStyleSettings
 * @brief   3DConnexion device settings
 *
 *
 * vtkTDxInteractorStyleSettings defines settings for 3DConnexion device such
 * as sensitivity, axis filters
 *
 * @sa
 * vtkInteractorStyle vtkRenderWindowInteractor
 * vtkTDxInteractorStyle
 */

#ifndef vtkTDxInteractorStyleSettings_h
#define vtkTDxInteractorStyleSettings_h

#include "vtkObject.h"
#include "vtkRenderingCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGCORE_EXPORT vtkTDxInteractorStyleSettings : public vtkObject
{
public:
  static vtkTDxInteractorStyleSettings* New();
  vtkTypeMacro(vtkTDxInteractorStyleSettings, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Sensitivity of the rotation angle. This can be any value:
   * positive, negative, null.
   * - x<-1.0: faster reversed
   * - x=-1.0: reversed neutral
   * - -1.0<x<0.0:  reversed slower
   * - x=0.0: no rotation
   * - 0.0<x<1.0: slower
   * - x=1.0: neutral
   * - x>1.0: faster
   */
  vtkSetMacro(AngleSensitivity, double);
  vtkGetMacro(AngleSensitivity, double);
  ///@}

  ///@{
  /**
   * Use or mask the rotation component around the X-axis. Initial value is
   * true.
   */
  vtkSetMacro(UseRotationX, bool);
  vtkGetMacro(UseRotationX, bool);
  ///@}

  ///@{
  /**
   * Use or mask the rotation component around the Y-axis. Initial value is
   * true.
   */
  vtkSetMacro(UseRotationY, bool);
  vtkGetMacro(UseRotationY, bool);
  ///@}

  ///@{
  /**
   * Use or mask the rotation component around the Z-axis. Initial value is
   * true.
   */
  vtkSetMacro(UseRotationZ, bool);
  vtkGetMacro(UseRotationZ, bool);
  ///@}

  ///@{
  /**
   * Sensitivity of the translation along the X-axis. This can be any value:
   * positive, negative, null.
   * - x<-1.0: faster reversed
   * - x=-1.0: reversed neutral
   * - -1.0<x<0.0:  reversed slower
   * - x=0.0: no translation
   * - 0.0<x<1.0: slower
   * - x=1.0: neutral
   * - x>1.0: faster
   * Initial value is 1.0
   */
  vtkSetMacro(TranslationXSensitivity, double);
  vtkGetMacro(TranslationXSensitivity, double);
  ///@}

  ///@{
  /**
   * Sensitivity of the translation along the Y-axis.
   * See comment of SetTranslationXSensitivity().
   */
  vtkSetMacro(TranslationYSensitivity, double);
  vtkGetMacro(TranslationYSensitivity, double);
  ///@}

  ///@{
  /**
   * Sensitivity of the translation along the Z-axis.
   * See comment of SetTranslationXSensitivity().
   */
  vtkSetMacro(TranslationZSensitivity, double);
  vtkGetMacro(TranslationZSensitivity, double);
  ///@}

protected:
  vtkTDxInteractorStyleSettings();
  ~vtkTDxInteractorStyleSettings() override;

  double AngleSensitivity;
  bool UseRotationX;
  bool UseRotationY;
  bool UseRotationZ;

  double TranslationXSensitivity;
  double TranslationYSensitivity;
  double TranslationZSensitivity;

private:
  vtkTDxInteractorStyleSettings(const vtkTDxInteractorStyleSettings&) = delete;
  void operator=(const vtkTDxInteractorStyleSettings&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
