/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageAppend.h
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
// .NAME vtkImageAppend - Collects data from multiple inputs into one image.
// .SECTION Description
// vtkImageAppend takes the components from multiple inputs and merges
// them into one output. The output images are append along the "AppendAxis".
// Except for the append axis, all inputs must have the same extent.  
// All inputs must have the same number of scalar components.  
// A future extension might be to pad or clip inputs to have the same extent.
// The output has the same origin and spacing as the first input.
// The origin and spacing of all other inputs are ignored.  All inputs
// must have the same scalar type.


#ifndef __vtkImageAppend_h
#define __vtkImageAppend_h


#include "vtkImageMultipleInputFilter.h"

class VTK_IMAGING_EXPORT vtkImageAppend : public vtkImageMultipleInputFilter
{
public:
  static vtkImageAppend *New();
  vtkTypeRevisionMacro(vtkImageAppend,vtkImageMultipleInputFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // This axis is expanded to hold the multiple images.  
  // The default AppendAxis is the X axis.
  // If you want to create a volue from a series of XY images, then you should
  // set the AppendAxis to 2 (Z axis).
  vtkSetMacro(AppendAxis, int);
  vtkGetMacro(AppendAxis, int);
  
  // Description:
  // By default "PreserveExtents" is off and the append axis is used.  
  // When "PreseveExtents" is on, the extent of the inputs is used to 
  // place the image in the output.  The whole extent of the output is 
  // the union of the input whole extents.  Any portion of the 
  // output not covered by the inputs is set to zero.  The origin and 
  // spacing is taken from the first input.
  vtkSetMacro(PreserveExtents, int);
  vtkGetMacro(PreserveExtents, int);
  vtkBooleanMacro(PreserveExtents, int);

protected:
  vtkImageAppend();
  ~vtkImageAppend();

  int PreserveExtents;
  int AppendAxis;
  // Array holds the AppendAxisExtent shift for each input.
  int *Shifts;

  void ExecuteInformation(vtkImageData **inputs, vtkImageData *output);
  void ComputeInputUpdateExtent(int inExt[6], int outExt[6], int whichInput);
  void ExecuteInformation(){this->vtkImageMultipleInputFilter::ExecuteInformation();};
  
  void ThreadedExecute(vtkImageData **inDatas, vtkImageData *outData,
                       int extent[6], int id);

  void InitOutput(int outExt[6], vtkImageData *outData);
private:
  vtkImageAppend(const vtkImageAppend&);  // Not implemented.
  void operator=(const vtkImageAppend&);  // Not implemented.
};

#endif




