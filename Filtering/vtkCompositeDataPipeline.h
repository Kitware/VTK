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
// .NAME vtkCompositeDataPipeline -
// .SECTION Description
// vtkCompositeDataPipeline

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
  // Bring the outputs up-to-date.
  virtual int Update();
  virtual int Update(int port);
  virtual int UpdateInformation();
  virtual int PropagateUpdateExtent(int outputPort);
  virtual int UpdateData(int outputPort);

  // Description:
  vtkDataObject* GetCompositeOutputData(int port);

  // Description:
  static vtkInformationIntegerKey* REQUEST_COMPOSITE_DATA();
  static vtkInformationIntegerKey* REQUEST_COMPOSITE_INFORMATION();
  static vtkInformationIntegerKey* REQUEST_COMPOSITE_UPDATE_EXTENT();
  static vtkInformationStringKey* COMPOSITE_DATA_TYPE_NAME();
  static vtkInformationDataObjectKey* COMPOSITE_DATA_SET();
  static vtkInformationObjectBaseKey* COMPOSITE_DATA_INFORMATION();
  static vtkInformationIntegerKey* MARKED_FOR_UPDATE();
  static vtkInformationDoubleKey* UPDATE_COST();

protected:
  vtkCompositeDataPipeline();
  ~vtkCompositeDataPipeline();

  // Time when information or data were last generated.
  vtkTimeStamp CompositeDataInformationTime;
  vtkTimeStamp CompositeDataTime;
  vtkTimeStamp SubPassTime;
  int InSubPass;

  virtual int ExecuteCompositeData(vtkInformation* request);
  virtual int ExecuteCompositeInformation(vtkInformation* request);

  virtual int ExecuteDataObject(vtkInformation* request);
  virtual int ExecuteInformation(vtkInformation* request);
  virtual int ExecuteData(vtkInformation* request);

  int CheckCompositeData(int port);

private:
  vtkCompositeDataPipeline(const vtkCompositeDataPipeline&);  // Not implemented.
  void operator=(const vtkCompositeDataPipeline&);  // Not implemented.
};

#endif
