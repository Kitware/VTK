#include "TaskParallelismWithPorts.h"
#include "vtkOutputPort.h"

// Task 4 for TaskParallelism.
// See TaskParallelismWithPorts.cxx for more information.
void task4(double data)
{  
  double extent = data;
  
  // The pipeline

  // Synthetic image source.
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

  // Gradient vector.
  vtkImageGradient* grad = vtkImageGradient::New();
  grad->SetDimensionality( 3 );
  grad->SetInput(source1->GetOutput());

  vtkImageShrink3D* mask = vtkImageShrink3D::New();
  mask->SetInput(grad->GetOutput());
  mask->SetShrinkFactors(5, 5, 5);


  // Label the scalar field as the active vectors.
  vtkAssignAttribute* aa = vtkAssignAttribute::New();
  aa->SetInput(mask->GetOutput());
  aa->Assign(vtkDataSetAttributes::SCALARS, vtkDataSetAttributes::VECTORS,
	     vtkAssignAttribute::POINT_DATA);

  vtkGlyphSource2D* arrow = vtkGlyphSource2D::New();
  arrow->SetGlyphTypeToArrow();
  arrow->SetScale(0.2);
  arrow->FilledOff();

  // Glyph the gradient vector (with arrows)
  vtkGlyph3D* glyph = vtkGlyph3D::New();
  glyph->SetInput(aa->GetOutput());
  glyph->SetSource(arrow->GetOutput());
  glyph->ScalingOff();
  glyph->OrientOn();
  glyph->SetVectorModeToUseVector();
  glyph->SetColorModeToColorByVector();

  // Output port
  vtkOutputPort* op = vtkOutputPort::New();
  op->SetInput(glyph->GetOutput());
  op->SetTag(11);

  // Process requests
  op->WaitForUpdate();

  // Cleanup
  op->Delete();
  source1->Delete();
  grad->Delete();
  aa->Delete();
  mask->Delete();
  glyph->Delete();
  arrow->Delete();

}






