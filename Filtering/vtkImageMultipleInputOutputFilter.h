/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMultipleInputOutputFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageMultipleInputOutputFilter - Generic filter that has N inputs.
// .SECTION Description
// vtkImageMultipleInputOutputFilter is a super class for filters that 
// have any number of inputs. Streaming is not available in this class yet.

// .SECTION See Also
// vtkImageToImageFilter vtkImageInPlaceFilter vtkImageTwoInputFilter



#ifndef __vtkImageMultipleInputOutputFilter_h
#define __vtkImageMultipleInputOutputFilter_h


#include "vtkImageMultipleInputFilter.h"


class VTK_FILTERING_EXPORT vtkImageMultipleInputOutputFilter : public vtkImageMultipleInputFilter
{
public:
  vtkTypeMacro(vtkImageMultipleInputOutputFilter,vtkImageMultipleInputFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get one input to this filter.
  vtkImageData *GetOutput(int num);
  vtkImageData *GetOutput();

  // Description:
  // The execute method created by the subclass.
  // This is kept public instead of protected since it is called
  // from a non-member thread function.
  virtual void ThreadedExecute(vtkImageData **inDatas, 
                               vtkImageData **outDatas,
                               int extent[6], int threadId);

protected:
  vtkImageMultipleInputOutputFilter();
  ~vtkImageMultipleInputOutputFilter();

  void ComputeInputUpdateExtents( vtkDataObject *output );
  
  virtual void ComputeInputUpdateExtent( int inExt[6], 
                                         int outExt[6], 
                                         int whichInput );


  void ExecuteData(vtkDataObject *out);

  // this should never be called
  virtual void ThreadedExecute(vtkImageData **inDatas, 
                               vtkImageData *outData,
                               int extent[6], int threadId);
  virtual void ExecuteInformation(vtkImageData **, vtkImageData *) {};

  // This one gets called by the superclass.
  void ExecuteInformation();
  // This is the one you should override.
  virtual void ExecuteInformation(vtkImageData **, vtkImageData **) {};
private:
  vtkImageMultipleInputOutputFilter(const vtkImageMultipleInputOutputFilter&);  // Not implemented.
  void operator=(const vtkImageMultipleInputOutputFilter&);  // Not implemented.
};

#endif







