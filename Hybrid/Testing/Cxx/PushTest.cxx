#include "vtkRegressionTestImage.h"
#include "vtkImageViewer2.h"
#include "vtkPushImageReader.h"
#include "vtkPushImageFilterSample.h"
#include "vtkPushPipeline.h"
#include "vtkCommand.h"

class PTCallback : public vtkCommand
{
public:
  static PTCallback *New() { return new PTCallback;}
  static int count;
  virtual void Execute(vtkObject *, unsigned long, void *)
    {
      count++;
    }
};

int PTCallback::count = 0;

int PushTest( int argc, char *argv[] )
{
  char *fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");
  
  vtkPushImageReader *reader = vtkPushImageReader::New();
  reader->SetDataByteOrderToLittleEndian();
  reader->SetDataExtent(0, 63, 0, 63, 1, 93);
  reader->SetFilePrefix(fname);
  reader->SetDataSpacing(1, 1, 2);
  
  vtkPushImageReader *reader2 = vtkPushImageReader::New();
  reader2->SetDataByteOrderToLittleEndian();
  reader2->SetDataExtent(0, 63, 0, 63, 1, 93);
  reader2->SetFilePrefix(fname);
  reader2->SetDataSpacing(1, 1, 2);
  delete [] fname;

  vtkPushImageFilterSample *ia = vtkPushImageFilterSample::New();
  ia->SetInput1(reader->GetOutput());
  ia->SetInput2(reader2->GetOutput());
  
  vtkImageViewer2 *iv = vtkImageViewer2::New();
  iv->SetInput(ia->GetOutput());
  ia->Delete();
  iv->SetColorWindow(2000);
  iv->SetColorLevel(1000);
  
  PTCallback *mc = PTCallback::New();
  iv->GetRenderWindow()->AddObserver(vtkCommand::StartEvent,mc);
  mc->Delete();
  
  // push some data
  vtkPushPipeline *pp = vtkPushPipeline::New();
  pp->AddPusher(reader);
  pp->Run(reader);

  iv->Delete();
  pp->Delete();
  reader->Delete();
  reader2->Delete();
  
  return (PTCallback::count != 23);
}
