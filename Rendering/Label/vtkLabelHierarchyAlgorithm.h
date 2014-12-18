/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLabelHierarchyAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkLabelHierarchyAlgorithm - Superclass for algorithms that produce only label hierarchies as output
// .SECTION Description
//
// vtkLabelHierarchyAlgorithm is a convenience class to make writing algorithms
// easier. It is also designed to help transition old algorithms to the new
// pipeline architecture. There are some assumptions and defaults made by this
// class you should be aware of. This class defaults such that your filter
// will have one input port and one output port. If that is not the case
// simply change it with SetNumberOfInputPorts etc. See this class
// constructor for the default. This class also provides a FillInputPortInfo
// method that by default says that all inputs will be DataObjects. If that
// isn't the case then please override this method in your subclass. This
// class breaks out the downstream requests into separate functions such as
// RequestData and RequestInformation.  You should
// implement RequestData( request, inputVec, outputVec) in subclasses.

#ifndef vtkLabelHierarchyAlgorithm_h
#define vtkLabelHierarchyAlgorithm_h

#include "vtkRenderingLabelModule.h" // For export macro
#include "vtkAlgorithm.h"

class vtkDataSet;
class vtkLabelHierarchy;

class VTKRENDERINGLABEL_EXPORT vtkLabelHierarchyAlgorithm : public vtkAlgorithm
{
public:
  static vtkLabelHierarchyAlgorithm *New();
  vtkTypeMacro(vtkLabelHierarchyAlgorithm,vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output data object for a port on this algorithm.
  vtkLabelHierarchy* GetOutput();
  vtkLabelHierarchy* GetOutput(int);
  virtual void SetOutput(vtkDataObject* d);

  // Description:
  // see vtkAlgorithm for details
  virtual int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*);

  // this method is not recommended for use, but lots of old style filters use it
  vtkDataObject* GetInput();
  vtkDataObject* GetInput(int port);
  vtkLabelHierarchy* GetLabelHierarchyInput(int port);

  // Description:
  // Assign a data object as input. Note that this method does not
  // establish a pipeline connection. Use SetInputConnection() to
  // setup a pipeline connection.
  void SetInputData( vtkDataObject* );
  void SetInputData( int, vtkDataObject* );

  // Description:
  // Assign a data object as input. Note that this method does not
  // establish a pipeline connection. Use AddInputConnection() to
  // setup a pipeline connection.
  void AddInputData( vtkDataObject* );
  void AddInputData( int, vtkDataObject* );

protected:
  vtkLabelHierarchyAlgorithm();
  ~vtkLabelHierarchyAlgorithm();

  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestDataObject(
    vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector );

  // convenience method
  virtual int RequestInformation(
    vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector );

  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestData(
    vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector );

  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestUpdateExtent(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector* );

  // see algorithm for more info
  virtual int FillOutputPortInformation( int port, vtkInformation* info );
  virtual int FillInputPortInformation( int port, vtkInformation* info );

private:
  vtkLabelHierarchyAlgorithm( const vtkLabelHierarchyAlgorithm& ); // Not implemented.
  void operator = ( const vtkLabelHierarchyAlgorithm& );  // Not implemented.
};

#endif
