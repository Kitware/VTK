/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageTwoInputFilter.h
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
// .NAME vtkImageTwoInputFilter - Generic superclass for filter that have
// two inputs.
// .SECTION Description
// vtkImageTwoInputFilter handles two input.  It can loop over extra axes,
// but does not support an input memory limit for streaming.  If bypass
// is on,  the data from the first input (input0) is passed along.
// The extents required from the inputs, do not have to be the same 
// (see vtkImageTwoOutputFilter).

#ifndef __vtkImageTwoInputFilter_h
#define __vtkImageTwoInputFilter_h


#include "vtkImageSource.h"
#include "vtkStructuredPointsToImage.h"
#include "vtkStructuredPoints.h"
class vtkImageRegion;
class vtkImageCache;


class VTK_EXPORT vtkImageTwoInputFilter : public vtkImageSource
{
public:
  vtkImageTwoInputFilter();
  static vtkImageTwoInputFilter *New() {return new vtkImageTwoInputFilter;};
  const char *GetClassName() {return "vtkImageTwoInputFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void SetInput1(vtkImageCache *input);
  void SetInput1(vtkStructuredPoints *spts)
    {this->SetInput1(spts->GetStructuredPointsToImage()->GetOutput());}
  virtual void SetInput2(vtkImageCache *input);
  void SetInput2(vtkStructuredPoints *spts)
    {this->SetInput2(spts->GetStructuredPointsToImage()->GetOutput());}

  void Update();
  void UpdateImageInformation();
  unsigned long int GetPipelineMTime();
  
  // Description:
  // Get input to this filter.
  vtkGetObjectMacro(Input1,vtkImageCache);
  vtkGetObjectMacro(Input2,vtkImageCache);

  // Description:
  // Turning bypass on will causse the filter to turn off and
  // simply pass the data from the first input (input0) through.  
  // It is implemented for consitancy with vtkImageFilter.
  vtkSetMacro(Bypass,int);
  vtkGetMacro(Bypass,int);
  vtkBooleanMacro(Bypass,int);

  // Description:
  // Filtered axes specify the axes which will be operated on.
  vtkGetMacro(NumberOfFilteredAxes, int);

protected:
  int FilteredAxes[4];
  int NumberOfFilteredAxes;
  vtkImageCache *Input1;     // One of the inputs to the filter
  vtkImageCache *Input2;     // One of the inputs to the filter
  int Bypass;
  int Updating;

  virtual void SetFilteredAxes(int num, int *axes);
  virtual void ExecuteImageInformation(vtkImageCache *in1, vtkImageCache *in2,
				       vtkImageCache *out);
  virtual void ComputeRequiredInputUpdateExtent(vtkImageCache *out,
						vtkImageCache *in1,
						vtkImageCache *in2);
  virtual void RecursiveLoopExecute(int dim, vtkImageRegion *inRegion1,
		       vtkImageRegion *inRegion2, vtkImageRegion *outRegion);
  virtual void Execute(vtkImageRegion *inRegion1, vtkImageRegion *inRegion2, 
		       vtkImageRegion *outRegion);
  
  
};

#endif







