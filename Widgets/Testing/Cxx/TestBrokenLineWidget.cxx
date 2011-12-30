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
  virtual void Execute(vtkObject *caller, unsigned long, void* )
  {
  cerr << "Executing Callback" << endl;
  vtkBrokenLineWidget *line = reinterpret_cast<vtkBrokenLineWidget*>( caller );
  line->GetPolyData( Poly );
  vtkPoints* points = Poly->GetPoints();
  }
vtkBLWCallback():Poly(0),Points(0){};
  vtkPolyData* Poly;
  vtkPoints* Points;
};

int TestBrokenLineWidget( int argc, char *argv[] )
{
  // Initialize test value
  int testIntValue = 0;
   
  // Read 3D unstructured input mesh
  char* fileName = vtkTestUtilities::ExpandDataFileName( argc, argv, "Data/AngularSector.vtk");
  vtkSmartPointer<vtkUnstructuredGridReader> reader 
    = vtkSmartPointer<vtkUnstructuredGridReader>::New();
  reader->SetFileName( fileName );
  delete [] fileName;
  reader->Update();
   
  // Get mesh from reader output
  vtkSmartPointer<vtkDataSetMapper> meshMapper = vtkSmartPointer<vtkDataSetMapper>::New();
  meshMapper->SetInputConnection( reader->GetOutputPort() );
  vtkSmartPointer<vtkActor> meshActor = vtkSmartPointer<vtkActor>::New();
  meshActor->SetMapper( meshMapper );
  meshActor->GetProperty()->SetColor( .23, .37, .17 );
  meshActor->GetProperty()->SetRepresentationToWireframe();

  // Create multi-block mesh for linear extractor
  reader->Update();
  vtkUnstructuredGrid* mesh = reader->GetOutput();
  vtkSmartPointer<vtkMultiBlockDataSet> meshMB = vtkSmartPointer<vtkMultiBlockDataSet>::New();
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
  interactor->Initialize();

  // Renderer for full mesh and attached widget
  vtkRenderer* ren1 = vtkRenderer::New();
  ren1->SetBackground( .4, .4, .4 );
  ren1->SetBackground2( .8, .8, .8 );
  ren1->GradientBackgroundOn();
  ren1->SetViewport( 0., 0., .5, 1. );
  ren1->AddActor( meshActor );
  window->AddRenderer( ren1 );

  // Create a good view angle
  vtkCamera* camera1 = ren1->GetActiveCamera();
  camera1->SetFocalPoint( .12, 0., 0. );
  camera1->SetPosition( .35, .3, .3 );
  camera1->SetViewUp( 0., 0., 1. );

  // Create broken line widget, attach it to input mesh
  vtkSmartPointer<vtkBrokenLineWidget> line = vtkSmartPointer<vtkBrokenLineWidget>::New();
  line->SetInteractor( interactor);
  line->SetInput( mesh );
  line->SetPriority( 1. );
  line->KeyPressActivationOff();
  line->PlaceWidget();
  line->ProjectToPlaneOff();
  line->On();
  line->SetResolution( 6 );
  line->SetHandleSizeFactor( 2. );

  // Create list of points to define broken line
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->InsertNextPoint( .23, .0, .0 );
  points->InsertNextPoint( .0, .0, .0 );
  points->InsertNextPoint( .23, .04, .04 );
  line->InitializeHandles( points );

  // Extract polygonal line and then its points
  vtkSmartPointer<vtkPolyData> linePD = vtkSmartPointer<vtkPolyData>::New();
  line->GetPolyData( linePD );
  vtkSmartPointer<vtkPolyDataMapper> lineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  lineMapper->SetInput( linePD );
  vtkSmartPointer<vtkActor> lineActor = vtkSmartPointer<vtkActor>::New();
  lineActor->SetMapper( lineMapper );
  lineActor->GetProperty()->SetColor( 1., 0., 0. );
  lineActor->GetProperty()->SetLineWidth( 2. );

  // Renderer for broken line and extracted selection
  vtkRenderer* ren2 = vtkRenderer::New();
  ren2->SetBackground( 1., 1., 1. );
  ren2->SetViewport( .5, 0., 1., 1. );
  ren2->AddActor( lineActor );
  ren2->SetActiveCamera( camera1 );
  window->AddRenderer( ren2 );

  // Invoke callback on polygonal line to interactively select elements
  vtkBLWCallback* lineCB = vtkBLWCallback::New();
  lineCB->Poly = linePD;
  lineCB->Points = linePD->GetPoints();
  line->AddObserver( vtkCommand::InteractionEvent, lineCB );

  // Create selection along broken line defined by list of points
  vtkLinearExtractor* linExt = vtkLinearExtractor::New();
  linExt->SetInput( meshMB );
  linExt->SetPoints( points );
  linExt->IncludeVerticesOff();
  linExt->SetVertexEliminationTolerance( 1.e-12 );

  // Extract selection from mesh
  vtkExtractSelection* extSel = vtkExtractSelection::New();
  extSel->SetInput( 0, meshMB );
  extSel->SetInputConnection( 1, linExt->GetOutputPort() );
  extSel->Update();
  vtkMultiBlockDataSet* outMB = vtkMultiBlockDataSet::SafeDownCast( extSel->GetOutput() );

  // Render extracted selection 
  vtkUnstructuredGrid* selection = vtkUnstructuredGrid::SafeDownCast( outMB->GetBlock( 0 ) );
  vtkDataSetMapper* selMapper = vtkDataSetMapper::New();
  selMapper->SetInput( selection );
  vtkActor* selActor = vtkActor::New();
  selActor->SetMapper( selMapper );
  selActor->GetProperty()->SetColor( 0., 0., 0. );
  selActor->GetProperty()->SetRepresentationToWireframe();
  ren2->AddActor( selActor );

  // Test Set Get handle positions
  double pos[3];
//    for( int i = 0; i < line->GetNumberOfHandles(); ++i )
//       {
//       line->GetHandlePosition( i, pos );
//       line->SetHandlePosition( i, pos );
//       }

  // Render and interact
  window->Render();
  interactor->Start();

  // Clean up
  ren1->Delete();
  ren2->Delete();
  window->Delete();
  interactor->Delete();
  selMapper->Delete();
  selActor->Delete();
  lineCB->Delete();
  linExt->Delete();
  extSel->Delete();

  return testIntValue;
}
