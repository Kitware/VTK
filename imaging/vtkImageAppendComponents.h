/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageAppendComponents.h
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
// .NAME vtkImageAppendComponents - Collects components from two inputs into
// one output.
// .SECTION Description
// vtkImageAppendComponents takes the components from two inputs and merges
// them into one output. If Input1 has M compoennts, and Input2 has N 
// components, the output will have M+N compoennts with input1
// components comming first.


#ifndef __vtkImageAppendComponents_h
#define __vtkImageAppendComponents_h


#include "vtkImageMultipleInputFilter.h"

class VTK_EXPORT vtkImageAppendComponents : public vtkImageMultipleInputFilter
{
public:
  static vtkImageAppendComponents *New(){return new vtkImageAppendComponents;};
  const char *GetClassName() {return "vtkImageAppendComponents";};

  // Description:
  // Do not use these: They are for legacy compatability back when this was a
  // two input filter.
  virtual void SetInput1(vtkImageCache *input){this->SetInput(0, input);}
  void SetInput1(vtkStructuredPoints *spts)
    {this->SetInput1(spts->GetStructuredPointsToImage()->GetOutput());}
  virtual void SetInput2(vtkImageCache *input){this->SetInput(1, input);}
  void SetInput2(vtkStructuredPoints *spts)
    {this->SetInput2(spts->GetStructuredPointsToImage()->GetOutput());}

protected:
  
  void ExecuteImageInformation();
  void ThreadedExecute(vtkImageData **inDatas, vtkImageData *outData,
		       int extent[6], int id);
};

#endif



