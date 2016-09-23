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
/**
 * @class   vtkImageAlgorithm
 * @brief   Generic algorithm superclass for image algs
 *
 * vtkImageAlgorithm is a filter superclass that hides much of the
 * pipeline  complexity. It handles breaking the pipeline execution
 * into smaller extents so that the vtkImageData limits are observed. It
 * also provides support for multithreading. If you don't need any of this
 * functionality, consider using vtkSimpleImageToImageFilter instead.
 * @sa
 * vtkSimpleImageToImageFilter
*/

#ifndef vtkImageAlgorithm_h
#define vtkImageAlgorithm_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkAlgorithm.h"

class vtkDataSet;
class vtkImageData;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkImageAlgorithm : public vtkAlgorithm
{
public:
  vtkTypeMacro(vtkImageAlgorithm,vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Get the output data object for a port on this algorithm.
   */
  vtkImageData* GetOutput();
  vtkImageData* GetOutput(int);
  virtual void SetOutput(vtkDataObject* d);
  //@}

  /**
   * Process a request from the executive.  For vtkImageAlgorithm, the
   * request will be delegated to one of the following methods: RequestData,
   * RequestInformation, or RequestUpdateExtent.
   */
  int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*) VTK_OVERRIDE;

  //@{
  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use SetInputConnection to
   * setup a pipeline connection.
   */
  void SetInputData(vtkDataObject *);
  void SetInputData(int, vtkDataObject*);
  //@}

  //@{
  /**
   * Get a data object for one of the input port connections.  The use
   * of this method is strongly discouraged, but some filters that were
   * written a long time ago still use this method.
   */
  vtkDataObject *GetInput(int port);
  vtkDataObject *GetInput() { return this->GetInput(0); };
  vtkImageData  *GetImageDataInput(int port);
  //@}

  //@{
  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use SetInputConnection to
   * setup a pipeline connection.
   */
  virtual void AddInputData(vtkDataObject *);
  virtual void AddInputData(int, vtkDataObject*);
  //@}

protected:
  vtkImageAlgorithm();
  ~vtkImageAlgorithm() VTK_OVERRIDE;

  /**
   * Subclasses can reimplement this method to collect information
   * from their inputs and set information for their outputs.
   */
  virtual int RequestInformation(vtkInformation* request,
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector);


  /**
   * Subclasses can reimplement this method to translate the update
   * extent requests from each output port into update extent requests
   * for the input connections.
   */
  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);

  /**
   * Convenience method to copy the scalar type and number of components
   * from the input data to the output data.  You will generally want to
   * call this from inside your RequestInformation method, unless you
   * want the output data to have a different scalar type or number of
   * components from the input.
   */
  virtual void CopyInputArrayAttributesToOutput(vtkInformation* request,
                                                vtkInformationVector** inputVector,
                                                vtkInformationVector* outputVector);


  /**
   * This is called in response to a REQUEST_DATA request from the
   * executive.  Subclasses should override either this method or the
   * ExecuteDataWithInformation method in order to generate data for
   * their outputs.  For images, the output arrays will already be
   * allocated, so all that is necessary is to fill in the voxel values.
   */
  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  /**
   * This is a convenience method that is implemented in many subclasses
   * instead of RequestData.  It is called by RequestData.
   */
  virtual void ExecuteDataWithInformation(vtkDataObject *output,
                                          vtkInformation* outInfo);

  //@{
  /**
   * This method is the old style execute method, provided for the sake
   * of backwards compatibility with older filters and readers.
   */
  virtual void ExecuteData(vtkDataObject *output);
  virtual void Execute();
  //@}

  //@{
  /**
   * Allocate the output data.  This will be called before RequestData,
   * it is not necessary for subclasses to call this method themselves.
   */
  virtual void AllocateOutputData(vtkImageData *out,
                                  vtkInformation* outInfo,
                                  int *uExtent);
  virtual vtkImageData *AllocateOutputData(vtkDataObject *out,
                                           vtkInformation *outInfo);
  //@}

  /**
   * Copy the other point and cell data.  Subclasses will almost never
   * need to reimplement this method.
   */
  virtual void CopyAttributeData(vtkImageData *in, vtkImageData *out,
                                 vtkInformationVector** inputVector);

  //@{
  /**
   * These method should be reimplemented by subclasses that have
   * more than a single input or single output.
   * See vtkAlgorithm for more information.
   */
  int FillOutputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;
  int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;
  //@}

private:
  vtkImageAlgorithm(const vtkImageAlgorithm&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageAlgorithm&) VTK_DELETE_FUNCTION;
};

#endif







