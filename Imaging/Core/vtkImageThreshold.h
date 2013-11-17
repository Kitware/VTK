/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageThreshold.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageThreshold -  Flexible threshold
// .SECTION Description
// vtkImageThreshold can do binary or continuous thresholding for lower, upper
// or a range of data.  The output data type may be different than the
// output, but defaults to the same type.


#ifndef __vtkImageThreshold_h
#define __vtkImageThreshold_h


#include "vtkImagingCoreModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class VTKIMAGINGCORE_EXPORT vtkImageThreshold : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageThreshold *New();
  vtkTypeMacro(vtkImageThreshold,vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The values greater than or equal to the value match.
  void ThresholdByUpper(double thresh);

  // Description:
  // The values less than or equal to the value match.
  void ThresholdByLower(double thresh);

  // Description:
  // The values in a range (inclusive) match
  void ThresholdBetween(double lower, double upper);

  // Description:
  // Determines whether to replace the pixel in range with InValue
  vtkSetMacro(ReplaceIn, int);
  vtkGetMacro(ReplaceIn, int);
  vtkBooleanMacro(ReplaceIn, int);

  // Description:
  // Replace the in range pixels with this value.
  void SetInValue(double val);
  vtkGetMacro(InValue, double);

  // Description:
  // Determines whether to replace the pixel out of range with OutValue
  vtkSetMacro(ReplaceOut, int);
  vtkGetMacro(ReplaceOut, int);
  vtkBooleanMacro(ReplaceOut, int);

  // Description:
  // Replace the in range pixels with this value.
  void SetOutValue(double val);
  vtkGetMacro(OutValue, double);

  // Description:
  // Get the Upper and Lower thresholds.
  vtkGetMacro(UpperThreshold, double);
  vtkGetMacro(LowerThreshold, double);

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
  void SetOutputScalarTypeToSignedChar()
    {this->SetOutputScalarType(VTK_SIGNED_CHAR);}
  void SetOutputScalarTypeToUnsignedChar()
    {this->SetOutputScalarType(VTK_UNSIGNED_CHAR);}

protected:
  vtkImageThreshold();
  ~vtkImageThreshold() {}

  double UpperThreshold;
  double LowerThreshold;
  int ReplaceIn;
  double InValue;
  int ReplaceOut;
  double OutValue;

  int OutputScalarType;

  virtual int RequestInformation (vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  void ThreadedRequestData(vtkInformation *request,
                           vtkInformationVector **inputVector,
                           vtkInformationVector *outputVector,
                           vtkImageData ***inData, vtkImageData **outData,
                           int extent[6], int id);

private:
  vtkImageThreshold(const vtkImageThreshold&);  // Not implemented.
  void operator=(const vtkImageThreshold&);  // Not implemented.
};

#endif



