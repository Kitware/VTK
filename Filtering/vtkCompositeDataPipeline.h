/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeDataPipeline.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCompositeDataPipeline - Executive supporting composite datasets.
// .SECTION Description
// vtkCompositeDataPipeline is an executive that supports the processing of
// composite dataset. It supports algorithms that are aware of composite
// dataset as well as those that are not. Portions of the pipeline that are
// note composite dataset-aware are looped by the next consumer that is
// composite dataset-aware. Type checking is performed at run
// time. Algorithms that are not composite dataset-aware have to support
// all dataset types contained in the composite dataset. The pipeline
// execution can be summarized as follows: 
//
// * REQUEST_COMPOSITE_INFORMATION: Similar to REQUEST_INFORMATION. The
// producers have to provide information about the contents of the
// composite dataset in this pass. This is accomplished by creating and
// populating a vtkHierarchicalDataInformation and setting it using the
// COMPOSITE_DATA_INFORMATION() key in the output information vector.
// An UPDATE_COST() should be set for each block in the composite dataset.
// A cost of 0 implies that the dataset can be produced easily (for example
// without accessing a remote disk) whereas a cost of 1 implies that the
// dataset cannot be produced by the current process.
//
// TODO : Fix this documentation. MARKED_FOR_UPDATE() is provided by
// the source ----->
// * REQUEST_COMPOSITE_UPDATE_EXTENT: Similar to REQUEST_UPDATE_EXTENT.
// The consumers have to provide information about the extent (update blocks)
// they require. This accomplished by adding a MARKED_FOR_UPDATE() key
// to the appropriate blocks in COMPOSITE_DATA_INFORMATION() 
// (vtkHierarchicalDataInformation).
//
// * REQUEST_COMPOSITE_DATA: Similar to REQUEST_DATA. This is where the
// algorithms execute. If a composite data algorithm is consuming the output
// of a simple data algorithm, the executive will execute the streaming demand
// driven pipeline passes for each block:
// @verbatim
// for each block
//    REQUEST_PIPELINE_MODIFIED_TIME()
//    REQUEST_DATA_OBJECT()
//    REQUEST_INFORMATION()
//    REQUEST_DATA()
// @endverbatim
// The request passed to these passes will contain a LEVEL() and INDEX() key
// of each block to be updated.
// Shallow copies of individual blocks are added to the composite input
// of the algorithm. Finally, the request is passed to the algorithm.
//
// .SECTION See also
//  vtkHierarchicalDataInformation vtkCompositeDataSet

#ifndef __vtkCompositeDataPipeline_h
#define __vtkCompositeDataPipeline_h

#include "vtkStreamingDemandDrivenPipeline.h"

class vtkInformationDoubleKey;
class vtkInformationIntegerVectorKey;
class vtkInformationObjectBaseKey;
class vtkInformationStringKey;
class vtkInformationDataObjectKey;

class VTK_FILTERING_EXPORT vtkCompositeDataPipeline : public vtkStreamingDemandDrivenPipeline
{
public:
  static vtkCompositeDataPipeline* New();
  vtkTypeRevisionMacro(vtkCompositeDataPipeline,vtkStreamingDemandDrivenPipeline);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Generalized interface for asking the executive to fullfill update
  // requests.
  virtual int ProcessRequest(vtkInformation* request);

  // Description:
  // Returns the data object stored with the COMPOSITE_DATA_SET() in the
  // output port
  vtkDataObject* GetCompositeOutputData(int port);

  // Description:
  // vtkCompositeDataPipeline specific keys
  static vtkInformationIntegerKey*    BEGIN_LOOP();
  static vtkInformationIntegerKey*    END_LOOP();
  static vtkInformationStringKey*     COMPOSITE_DATA_TYPE_NAME();
  static vtkInformationObjectBaseKey* COMPOSITE_DATA_INFORMATION();
  static vtkInformationIntegerKey*    MARKED_FOR_UPDATE();
  static vtkInformationDoubleKey*     UPDATE_COST();
  static vtkInformationStringKey*     INPUT_REQUIRED_COMPOSITE_DATA_TYPE();

protected:
  vtkCompositeDataPipeline();
  ~vtkCompositeDataPipeline();

  virtual int ForwardUpstream(vtkInformation* request);
  virtual int ForwardUpstream(int i, int j, vtkInformation* request);

  virtual void CopyFromDataToInformation(
    vtkDataObject* dobj, vtkInformation* inInfo);
  virtual void PushInformation(vtkInformation*);
  virtual void PopInformation (vtkInformation*);

  // Composite data pipeline times. Similar to superclass'
  vtkTimeStamp SubPassTime;

  // If true, the producer is being driven in a loop (dumb filters
  // between composite consumer and producer)
  int InSubPass;

  virtual int ExecuteDataObjectForBlock(vtkInformation* request);
  virtual int ExecuteDataObject(vtkInformation* request);

  virtual int ExecuteInformationForBlock(vtkInformation* request);
  virtual int ExecuteInformation(vtkInformation* request);

  virtual int ExecuteDataForBlock(vtkInformation* request);
  virtual int ExecuteData(vtkInformation* request);

  int CheckCompositeData(int port);

  vtkInformation* InformationCache;

private:
  vtkCompositeDataPipeline(const vtkCompositeDataPipeline&);  // Not implemented.
  void operator=(const vtkCompositeDataPipeline&);  // Not implemented.
};

#endif
