/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageImport.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkImageImport - Feed generic memory into image pipeline.
// .SECTION Description vtkImageImport allows the user to feed any image
// into the image pipeline.



#ifndef __vtkImageImport_h
#define __vtkImageImport_h

#include "vtkImageCachedSource.h"
#include "vtkImageRegion.h"
#include "vtkImageExport.h"


class VTK_EXPORT vtkImageImport : public vtkImageCachedSource
{
public:
  vtkImageImport();
  ~vtkImageImport();
  static vtkImageImport *New() {return new vtkImageImport;};
  const char *GetClassName() {return "vtkImageImport";};
  void PrintSelf(ostream& os, vtkIndent indent);   
  
  // Description:
  // Set/Get the data type of the imported memory.
  void SetDataScalarTypeToFloat(){this->SetDataScalarType(VTK_FLOAT);}
  void SetDataScalarTypeToInt(){this->SetDataScalarType(VTK_INT);}
  void SetDataScalarTypeToShort(){this->SetDataScalarType(VTK_SHORT);}
  void SetDataScalarTypeToUnsignedShort()
    {this->SetDataScalarType(VTK_UNSIGNED_SHORT);}
  void SetDataScalarTypeToUnsignedChar()
    {this->SetDataScalarType(VTK_UNSIGNED_CHAR);}
  vtkSetMacro(DataScalarType, int);
  vtkGetMacro(DataScalarType, int);

  // Description:
  // Set/Get the data order of the imported memory.  The first axis is the
  // inner most loop.
  void SetDataAxes(int dim, int *axes);
  vtkImageSetMacro(DataAxes, int);
  void GetDataAxes(int dim, int *axes){this->Region->GetAxes(dim, axes);};
  vtkImageGetMacro(DataAxes, int);
  
  // Description:
  // Set/Get the extent of the data.
  void SetDataExtent(int dim, int *extent);
  vtkImageSetExtentMacro(DataExtent);
  void GetDataExtent(int dim, int *extent)
    {this->Region->GetExtent(dim, extent);};
  vtkImageGetExtentMacro(DataExtent);
  
  // Description:
  // This method should be used when a single chunk of 
  // memory is being imported.  It is the users responsibility to delete 
  // the memory, but the memory cannot be delete before the first update.
  void SetPointer(void *ptr);
  
  // Description:
  // This method should be used when the imported data is stored in
  // a series of images which are not necessarily in contiguous memory
  // It is the users responsibility to ultimately delete 
  // the memory, but the memory cannot be delete before the first update.
  void SetPointers(void **ptrs);
  
  // Description:
  // This method is used for testing the import/export fitlers.
  // It gets the pointer from the exporter, and calls SetPointer.
  // The two filters must be setup properly before this method is called.
  void TestExport(vtkImageExport *export)
    {this->SetPointer(export->GetPointer());}

  // Description:
  // For debugging.
  vtkGetObjectMacro(Region,vtkImageRegion);
  
protected:
  vtkImageRegion *Region;
  int DataAxes[VTK_IMAGE_DIMENSIONS];
  int DataExtent[VTK_IMAGE_EXTENT_DIMENSIONS];
  int DataScalarType;
  void *Pointer;
  void **Pointers;
  int Initialized;

  void InitializeRegion();

  void Update(vtkImageRegion *region);
  void UpdateImageInformation(vtkImageRegion *region);
};


#endif


