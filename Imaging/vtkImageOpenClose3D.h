/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageOpenClose3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageOpenClose3D - Will perform opening or closing.
// .SECTION Description
// vtkImageOpenClose3D performs opening or closing by having two 
// vtkImageErodeDilates in series.  The size of operation
// is determined by the method SetKernelSize, and the operator is an ellipse.
// OpenValue and CloseValue determine how the filter behaves.  For binary
// images Opening and closing behaves as expected.
// Close value is first dilated, and then eroded.
// Open value is first eroded, and then dilated.
// Degenerate two dimensional opening/closing can be achieved by setting the
// one axis the 3D KernelSize to 1.
// Values other than open value and close value are not touched.
// This enables the filter to processes segmented images containing more than
// two tags.


#ifndef __vtkImageOpenClose3D_h
#define __vtkImageOpenClose3D_h


#include "vtkImageAlgorithm.h"

class vtkImageDilateErode3D;

class VTK_IMAGING_EXPORT vtkImageOpenClose3D : public vtkImageAlgorithm
{
public:
  // Description:
  // Default open value is 0, and default close value is 255.
  static vtkImageOpenClose3D *New();
  vtkTypeMacro(vtkImageOpenClose3D,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This method considers the sub filters MTimes when computing this objects
  // modified time.
  unsigned long int GetMTime();
  
  // Description:
  // Turn debugging output on. (in sub filters also)
  void DebugOn();
  void DebugOff();

  // Description:
  // Pass modified message to sub filters.
  void Modified();
  
  // Foward Source messages to filter1

  // Description:
  // Selects the size of gaps or objects removed.
  void SetKernelSize(int size0, int size1, int size2);

  // Description:
  // Determines the value that will opened.  
  // Open value is first eroded, and then dilated.
  void SetOpenValue(double value);
  double GetOpenValue();

  // Description:
  // Determines the value that will closed.
  // Close value is first dilated, and then eroded
  void SetCloseValue(double value);
  double GetCloseValue();
  
  // Description:
  // Needed for Progress functions
  vtkGetObjectMacro(Filter0, vtkImageDilateErode3D);
  vtkGetObjectMacro(Filter1, vtkImageDilateErode3D);

  // Description:
  // see vtkAlgorithm for details
  virtual int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*);

  // Description:
  // Override to send the request to internal pipeline.
  virtual int
  ComputePipelineMTime(vtkInformation* request,
                       vtkInformationVector** inInfoVec,
                       vtkInformationVector* outInfoVec,
                       int requestFromOutputPort,
                       unsigned long* mtime);

protected:
  vtkImageOpenClose3D();
  ~vtkImageOpenClose3D();
  
  vtkImageDilateErode3D *Filter0;
  vtkImageDilateErode3D *Filter1;

  virtual void ReportReferences(vtkGarbageCollector*);
private:
  vtkImageOpenClose3D(const vtkImageOpenClose3D&);  // Not implemented.
  void operator=(const vtkImageOpenClose3D&);  // Not implemented.
};

#endif



