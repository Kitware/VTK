#include "vtkRegressionTestImage.h"
#include "vtkImageViewer2.h"
#include "vtkPushImageReader.h"
#include "vtkPushImageFilterSample.h"
#include "vtkPushPipeline.h"
#include "vtkCommand.h"

class myCallback : public vtkCommand
{
public:
  static myCallback *New() { return new myCallback;}
  static int count;
  virtual void Execute(vtkObject *caller, unsigned long, void *callData)
    {
      count++;
    }
};

int myCallback::count = 0;

int main( int argc, char *argv[] )
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
  iv->SetColorWindow(2000);
  iv->SetColorLevel(1000);
  
  myCallback *mc = myCallback::New();
  iv->GetRenderWindow()->AddObserver(vtkCommand::StartEvent,mc);
  mc->Delete();
  
  // push some data
  vtkPushPipeline *pp = vtkPushPipeline::New();
  pp->AddPusher(reader);
  pp->Run(reader);

  return (myCallback::count != 23);
}
