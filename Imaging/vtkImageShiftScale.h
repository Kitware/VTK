/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageShiftScale.h
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
// .NAME vtkImageShiftScale - shift and scale an input image
// .SECTION Description
// With vtkImageShiftScale Pixels are shifted and then scaled. As
// a convenience, this class allows you to set the output scalar type
// similar to vtkImageCast. This is because shift scale operations
// frequently convert data types.


#ifndef __vtkImageShiftScale_h
#define __vtkImageShiftScale_h


#include "vtkImageToImageFilter.h"

class VTK_IMAGING_EXPORT vtkImageShiftScale : public vtkImageToImageFilter
{
public:
  static vtkImageShiftScale *New();
  vtkTypeRevisionMacro(vtkImageShiftScale,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the shift value.
  vtkSetMacro(Shift,float);
  vtkGetMacro(Shift,float);

  // Description:
  // Set/Get the scale value.
  vtkSetMacro(Scale,float);
  vtkGetMacro(Scale,float);

  // Description:
  // Set the desired output scalar type. The result of the shift 
  // and scale operations is cast to the type specified.
  vtkSetMacro(OutputScalarType, int);
  vtkGetMacro(OutputScalarType, int);
  void SetOutputScalarTypeToDouble()
    {this->SetOutputScalarType(VTK_DOUBLE);}
  void SetOutputScalarTypeToFloat()
    {this->SetOutputScalarType(VTK_FLOAT);}
  void SetOutputScalarTypeToLong()
    {this->SetOutputScalarType(VTK_LONG);}
  void SetOutputScalarTypeToUnsignedLong()
    {this->SetOutputScalarType(VTK_UNSIGNED_LONG);};
  void SetOutputScalarTypeToInt()
    {this->SetOutputScalarType(VTK_INT);}
  void SetOutputScalarTypeToUnsignedInt()
    {this->SetOutputScalarType(VTK_UNSIGNED_INT);}
  void SetOutputScalarTypeToShort()
    {this->SetOutputScalarType(VTK_SHORT);}
  void SetOutputScalarTypeToUnsignedShort()
    {this->SetOutputScalarType(VTK_UNSIGNED_SHORT);}
  void SetOutputScalarTypeToChar()
    {this->SetOutputScalarType(VTK_CHAR);}
  void SetOutputScalarTypeToUnsignedChar()
    {this->SetOutputScalarType(VTK_UNSIGNED_CHAR);}

  // Description:
  // When the ClampOverflow flag is on, the data is thresholded so that
  // the output value does not exceed the max or min of the data type.
  // By default, ClampOverflow is off.
  vtkSetMacro(ClampOverflow, int);
  vtkGetMacro(ClampOverflow, int);
  vtkBooleanMacro(ClampOverflow, int);
  
protected:
  vtkImageShiftScale();
  ~vtkImageShiftScale() {};

  float Shift;
  float Scale;
  int OutputScalarType;
  int ClampOverflow;
  
  void ExecuteInformation(vtkImageData *inData, vtkImageData *outData);
  void ExecuteInformation(){this->vtkImageToImageFilter::ExecuteInformation();};
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData,
                       int extent[6], int id);
private:
  vtkImageShiftScale(const vtkImageShiftScale&);  // Not implemented.
  void operator=(const vtkImageShiftScale&);  // Not implemented.
};

#endif



