/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageImport.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

Copyright (c) 1993-1999 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkImageImport - Import data from a C array.
// .SECTION Description
// vtkImageImport provides methods needed to import data from a C array.

// .SECTION See Also
// vtkImageSource

#ifndef __vtkImageImport_h
#define __vtkImageImport_h

#include <iostream.h>
#include <fstream.h>
#include "vtkImageSource.h"
#include "vtkTransform.h"

class VTK_EXPORT vtkImageImport : public vtkImageSource
{
public:
  static vtkImageImport *New();
  const char *GetClassName() {return "vtkImageImport";};
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
  // As a convienience, the OutputScalarType is set to the same value.
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

  // Description:
  // This method returns the largest data that can be generated.
  void ExecuteInformation();
  
protected:
  vtkImageImport();
  ~vtkImageImport();
  vtkImageImport(const vtkImageImport&) {};
  void operator=(const vtkImageImport&) {};

  // Generate more than requested.  Called by the superclass before
  // an execute, and before output memory is allocated.
  void ModifyOutputUpdateExtent();

  void *ImportVoidPointer;
  int SaveUserArray;

  
  int NumberOfScalarComponents;
  int DataScalarType;

  int DataExtent[6];
  float DataSpacing[3];
  float DataOrigin[3];
  
  void Execute(vtkImageData *data);
};



#endif




