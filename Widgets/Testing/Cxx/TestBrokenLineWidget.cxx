#include "vtkSmartPointer.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkDataSetMapper.h"
#include "vtkExtractSelection.h"
#include "vtkInformation.h"
#include "vtkLinearExtractor.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPlaneSource.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkBrokenLineWidget.h"
#include "vtkTextProperty.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridReader.h"

#include "vtkTestUtilities.h"

// Callback for the broken line widget interaction
class vtkBLWCallback : public vtkCommand
{
public:
  static vtkBLWCallback *New()
  { return new vtkBLWCallback; }
  virtual void Execute(vtkObject *caller, unsigned long, void*)
  {
    vtkBrokenLineWidget *line = reinterpret_cast<vtkBrokenLineWidget*>(caller);
    line->GetPolyData(Poly);
  }
  vtkBLWCallback():Poly(0){};
  vtkPolyData* Poly;
};

int TestBrokenLineWidget( int argc, char *argv[] )
{
  // Initialize test value
  int testIntValue = 0;
  
  // Read 3D unstructured input mesh
  char* fileName = vtkTestUtilities::ExpandDataFileName( argc, argv, "Data/AngularSector.vtk");
  vtkUnstructuredGridReader* reader = vtkUnstructuredGridReader::New();
  reader->SetFileName( fileName );
  delete [] fileName;
  reader->Update();
  
  // Get mesh from reader output
  vtkDataSetMapper* meshMapper = vtkDataSetMapper::New();
  meshMapper->SetInputConnection( reader->GetOutputPort() );
  vtkActor* meshActor = vtkActor::New();
  meshActor->SetMapper( meshMapper );
  meshActor->GetProperty()->SetColor( .23, .37, .17 );
  meshActor->GetProperty()->SetRepresentationToWireframe();

  // Create multi-block mesh for linear extractor
  reader->Update();
  vtkUnstructuredGrid* mesh = reader->GetOutput();
  vtkMultiBlockDataSet* meshMB = vtkMultiBlockDataSet::New();
  meshMB->SetNumberOfBlocks( 1 );
  meshMB->GetMetaData( static_cast<unsigned>( 0 ) )->Set( vtkCompositeDataSet::NAME(), "Mesh" ); 
  meshMB->SetBlock( 0, mesh );

  // Render window
  vtkRenderWindow* window = vtkRenderWindow::New();
  window->SetMultiSamples( 0 );
  window->SetSize( 600, 300 );

  // Interactor
  vtkRenderWindowInteractor* interactor = vtkRenderWindowInteractor::New();
  interactor->SetRenderWindow( window );
  
  // Renderer 1
  vtkRenderer* ren1 = vtkRenderer::New();
  ren1->SetBackground( .4, .4, .4 );
  ren1->SetBackground2( .8, .8, .8 );
  ren1->GradientBackgroundOn();
  ren1->SetViewport( 0., 0., .5, 1. );
  ren1->AddActor( meshActor );
  window->AddRenderer( ren1 );

  // Create a good view angle for renderer 1
  vtkCamera* camera1 = ren1->GetActiveCamera();
  camera1->SetFocalPoint( .12, 0., 0. );
  camera1->SetPosition( .35, .3, .3 );
  camera1->SetViewUp( 0., 0., 1. );

  // Renderer 2
  vtkRenderer* ren2 = vtkRenderer::New();
  ren2->SetBackground( 1., 1., 1. );
  ren2->SetViewport( .5, 0., 1., 1. );
  window->AddRenderer( ren2 );

  // Create broken line widget, attach it to input mesh
  vtkBrokenLineWidget* line = vtkBrokenLineWidget::New();
  line->SetInteractor( interactor);
  line->SetInput( mesh );
  line->SetPriority( 1. );
  line->KeyPressActivationOff();
  line->PlaceWidget();
  line->ProjectToPlaneOff();
  line->On();
  //line->SetNumberOfHandles(4);
  //line->SetNumberOfHandles(5);
  line->SetResolution(6);

  // Extract polygonal line and then its points
  vtkPolyData* polyLine = vtkPolyData::New();
  line->GetPolyData( polyLine );

  // Create selection along broken line defined by list of points
  vtkPoints* points = polyLine->GetPoints();
  points->Print(cerr);
  vtkLinearExtractor* linExt = vtkLinearExtractor::New();
  linExt->SetInput( meshMB );
  linExt->SetPoints( points );
  linExt->IncludeVerticesOff();
  linExt->SetVertexEliminationTolerance( 1.e-12 );

  // Extract selection from mesh
  vtkExtractSelection* extSel = vtkExtractSelection::New();
  extSel->SetInput( 0, mesh );
  extSel->SetInputConnection( 1, linExt->GetOutputPort() );
  extSel->Update();
  extSel->Print(cerr);
  //vtkMultiBlockDataSet* outputMB = vtkMultiBlockDataSet::SafeDownCast( extSel->GetOutput() );
  linExt->GetOutput()->Print(cerr);
  //vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::SafeDownCast( outputMB->GetBlock( 0 ) );
  vtkDataSetMapper* selMapper = vtkDataSetMapper::New();
  //selMapper->SetInput( ugrid );
  vtkActor* selActor = vtkActor::New();
  //selActor->SetMapper( selMapper );
  selActor->GetProperty()->SetColor( .23, .37, .17 );
  selActor->GetProperty()->SetRepresentationToWireframe();


  // Invoke callback on polygonal line to interactively select elements
  vtkBLWCallback* lineCB = vtkBLWCallback::New();
  lineCB->Poly = polyLine;
  line->AddObserver( vtkCommand::InteractionEvent, lineCB );

  // Test Set Get handle positions
  double pos[3];
//   for( int i = 0; i < line->GetNumberOfHandles(); ++i )
//     {
//     line->GetHandlePosition( i, pos );
//     line->SetHandlePosition( i, pos );
//     }

  // Render and interact
  interactor->Initialize();
  window->Render();
  interactor->Start();

  // Clean up
  ren1->Delete();
  ren2->Delete();
  window->Delete();
  interactor->Delete();
  meshMB->Delete();
  reader->Delete();
  meshMapper->Delete();
  meshActor->Delete();
  selMapper->Delete();
  selActor->Delete();
  line->Delete();
  lineCB->Delete();
  polyLine->Delete();
  linExt->Delete();
  extSel->Delete();

  return testIntValue;
}
