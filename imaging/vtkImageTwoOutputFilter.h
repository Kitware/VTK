/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageTwoOutputFilter.h
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
// .NAME vtkImageTwoOutputFilter - Superclass of filters that have two outputs.
// .SECTION Description
// vtkImageTwoOutputFilter is a super class for filters that have one input
// and two outputs.  It still loops over the extra dimensions, but streaming
// has not been implemented for this superclass yet.  If you really need to 
// stream restrict (Input memory limit) a filter down stream.
// One restiction on multiple output filters is that the extent of all
// outputs have to be the same. (i.e. a filter can produce a small image
// on output number one, and a large image on output number two.
// This restriction is because the filter does not know which output
// a request is originating.  Also, the OutputScalarType must be the same
// for the two filters.  This is not an inherent limitation of the pipeline,
// but just an implementation descision.  I have also removed the ability
// to write your own update method.  Filters must be writen with
// Execute methods.


#ifndef __vtkImageTwoOutputFilter_h
#define __vtkImageTwoOutputFilter_h


#include "vtkImageSource.h"
#include "vtkStructuredPoints.h"
#include "vtkStructuredPointsToImage.h"
class vtkImageRegion;
class vtkImageCache;

class VTK_EXPORT vtkImageTwoOutputFilter : public vtkImageSource
{
public:
  vtkImageTwoOutputFilter();
  static vtkImageTwoOutputFilter *New() {return new vtkImageTwoOutputFilter;};
  const char *GetClassName() {return "vtkImageTwoOutputFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void SetInput(vtkImageCache *input);
  void SetInput(vtkStructuredPoints *spts)
    {this->SetInput(spts->GetStructuredPointsToImage()->GetOutput());}

  void Update();
  void UpdateImageInformation();
  unsigned long int GetPipelineMTime();
  
  // Description:
  // Get input to this filter.
  vtkGetObjectMacro(Input,vtkImageCache);

  // Description:
  // Get the two outputs.
  vtkImageCache *GetOutput1(){return this->GetOutput();}
  vtkImageCache *GetOutput2();

  // Description:
  // Filtered axes specify the axes which will be operated on.
  vtkGetMacro(NumberOfFilteredAxes, int);

  // Description:
  // Turning bypass on will cause the filter to turn off and
  // simply pass the data through.  This functionality is implemented
  // for consistancey with vtkImageFilter.
  vtkSetMacro(Bypass,int);
  vtkGetMacro(Bypass,int);
  vtkBooleanMacro(Bypass,int);
  
protected:
  int FilteredAxes[4];
  int NumberOfFilteredAxes;
  vtkImageCache *Input;     
  // Ouput1 is the same as Output
  vtkImageCache *Output2;
  int Bypass;
  int Updating;
  
  void SetReleaseDataFlag(int value);
  
  virtual void SetFilteredAxes(int num, int *axes);
  virtual void ExecuteImageInformation();
  virtual void ComputeRequiredInputUpdateExtent();
  
  virtual void RecursiveLoopExecute(int dim, vtkImageRegion *inRegion, 
				    vtkImageRegion *outRegion1, 
				    vtkImageRegion *outRegion2);
  virtual void Execute(vtkImageRegion *inRegion, 
		       vtkImageRegion *outRegion1, 
		       vtkImageRegion *outRegion2);

  virtual void CheckCache2();
};

#endif







