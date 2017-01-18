/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageShiftScale.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageShiftScale
 * @brief   shift and scale an input image
 *
 * With vtkImageShiftScale Pixels are shifted (a constant value added)
 * and then scaled (multiplied by a scalar. As a convenience, this class
 * allows you to set the output scalar type similar to vtkImageCast.
 * This is because shift scale operations frequently convert data types.
*/

#ifndef vtkImageShiftScale_h
#define vtkImageShiftScale_h


#include "vtkImagingCoreModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class VTKIMAGINGCORE_EXPORT vtkImageShiftScale : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageShiftScale *New();
  vtkTypeMacro(vtkImageShiftScale,vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Set/Get the shift value. This value is added to each pixel
   */
  vtkSetMacro(Shift,double);
  vtkGetMacro(Shift,double);
  //@}

  //@{
  /**
   * Set/Get the scale value. Each pixel is multiplied by this value.
   */
  vtkSetMacro(Scale,double);
  vtkGetMacro(Scale,double);
  //@}

  //@{
  /**
   * Set the desired output scalar type. The result of the shift
   * and scale operations is cast to the type specified.
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
  void SetOutputScalarTypeToUnsignedChar()
    {this->SetOutputScalarType(VTK_UNSIGNED_CHAR);}
  //@}

  //@{
  /**
   * When the ClampOverflow flag is on, the data is thresholded so that
   * the output value does not exceed the max or min of the data type.
   * Clamping is safer because otherwise you might invoke undefined
   * behavior (and may crash) if the type conversion is out of range
   * of the data type.  On the other hand, clamping is slower.
   * By default, ClampOverflow is off.
   */
  vtkSetMacro(ClampOverflow, int);
  vtkGetMacro(ClampOverflow, int);
  vtkBooleanMacro(ClampOverflow, int);
  //@}

protected:
  vtkImageShiftScale();
  ~vtkImageShiftScale() VTK_OVERRIDE;

  double Shift;
  double Scale;
  int OutputScalarType;
  int ClampOverflow;

  int RequestInformation(vtkInformation*,
                                 vtkInformationVector**,
                                 vtkInformationVector*) VTK_OVERRIDE;

  void ThreadedRequestData(vtkInformation*,
                                   vtkInformationVector**,
                                   vtkInformationVector*,
                                   vtkImageData*** inData,
                                   vtkImageData** outData,
                                   int outExt[6],
                                   int threadId) VTK_OVERRIDE;
private:
  vtkImageShiftScale(const vtkImageShiftScale&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageShiftScale&) VTK_DELETE_FUNCTION;
};

#endif
