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

#ifndef __vtkLabelHierarchyAlgorithm_h
#define __vtkLabelHierarchyAlgorithm_h

#include "vtkAlgorithm.h"

class vtkDataSet;
class vtkLabelHierarchy;

class VTK_RENDERING_EXPORT vtkLabelHierarchyAlgorithm : public vtkAlgorithm
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
  // Set an input of this algorithm. You should not override these
  // methods because they are not the only way to connect a pipeline.
  // Note that these methods support old-style pipeline connections.
  // When writing new code you should use the more general
  // vtkAlgorithm::SetInputConnection().  These methods transform the
  // input index to the input port index, not an index of a connection
  // within a single port.
  void SetInput( vtkDataObject* );
  void SetInput( int, vtkDataObject* );

  // Description:
  // Add an input of this algorithm.  Note that these methods support
  // old-style pipeline connections.  When writing new code you should
  // use the more general vtkAlgorithm::AddInputConnection().  See
  // SetInput() for details.
  void AddInput( vtkDataObject* );
  void AddInput( int, vtkDataObject* );

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
