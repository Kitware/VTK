/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataObject.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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
// .NAME vtkDataObject - general representation of visualization data
// .SECTION Description
// vtkDataObject is an general representation of visualization data. It serves
// to encapsulate instance variables and methods for visualization network 
// execution, as well as representing data consisting of a field (i.e., just
// an unstructured pile of data). This is to be compared with a vtkDataSet,
// which is data with geometric and/or topological structure.
//
// vtkDataObjects are used to represent arbitrary repositories of data via the
// vtkFieldData instance variable. These data must be eventually mapped into a
// concrete subclass of vtkDataSet before they can actually be displayed.
//
// .SECTION See Also
// vtkDataSet vtkFieldData vtkDataObjectSource vtkDataObjectFilter
// vtkDataObjectMapper vtkDataObjectToDataSet 
// vtkFieldDataToAttributeDataFilter

#ifndef __vtkDataObject_h
#define __vtkDataObject_h

#include "vtkObject.h"
#include "vtkFieldData.h"

class vtkProcessObject;
class vtkSource;
class vtkExtentTranslator;

#define VTK_PIECES_EXTENT   0
#define VTK_3D_EXTENT       1

class VTK_EXPORT vtkDataObject : public vtkObject
{
public:
  static vtkDataObject *New();

  vtkTypeMacro(vtkDataObject,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create concrete instance of this data object.
  virtual vtkDataObject *MakeObject() {return vtkDataObject::New();}

  // Description:
  // Set/Get the source object creating this data object.
  vtkGetObjectMacro(Source,vtkSource);
  void SetSource(vtkSource *s);
  
  // Description:
  // Data objects are composite objects and need to check each part for MTime.
  // The information object also needs to be considered.
  unsigned long int GetMTime();

  // Rescription:
  // Restore data object to initial state,
  virtual void Initialize();

  // Description:
  // Release data back to system to conserve memory resource. Used during
  // visualization network execution.  Releasing this data does not make 
  // down-stream data invalid, so it does not modify the MTime of this 
  // data object.
  void ReleaseData();

  // Description:
  // Return flag indicating whether data should be released after use  
  // by a filter.
  int ShouldIReleaseData();

  // Description:
  // Get the flag indicating the data has been released.
  vtkGetMacro(DataReleased,int);
  
  // Description:
  // Turn on/off flag to control whether this object's data is released
  // after being used by a filter.
  vtkSetMacro(ReleaseDataFlag,int);
  vtkGetMacro(ReleaseDataFlag,int);
  vtkBooleanMacro(ReleaseDataFlag,int);

  // Description:
  // Turn on/off flag to control whether every object releases its data
  // after being used by a filter.
  static void SetGlobalReleaseDataFlag(int val);
  void GlobalReleaseDataFlagOn() {this->SetGlobalReleaseDataFlag(1);};
  void GlobalReleaseDataFlagOff() {this->SetGlobalReleaseDataFlag(0);};
  static int GetGlobalReleaseDataFlag();

  // Description:
  // Assign or retrieve field data to this data object.
  vtkSetObjectMacro(FieldData,vtkFieldData);
  vtkGetObjectMacro(FieldData,vtkFieldData);
  
  // Handle the source/data loop.
  void UnRegister(vtkObject *o);

  // Description:
  // Get the net reference count. That is the count minus
  // any self created loops. This is used in the Source/Data
  // registration to properly free the objects.
  virtual int GetNetReferenceCount() {return this->ReferenceCount;};

  // Description:
  // Provides opportunity for the data object to insure internal 
  // consistency before access. Also causes owning source/filter 
  // (if any) to update itself. The Update() method is composed of 
  // UpdateInformation(), PropagateUpdateExtent(), 
  // TriggerAsynchronousUpdate(), and UpdateData().
  virtual void Update();

  // Description:
  // WARNING: INTERNAL METHOD - NOT FOR GENERAL USE. 
  // THIS METHOD IS PART OF THE PIPELINE UPDATE FUNCTIONALITY.
  // Update all the "easy to update" information about the object such
  // as the extent which will be used to control the update.
  // This propagates all the way up then back down the pipeline.
  // As a by-product the PipelineMTime is updated.
  virtual void UpdateInformation();

  // Description:
  // WARNING: INTERNAL METHOD - NOT FOR GENERAL USE. 
  // THIS METHOD IS PART OF THE PIPELINE UPDATE FUNCTIONALITY.
  // The update extent for this object is propagated up the pipeline.
  // This propagation may early terminate based on the PipelineMTime.
  virtual void PropagateUpdateExtent();

  // Description:
  // WARNING: INTERNAL METHOD - NOT FOR GENERAL USE. 
  // THIS METHOD IS PART OF THE PIPELINE UPDATE FUNCTIONALITY.
  // Propagate back up the pipeline for ports and trigger the update on the
  // other side of the port to allow for asynchronous parallel processing in
  // the pipeline.
  // This propagation may early terminate based on the PipelineMTime.
  virtual void TriggerAsynchronousUpdate();

  // Description:
  // WARNING: INTERNAL METHOD - NOT FOR GENERAL USE. 
  // THIS METHOD IS PART OF THE PIPELINE UPDATE FUNCTIONALITY.
  // Propagate the update back up the pipeline, and perform the actual 
  // work of updating on the way down. When the propagate arrives at a
  // port, block and wait for the asynchronous update to finish on the
  // other side.
  // This propagation may early terminate based on the PipelineMTime.
  virtual void UpdateData();

  // Description:
  // Get the estimated size of this data object itself. Should be called
  // after UpdateInformation() and PropagateUpdateExtent() have both been 
  // called. Should be overridden in a subclass - otherwise the default
  // is to assume that this data object requires no memory.
  // The size is returned in kilobytes.
  virtual unsigned long GetEstimatedMemorySize();

  // Description:
  // A generic way of specifying an update extent.  Subclasses
  // must decide what a piece is.  When the NumberOfPieces is zero, then
  // no data is requested, and the source will not execute.
  virtual void SetUpdateExtent(int vtkNotUsed(piece),int vtkNotUsed(numPieces),
			       int vtkNotUsed(ghostLevel))
    {vtkErrorMacro("Subclass did not implement 'SetUpdateExtent'");}
  void SetUpdateExtent(int piece, int numPieces)
    {this->SetUpdateExtent(piece, numPieces, 0);}

  // Description:
  // Set the update extent for data objects that use 3D extents. Using this
  // method on data objects that set extents as pieces (such as vtkPolyData or
  // vtkUnstructuredGrid) has no real effect.
  // Don't use the set macro to set the update extent
  // since we don't want this object to be modified just due to
  // a change in update extent. When the volume of the extent is zero (0, -1,..), 
  // then no data is requested, and the source will not execute.
  virtual void SetUpdateExtent(int x1, int x2, int y1, int y2, int z1, int z2);
  virtual void SetUpdateExtent( int ext[6] );
  vtkGetVector6Macro( UpdateExtent, int );

  // Description:
  // Return class name of data type. This is one of VTK_STRUCTURED_GRID, 
  // VTK_STRUCTURED_POINTS, VTK_UNSTRUCTURED_GRID, VTK_POLY_DATA, or
  // VTK_RECTILINEAR_GRID (see vtkSetGet.h for definitions).
  // THIS METHOD IS THREAD SAFE
  virtual int GetDataObjectType() {return VTK_DATA_OBJECT;}
  
  // Description:
  // Used by Threaded ports to determine if they should initiate an
  // asynchronous update (still in development).
  unsigned long GetUpdateTime();

  // Description:
  // If the whole input extent is required to generate the requested output
  // extent, this method can be called to set the input update extent to the
  // whole input extent. This method assumes that the whole extent is known
  // (that UpdateInformation has been called)
  void SetUpdateExtentToWholeExtent();

  void SetPipelineMTime(unsigned long time) {this->PipelineMTime = time; }
  vtkGetMacro(PipelineMTime, unsigned long);

  // Description:
  // Return the actual size of the data in kilobytes. This number
  // is valid only after the pipeline has updated. The memory size
  // returned is guaranteed to be greater than or equal to the
  // memory required to represent the data (e.g., extra space in
  // arrays, etc. are not included in the return value).
  virtual unsigned long GetActualMemorySize();

  // Description:
  // Copy the generic information (WholeExtent ...)
  void CopyInformation( vtkDataObject *data );

  // Description:
  // By default, there is no type specific information
  virtual void CopyTypeSpecificInformation( vtkDataObject *data ) 
    {this->CopyInformation( data );};
  
  // Description:
  // Set / Get the update piece and the update number of pieces. Similar
  // to update extent in 3D.
  void SetUpdatePiece(int piece);
  void SetUpdateNumberOfPieces(int num);
  vtkGetMacro( UpdatePiece, int );
  vtkGetMacro( UpdateNumberOfPieces, int );
  
  // Description:
  // Set / Get the update ghost level and the update number of ghost levels.
  // Similar to update extent in 3D.
  void SetUpdateGhostLevel(int level);
  vtkGetMacro(UpdateGhostLevel, int);
  
  // Description:
  // This request flag indicates whether the requester can handle 
  // more data than requested.  Right now it is used in vtkImageData.
  // Image filters can return more data than requested.  The the 
  // consumer cannot handle this (i.e. DataSetToDataSetFitler)
  // the image will crop itself.  This functionality used to be in 
  // ImageToStructuredPoints.
  void SetRequestExactExtent(int v);
  vtkGetMacro(RequestExactExtent, int);
  vtkBooleanMacro(RequestExactExtent, int);
  
  // Description:
  // Set/Get the whole extent of this data object
  vtkSetVector6Macro( WholeExtent, int );
  vtkGetVector6Macro( WholeExtent, int );
  
  // Description:
  // This method is called by the source when it executes to generate data.
  // It is sort of the opposite of ReleaseData.
  // It sets the DataReleased flag to 0, and sets a new UpdateTime.
  void DataHasBeenGenerated();

  // Description:
  // make the output data ready for new data to be inserted. For most 
  // objects we just call Initialize. But for vtkImageData we leave the old
  // data in case the memory can be reused.
  virtual void PrepareForNewData() {this->Initialize();};

  // Description:
  // Shallow and Deep copy.  These copy the data, but not any of the 
  // pipeline connections.
  virtual void ShallowCopy(vtkDataObject *src);  
  virtual void DeepCopy(vtkDataObject *src);

  // Description:
  // Locality is used internally by the pipeline update mechanism.
  // It is used to get parralel execution when a filter has multiple
  // inputs with ports upstream.
  vtkSetMacro(Locality, float);
  vtkGetMacro(Locality, float);

  // Description:
  // An object that will translate pieces into structured extents.
  void SetExtentTranslator(vtkExtentTranslator *translator);
  vtkExtentTranslator *GetExtentTranslator();  

  // Description:
  // Get the number of consumers
  vtkGetMacro(NumberOfConsumers,int);
  
  // Description:
  // Add or remove or get or check a consumer, 
  void AddConsumer(vtkProcessObject *c);
  void RemoveConsumer(vtkProcessObject *c);
  vtkProcessObject *GetConsumer(int i);
  int IsConsumer(vtkProcessObject *c);
  
protected:

  vtkDataObject();
  ~vtkDataObject();
  vtkDataObject(const vtkDataObject&) {};
  void operator=(const vtkDataObject&) {};

  // General field data associated with data object      
  vtkFieldData  *FieldData;  

  // Who generated this data as output?
  vtkSource     *Source;     

  // Keep track of data release during network execution
  int DataReleased; 

  // how many consumers does this object have
  int NumberOfConsumers;
  vtkProcessObject **Consumers;
  
  // Description:
  // Return non zero if the UpdateExtent is outside of the Extent
  int UpdateExtentIsOutsideOfTheExtent();
  
  // Description:
  // This detects when the UpdateExtent will generate no data, and
  // UpdateData on the source is not necessary.  This condition is satisfied
  // when the UpdateExtent has zero volume (0,-1,...) 
  // of the UpdateNumberOfPieces is 0.
  int UpdateExtentIsEmpty();
  
  // Description:
  // Default behavior is to make sure that the update extent lies within
  // the whole extent. If it does not, an error condition occurs and this
  // method returns 0. If it is ok, then 1 is returned. Since uninitialized
  // extents are initialized to the whole extent during UpdateInformation()
  // there should not be errors. If a data object subclass wants to try to 
  // take care of errors silently, then this method should be overridden.
  virtual int VerifyUpdateExtent();

  // The ExtentType will be left as VTK_PIECES_EXTENT for data objects 
  // such as vtkPolyData and vtkUnstructuredGrid. The ExtentType will be 
  // changed to VTK_3D_EXTENT for data objects with 3D structure such as 
  // vtkImageData (and its subclass vtkStructuredPoints), vtkRectilinearGrid,
  // and vtkStructuredGrid. The default is the have an extent in pieces,
  // with only one piece (no streaming possible).
  virtual int GetExtentType() { return VTK_PIECES_EXTENT; };

  // If the ExtentType is VTK_3D_EXTENT, then these three extent variables
  // represent the whole extent, the extent currently in memory, and the
  // requested update extent. The extent is given as 3 min/max pairs.
  int WholeExtent[6];
  int Extent[6];
  int UpdateExtent[6];
  // First update, the update extent will be set to the whole extent.
  unsigned char UpdateExtentInitialized;  
  // An object to translate from unstructured pieces to structured extents.
  vtkExtentTranslator *ExtentTranslator;
 
  // Unstructured request stuff
  int NumberOfPieces;
  int Piece;
  int UpdateNumberOfPieces;
  int UpdatePiece;
  
  // This request flag indicates whether the requester can handle 
  // more data than requested.  Right now it is used in vtkImageData.
  // Image filters can return more data than requested.  The the 
  // consumer cannot handle this (i.e. DataSetToDataSetFitler)
  // this image will crop itself.  This functionality used to be in 
  // ImageToStructuredPoints.
  int RequestExactExtent;

  // This method cops the data object (if necesary) so that the extent
  // matches the update extent.
  virtual void Crop();
  
  int GhostLevel;
  int UpdateGhostLevel;

  // Data will release after use by a filter if this flag is set
  int ReleaseDataFlag; 

  // When was this data last generated?
  vtkTimeStamp UpdateTime;  

  // The Maximum MTime of all upstream filters and data objects.
  // This does not include the MTime of this data object.
  unsigned long PipelineMTime;

  // Was the update extent propagated down the pipeline
  int LastUpdateExtentWasOutsideOfTheExtent;
  
  // A value indicating whether we have a port upstream and how
  // many filters removed it is.  
  // 0.0 : no ports.
  // 1.0 : my source is a port.
  // 0.5 : the next upstream filter is a port ...
  float Locality;  

private:
  // Helper method for the ShallowCopy and DeepCopy methods.
  void InternalDataObjectCopy(vtkDataObject *src);
};

#endif

