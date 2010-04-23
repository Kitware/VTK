/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDataSetAlgorithm - Superclass for algorithms that produce output of the same type as input
// .SECTION Description
// vtkDataSetAlgorithm is a convenience class to make writing algorithms
// easier. It is also designed to help transition old algorithms to the new
// pipeline architecture. Ther are some assumptions and defaults made by this
// class you should be aware of. This class defaults such that your filter
// will have one input port and one output port. If that is not the case
// simply change it with SetNumberOfInputPorts etc. See this classes
// contstructor for the default. This class also provides a FillInputPortInfo
// method that by default says that all inputs will be DataSet. If that isn't
// the case then please override this method in your subclass. This class
// breaks out the downstream requests into seperate functions such as
// RequestDataObject RequestData and RequestInformation. The default
// implementation of RequestDataObject will create an output data of the 
// same type as the input.


#ifndef __vtkDataSetAlgorithm_h
#define __vtkDataSetAlgorithm_h

#include "vtkAlgorithm.h"

class vtkDataSet;
class vtkImageData;
class vtkPolyData;
class vtkStructuredPoints;
class vtkStructuredGrid;
class vtkUnstructuredGrid;
class vtkRectilinearGrid;

class VTK_FILTERING_EXPORT vtkDataSetAlgorithm : public vtkAlgorithm
{
public:
  static vtkDataSetAlgorithm *New();
  vtkTypeMacro(vtkDataSetAlgorithm,vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output data object for a port on this algorithm.
  vtkDataSet* GetOutput();
  vtkDataSet* GetOutput(int);

  // Description:
  // Get the input data object. This method is not recommended for use, but
  // lots of old style filters use it.
  vtkDataObject* GetInput();
  
  // Description:
  // Get the output as vtkPolyData.
  vtkPolyData *GetPolyDataOutput();

  // Description:
  // Get the output as vtkStructuredPoints.
  vtkStructuredPoints *GetStructuredPointsOutput();

  // Description:
  // Get the output as vtkStructuredPoints.
  vtkImageData *GetImageDataOutput();

  // Description:
  // Get the output as vtkStructuredGrid.
  vtkStructuredGrid *GetStructuredGridOutput();

  // Description:
  // Get the output as vtkUnstructuredGrid.
  vtkUnstructuredGrid *GetUnstructuredGridOutput();

  // Description:
  // Get the output as vtkRectilinearGrid. 
  vtkRectilinearGrid *GetRectilinearGridOutput();

  // Description:
  // Set an input of this algorithm. You should not override these
  // methods because they are not the only way to connect a pipeline.
  // Note that these methods support old-style pipeline connections.
  // When writing new code you should use the more general
  // vtkAlgorithm::SetInputConnection().  These methods transform the
  // input index to the input port index, not an index of a connection
  // within a single port.
  void SetInput(vtkDataObject*);
  void SetInput(int, vtkDataObject*);
  void SetInput(vtkDataSet*);
  void SetInput(int, vtkDataSet*);

  // Description:
  // Add an input of this algorithm.  Note that these methods support
  // old-style pipeline connections.  When writing new code you should
  // use the more general vtkAlgorithm::AddInputConnection().  See
  // SetInput() for details.
  void AddInput(vtkDataObject *);
  void AddInput(vtkDataSet*);
  void AddInput(int, vtkDataSet*);
  void AddInput(int, vtkDataObject*);

  // Description:
  // see vtkAlgorithm for details
  virtual int ProcessRequest(vtkInformation* request, 
                             vtkInformationVector** inputVector,
                             vtkInformationVector* outputVector);

protected:
  vtkDataSetAlgorithm();
  ~vtkDataSetAlgorithm() {};

  // Description:
  // This is called within ProcessRequest when a request asks for 
  // Information. Typically an algorithm provides whatever lightweight 
  // information about its output that it can here without doing any 
  // lengthy computations. This happens in the first pass of the pipeline
  // execution.
  virtual int RequestInformation(vtkInformation*, 
                                 vtkInformationVector**, 
                                 vtkInformationVector*) {return 1;};
  
  // Description:
  // This is called within ProcessRequest when each filter in the pipeline
  // decides what portion of its input is needed to create the portion of its
  // output that the downstream filter asks for. This happens during the
  // second pass in the pipeline execution process.
  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*) 
    {
      return 1;
    };


  // Description:
  // This is called within ProcessRequest to when a request asks the
  // algorithm to create empty output data objects. This typically happens
  // early on in the execution of the pipeline. The default behavior is to 
  // create an output DataSet of the same type as the input for each 
  // output port. This method can be overridden to change the output 
  // data type of an algorithm. This happens in the third pass of the 
  // pipeline execution.
  virtual int RequestDataObject(vtkInformation* request, 
                                vtkInformationVector** inputVector, 
                                vtkInformationVector* outputVector);
  
  // Description:
  // This is called within ProcessRequest when a request asks the algorithm
  // to do its work. This is the method you should override to do whatever the
  // algorithm is designed to do. This happens during the fourth pass in the
  // pipeline execution process.
  virtual int RequestData(vtkInformation*, 
                          vtkInformationVector**, 
                          vtkInformationVector*) {return 1;};
  

  // see algorithm for more info
  virtual int FillOutputPortInformation(int port, vtkInformation* info);
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  vtkDataObject *GetInput(int port);

private:
  vtkDataSetAlgorithm(const vtkDataSetAlgorithm&);  // Not implemented.
  void operator=(const vtkDataSetAlgorithm&);  // Not implemented.
};

#endif


