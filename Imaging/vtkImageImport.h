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
// vtkImageImport provides methods needed to import image data from a source
// independent of VTK, such as a simple C array or a third-party pipeline.
// Note that the VTK convention is for the image voxel index (0,0,0) to be
// the lower-left corner of the image, while most 2D image formats use
// the upper-left corner.  You can use vtkImageFlip to correct the 
// orientation after the image has been loaded into VTK.
// Note that is also possible to import the raw data from a Python string
// instead of from a C array.
// .SECTION See Also
// vtkImageExport

#ifndef __vtkImageImport_h
#define __vtkImageImport_h

#include "vtkImageSource.h"
#include "vtkTransform.h"

class VTK_IMAGING_EXPORT vtkImageImport : public vtkImageSource
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
  // Get/Set the extent of the data buffer.  The dimensions of your data
  // must be equal to (extent[1]-extent[0]+1) * (extent[3]-extent[2]+1) * 
  // (extent[5]-DataExtent[4]+1).  For example, for a 2D image use
  // (0,width-1, 0,height-1, 0,0).
  vtkSetVector6Macro(DataExtent,int);
  vtkGetVector6Macro(DataExtent,int);
  void SetDataExtentToWholeExtent()
    {this->SetDataExtent(this->GetWholeExtent());}
  
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

  // Description:
  // Get/Set the whole extent of the image.  This is the largest possible
  // extent.  Set the DataExtent to the extent of the image in the buffer
  // pointed to by the ImportVoidPointer.
  vtkSetVector6Macro(WholeExtent,int);
  vtkGetVector6Macro(WholeExtent,int);
  
  // Description:
  // Propagates the update extent through the callback if it is set.
  virtual void PropagateUpdateExtent(vtkDataObject *output);

  //BTX
  // Description:
  // These are function pointer types for the pipeline connection
  // callbacks.  See furhter documentation on each individual callback.
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
  // Set/Get the callback for propagating UpdateInformation calls to a
  // third-party pipeline.  The callback should make sure that the
  // third-party pipeline information is up to date.
  vtkSetMacro(UpdateInformationCallback, UpdateInformationCallbackType);
  vtkGetMacro(UpdateInformationCallback, UpdateInformationCallbackType);
  
  // Description:
  // Set/Get the callback for checking whether the third-party
  // pipeline has been modified since the last invocation of the
  // callback.  The callback should return 1 for modified, and 0 for
  // not modified.  The first call should always return modified.
  vtkSetMacro(PipelineModifiedCallback, PipelineModifiedCallbackType);
  vtkGetMacro(PipelineModifiedCallback, PipelineModifiedCallbackType);

  // Description:
  // Set/Get the callback for getting the whole extent of the input
  // image from a third-party pipeline.  The callback should return a
  // vector of six integers describing the extent of the whole image
  // (x1 x2 y1 y2 z1 z2).
  vtkSetMacro(WholeExtentCallback, WholeExtentCallbackType);
  vtkGetMacro(WholeExtentCallback, WholeExtentCallbackType);

  // Description:
  // Set/Get the callback for getting the spacing of the input image
  // from a third-party pipeline.  The callback should return a vector
  // of three float values describing the spacing (dx dy dz).
  vtkSetMacro(SpacingCallback, SpacingCallbackType);
  vtkGetMacro(SpacingCallback, SpacingCallbackType);

  // Description:
  // Set/Get the callback for getting the origin of the input image
  // from a third-party pipeline.  The callback should return a vector
  // of three float values describing the origin (x0 y0 z0).
  vtkSetMacro(OriginCallback, OriginCallbackType);
  vtkGetMacro(OriginCallback, OriginCallbackType);

  // Description:
  // Set/Get the callback for getting the scalar value type of the
  // input image from a third-party pipeline.  The callback should
  // return a string with the name of the type.
  vtkSetMacro(ScalarTypeCallback, ScalarTypeCallbackType);
  vtkGetMacro(ScalarTypeCallback, ScalarTypeCallbackType);
  
  // Description:  
  // Set/Get the callback for getting the number of components of the
  // input image from a third-party pipeline.  The callback should
  // return an integer with the number of components.
  vtkSetMacro(NumberOfComponentsCallback, NumberOfComponentsCallbackType);
  vtkGetMacro(NumberOfComponentsCallback, NumberOfComponentsCallbackType);
  
  // Description:  
  // Set/Get the callback for propagating the pipeline update extent
  // to a third-party pipeline.  The callback should take a vector of
  // six integers describing the extent.  This should cause the
  // third-party pipeline to provide data which contains at least this
  // extent after the next UpdateData callback.
  vtkSetMacro(PropagateUpdateExtentCallback,PropagateUpdateExtentCallbackType);
  vtkGetMacro(PropagateUpdateExtentCallback,PropagateUpdateExtentCallbackType);
  
  // Description:  
  // Set/Get the callback for propagating UpdateData calls to a
  // third-party pipeline.  The callback should make sure the
  // third-party pipeline is up to date.
  vtkSetMacro(UpdateDataCallback, UpdateDataCallbackType);
  vtkGetMacro(UpdateDataCallback, UpdateDataCallbackType);
  
  // Description:  
  // Set/Get the callback for getting the data extent of the input
  // image from a third-party pipeline.  The callback should return a
  // vector of six integers describing the extent of the buffered
  // portion of the image (x1 x2 y1 y2 z1 z2).  The buffer location
  // should be set with the BufferPointerCallback.
  vtkSetMacro(DataExtentCallback, DataExtentCallbackType);
  vtkGetMacro(DataExtentCallback, DataExtentCallbackType);
  
  // Description:  
  // Set/Get the callback for getting a pointer to the data buffer of
  // an image from a third-party pipeline.  The callback should return
  // a pointer to the beginning of the buffer.  The extent of the
  // buffer should be set with the DataExtentCallback.
  vtkSetMacro(BufferPointerCallback, BufferPointerCallbackType);
  vtkGetMacro(BufferPointerCallback, BufferPointerCallbackType);

  // Description:
  // Set/Get the user data which will be passed as the first argument
  // to all of the third-party pipeline callbacks.
  vtkSetMacro(CallbackUserData, void*);
  vtkGetMacro(CallbackUserData, void*);  
  
  //ETX

