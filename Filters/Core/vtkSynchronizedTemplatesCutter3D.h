/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSynchronizedTemplatesCutter3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSynchronizedTemplatesCutter3D
 * @brief   generate cut surface from structured points
 *
 *
 * vtkSynchronizedTemplatesCutter3D is an implementation of the synchronized
 * template algorithm. Note that vtkCutFilter will automatically
 * use this class when appropriate.
 *
 * @sa
 * vtkContourFilter vtkSynchronizedTemplates3D
*/

#ifndef vtkSynchronizedTemplatesCutter3D_h
#define vtkSynchronizedTemplatesCutter3D_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkSynchronizedTemplates3D.h"

class vtkImplicitFunction;

class VTKFILTERSCORE_EXPORT vtkSynchronizedTemplatesCutter3D : public vtkSynchronizedTemplates3D
{
public:
  static vtkSynchronizedTemplatesCutter3D *New();

  vtkTypeMacro(vtkSynchronizedTemplatesCutter3D,vtkSynchronizedTemplates3D);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Needed by templated functions.
   */
  void ThreadedExecute(vtkImageData *data, vtkInformation *outInfo, int);

  //@{
  /**
   * Specify the implicit function to perform the cutting.
   */
  virtual void SetCutFunction(vtkImplicitFunction*);
  vtkGetObjectMacro(CutFunction,vtkImplicitFunction);
  //@}

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  vtkSetClampMacro(OutputPointsPrecision, int, SINGLE_PRECISION, DEFAULT_PRECISION);
  vtkGetMacro(OutputPointsPrecision, int);
  //@}

protected:
  vtkSynchronizedTemplatesCutter3D();
  ~vtkSynchronizedTemplatesCutter3D() VTK_OVERRIDE;

  vtkImplicitFunction *CutFunction;
  int OutputPointsPrecision;

  int RequestData(vtkInformation *,
                  vtkInformationVector **,
                  vtkInformationVector *) VTK_OVERRIDE;

private:
  vtkSynchronizedTemplatesCutter3D(const vtkSynchronizedTemplatesCutter3D&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSynchronizedTemplatesCutter3D&) VTK_DELETE_FUNCTION;
};

#endif

