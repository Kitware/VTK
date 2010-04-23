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
// .NAME vtkImageEllipsoidSource - Create a binary image of an ellipsoid.
// .SECTION Description
// vtkImageEllipsoidSource creates a binary image of a ellipsoid.  It was created
// as an example of a simple source, and to test the mask filter.
// It is also used internally in vtkImageDilateErode3D.



#ifndef __vtkImageEllipsoidSource_h
#define __vtkImageEllipsoidSource_h

#include "vtkImageAlgorithm.h"

class VTK_IMAGING_EXPORT vtkImageEllipsoidSource : public vtkImageAlgorithm
{
public:
  static vtkImageEllipsoidSource *New();
  vtkTypeMacro(vtkImageEllipsoidSource,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);   
  
  // Description:
  // Set/Get the extent of the whole output image.
  void SetWholeExtent(int extent[6]);
  void SetWholeExtent(int minX, int maxX, int minY, int maxY, 
                            int minZ, int maxZ);
  void GetWholeExtent(int extent[6]);
  int *GetWholeExtent() {return this->WholeExtent;}
  
  // Description:
  // Set/Get the center of the ellipsoid.
  vtkSetVector3Macro(Center, double);
  vtkGetVector3Macro(Center, double);
  
  // Description:
  // Set/Get the radius of the ellipsoid.
  vtkSetVector3Macro(Radius, double);
  vtkGetVector3Macro(Radius, double);

  // Description:
  // Set/Get the inside pixel values.
  vtkSetMacro(InValue,double);
  vtkGetMacro(InValue,double);

  // Description:
  // Set/Get the outside pixel values.
  vtkSetMacro(OutValue,double);
  vtkGetMacro(OutValue,double);
  
  // Description:
  // Set what type of scalar data this source should generate.
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
  vtkImageEllipsoidSource(const vtkImageEllipsoidSource&);  // Not implemented.
  void operator=(const vtkImageEllipsoidSource&);  // Not implemented.
};


#endif


