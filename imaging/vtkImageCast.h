/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCast.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Abdalmajeid M. Alyassin who developed this class.

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
// .NAME vtkImageCast -  Image Data type Casting Filter
// .SECTION Description
// vtkImageCast filter casts the input type to match the output type in
// the image processing pipeline.  The filter does nothing if the input
// already has the correct type.  To specify the "CastTo" type,
// use "SetOutputScalarType" method.

// .SECTION See Also
// vtkImageThreshold vtkImageShiftScale

#ifndef __vtkImageCast_h
#define __vtkImageCast_h


#include "vtkImageFilter.h"

class VTK_EXPORT vtkImageCast : public vtkImageFilter
{
public:
  vtkImageCast();
  static vtkImageCast *New() {return new vtkImageCast;};
  const char *GetClassName() {return "vtkImageCast";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the desired output scalar type to cast to
  vtkSetMacro(OutputScalarType,int);
  vtkGetMacro(OutputScalarType,int);
  void SetOutputScalarTypeToFloat(){this->SetOutputScalarType(VTK_FLOAT);}
  void SetOutputScalarTypeToInt(){this->SetOutputScalarType(VTK_INT);}
  void SetOutputScalarTypeToShort(){this->SetOutputScalarType(VTK_SHORT);}
  void SetOutputScalarTypeToUnsignedShort()
    {this->SetOutputScalarType(VTK_UNSIGNED_SHORT);}
  void SetOutputScalarTypeToUnsignedChar()
    {this->SetOutputScalarType(VTK_UNSIGNED_CHAR);}

  // Description:
  // When the ClampOverflow flag is on, the data is thresholded so that
  // the output value does not exceed the amx or min of the data type.
  // By defualt ClampOverflow is off.
  vtkSetMacro(ClampOverflow, int);
  vtkGetMacro(ClampOverflow, int);
  vtkBooleanMacro(ClampOverflow, int);
  
  
protected:
  int ClampOverflow;
  int OutputScalarType;
  void ExecuteImageInformation();
  void InternalUpdate(vtkImageData *data);
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData, 
		       int ext[6], int id);
};

#endif