protected:
  vtkImageImport();
  ~vtkImageImport();

  virtual void UpdateInformation();
  virtual void ExecuteInformation();
  
  void InvokeUpdateInformationCallbacks();
  void InvokeExecuteInformationCallbacks();
  void InvokeExecuteDataCallbacks();
  void LegacyCheckWholeExtent();
  
  void *ImportVoidPointer;
  int SaveUserArray;
  
  int NumberOfScalarComponents;
  int DataScalarType;

  int WholeExtent[6];
  int DataExtent[6];
  float DataSpacing[3];
  float DataOrigin[3];

  void* CallbackUserData;
  
  //BTX
  UpdateInformationCallbackType     UpdateInformationCallback;
  PipelineModifiedCallbackType      PipelineModifiedCallback;
  WholeExtentCallbackType           WholeExtentCallback;
  SpacingCallbackType               SpacingCallback;
  OriginCallbackType                OriginCallback;
  ScalarTypeCallbackType            ScalarTypeCallback;
  NumberOfComponentsCallbackType    NumberOfComponentsCallback;  
  PropagateUpdateExtentCallbackType PropagateUpdateExtentCallback;  
  UpdateDataCallbackType            UpdateDataCallback;
  DataExtentCallbackType            DataExtentCallback;
  BufferPointerCallbackType         BufferPointerCallback;
  //ETX
  
  virtual void ExecuteData(vtkDataObject *d);

private:
  vtkImageImport(const vtkImageImport&);  // Not implemented.
  void operator=(const vtkImageImport&);  // Not implemented.
};

#endif
