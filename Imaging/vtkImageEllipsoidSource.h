/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageEllipsoidSource.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkImageEllipsoidSource - Create a binary image of an ellipsoid.
// .SECTION Description
// vtkImageEllipsoidSource creates a binary image of a ellipsoid.  It was created
// as an example of a simple source, and to test the mask filter.
// It is also used internally in vtkImageDilateErode3D.



#ifndef __vtkImageEllipsoidSource_h
#define __vtkImageEllipsoidSource_h

#include "vtkImageSource.h"

class VTK_EXPORT vtkImageEllipsoidSource : public vtkImageSource
{
public:
  static vtkImageEllipsoidSource *New();
  vtkTypeMacro(vtkImageEllipsoidSource,vtkImageSource);
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
  vtkSetVector3Macro(Center, float);
  vtkGetVector3Macro(Center, float);
  
  // Description:
  // Set/Get the radius of the ellipsoid.
  vtkSetVector3Macro(Radius, float);
  vtkGetVector3Macro(Radius, float);

  // Description:
  // Set/Get the inside pixel values.
  vtkSetMacro(InValue,float);
  vtkGetMacro(InValue,float);

  // Description:
  // Set/Get the outside pixel values.
  vtkSetMacro(OutValue,float);
  vtkGetMacro(OutValue,float);
  
  // Description:
  // Set what type of scalar data this source should generate.
  vtkSetMacro(OutputScalarType,int);
  vtkGetMacro(OutputScalarType,int);
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
  vtkImageEllipsoidSource();
  ~vtkImageEllipsoidSource();
  vtkImageEllipsoidSource(const vtkImageEllipsoidSource&);
  void operator=(const vtkImageEllipsoidSource&);

  int WholeExtent[6];
  float Center[3];
  float Radius[3];
  float InValue;
  float OutValue;
  int OutputScalarType;
  
  virtual void ExecuteInformation();
  virtual void ExecuteData(vtkDataObject *outData);
};


#endif


