/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperOctreeAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHyperOctreeAlgorithm - Superclass for algorithms that produce only octree as output
// .SECTION Description

// vtkOctreeAlgorithm is a convenience class to make writing algorithms
// easier. It is also designed to help transition old algorithms to the new
// pipeline architecture. Ther are some assumptions and defaults made by this
// class you should be aware of. This class defaults such that your filter
// will have one input port and one output port. If that is not the case
// simply change it with SetNumberOfInputPorts etc. See this classes
// constructor for the default. This class also provides a FillInputPortInfo
// method that by default says that all inputs will be RectilinearGrid. If that
// isn't the case then please override this method in your subclass. This
// class breaks out the downstream requests into seperate functions such as
// ExecuteData and ExecuteInformation.  For new algorithms you should
// implement RequestData( request, inputVec, outputVec) but for older filters
// there is a default implementation that calls the old ExecuteData(output)
// signature, for even older filters that don;t implement ExecuteData the
// default implementation calls the even older Execute() signature.

#ifndef __vtkHyperOctreeAlgorithm_h
#define __vtkHyperOctreeAlgorithm_h

#include "vtkAlgorithm.h"
#include "vtkHyperOctree.h" // makes things a bit easier

class vtkDataSet;
class vtkHyperOctree;

class VTK_FILTERING_EXPORT vtkHyperOctreeAlgorithm : public vtkAlgorithm
{
public:
  vtkTypeMacro(vtkHyperOctreeAlgorithm,vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output data object for a port on this algorithm.
  vtkHyperOctree *GetOutput();
  vtkHyperOctree *GetOutput(int);
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
  vtkHyperOctree *GetHyperOctreeInput(int port);

  // Description:
  // Set an input of this algorithm.
  void SetInput(vtkDataObject *);
  void SetInput(int, vtkDataObject*);

  // Description:
  // Add an input of this algorithm.
  void AddInput(vtkDataObject *);
  void AddInput(int, vtkDataObject*);

protected:
  vtkHyperOctreeAlgorithm();
  ~vtkHyperOctreeAlgorithm();

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
  vtkHyperOctreeAlgorithm(const vtkHyperOctreeAlgorithm&);  // Not implemented.
  void operator=(const vtkHyperOctreeAlgorithm&);  // Not implemented.
};

#endif
