/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMultipleInputFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkImageMultipleInputFilter - Generic filter that has N inputs.
// .SECTION Description
// vtkImageMultipleInputFilter is a super class for filters that 
// have any number of inputs. Steaming is not available in this class yet.

// .SECTION See Also
// vtkImageToImageFilter vtImageInPlaceFilter vtkImageTwoInputFilter
// vtkImageTwoOutputFilter



#ifndef __vtkImageMultipleInputFilter_h
#define __vtkImageMultipleInputFilter_h


#include "vtkImageSource.h"
#include "vtkMultiThreader.h"


class VTK_EXPORT vtkImageMultipleInputFilter : public vtkImageSource
{
public:
  static vtkImageMultipleInputFilter *New() 
    {return new vtkImageMultipleInputFilter;};
  const char *GetClassName() {return "vtkImageMultipleInputFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set an Input of this filter. 
  virtual void SetInput(int num, vtkImageData *input);

  // Description:
  // Adds an input to the first null position in the input list.
  // Expands the list memory if necessary
  virtual void AddInput(vtkImageData *input);
  
  // Description:
  // Get one input to this filter.
  vtkImageData *GetInput(int num);
  vtkImageData *GetInput();

  // Description:
  // Turning bypass on will cause the filter to turn off and
  // simply pass the data from the first input (input0) through.  
  // It is implemented for consistancy with vtkImageToImageFilter.
  vtkSetMacro(Bypass,int);
  vtkGetMacro(Bypass,int);
  vtkBooleanMacro(Bypass,int);

  // Description:
  // Get/Set the number of threads to create when rendering
  vtkSetClampMacro( NumberOfThreads, int, 1, VTK_MAX_THREADS );
  vtkGetMacro( NumberOfThreads, int );

  // Description:
  // The execute method created by the subclass.
  virtual void ThreadedExecute(vtkImageData **inDatas, 
			       vtkImageData *outData,
			       int extent[6], int threadId);

  // Description:
  // Putting this here until I merge graphics and imaging streaming.
  virtual int SplitExtent(int splitExt[6], int startExt[6], 
			  int num, int total);

protected:
  vtkImageMultipleInputFilter();
  ~vtkImageMultipleInputFilter();
  vtkImageMultipleInputFilter(const vtkImageMultipleInputFilter&) {};
  void operator=(const vtkImageMultipleInputFilter&) {};

  vtkMultiThreader *Threader;
  int Bypass;
  int NumberOfThreads;
  
  int ComputeDivisionExtents(vtkDataObject *out,
			     int division, int numDivisions);
  virtual void ComputeRequiredInputUpdateExtent(int inExt[6], int outExt[6],
					int whichInput);

  void Execute();
  virtual void Execute(vtkImageData **inDatas, vtkImageData *outData);

  // This one gets called by the superclass.
  void ExecuteInformation();
  // This is the one you should override.
  virtual void ExecuteInformation(vtkImageData **inDatas, 
				  vtkImageData *outData) {};

  // legacy  !!!!! ------------------------
  virtual void ExecuteImageInformation() {this->LegacyHack = 0;}
  int LegacyHack;
  
};

#endif







