// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTemporalStatistics.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkStreamerBase - Superclass for filters that stream input pipeline
//
// .SECTION Description
// This class can be used as a superclass for filters that want to
// stream their input pipeline by making multiple execution passes.
// The subclass needs to set NumberOfPasses to > 1 before execution (
// usuall in the constructor or in RequestInformation) to initiate
// streaming. vtkStreamerBase will handle streaming while calling
// ExecutePass() during each pass. CurrentIndex can be used to obtain
// the index for the current pass. Finally, PostExecute() is called
// after the last pass and can be used to cleanup any internal data
// structures and create the actual output.


#ifndef _vtkStreamerBase_h
#define _vtkStreamerBase_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkAlgorithm.h"

class VTKFILTERSCORE_EXPORT vtkStreamerBase : public vtkAlgorithm
{
public:
  vtkTypeMacro(vtkStreamerBase, vtkAlgorithm);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // see vtkAlgorithm for details
  virtual int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*);

protected:
  vtkStreamerBase();
  ~vtkStreamerBase();

  virtual int RequestInformation(vtkInformation*,
                                 vtkInformationVector**,
                                 vtkInformationVector*)
  {
    return 1;
  }

  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*) = 0;

  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector);

  // This method is called during each execution pass. Subclasses
  // should implement this to do actual work.
  virtual int ExecutePass(vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector) = 0;

  // This method is called after streaming is completed. Subclasses
  // can override this method to perform cleanup.
  virtual int PostExecute(vtkInformationVector **,
                          vtkInformationVector *)
  {
    return 1;
  }

  unsigned int NumberOfPasses;
  unsigned int CurrentIndex;

private:
  vtkStreamerBase(const vtkStreamerBase &); // Not implemented.
  void operator=(const vtkStreamerBase &);        // Not implemented.

};

#endif //_vtkStreamerBase_h
