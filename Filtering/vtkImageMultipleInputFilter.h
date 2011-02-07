/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMultipleInputFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageMultipleInputFilter - Generic filter that has N inputs.
// .SECTION Description
// vtkImageMultipleInputFilter is a super class for filters that
// have any number of inputs. Streaming is not available in this class yet.

// .SECTION See Also
// vtkImageToImageFilter vtkImageInPlaceFilter vtkImageTwoInputFilter



#ifndef __vtkImageMultipleInputFilter_h
#define __vtkImageMultipleInputFilter_h

#include "vtkImageSource.h"

class vtkMultiThreader;

class VTK_FILTERING_EXPORT vtkImageMultipleInputFilter : public vtkImageSource
{
public:
  vtkTypeMacro(vtkImageMultipleInputFilter,vtkImageSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set an Input of this filter.
  virtual void SetInput(int num, vtkImageData *input);

  // Description:
  // Adds an input to the first null position in the input list.
  // Expands the list memory if necessary
  virtual void AddInput(vtkImageData *input);
  virtual void RemoveInput(vtkImageData *input);

  // Description:
  // Get one input to this filter.
  vtkImageData *GetInput(int num);
  vtkImageData *GetInput();

  // Description:
  // Turning bypass on will cause the filter to turn off and
  // simply pass the data from the first input (input0) through.
  // It is implemented for consistency with vtkImageToImageFilter.
  vtkSetMacro(Bypass,int);
  vtkGetMacro(Bypass,int);
  vtkBooleanMacro(Bypass,int);

  // Description:
  // Get/Set the number of threads to create when rendering
  vtkSetClampMacro( NumberOfThreads, int, 1, VTK_MAX_THREADS );
  vtkGetMacro( NumberOfThreads, int );

  // Description:
  // Putting this here until I merge graphics and imaging streaming.
  virtual int SplitExtent(int splitExt[6], int startExt[6],
                          int num, int total);

  // Description:
  // The execute method created by the subclass.
  // This is kept public instead of protected since it is called
  // from a non-member thread function.
  virtual void ThreadedExecute(vtkImageData **inDatas,
                               vtkImageData *outData,
                               int extent[6], int threadId);



protected:
  vtkImageMultipleInputFilter();
  ~vtkImageMultipleInputFilter();

  vtkMultiThreader *Threader;
  int Bypass;
  int NumberOfThreads;

  void ComputeInputUpdateExtents( vtkDataObject *output );

  virtual void ComputeInputUpdateExtent( int inExt[6],
                                         int outExt[6],
                                         int whichInput );


  void ExecuteData(vtkDataObject *output);
  void MultiThread(vtkImageData **indatas, vtkImageData *outdata);

  // This one gets called by the superclass.
  void ExecuteInformation();
  // This is the one you should override.
  virtual void ExecuteInformation(vtkImageData **, vtkImageData *) {};

  virtual int FillInputPortInformation(int, vtkInformation*);

private:
  // hide the superclass' AddInput() from the user and the compiler
  void AddInput(vtkDataObject *)
    { vtkErrorMacro( << "AddInput() must be called with a vtkImageData not a vtkDataObject."); };
  void RemoveInput(vtkDataObject *)
    { vtkErrorMacro( << "RemoveInput() must be called with a vtkImageData not a vtkDataObject."); };
private:
  vtkImageMultipleInputFilter(const vtkImageMultipleInputFilter&);  // Not implemented.
  void operator=(const vtkImageMultipleInputFilter&);  // Not implemented.
};

#endif







