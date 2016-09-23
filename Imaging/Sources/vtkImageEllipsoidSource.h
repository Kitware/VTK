/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageEllipsoidSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#include "vtkImagingSourcesModule.h" // For export macro
#include "vtkImageAlgorithm.h"

class VTKIMAGINGSOURCES_EXPORT vtkImageEllipsoidSource : public vtkImageAlgorithm
{
public:
  static vtkImageEllipsoidSource *New();
  vtkTypeMacro(vtkImageEllipsoidSource,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Set/Get the extent of the whole output image.
   */
  void SetWholeExtent(int extent[6]);
  void SetWholeExtent(int minX, int maxX, int minY, int maxY,
                            int minZ, int maxZ);
  void GetWholeExtent(int extent[6]);
  int *GetWholeExtent() {return this->WholeExtent;}
  //@}

  //@{
  /**
   * Set/Get the center of the ellipsoid.
   */
  vtkSetVector3Macro(Center, double);
  vtkGetVector3Macro(Center, double);
  //@}

  //@{
  /**
   * Set/Get the radius of the ellipsoid.
   */
  vtkSetVector3Macro(Radius, double);
  vtkGetVector3Macro(Radius, double);
  //@}

  //@{
  /**
   * Set/Get the inside pixel values.
   */
  vtkSetMacro(InValue,double);
  vtkGetMacro(InValue,double);
  //@}

  //@{
  /**
   * Set/Get the outside pixel values.
   */
  vtkSetMacro(OutValue,double);
  vtkGetMacro(OutValue,double);
  //@}

  //@{
  /**
   * Set what type of scalar data this source should generate.
   */
  vtkSetMacro(OutputScalarType,int);
  vtkGetMacro(OutputScalarType,int);
  void SetOutputScalarTypeToFloat()
    {this->SetOutputScalarType(VTK_FLOAT);}
  void SetOutputScalarTypeToDouble()
    {this->SetOutputScalarType(VTK_DOUBLE);}
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

protected:
  vtkImageEllipsoidSource();
  ~vtkImageEllipsoidSource();

  int WholeExtent[6];
  double Center[3];
  double Radius[3];
  double InValue;
  double OutValue;
  int OutputScalarType;

  virtual int RequestInformation (vtkInformation *,
                                  vtkInformationVector **,
                                  vtkInformationVector *);

  virtual int RequestData(vtkInformation *,
                          vtkInformationVector **, vtkInformationVector *);

private:
  vtkImageEllipsoidSource(const vtkImageEllipsoidSource&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageEllipsoidSource&) VTK_DELETE_FUNCTION;
};


#endif


