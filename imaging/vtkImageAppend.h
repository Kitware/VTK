/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageAppend.h
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
// .NAME vtkImageAppend - Collects data from multiple inputs into one image.
// .SECTION Description
// vtkImageAppend takes the components from multiple inputs and merges
// them into one output. The output images are append along the "AppendAxis".
// Except for the append axis, all inputs must have the same extent.  
// All inputs must have the same number of scalar compnenets.  
// A future extension might be to pad or clip inputs to have the same extent.
// The output has the same origin and spacing as the first input.
// The origin and spacing of all other inputs are ignored.  All inputs
// must have the same scalar type.


#ifndef __vtkImageAppend_h
#define __vtkImageAppend_h


#include "vtkImageMultipleInputFilter.h"

class VTK_EXPORT vtkImageAppend : public vtkImageMultipleInputFilter
{
public:
  static vtkImageAppend *New(){return new vtkImageAppend;};
  const char *GetClassName() {return "vtkImageAppend";};
  vtkImageAppend();
  ~vtkImageAppend();
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // This axis is expanded to hold the multiple images.
  vtkSetMacro(AppendAxis, int);
  vtkGetMacro(AppendAxis, int);
  
protected:
  int AppendAxis;
  // Array holds the AppendAxisExtent shift for each input.
  int *Shifts;

  void ExecuteImageInformation();
  void ComputeRequiredInputUpdateExtent(int inExt[6], int outExt[6],
					int whichInput);
  
  void ThreadedExecute(vtkImageData **inDatas, vtkImageData *outData,
		       int extent[6], int id);
};

#endif




