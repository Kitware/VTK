/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageAlgorithm - Generic algorithm superclass for image algs
// .SECTION Description
// vtkImageAlgorithm is a filter superclass that hides much of the
// pipeline  complexity. It handles breaking the pipeline execution
// into smaller extents so that the vtkImageData limits are observed. It
// also provides support for multithreading. If you don't need any of this
// functionality, consider using vtkSimpleImageToImageFilter instead.
// .SECTION See also
// vtkSimpleImageToImageFilter

#ifndef __vtkImageAlgorithm_h
#define __vtkImageAlgorithm_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkAlgorithm.h"

class vtkDataSet;
class vtkImageData;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkImageAlgorithm : public vtkAlgorithm
{
public:
  vtkTypeMacro(vtkImageAlgorithm,vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output data object for a port on this algorithm.
  vtkImageData* GetOutput();
  vtkImageData* GetOutput(int);
  virtual void SetOutput(vtkDataObject* d);

  // Description:
  // see vtkAlgorithm for details
  virtual int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*);

  // Description:
  // Assign a data object as input. Note that this method does not
  // establish a pipeline connection. Use SetInputConnection() to
  // setup a pipeline connection.
  void SetInputData(vtkDataObject *);
  void SetInputData(int, vtkDataObject*);

  // this method is not recommended for use, but lots of old style filters
  // use it
  vtkDataObject *GetInput(int port);
  vtkDataObject *GetInput() { return this->GetInput(0); };
  vtkImageData  *GetImageDataInput(int port);

  // Description:
  // Assign a data object as input. Note that this method does not
  // establish a pipeline connection. Use SetInputConnection() to
  // setup a pipeline connection.
  virtual void AddInputData(vtkDataObject *);
  virtual void AddInputData(int, vtkDataObject*);

protected:
  vtkImageAlgorithm();
  ~vtkImageAlgorithm();

  // convenience method
  virtual int RequestInformation(vtkInformation* request,
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector);
  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);

  // convenience method to copy the selected scalars type and num components
  // to the output info. Call this from inside your RequestInformation
  virtual void CopyInputArrayAttributesToOutput(vtkInformation* request,
                                                vtkInformationVector** inputVector,
                                                vtkInformationVector* outputVector);


  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  // Description:
  virtual void ExecuteDataWithInformation(vtkDataObject *output, vtkInformation* outInfo);
  // This method is the old style execute method
  virtual void ExecuteData(vtkDataObject *output);
  virtual void Execute();

  // just allocate the output data
  virtual void AllocateOutputData(vtkImageData *out,
                                  vtkInformation* outInfo,
                                  int *uExtent);
  virtual vtkImageData *AllocateOutputData(vtkDataObject *out,
                                           vtkInformation *outInfo);

  // copy the other point and cell data
  virtual void CopyAttributeData(vtkImageData *in, vtkImageData *out,
                                 vtkInformationVector** inputVector);

  // see algorithm for more info
  virtual int FillOutputPortInformation(int port, vtkInformation* info);
  virtual int FillInputPortInformation(int port, vtkInformation* info);

private:
  vtkImageAlgorithm(const vtkImageAlgorithm&);  // Not implemented.
  void operator=(const vtkImageAlgorithm&);  // Not implemented.
};

#endif







