/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageThreshold.h
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
// .NAME vtkImageThreshold -  Flexible threshold
// .SECTION Description
// vtkImageThreshold Can do binary or continuous thresholding for lower, upper
// or a range of data.  The output data type may be different than the
// output, but defaults to the same type.


#ifndef __vtkImageThreshold_h
#define __vtkImageThreshold_h


#include "vtkImageToImageFilter.h"

class VTK_IMAGING_EXPORT vtkImageThreshold : public vtkImageToImageFilter
{
public:
  static vtkImageThreshold *New();
  vtkTypeRevisionMacro(vtkImageThreshold,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // The values greater than or equal to the value match.
  void ThresholdByUpper(float thresh);
  
  // Description:
  // The values less than or equal to the value match.
  void ThresholdByLower(float thresh);
  
  // Description:
  // The values in a range (inclusive) match
  void ThresholdBetween(float lower, float upper);
  
  // Description:
  // Determines whether to replace the pixel in range with InValue
  vtkSetMacro(ReplaceIn, int);
  vtkGetMacro(ReplaceIn, int);
  vtkBooleanMacro(ReplaceIn, int);
  
  // Description:
  // Replace the in range pixels with this value.
  void SetInValue(float val);
  vtkGetMacro(InValue, float);
  
  // Description:
  // Determines whether to replace the pixel out of range with OutValue
  vtkSetMacro(ReplaceOut, int);
  vtkGetMacro(ReplaceOut, int);
  vtkBooleanMacro(ReplaceOut, int);

  // Description:
  // Replace the in range pixels with this value.
  void SetOutValue(float val);
  vtkGetMacro(OutValue, float);
  
  // Description:
  // Get the Upper and Lower thresholds.
  vtkGetMacro(UpperThreshold, float);
  vtkGetMacro(LowerThreshold, float);
  
  // Description:
  // Set the desired output scalar type to cast to
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
  
protected:
  vtkImageThreshold();
  ~vtkImageThreshold() {};

  float UpperThreshold;
  float LowerThreshold;
  int ReplaceIn;
  float InValue;
  int ReplaceOut;
  float OutValue;
  
  int OutputScalarType;

  void ExecuteInformation(vtkImageData *inData, vtkImageData *outData);
  void ExecuteInformation(){this->vtkImageToImageFilter::ExecuteInformation();};
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData, 
                       int extent[6], int id);
private:
  vtkImageThreshold(const vtkImageThreshold&);  // Not implemented.
  void operator=(const vtkImageThreshold&);  // Not implemented.
};

#endif



