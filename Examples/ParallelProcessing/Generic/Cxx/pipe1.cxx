#include "vtkOutputPort.h"
#include "vtkRTAnalyticSource.h"
#include "PipelineParallelism.h"

static float XFreq = 60;

// Increments XFreq of the synthetic source
void IncrementXFreq(void* sr)
{
  vtkRTAnalyticSource* source1 = reinterpret_cast<vtkRTAnalyticSource*>(sr);
  XFreq = XFreq + 10;
  source1->SetXFreq(XFreq);
}

// Pipe 1 for PipelineParallelism.
// See PipelineParallelism.cxx for more information.
void pipe1(vtkMultiProcessController* controller, void* arg)
{
  double extent = 20;

  // Synthetic image source.
  vtkRTAnalyticSource* source1 = vtkRTAnalyticSource::New();
  source1->SetWholeExtent (-1*extent, extent, -1*extent, extent, 
	  -1*extent ,extent );
  source1->SetCenter(0, 0, 0);
  source1->SetStandardDeviation( 0.5 );
  source1->SetMaximum( 255.0 );
  source1->SetXFreq( XFreq );
  source1->SetXMag( 10 );
  source1->SetYFreq( 30 );
  source1->SetYMag( 18 );
  source1->SetZFreq( 40 );
  source1->SetZMag( 5 );
  source1->GetOutput()->SetSpacing(2.0/extent,2.0/extent,2.0/extent);

  // Output port
  vtkOutputPort* op = vtkOutputPort::New();
  op->SetInput(source1->GetOutput());
  op->SetTag(11);
  // Turn this on for pipeline parallelism.
  op->PipelineFlagOn();
  // Called every time data is requested from the output port
  op->SetParameterMethod(IncrementXFreq, source1);
  // Process requests
  op->WaitForUpdate();
  
  // Cleanup
  op->Delete();
  source1->Delete();

}


