/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageExport.h
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
// .NAME vtkImageExport - Writes images to files.
// .SECTION Description
// vtkImageExport writes images to files with any data type. The data type of
// the file is the same scalar type as the input.  The dimensionality
// determines whether the data will be written in one or multiple files.
// This class is used as the superclass of most image writing classes 
// such as vtkBMPExport etc. It supports streaming.

#ifndef __vtkImageExport_h
#define __vtkImageExport_h

#include "vtkProcessObject.h"
#include "vtkImageData.h"

class VTK_EXPORT vtkImageExport : public vtkProcessObject
{
public:
  static vtkImageExport *New();
  vtkTypeMacro(vtkImageExport,vtkProcessObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Get the number of bytes required (for safety checks, etc)
  int GetDataMemorySize();

  // Description:
  // Get the (x,y,z) index dimensions of the data 
  // (warning: C arrays are indexed like this: array[z][y][x]) 
  void GetDataDimensions(int *ptr);
  int *GetDataDimensions() { 
    this->GetDataDimensions(this->DataDimensions);
    return this->DataDimensions; }

  // Description:
  // Get the number of scalar components of the data
  int GetDataNumberOfScalarComponents() {
    if (this->GetInput() == NULL) { return 1; }
    this->GetInput()->UpdateInformation();
    return this->GetInput()->GetNumberOfScalarComponents(); };

  // Description: 
  // Get misc. information about the data
  int *GetDataExtent() {
    static int defaultextent[6] = {0, 0, 0, 0, 0, 0};
    if (this->GetInput() == NULL) { return defaultextent; }
    this->GetInput()->UpdateInformation();
    return this->GetInput()->GetWholeExtent(); };
  void GetDataExtent(int *ptr) {   
    if (this->GetInput() == NULL) { 
      ptr[0] = ptr[1] = ptr[2] = ptr[3] = ptr[4] = ptr[5] = 0; return; }
    this->GetInput()->UpdateInformation();
    this->GetInput()->GetWholeExtent(ptr); };
  float *GetDataSpacing() { 
    static float defaultspacing[3] = {1, 1, 1}; 
    if (this->GetInput() == NULL) { return defaultspacing; }
    this->GetInput()->UpdateInformation();
    return this->GetInput()->GetSpacing(); };
  void GetDataSpacing(float *ptr) { 
    if (this->GetInput() == NULL) { ptr[0] = ptr[1] = ptr[2] = 0.0; return; }
    this->GetInput()->UpdateInformation();
    this->GetInput()->GetSpacing(ptr); };
  float *GetDataOrigin() { 
    static float defaultorigin[3] = {0, 0, 0};
    if (this->GetInput() == NULL) { return defaultorigin; }
    this->GetInput()->UpdateInformation();
    return this->GetInput()->GetOrigin(); };
  void GetDataOrigin(float *ptr) { 
    if (this->GetInput() == NULL) { ptr[0] = ptr[1] = ptr[2] = 0.0; return; }
    this->GetInput()->UpdateInformation();
    this->GetInput()->GetOrigin(ptr); };
  int GetDataScalarType() {
    if (this->GetInput() == NULL) { return VTK_UNSIGNED_CHAR; }
    this->GetInput()->UpdateInformation();
    return this->GetInput()->GetScalarType(); };

  // Description:
  // Set/Get the input object from the image pipeline.
  void SetInput(vtkImageData *input);
  vtkImageData *GetInput();

  // Description:
  // Set/Get whether the data goes to the exported memory starting 
  // in the lower left corner or upper left corner.  Default: On.
  // WARNING: this flag is used only with the Export() method,
  // it is ignored by GetPointerToData().
  vtkBooleanMacro(ImageLowerLeft, int);
  vtkGetMacro(ImageLowerLeft, int);
  vtkSetMacro(ImageLowerLeft, int);

  // Description:
  // Set the void pointer to export to.
  void SetExportVoidPointer(void *);
  void *GetExportVoidPointer() { return this->ExportVoidPointer; };

  // Description:
  // The main interface: export to the memory pointed to by
  // SetExportVoidPointer(), or specify a pointer directly.
  void Export() { this->Export(this->ExportVoidPointer); };
  virtual void Export(void *);

  // Description:
  // An alternative to Export(): Use with caution.   
  // Get a pointer to the image memory
  // (the pointer might be different each time this is called).
  // WARNING: This method ignores the ImageLowerLeft flag.
  void *GetPointerToData();
  
protected:
  vtkImageExport();
  ~vtkImageExport();
  vtkImageExport(const vtkImageExport&);
  void operator=(const vtkImageExport&);

  int ImageLowerLeft;
  int DataDimensions[3];
  void *ExportVoidPointer;
};

#endif


