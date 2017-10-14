/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAnnotationLayersAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkAnnotationLayersAlgorithm
 * @brief   Superclass for algorithms that produce only vtkAnnotationLayers as output
 *
 *
 * vtkAnnotationLayersAlgorithm is a convenience class to make writing algorithms
 * easier. It is also designed to help transition old algorithms to the new
 * pipeline architecture. There are some assumptions and defaults made by this
 * class you should be aware of. This class defaults such that your filter
 * will have one input port and one output port. If that is not the case
 * simply change it with SetNumberOfInputPorts etc. See this class
 * constructor for the default. This class also provides a FillInputPortInfo
 * method that by default says that all inputs will be vtkAnnotationLayers. If that
 * isn't the case then please override this method in your subclass.
 * You should implement the subclass's algorithm into
 * RequestData( request, inputVec, outputVec).
*/

#ifndef vtkAnnotationLayersAlgorithm_h
#define vtkAnnotationLayersAlgorithm_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkAlgorithm.h"
#include "vtkAnnotationLayers.h" // makes things a bit easier

class vtkDataSet;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkAnnotationLayersAlgorithm : public vtkAlgorithm
{
public:
  static vtkAnnotationLayersAlgorithm *New();
  vtkTypeMacro(vtkAnnotationLayersAlgorithm,vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * see vtkAlgorithm for details
   */
  int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*) override;

  /**
   * Get the output data object for a port on this algorithm.
   */
  vtkAnnotationLayers* GetOutput() { return this->GetOutput(0); }
  vtkAnnotationLayers* GetOutput(int index);

  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use SetInputConnection() to
   * setup a pipeline connection.
   */
  void SetInputData(vtkDataObject * obj) { this->SetInputData(0, obj); }
  void SetInputData(int index, vtkDataObject* obj);

protected:
  vtkAnnotationLayersAlgorithm();
  ~vtkAnnotationLayersAlgorithm() override;

  // convenience method
  virtual int RequestInformation(vtkInformation* request,
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector);

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);

  // see algorithm for more info
  int FillOutputPortInformation(int port, vtkInformation* info) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkAnnotationLayersAlgorithm(const vtkAnnotationLayersAlgorithm&) = delete;
  void operator=(const vtkAnnotationLayersAlgorithm&) = delete;
};

#endif
