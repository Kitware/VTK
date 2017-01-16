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
/**
 * @class   vtkImageThreshold
 * @brief    Flexible threshold
 *
 * vtkImageThreshold can do binary or continuous thresholding for lower, upper
 * or a range of data.  The output data type may be different than the
 * output, but defaults to the same type.
*/

#ifndef vtkImageThreshold_h
#define vtkImageThreshold_h


#include "vtkImagingCoreModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class VTKIMAGINGCORE_EXPORT vtkImageThreshold : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageThreshold *New();
  vtkTypeMacro(vtkImageThreshold,vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * The values greater than or equal to the value match.
   */
  void ThresholdByUpper(double thresh);

  /**
   * The values less than or equal to the value match.
   */
  void ThresholdByLower(double thresh);

  /**
   * The values in a range (inclusive) match
   */
  void ThresholdBetween(double lower, double upper);

  //@{
  /**
   * Determines whether to replace the pixel in range with InValue
   */
  vtkSetMacro(ReplaceIn, int);
  vtkGetMacro(ReplaceIn, int);
  vtkBooleanMacro(ReplaceIn, int);
  //@}

  //@{
  /**
   * Replace the in range pixels with this value.
   */
  void SetInValue(double val);
  vtkGetMacro(InValue, double);
  //@}

  //@{
  /**
   * Determines whether to replace the pixel out of range with OutValue
   */
  vtkSetMacro(ReplaceOut, int);
  vtkGetMacro(ReplaceOut, int);
  vtkBooleanMacro(ReplaceOut, int);
  //@}

  //@{
  /**
   * Replace the in range pixels with this value.
   */
  void SetOutValue(double val);
  vtkGetMacro(OutValue, double);
  //@}

  //@{
  /**
   * Get the Upper and Lower thresholds.
   */
  vtkGetMacro(UpperThreshold, double);
  vtkGetMacro(LowerThreshold, double);
  //@}

  //@{
  /**
   * Set the desired output scalar type to cast to
   */
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
  //@}

protected:
  vtkImageThreshold();
  ~vtkImageThreshold()VTK_OVERRIDE {}

  double UpperThreshold;
  double LowerThreshold;
  int ReplaceIn;
  double InValue;
  int ReplaceOut;
  double OutValue;

  int OutputScalarType;

  int RequestInformation (vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;

  void ThreadedRequestData(vtkInformation *request,
                           vtkInformationVector **inputVector,
                           vtkInformationVector *outputVector,
                           vtkImageData ***inData, vtkImageData **outData,
                           int extent[6], int id) VTK_OVERRIDE;

private:
  vtkImageThreshold(const vtkImageThreshold&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageThreshold&) VTK_DELETE_FUNCTION;
};

#endif



