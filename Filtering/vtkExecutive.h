/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExecutive.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExecutive - Superclass for all pipeline executives in VTK.
// .SECTION Description
// vtkExecutive is the superclass for all pipeline executives in VTK.
// A VTK executive is responsible for controlling one instance of
// vtkAlgorithm.  A pipeline consists of one or more executives that
// control data flow.  Every reader, source, writer, or data
// processing algorithm in the pipeline is implemented in an instance
// of vtkAlgorithm.

#ifndef __vtkExecutive_h
#define __vtkExecutive_h

#include "vtkObject.h"

class vtkAlgorithm;
class vtkAlgorithmOutput;
class vtkAlgorithmToExecutiveFriendship;
class vtkDataObject;
class vtkInformation;
class vtkInformationExecutiveKey;
class vtkInformationIntegerKey;

class VTK_FILTERING_EXPORT vtkExecutive : public vtkObject
{
public:
  static vtkExecutive* New();
  vtkTypeRevisionMacro(vtkExecutive,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the algorithm to which this executive has been assigned.
  vtkAlgorithm* GetAlgorithm();

  // Description:
  // Bring the algorithm's outputs up-to-date.  Returns 1 for success
  // and 0 for failure.
  virtual int Update();
  virtual int Update(int port);

  // Description:
  // Get the information object for an output port of the algorithm.
  virtual vtkInformation* GetOutputInformation(int port);

  // Description:
  // Get/Set the data object for an output port of the algorithm.
  virtual vtkDataObject* GetOutputData(int port);
  virtual void SetOutputData(int port, vtkDataObject*);

  // Description:
  // Get the data object for an output port of the algorithm.
  virtual vtkDataObject* GetInputData(int port, int connection);


  // Description:
  // Get the output port that produces the given data object.
  virtual vtkAlgorithmOutput* GetProducerPort(vtkDataObject*);

  // Description:
  // Decrement the count of references to this object and participate
  // in garbage collection.
  virtual void UnRegister(vtkObjectBase* o);

  // Description:
  // Information key to store a pointer to an executive in an
  // information object.
  static vtkInformationExecutiveKey* EXECUTIVE();

  // Description:
  // Information key to store a port number in an information object.
  static vtkInformationIntegerKey* PORT_NUMBER();

protected:
  vtkExecutive();
  ~vtkExecutive();

  // Helper methods for subclasses.
  int InputPortIndexInRange(int port, const char* action);
  int OutputPortIndexInRange(int port, const char* action);

  // Garbage collection support.
  virtual void GarbageCollectionStarting();
  int GarbageCollecting;
  virtual void ReportReferences(vtkGarbageCollector*);
  virtual void RemoveReferences();

  virtual void SetAlgorithm(vtkAlgorithm* algorithm);

  // The algorithm managed by this executive.
  vtkAlgorithm* Algorithm;

  //BTX
  friend class vtkAlgorithmToExecutiveFriendship;
  //ETX
private:
  vtkExecutive(const vtkExecutive&);  // Not implemented.
  void operator=(const vtkExecutive&);  // Not implemented.
};

#endif
