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
// .NAME vtkThreadedSynchronizedTemplatesCutter3D - generate cut surface from structured points

// .SECTION Description
// vtkThreadedSynchronizedTemplatesCutter3D is an implementation of the
// synchronized template algorithm.

// .SECTION See Also
// vtkContourFilter vtkSynchronizedTemplates3D vtkThreadedSynchronizedTemplates3D vtkSynchronizedTemplatesCutter3D

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
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Needed by templated functions.
  void ThreadedExecute(vtkImageData *data, vtkInformation *outInfo, int);

  // Description
  // Specify the implicit function to perform the cutting.
  virtual void SetCutFunction(vtkImplicitFunction*);
  vtkGetObjectMacro(CutFunction,vtkImplicitFunction);

  // Description:
  // Set/get the desired precision for the output types. See the documentation
  // for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
  // the available precision settings.
  vtkSetClampMacro(OutputPointsPrecision, int, SINGLE_PRECISION, DEFAULT_PRECISION);
  vtkGetMacro(OutputPointsPrecision, int);

  // Description:
  // Override GetMTime because we delegate to vtkContourValues and refer to
  // vtkImplicitFunction.
  unsigned long GetMTime();

protected:
  vtkThreadedSynchronizedTemplatesCutter3D();
  ~vtkThreadedSynchronizedTemplatesCutter3D();

  vtkImplicitFunction *CutFunction;
  int OutputPointsPrecision;

  virtual int RequestData(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *);

  virtual int FillOutputPortInformation(int port, vtkInformation* info);
private:
  vtkThreadedSynchronizedTemplatesCutter3D(const vtkThreadedSynchronizedTemplatesCutter3D&);  // Not implemented.
  void operator=(const vtkThreadedSynchronizedTemplatesCutter3D&);  // Not implemented.
};

#endif
