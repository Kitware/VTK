/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThreadedSynchronizedTemplatesCutter3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkThreadedSynchronizedTemplatesCutter3D
 * @brief   generate cut surface from structured points
 *
 *
 * vtkThreadedSynchronizedTemplatesCutter3D is an implementation of the
 * synchronized template algorithm.
 *
 * @sa
 * vtkContourFilter vtkSynchronizedTemplates3D vtkThreadedSynchronizedTemplates3D vtkSynchronizedTemplatesCutter3D
*/

#ifndef vtkThreadedSynchronizedTemplatesCutter3D_h
#define vtkThreadedSynchronizedTemplatesCutter3D_h

#include "vtkFiltersSMPModule.h" // For export macro
#include "vtkThreadedSynchronizedTemplates3D.h"

class vtkImplicitFunction;

class VTKFILTERSSMP_EXPORT vtkThreadedSynchronizedTemplatesCutter3D : public vtkThreadedSynchronizedTemplates3D
{
public:
  static vtkThreadedSynchronizedTemplatesCutter3D *New();

  vtkTypeMacro(vtkThreadedSynchronizedTemplatesCutter3D,vtkThreadedSynchronizedTemplates3D);
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

  /**
   * Override GetMTime because we delegate to vtkContourValues and refer to
   * vtkImplicitFunction.
   */
  vtkMTimeType GetMTime() VTK_OVERRIDE;

protected:
  vtkThreadedSynchronizedTemplatesCutter3D();
  ~vtkThreadedSynchronizedTemplatesCutter3D() VTK_OVERRIDE;

  vtkImplicitFunction *CutFunction;
  int OutputPointsPrecision;

  int RequestData(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *) VTK_OVERRIDE;

  int FillOutputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;
private:
  vtkThreadedSynchronizedTemplatesCutter3D(const vtkThreadedSynchronizedTemplatesCutter3D&) VTK_DELETE_FUNCTION;
  void operator=(const vtkThreadedSynchronizedTemplatesCutter3D&) VTK_DELETE_FUNCTION;
};

#endif
