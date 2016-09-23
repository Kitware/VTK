/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTDxInteractorStyleSettings.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkObject.h"

class VTKRENDERINGCORE_EXPORT vtkTDxInteractorStyleSettings : public vtkObject
{
public:
  static vtkTDxInteractorStyleSettings *New();
  vtkTypeMacro(vtkTDxInteractorStyleSettings,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
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
  vtkSetMacro(AngleSensitivity,double);
  vtkGetMacro(AngleSensitivity,double);
  //@}

  //@{
  /**
   * Use or mask the rotation component around the X-axis. Initial value is
   * true.
   */
  vtkSetMacro(UseRotationX,bool);
  vtkGetMacro(UseRotationX,bool);
  //@}

  //@{
  /**
   * Use or mask the rotation component around the Y-axis. Initial value is
   * true.
   */
  vtkSetMacro(UseRotationY,bool);
  vtkGetMacro(UseRotationY,bool);
  //@}

  //@{
  /**
   * Use or mask the rotation component around the Z-axis. Initial value is
   * true.
   */
  vtkSetMacro(UseRotationZ,bool);
  vtkGetMacro(UseRotationZ,bool);
  //@}

  //@{
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
  vtkSetMacro(TranslationXSensitivity,double);
  vtkGetMacro(TranslationXSensitivity,double);
  //@}

  //@{
  /**
   * Sensitivity of the translation along the Y-axis.
   * See comment of SetTranslationXSensitivity().
   */
  vtkSetMacro(TranslationYSensitivity,double);
  vtkGetMacro(TranslationYSensitivity,double);
  //@}

  //@{
  /**
   * Sensitivity of the translation along the Z-axis.
   * See comment of SetTranslationXSensitivity().
   */
  vtkSetMacro(TranslationZSensitivity,double);
  vtkGetMacro(TranslationZSensitivity,double);
  //@}

protected:
  vtkTDxInteractorStyleSettings();
  virtual ~vtkTDxInteractorStyleSettings();

  double AngleSensitivity;
  bool UseRotationX;
  bool UseRotationY;
  bool UseRotationZ;

  double TranslationXSensitivity;
  double TranslationYSensitivity;
  double TranslationZSensitivity;

private:
  vtkTDxInteractorStyleSettings(const vtkTDxInteractorStyleSettings&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTDxInteractorStyleSettings&) VTK_DELETE_FUNCTION;
};
#endif
