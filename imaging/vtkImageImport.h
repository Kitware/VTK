/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageImport.h
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
// .NAME vtkImageImport - Import data from a C array.
// .SECTION Description
// vtkImageImport provides methods needed to import data from a C array.
// Note that the VTK convention is for the image voxel index (0,0,0) to be
// the lower-left corner of the image, while most 2D image formats use
// the upper-left corner.  You can use vtkImageFlip to correct the 
// orientation.
// .SECTION See Also
// vtkImageSource

#ifndef __vtkImageImport_h
#define __vtkImageImport_h

#include "vtkImageSource.h"
#include "vtkTransform.h"

class VTK_EXPORT vtkImageImport : public vtkImageSource
{
public:
  static vtkImageImport *New();
  vtkTypeMacro(vtkImageImport,vtkImageSource);
  void PrintSelf(ostream& os, vtkIndent indent);   

  // Description:
  // Import data and make an internal copy of it.  If you do not want
  // VTK to copy the data, then use SetImportVoidPointer instead (do
  // not use both).  Give the size of the data array in bytes.
  void CopyImportVoidPointer(void *ptr, int size);
  
  // Description:
  // Set the pointer from which the image data is imported.  VTK will
  // not make its own copy of the data, it will access the data directly
  // from the supplied array.  VTK will not attempt to delete the data
  // nor modify the data.
  void SetImportVoidPointer(void *ptr);
  void *GetImportVoidPointer() {return this->ImportVoidPointer;};

  // Description:
  // Set the pointer from which the image data is imported.  Set save to 1 
  // (the default) unless you want VTK to delete the array via C++ delete
  // when the vtkImageImport object is deallocated.  VTK will not make its
  // own copy of the data, it will access the data directly from the
  // supplied array.
  void SetImportVoidPointer(void *ptr, int save);
  
  // Description:
  // Set/Get the data type of pixels in the imported data.  This is used
  // as the scalar type of the Output.  Default: Short.
  vtkSetMacro(DataScalarType,int);
  void SetDataScalarTypeToDouble(){this->SetDataScalarType(VTK_DOUBLE);}
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
  // Set/Get the number of scalar components, for RGB images this must be 3.
  // Default: 1.
  vtkSetMacro(NumberOfScalarComponents,int);
  vtkGetMacro(NumberOfScalarComponents,int);
  
  // Description:
  // Get/Set the extent of the data.  The dimensions of your data must
  // be equal to (extent[1]-extent[0]+1) * (extent[3]-extent[2]+1) * 
  // (extent[5]-DataExtent[4]+1).  For example, for a 2D image use
  // (0,width-1, 0,height-1, 0,0).
  vtkSetVector6Macro(DataExtent,int);
  vtkGetVector6Macro(DataExtent,int);
  
  // Description:
  // Set/Get the spacing (typically in mm) between image voxels.
  // Default: (1.0, 1.0, 1.0).
  vtkSetVector3Macro(DataSpacing,float);
  vtkGetVector3Macro(DataSpacing,float);
  
  // Description:
  // Set/Get the origin of the data, i.e. the coordinates (usually in mm)
  // of voxel (0,0,0).  Default: (0.0, 0.0, 0.0). 
  vtkSetVector3Macro(DataOrigin,float);
  vtkGetVector3Macro(DataOrigin,float);

protected:
  vtkImageImport();
  ~vtkImageImport();
  vtkImageImport(const vtkImageImport&);
  void operator=(const vtkImageImport&);

  virtual void ExecuteInformation();
  
  void *ImportVoidPointer;
  int SaveUserArray;
  
  int NumberOfScalarComponents;
  int DataScalarType;

  int DataExtent[6];
  float DataSpacing[3];
  float DataOrigin[3];
  
  virtual void ExecuteData(vtkDataObject *d);
};



#endif




