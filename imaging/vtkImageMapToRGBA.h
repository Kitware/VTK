/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMapToRGBA.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

Copyright (c) 1993-1999 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkImageMapToRGBA - map the input image through a lookup table
// .SECTION Description
// The vtkImageMapToRGBA filter will take an input image of any valid
// scalar type, and map the first component of the image through a
// lookup table.  The result is an RGBA image of type VTK_UNSIGNED_CHAR.

// .SECTION See Also
// vtkLookupTable

#ifndef __vtkImageMapToRGBA_h
#define __vtkImageMapToRGBA_h


#include "vtkImageToImageFilter.h"
#include "vtkScalarsToColors.h"

class VTK_EXPORT vtkImageMapToRGBA : public vtkImageToImageFilter
{
public:
  vtkImageMapToRGBA();
  ~vtkImageMapToRGBA();
  static vtkImageMapToRGBA *New() {return new vtkImageMapToRGBA;};
  const char *GetClassName() {return "vtkImageMapToRGBA";};
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkSetObjectMacro(LookupTable,vtkScalarsToColors);
  vtkGetObjectMacro(LookupTable,vtkScalarsToColors);

protected:
  vtkScalarsToColors *LookupTable;
  
  void ExecuteInformation();
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData,
		       int extent[6], int id);
};

#endif







