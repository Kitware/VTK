// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageEllipsoidSource
 * @brief   Create a binary image of an ellipsoid.
 *
 * vtkImageEllipsoidSource creates a binary image of a ellipsoid.  It was created
 * as an example of a simple source, and to test the mask filter.
 * It is also used internally in vtkImageDilateErode3D.
 */

#ifndef vtkImageEllipsoidSource_h
#define vtkImageEllipsoidSource_h

#include "vtkImageAlgorithm.h"
#include "vtkImagingSourcesModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGSOURCES_EXPORT vtkImageEllipsoidSource : public vtkImageAlgorithm
{
public:
  static vtkImageEllipsoidSource* New();
  vtkTypeMacro(vtkImageEllipsoidSource, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the extent of the whole output image.
   */
  void SetWholeExtent(int extent[6]);
  void SetWholeExtent(int minX, int maxX, int minY, int maxY, int minZ, int maxZ);
  void GetWholeExtent(int extent[6]);
  int* GetWholeExtent() VTK_SIZEHINT(6) { return this->WholeExtent; }
  ///@}

  ///@{
  /**
   * Set/Get the center of the ellipsoid.
   */
  vtkSetVector3Macro(Center, double);
  vtkGetVector3Macro(Center, double);
  ///@}

  ///@{
  /**
   * Set/Get the radius of the ellipsoid.
   */
  vtkSetVector3Macro(Radius, double);
  vtkGetVector3Macro(Radius, double);
  ///@}

  ///@{
  /**
   * Set/Get the inside pixel values.
   */
  vtkSetMacro(InValue, double);
  vtkGetMacro(InValue, double);
  ///@}

  ///@{
  /**
   * Set/Get the outside pixel values.
   */
  vtkSetMacro(OutValue, double);
  vtkGetMacro(OutValue, double);
  ///@}

  ///@{
  /**
   * Set what type of scalar data this source should generate.
   */
  vtkSetMacro(OutputScalarType, int);
  vtkGetMacro(OutputScalarType, int);
  void SetOutputScalarTypeToFloat() { this->SetOutputScalarType(VTK_FLOAT); }
  void SetOutputScalarTypeToDouble() { this->SetOutputScalarType(VTK_DOUBLE); }
  void SetOutputScalarTypeToLong() { this->SetOutputScalarType(VTK_LONG); }
  void SetOutputScalarTypeToUnsignedLong() { this->SetOutputScalarType(VTK_UNSIGNED_LONG); }
  void SetOutputScalarTypeToInt() { this->SetOutputScalarType(VTK_INT); }
  void SetOutputScalarTypeToUnsignedInt() { this->SetOutputScalarType(VTK_UNSIGNED_INT); }
  void SetOutputScalarTypeToShort() { this->SetOutputScalarType(VTK_SHORT); }
  void SetOutputScalarTypeToUnsignedShort() { this->SetOutputScalarType(VTK_UNSIGNED_SHORT); }
  void SetOutputScalarTypeToChar() { this->SetOutputScalarType(VTK_CHAR); }
  void SetOutputScalarTypeToUnsignedChar() { this->SetOutputScalarType(VTK_UNSIGNED_CHAR); }
  ///@}

protected:
  vtkImageEllipsoidSource();
  ~vtkImageEllipsoidSource() override;

  int WholeExtent[6];
  double Center[3];
  double Radius[3];
  double InValue;
  double OutValue;
  int OutputScalarType;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkImageEllipsoidSource(const vtkImageEllipsoidSource&) = delete;
  void operator=(const vtkImageEllipsoidSource&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
