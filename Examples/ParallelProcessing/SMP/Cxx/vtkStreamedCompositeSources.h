#ifndef vtkStreamedCompositeSources_h
#define vtkStreamedCompositeSources_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkStreamedCompositeDataPipeline.h"

class VTK_EXPORT vtkStreamedCompositeSources :
  public vtkStreamedCompositeDataPipeline
{
  vtkStreamedCompositeSources(const vtkStreamedCompositeSources&);
  void operator= (const vtkStreamedCompositeSources&);

public:
  static vtkStreamedCompositeSources* New();
  vtkTypeMacro(vtkStreamedCompositeSources, vtkStreamedCompositeDataPipeline);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  virtual int ExecuteData(
      vtkInformation* request,
      vtkInformationVector** inInfoVec,
      vtkInformationVector* outInfoVec);
  virtual int ProcessBlock(
      vtkInformation* request,
      vtkInformationVector** inInfoVec,
      vtkInformationVector* outInfoVec);

  vtkStreamedCompositeSources();
  ~vtkStreamedCompositeSources();

//private:
 // void InitLocalData();
};

#endif
