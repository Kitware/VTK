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
// .NAME vtkImageExport - Export VTK images to third-party systems.
// .SECTION Description
// vtkImageExport provides a way of exporting image data at the end
// of a pipeline to a third-party system or to a simple C array.
// Applications can use this to get direct access to the image data
// in memory.  A callback interface is provided to allow connection
// of the VTK pipeline to a third-party pipeline.  This interface
// conforms to the interface of vtkImageImport.
// In Python it is possible to use this class to write the image data
// into a python string that has been pre-allocated to be the correct
// size.
// .SECTION See Also
// vtkImageImport

#ifndef __vtkImageExport_h
#define __vtkImageExport_h

#include "vtkProcessObject.h"
#include "vtkImageData.h"

class VTK_IMAGING_EXPORT vtkImageExport : public vtkProcessObject
{
public:
  static vtkImageExport *New();
  vtkTypeMacro(vtkImageExport,vtkProcessObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Get the number of bytes required for the output C array.
  int GetDataMemorySize();

  // Description:
  // Get the (x,y,z) index dimensions of the data.  Please note
  // that C arrays are indexed in decreasing order, i.e. array[z][y][x]. 
  void GetDataDimensions(int *ptr);
  int *GetDataDimensions() { 
    this->GetDataDimensions(this->DataDimensions);
    return this->DataDimensions; }

  // Description:
  // Get the number of scalar components of the data.  Please note that
  // when you index into a C array, the scalar component index comes
  // last, i.e. array[z][y][x][c].
  int GetDataNumberOfScalarComponents() {
    if (this->GetInput() == NULL) { return 1; }
    this->GetInput()->UpdateInformation();
    return this->GetInput()->GetNumberOfScalarComponents(); };

  // Description:
  // Get the scalar type of the data.  The scalar type of the C array
  // must match the scalar type of the data.
  int GetDataScalarType() {
    if (this->GetInput() == NULL) { return VTK_UNSIGNED_CHAR; }
    this->GetInput()->UpdateInformation();
    return this->GetInput()->GetScalarType(); };
  const char *GetDataScalarTypeAsString() { 
    return vtkImageScalarTypeNameMacro(this->GetDataScalarType()); }

  // Description: 
  // Get miscellaneous additional information about the data.
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

  // Description:
  // Set/Get the input object from the image pipeline.
  void SetInput(vtkImageData *input);
  vtkImageData *GetInput();

  // Description:
  // Set/Get whether the data goes to the exported memory starting 
  // in the lower left corner or upper left corner.  Default: On.
  // When this flag is Off, the image will be flipped vertically
  // before it is exported.
  // WARNING: this flag is used only with the Export() method,
  // it is ignored by GetPointerToData().
  vtkBooleanMacro(ImageLowerLeft, int);
  vtkGetMacro(ImageLowerLeft, int);
  vtkSetMacro(ImageLowerLeft, int);

  // Description:
  // Set the void pointer of the C array to export the data to.
  // From python, you can specify a pointer to a string that is
  // large enough to hold the data.
  void SetExportVoidPointer(void *);
  void *GetExportVoidPointer() { return this->ExportVoidPointer; };

  // Description:
  // The main interface: update the pipeline and export the image
  // to the memory pointed to by SetExportVoidPointer().  You can
  // also specify a void pointer when you call Export().
  void Export() { this->Export(this->ExportVoidPointer); };
  virtual void Export(void *);

  // Description:
  // An alternative to Export(): Use with caution.   Update the
  // pipeline and return a pointer to the image memory.  The
  // pointer is only valid until the next time that the pipeline
  // is updated.
  // WARNING: This method ignores the ImageLowerLeft flag.
  void *GetPointerToData();

  // Description:
  // Get the user data that should be passed to the callback functions.
  void* GetCallbackUserData();
  
  //BTX
  // Description:
  // These are function pointer types for the pipeline connection
  // callbacks.  See furhter documentation in vtkImageImport.h.
  typedef void (*UpdateInformationCallbackType)(void*);
  typedef int (*PipelineModifiedCallbackType)(void*);
  typedef int* (*WholeExtentCallbackType)(void*);
  typedef float* (*SpacingCallbackType)(void*);
  typedef float* (*OriginCallbackType)(void*);
  typedef const char* (*ScalarTypeCallbackType)(void*); 
  typedef int (*NumberOfComponentsCallbackType)(void*);
  typedef void (*PropagateUpdateExtentCallbackType)(void*, int*);
  typedef void (*UpdateDataCallbackType)(void*);
  typedef int* (*DataExtentCallbackType)(void*);
  typedef void* (*BufferPointerCallbackType)(void*);
  
  // Description:
  // Get pointers to the pipeline interface callbacks.
  UpdateInformationCallbackType     GetUpdateInformationCallback() const;
  PipelineModifiedCallbackType      GetPipelineModifiedCallback() const;
  WholeExtentCallbackType           GetWholeExtentCallback() const;
  SpacingCallbackType               GetSpacingCallback() const;
  OriginCallbackType                GetOriginCallback() const;
  ScalarTypeCallbackType            GetScalarTypeCallback() const;
  NumberOfComponentsCallbackType    GetNumberOfComponentsCallback() const;
  PropagateUpdateExtentCallbackType GetPropagateUpdateExtentCallback() const;
  UpdateDataCallbackType            GetUpdateDataCallback() const;
  DataExtentCallbackType            GetDataExtentCallback() const;
  BufferPointerCallbackType         GetBufferPointerCallback() const;
  //ETX

protected:
  vtkImageExport();
  ~vtkImageExport();
  
  virtual void UpdateInformationCallback();
  virtual int PipelineModifiedCallback();
  virtual void UpdateDataCallback();  
  virtual int* WholeExtentCallback();
  virtual float* SpacingCallback();
  virtual float* OriginCallback();
  virtual const char* ScalarTypeCallback();
  virtual int NumberOfComponentsCallback();
  virtual void PropagateUpdateExtentCallback(int*);
  virtual int* DataExtentCallback();
  virtual void* BufferPointerCallback();

  int ImageLowerLeft;
  int DataDimensions[3];
  void *ExportVoidPointer;

  unsigned long LastPipelineMTime;

private:  
  vtkImageExport(const vtkImageExport&);  // Not implemented.
  void operator=(const vtkImageExport&);  // Not implemented.

  static void UpdateInformationCallbackFunction(void*);
  static int PipelineModifiedCallbackFunction(void*);
  static int* WholeExtentCallbackFunction(void*);
  static float* SpacingCallbackFunction(void*);
  static float* OriginCallbackFunction(void*);
  static const char* ScalarTypeCallbackFunction(void*); 
  static int NumberOfComponentsCallbackFunction(void*);
  static void PropagateUpdateExtentCallbackFunction(void*, int*);
  static void UpdateDataCallbackFunction(void*);
  static int* DataExtentCallbackFunction(void*);
  static void* BufferPointerCallbackFunction(void*);
};

#endif


