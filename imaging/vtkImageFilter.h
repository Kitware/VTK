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
// vtkImageFilter is a filter class that can hide som of the pipeline 
// complexity.  This super class will loop over extra dimensions so the
// subclass can deal with simple low dimensional regions.
// The subclass can implement a filer in two different ways.  It can
// create an "Update(vtkImageRegion *out)" method, and get the input
// region itself. Or, it can create an "Execute(vtkImageRegion *in, *out)"
// and let the superclass get the input.  The execute method requires the
// UseExecuteMethod is on and also requires some helper method.  
// "ComputeRequiredInputExtent(vtkImageRegion *out, *in)" must set the
// extent of in required to compute out. 
// The advantage of using the execute method is that this super class
// will automatically break the execution into pieces if the 
// InputMemroyLimit is violated or the input request fails.
// This creates streaming where the pipeline processes images
// in dynamically sized pieces.



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
  void UpdatePointData(int dim, vtkImageRegion *outRegion);
  void UpdateImageInformation(vtkImageRegion *region);
  unsigned long int GetPipelineMTime();
  
  // Description:
  // Get input to this filter.
  vtkGetObjectMacro(Input,vtkImageSource);

  // Description:
  // Set/Get input memory limit.  Make this smaller to stream.
  vtkSetMacro(InputMemoryLimit,long);
  vtkGetMacro(InputMemoryLimit,long);

  // Description:
  // This inserts an axis at a specific point.  This is used for the filters
  // that want to put the component axis at a specific location.
  void InsertAxis(int num, int axis);
  
  // Description:
  // Set/Get the order to split region requests if we need to stream
  void SetSplitOrder(int num, int *axes);
  vtkImageSetMacro(SplitOrder,int);
  void GetSplitOrder(int num, int *axes);
  vtkImageGetMacro(SplitOrder,int);
  int *GetSplitOrder() {return this->SplitOrder;};

  // Description:
  // The basic operation is on regions with this dimensionality.  
  // Filters that operate on pixels would have dimensionality 0.
  // 2D Image filters would have dimensionality 2.
  vtkGetMacro(Dimensionality, int);
  
protected:
  vtkImageSource *Input;     
  
  int Dimensionality;
  // This is set in the subclass constructor and (probably) should not be 
  // changed.  It indicates the dimensionality of the regions the
  // execute/update methods expect.  It may be larger than dimensionality
  // to make the filter faster (this supper class is not efficient at
  // looping over extra dimensions.
  int ExecuteDimensionality;
  
  
  // Flag toggles between use of execute and update methods.
  int UseExecuteMethod; 
  int SplitOrder[VTK_IMAGE_DIMENSIONS];
  int NumberOfSplitAxes;
  long InputMemoryLimit;

  // Description:
  // Specify whether if subclass will use an execute method or an update method
  vtkSetMacro(UseExecuteMethod,int);
  vtkGetMacro(UseExecuteMethod,int);
  vtkBooleanMacro(UseExecuteMethod,int);
   
  // Description:
  // These are conveniance functions for writing filters that have their
  // own UpdateRegion methods.  They create the region object as well as 
  // getting the input source to fill it with data.  The extent
  // of the unspecified dimensions default to [0, 0];
  // Used in vtkImageScatterPlot.
  vtkImageRegion *GetInputRegion(int dim, int *extent);    

  // Description:
  // Recursive for streaming
  void UpdatePointData2(int dim, vtkImageRegion *inRegion, 
			vtkImageRegion *outRegion);

  virtual void ComputeOutputImageInformation(vtkImageRegion *inRegion,
					     vtkImageRegion *outRegion);
  virtual void ComputeRequiredInputRegionExtent(vtkImageRegion *outRegion,
						vtkImageRegion *inRegion);
  virtual void Execute(int dim, vtkImageRegion *inRegion, 
		       vtkImageRegion *outRegion);
  virtual void Execute(vtkImageRegion *inRegion, vtkImageRegion *outRegion);
};

#endif







