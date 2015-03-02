#ifndef vtkStreamedCompositeDataPipeline_h
#define vtkStreamedCompositeDataPipeline_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkCompositeDataPipeline.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPThreadLocalObject.h"

class vtkTimerLog;

class VTK_EXPORT vtkStreamedCompositeDataPipeline :
  public vtkCompositeDataPipeline
{
  //BTX
  friend class vtkStreamingFunctor;
  //ETX

  vtkStreamedCompositeDataPipeline(const vtkStreamedCompositeDataPipeline&);
  void operator= (const vtkStreamedCompositeDataPipeline&);

public:
  static vtkStreamedCompositeDataPipeline* New();
  vtkTypeMacro(vtkStreamedCompositeDataPipeline, vtkCompositeDataPipeline);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkInformationObjectBaseKey* STREAM_BLOCK_ID();
  static vtkInformationObjectBaseKey* STREAM_BLOCK();
  static vtkInformationObjectBaseKey* START_STREAM();
  static vtkInformationRequestKey* STREAM_DATA();
  static vtkInformationRequestKey* INIT_STREAM();
  static vtkInformationRequestKey* FINALIZE_STREAM();

protected:
  virtual int ProcessRequest(
      vtkInformation* request,
      vtkInformationVector** inInfoVec,
      vtkInformationVector* outInfoVec);
  virtual int ExecuteData(
      vtkInformation* request,
      vtkInformationVector** inInfoVec,
      vtkInformationVector* outInfoVec);
  virtual int ProcessBlock(
      vtkInformation* request,
      vtkInformationVector** inInfoVec,
      vtkInformationVector* outInfoVec);
  virtual int NeedToExecuteData(
      int outputPort,
      vtkInformationVector** inInfoVec,
      vtkInformationVector* outInfoVec);
  virtual void Modified();

  int CompositePort;
  vtkSMPThreadLocalObject<vtkInformation> LocalRequests;
  vtkSMPThreadLocal<vtkInformationVector**> LocalInputInformations;
  vtkSMPThreadLocalObject<vtkInformationVector> LocalOutputInformations;

  vtkStreamedCompositeDataPipeline();
  ~vtkStreamedCompositeDataPipeline();

private:
  void InitLocalData();
};

#endif
