/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageGridSource.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

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
// .NAME vtkImageGridSource - Create an image of a grid.
// .SECTION Description
// vtkImageGridSource produces an image of a grid.  The
// default output type is float.

#ifndef __vtkImageGridSource_h
#define __vtkImageGridSource_h

#include "vtkImageSource.h"

class VTK_IMAGING_EXPORT vtkImageGridSource : public vtkImageSource
{
public:
  static vtkImageGridSource *New();
  vtkTypeMacro(vtkImageGridSource,vtkImageSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the grid spacing in pixel units.  Default (10,10,0).
  // A value of zero means no grid.
  vtkSetVector3Macro(GridSpacing,int);
  vtkGetVector3Macro(GridSpacing,int);
  
  // Description:
  // Set/Get the grid origin, in ijk integer values.  Default (0,0,0).
  vtkSetVector3Macro(GridOrigin,int);
  vtkGetVector3Macro(GridOrigin,int);

  // Description:
  // Set the grey level of the lines. Default 1.0.
  vtkSetMacro(LineValue,float);
  vtkGetMacro(LineValue,float);

  // Description:
  // Set the grey level of the fill. Default 0.0.
  vtkSetMacro(FillValue,float);
  vtkGetMacro(FillValue,float);

  // Description:
  // Set/Get the data type of pixels in the imported data.
  // As a convenience, the OutputScalarType is set to the same value.
  vtkSetMacro(DataScalarType,int);
  void SetDataScalarTypeToFloat(){this->SetDataScalarType(VTK_FLOAT);}
  void SetDataScalarTypeToInt(){this->SetDataScalarType(VTK_INT);}
  void SetDataScalarTypeToShort(){this->SetDataScalarType(VTK_SHORT);}
  void SetDataScalarTypeToUnsignedShort()
    {this->SetDataScalarType(VTK_UNSIGNED_SHORT);}
  void SetDataScalarTypeToUnsignedChar()
    {this->SetDataScalarType(VTK_UNSIGNED_CHAR);}
  vtkGetMacro(DataScalarType, int);
  const char *GetDataScalarTypeAsString() { 
    return vtkImageScalarTypeNameMacro(this->DataScalarType); }

  // Description:
  // Set/Get the extent of the whole output image,
  // Default: (0,255,0,255,0,0)
  vtkSetVector6Macro(DataExtent,int);
  vtkGetVector6Macro(DataExtent,int);

  // Description:
  // Set/Get the pixel spacing.
  vtkSetVector3Macro(DataSpacing,float);
  vtkGetVector3Macro(DataSpacing,float);
  
  // Description:
  // Set/Get the origin of the data.
  vtkSetVector3Macro(DataOrigin,float);
  vtkGetVector3Macro(DataOrigin,float);

protected:
  vtkImageGridSource();
  ~vtkImageGridSource() {};
  vtkImageGridSource(const vtkImageGridSource&);
  void operator=(const vtkImageGridSource&);

  int GridSpacing[3];
  int GridOrigin[3];

  float LineValue;
  float FillValue;

  int DataScalarType;

  int DataExtent[6];
  float DataSpacing[3];
  float DataOrigin[3];

  virtual void ExecuteInformation();
  virtual void ExecuteData(vtkDataObject *data);
};


#endif
