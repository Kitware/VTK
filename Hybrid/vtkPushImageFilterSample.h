/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPushImageFilterSample.h
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
// .NAME vtkPushImageFilterSample - example of a PushImageFilter
// .SECTION Description
// vtkPushImageFilterSample an example of a complex filter using the
// PushPipeline.

#ifndef __vtkPushImageFilterSample_h
#define __vtkPushImageFilterSample_h

#include "vtkImageTwoInputFilter.h"

class VTK_HYBRID_EXPORT vtkPushImageFilterSample : public vtkImageTwoInputFilter
{
public:
  static vtkPushImageFilterSample *New();
  vtkTypeRevisionMacro(vtkPushImageFilterSample,vtkImageTwoInputFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkPushImageFilterSample();
  ~vtkPushImageFilterSample() {};

  void ExecuteInformation(vtkImageData **inDatas, vtkImageData *outData);
  void ExecuteInformation(){this->vtkImageTwoInputFilter::ExecuteInformation();};
  void ThreadedExecute(vtkImageData **inDatas, vtkImageData *outData,
                       int extent[6], int id);
private:
  vtkPushImageFilterSample(const vtkPushImageFilterSample&);  // Not implemented.
  void operator=(const vtkPushImageFilterSample&);  // Not implemented.
};

#endif



