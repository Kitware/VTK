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
  // Import data and make an internal copy of it. You should use 
  // this OR SetImportVoidPointer. The last one called will
  // control how the current memory is handled. The size is
  // the size of the data in BYTES.
  void CopyImportVoidPointer(void *ptr, int size);
  
  // Description:
  // Set the pointer from which the image data is imported.
  void SetImportVoidPointer(void *ptr);
  void *GetImportVoidPointer() {return this->ImportVoidPointer;};

  // Description:
  // Set the pointer from which the image data is imported.  Set save to 1 to
  // keep the vtk from deleting the array when it cleans up or reallocates
  // memory.  The class uses the actual array provided; it does not copy the
  // data from the suppled array.
  void SetImportVoidPointer(void *ptr, int save);
  
  // Description:
  // Set/Get the data type of pixels in the imported data.
  // As a convenience, the OutputScalarType is set to the same value.
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
  // Set/Get the number of scalar components
  vtkSetMacro(NumberOfScalarComponents,int);
  vtkGetMacro(NumberOfScalarComponents,int);
  
  // Description:
  // Get/Set the extent of the data on disk.  
  vtkSetVector6Macro(DataExtent,int);
  vtkGetVector6Macro(DataExtent,int);
  
  // Description:
  // Set/Get the spacing of the data in the file.
  vtkSetVector3Macro(DataSpacing,float);
  vtkGetVector3Macro(DataSpacing,float);
  
  // Description:
  // Set/Get the origin of the data (location of first pixel in the file).
  vtkSetVector3Macro(DataOrigin,float);
  vtkGetVector3Macro(DataOrigin,float);

protected:
  vtkImageImport();
  ~vtkImageImport();
  vtkImageImport(const vtkImageImport&) {};
  void operator=(const vtkImageImport&) {};

  // Description:
  // This method returns the largest data that can be generated.
  void ExecuteInformation();
  
  void *ImportVoidPointer;
  int SaveUserArray;

  
  int NumberOfScalarComponents;
  int DataScalarType;

  int DataExtent[6];
  float DataSpacing[3];
  float DataOrigin[3];
  
  void Execute(vtkImageData *d) {this->vtkImageSource::Execute(d);}
  void Execute();
};



#endif




