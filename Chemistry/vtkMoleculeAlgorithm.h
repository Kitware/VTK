/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMoleculeAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMoleculeAlgorithm - Superclass for algorithms that operate on
// vtkMolecules

// .SECTION Description

// vtkMoleculeAlgorithm is a convenience class to make writing algorithms
// easier. There are some assumptions and defaults made by this class you
// should be aware of. This class defaults such that your filter will have
// one input port and one output port. If that is not the case simply change
// it with SetNumberOfInputPorts etc. See this class constructor for the
// default. This class also provides a FillInputPortInfo method that by
// default says that all inputs will be vtkMolecules. If that isn't the case
// then please override this method in your subclass. You should implement
// the subclass's algorithm into RequestData( request, inputVec, outputVec).

#ifndef __vtkMoleculeAlgorithm_h
#define __vtkMoleculeAlgorithm_h

#include "vtkAlgorithm.h"

class vtkDataSet;
class vtkMolecule;

class VTK_CHEMISTRY_EXPORT vtkMoleculeAlgorithm : public vtkAlgorithm
{
public:
  static vtkMoleculeAlgorithm *New();
  vtkTypeMacro(vtkMoleculeAlgorithm,vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output data object for a port on this algorithm.
  vtkMolecule* GetOutput();
  vtkMolecule* GetOutput(int);
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
  vtkMolecule *GetMoleculeInput(int port);

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
  vtkMoleculeAlgorithm();
  ~vtkMoleculeAlgorithm();

  // convenience method
  virtual int RequestInformation(vtkInformation* request,
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector);

  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);

  // Description:
  // This method is the old style execute method
  virtual void ExecuteData(vtkDataObject *output);
  virtual void Execute();

  // see algorithm for more info
  virtual int FillOutputPortInformation(int port, vtkInformation* info);
  virtual int FillInputPortInformation(int port, vtkInformation* info);

private:
  vtkMoleculeAlgorithm(const vtkMoleculeAlgorithm&);  // Not implemented.
  void operator=(const vtkMoleculeAlgorithm&);  // Not implemented.
};

#endif
