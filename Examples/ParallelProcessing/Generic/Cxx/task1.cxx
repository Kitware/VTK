#include "TaskParallelism.h"

vtkPolyDataMapper* task1(vtkRenderWindow* renWin, double data,
			 vtkCamera* cam)
{
  double extent = data;

// The pipeline

//  source
  vtkRTAnalyticSource* source1 = vtkRTAnalyticSource::New();
  source1->SetWholeExtent (-1*extent, extent, -1*extent, extent, 
	  -1*extent ,extent );
  source1->SetCenter(0, 0, 0);
  source1->SetStandardDeviation( 0.5 );
  source1->SetMaximum( 255.0 );
  source1->SetXFreq( 60 );
  source1->SetXMag( 10 );
  source1->SetYFreq( 30 );
  source1->SetYMag( 18 );
  source1->SetZFreq( 40 );
  source1->SetZMag( 5 );
  source1->GetOutput()->SetSpacing(2.0/extent,2.0/extent,2.0/extent);

// Iso-surfacing
  vtkContourFilter* contour = vtkContourFilter::New();
  contour->SetInput(source1->GetOutput());
  contour->SetNumberOfContours(1);
  contour->SetValue(0, 220);

// Magnitude of the gradient vector
  vtkImageGradientMagnitude* magn = vtkImageGradientMagnitude::New();
  magn->SetDimensionality(3);
  magn->SetInput(source1->GetOutput());

// Probe magnitude with iso-surface
  vtkProbeFilter* probe = vtkProbeFilter::New();
  probe->SetInput(contour->GetOutput());
  probe->SetSource(magn->GetOutput());
  probe->SpatialMatchOn();

  vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
  mapper->SetInput(probe->GetPolyDataOutput());
  mapper->SetScalarRange(50, 180);
  mapper->ImmediateModeRenderingOn();

  vtkActor* actor = vtkActor::New();
  actor->SetMapper(mapper);

  vtkRenderer* ren = vtkRenderer::New();
  renWin->AddRenderer(ren);

  ren->AddActor(actor);
  ren->SetActiveCamera( cam );

// Cleanup
  source1->Delete();
  contour->Delete();
  magn->Delete();
  probe->Delete();
  actor->Delete();
  ren->Delete();

  return mapper;

}


