/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataObjectAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDataObjectAlgorithm - Superclass for algorithms that produce only data object as output
// .SECTION Description

// vtkDataObjectAlgorithm is a convenience class to make writing algorithms
// easier. It is also designed to help transition old algorithms to the new
// pipeline architecture. Ther are some assumptions and defaults made by this
// class you should be aware of. This class defaults such that your filter
// will have one input port and one output port. If that is not the case
// simply change it with SetNumberOfInputPorts etc. See this classes
// constructor for the default. This class also provides a FillInputPortInfo
// method that by default says that all inputs will be DataObject. If that
// isn't the case then please override this method in your subclass. This
// class breaks out the downstream requests into seperate functions such as
// ExecuteData and ExecuteInformation.  For new algorithms you should
// implement RequestData( request, inputVec, outputVec) but for older filters
// there is a default implementation that calls the old ExecuteData(output)
// signature, for even older filters that don;t implement ExecuteData the
// default implementation calls the even older Execute() signature.

#ifndef __vtkDataObjectAlgorithm_h
#define __vtkDataObjectAlgorithm_h

#include "vtkAlgorithm.h"
#include "vtkDataObject.h" // makes things a bit easier

class vtkDataSet;
class vtkDataObject;

class VTK_FILTERING_EXPORT vtkDataObjectAlgorithm : public vtkAlgorithm
{
public:
  static vtkDataObjectAlgorithm *New();
  vtkTypeMacro(vtkDataObjectAlgorithm,vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output data object for a port on this algorithm.
  vtkDataObject* GetOutput();
  vtkDataObject* GetOutput(int);
  virtual void SetOutput(vtkDataObject* d);

  // Description:
  // see vtkAlgorithm for details
  virtual int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*);

  // this method is not recommended for use, but lots of old style filters
  // use it
  vtkDataObject* GetInput();
  vtkDataObject *GetInput(int port);

  // Description:
  // Set an input of this algorithm. You should not override these
  // methods because they are not the only way to connect a pipeline.
  // Note that these methods support old-style pipeline connections.
  // When writing new code you should use the more general
  // vtkAlgorithm::SetInputConnection().  These methods transform the
  // input index to the input port index, not an index of a connection
  // within a single port.
  void SetInput(vtkDataObject *);
  void SetInput(int, vtkDataObject*);

  // Description:
  // Add an input of this algorithm.  Note that these methods support
  // old-style pipeline connections.  When writing new code you should
  // use the more general vtkAlgorithm::AddInputConnection().  See
  // SetInput() for details.
  void AddInput(vtkDataObject *);
  void AddInput(int, vtkDataObject*);

protected:
  vtkDataObjectAlgorithm();
  ~vtkDataObjectAlgorithm();

  // convenience method
  virtual int RequestInformation(vtkInformation* request,
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector);

  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestDataObject(vtkInformation*,
                                vtkInformationVector**,
                                vtkInformationVector*)
  {
    return 1;
  }

  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*)
    {
      return 1;
    };

  // Description:
  // This detects when the UpdateExtent will generate no data.
  // This condition is satisfied when the UpdateExtent has 
  // zero volume (0,-1,...) or the UpdateNumberOfPieces is 0.
  // The source uses this call to determine whether to call Execute.
  int UpdateExtentIsEmpty(vtkDataObject *output);

  // Description:
  // This method is the old style execute method
  virtual void ExecuteData(vtkDataObject *output);
  virtual void Execute();

  // see algorithm for more info
  virtual int FillOutputPortInformation(int port, vtkInformation* info);
  virtual int FillInputPortInformation(int port, vtkInformation* info);

private:
  vtkDataObjectAlgorithm(const vtkDataObjectAlgorithm&);  // Not implemented.
  void operator=(const vtkDataObjectAlgorithm&);  // Not implemented.
};

#endif
