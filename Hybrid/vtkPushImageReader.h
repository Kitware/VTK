/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPushImageReader.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPushImageReader - read image files, compatible with PushPipeline
// .SECTION Description
// vtkPushImageReader is a source object that reads Image files.
// It should be able to read most raw images
//
// .SECTION See Also
// vtkPushPipeline

#ifndef __vtkPushImageReader_h
#define __vtkPushImageReader_h

#include "vtkImageReader2.h"

class vtkPushPipeline;

class VTK_HYBRID_EXPORT vtkPushImageReader : public vtkImageReader2
{
public:
  static vtkPushImageReader *New();
  vtkTypeRevisionMacro(vtkPushImageReader,vtkImageReader2);

  // Description:
  // What is the current slice of this reader
  vtkGetMacro(CurrentSlice,int);
  vtkSetMacro(CurrentSlice,int);
  
  // Description:
  // Push data from this reader
  void Push();

  // Description:
  // Run the pipeline the reader is conected to until it is out of data
  void Run();

  // Description:
  // Set the push pipeline for this reader
  virtual void SetPushPipeline(vtkPushPipeline *);
  vtkGetObjectMacro(PushPipeline,vtkPushPipeline);
  
protected:
  vtkPushImageReader();
  ~vtkPushImageReader();

  vtkPushPipeline *PushPipeline;
  
  int CurrentSlice;
  virtual void ExecuteInformation();
  virtual void ExecuteData(vtkDataObject *out);
private:
  vtkPushImageReader(const vtkPushImageReader&);  // Not implemented.
  void operator=(const vtkPushImageReader&);  // Not implemented.
};
#endif


