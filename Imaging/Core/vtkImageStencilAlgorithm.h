/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageStencilAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageStencilAlgorithm - producer of vtkImageStencilData
// .SECTION Description
// vtkImageStencilAlgorithm is a superclass for filters that generate
// the special vtkImageStencilData type.  This data type is a special
// representation of a binary image that can be used as a mask by
// several imaging filters.
// .SECTION see also
// vtkImageStencilData vtkImageStencilSource

#ifndef vtkImageStencilAlgorithm_h
#define vtkImageStencilAlgorithm_h


#include "vtkImagingCoreModule.h" // For export macro
#include "vtkAlgorithm.h"

class vtkImageStencilData;

class VTKIMAGINGCORE_EXPORT vtkImageStencilAlgorithm : public vtkAlgorithm
{
public:
  static vtkImageStencilAlgorithm *New();
  vtkTypeMacro(vtkImageStencilAlgorithm, vtkAlgorithm);

  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get or set the output for this source.
  void SetOutput(vtkImageStencilData *output);
  vtkImageStencilData *GetOutput();

  // Description:
  // see vtkAlgorithm for details
  virtual int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*);

protected:
  vtkImageStencilAlgorithm();
  ~vtkImageStencilAlgorithm();

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *);
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **,
                                 vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
                                  vtkInformationVector *);
  vtkImageStencilData *AllocateOutputData(vtkDataObject *out, int* updateExt);

  virtual int FillOutputPortInformation(int, vtkInformation*);

private:
  vtkImageStencilAlgorithm(const vtkImageStencilAlgorithm&);  // Not implemented.
  void operator=(const vtkImageStencilAlgorithm&);  // Not implemented.
};

#endif
