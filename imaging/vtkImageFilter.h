/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageFilter.h
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
// .NAME vtkImageFilter - Generic filter that has one input..
// .SECTION Description
// vtkImageFilter is a filter class that can dynamically split up a task
// if a region is too large.  This functionality is hidden
// in this superclass so filter writers never need to worry about this
// feature.  The filter designer can supply the functions:
//   1: UpdateImageInformation: Given the ImageExtent of the input, this function
// returns the ImageExtent of the output.
//   2: GetRequiredRegionExtent:  Returns the offset and size of the input
// region required to generate the given output region.
//   3: Execute3d: Given an input region and an output region, this method
// fills in the output using the input.
//   4: Execute2d: Same as Execute3d, but the method assumes the regions
// have a single image.
//   5: Execute1d: Same as Execute3d and Execute2d, but the method assumes
// the regions are one dimensional lines.
//   If this structure does not suit the filter designer, the
// UpdateRegion method can be overwritten, and the subclass can get
// its input regions on its own.  If an input generate fails,  the filter
// can split the task its self, or simple fail the whole generate leaving
// the task of splitting the problem to  the consumer.  This method has
// to deal with the full dimensionality of the data.



#ifndef __vtkImageFilter_h
#define __vtkImageFilter_h


#include "vtkImageCachedSource.h"
#include "vtkImageRegion.h"

class vtkImageFilter : public vtkImageCachedSource
{
public:
  vtkImageFilter();
  char *GetClassName() {return "vtkImageFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void SetInput(vtkImageSource *input);
  void UpdateRegion(vtkImageRegion *outRegion);
  void UpdateImageInformation(vtkImageRegion *region);
  unsigned long int GetPipelineMTime();
  
  vtkSetMacro(UseExecuteMethod,int);
  vtkGetMacro(UseExecuteMethod,int);
  vtkBooleanMacro(UseExecuteMethod,int);
  
  // Description:
  // Get input to this filter.
  vtkGetObjectMacro(Input,vtkImageSource);

protected:
  vtkImageSource *Input;     // the input to the filter
  int UseExecuteMethod;      // Use UpdateRegion or Execute method?

  // Description:
  // These are conveniance functions for writing filters that have their
  // own UpdateRegion methods.  They create the region object as well as 
  // getting the input source to fill it with data.  The extent
  // of the unspecified dimensions default to [0, 0];
  // Used in vtkImageScatterPlot.
  vtkImageRegion *GetInputRegion(int *extent, int dim);    
  vtkImageRegion *GetInputRegion5d(int extent[10])
    {return this->GetInputRegion(extent, 5);};
  vtkImageRegion *GetInputRegion4d(int extent[8])
    {return this->GetInputRegion(extent, 4);};
  vtkImageRegion *GetInputRegion3d(int extent[6])
    {return this->GetInputRegion(extent, 3);};
  vtkImageRegion *GetInputRegion2d(int extent[4])
    {return this->GetInputRegion(extent, 2);};
  vtkImageRegion *GetInputRegion1d(int extent[2])
    {return this->GetInputRegion(extent, 1);};
  
  void UpdateRegionTiled(vtkImageRegion *outRegion);
  virtual void SplitRegion(vtkImageRegion *region, int *pieceSize);
  virtual void ComputeOutputImageInformation(vtkImageRegion *inRegion,
					     vtkImageRegion *outRegion);
  virtual void ComputeRequiredInputRegionExtent(vtkImageRegion *outRegion,
						vtkImageRegion *inRegion);
  virtual void Execute5d(vtkImageRegion *inRegion, vtkImageRegion *outRegion);
  virtual void Execute4d(vtkImageRegion *inRegion, vtkImageRegion *outRegion);
  virtual void Execute3d(vtkImageRegion *inRegion, vtkImageRegion *outRegion);
  virtual void Execute2d(vtkImageRegion *inRegion, vtkImageRegion *outRegion);
  virtual void Execute1d(vtkImageRegion *inRegion,vtkImageRegion *outRegion);
};

#endif







