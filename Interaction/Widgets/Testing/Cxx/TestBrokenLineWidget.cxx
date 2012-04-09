#include "vtkSmartPointer.h"

#include "vtkActor.h"
#include "vtkBrokenLineWidget.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkDataSetMapper.h"
#include "vtkExtractSelection.h"
#include "vtkInformation.h"
#include "vtkLinearSelector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProgrammableFilter.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridReader.h"

#include "vtkTestUtilities.h"

#include <vtksys/ios/sstream>

// Callback for the broken line widget interaction
class vtkBLWCallback : public vtkCommand
{
public:
  static vtkBLWCallback *New()
  { return new vtkBLWCallback; }
  virtual void Execute( vtkObject *caller, unsigned long, void* )
  {
    // Retrieve polydata line
    vtkBrokenLineWidget *line = reinterpret_cast<vtkBrokenLineWidget*>( caller );
    line->GetPolyData( Poly );

    // Update linear extractor with current points
    this->Selector->SetPoints( Poly->GetPoints() );

    // Update selection from mesh
    this->Extractor->Update();
    vtkMultiBlockDataSet* outMB = vtkMultiBlockDataSet::SafeDownCast( this->Extractor->GetOutput() );
    vtkUnstructuredGrid* selection = vtkUnstructuredGrid::SafeDownCast( outMB->GetBlock( 0 ) );
    this->Mapper->SetInputData( selection );

    // Update cardinality of selection
    vtksys_ios::ostringstream txt;
    txt << "Number of selected elements: " << ( selection ? selection->GetNumberOfCells() : 0 );
    this->Text->SetInput( txt.str().c_str() );
  }
vtkBLWCallback():Poly(0),Selector(0),Extractor(0),Mapper(0),Text(0) {};
  vtkPolyData* Poly;
  vtkLinearSelector* Selector;
  vtkExtractSelection* Extractor;
  vtkDataSetMapper* Mapper;
  vtkTextActor* Text;
};

int TestBrokenLineWidget( int argc, char *argv[] )
{
  // Create render window and interactor
  vtkSmartPointer<vtkRenderWindow> win = vtkSmartPointer<vtkRenderWindow>::New();
  win->SetMultiSamples( 0 );
  win->SetSize( 600, 300 );
  vtkSmartPointer<vtkRenderWindowInteractor> iren = vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow( win );
  iren->Initialize();

  // Create 2 viewports in window
  vtkSmartPointer<vtkRenderer> ren1 = vtkSmartPointer<vtkRenderer>::New();
  ren1->SetBackground( .4, .4, .4 );
  ren1->SetBackground2( .8, .8, .8 );
  ren1->GradientBackgroundOn();
  ren1->SetViewport( 0., 0., .5, 1. );
  win->AddRenderer( ren1 );
  vtkSmartPointer<vtkRenderer> ren2 = vtkSmartPointer<vtkRenderer>::New();
  ren2->SetBackground( 1., 1., 1. );
  ren2->SetViewport( .5, 0., 1., 1. );
  win->AddRenderer( ren2 );

  // Create a good view angle
  vtkCamera* camera = ren1->GetActiveCamera();
  camera->SetFocalPoint( .12, 0., 0. );
  camera->SetPosition( .38, .3, .15 );
  camera->SetViewUp( 0., 0., 1. );
  ren2->SetActiveCamera( camera );

  // Read 3D unstructured input mesh
  char* fileName = vtkTestUtilities::ExpandDataFileName( argc, argv, "Data/AngularSector.vtk");
  vtkSmartPointer<vtkUnstructuredGridReader> reader
    = vtkSmartPointer<vtkUnstructuredGridReader>::New();
  reader->SetFileName( fileName );
  delete [] fileName;
  reader->Update();

  // Create mesh actor to be rendered in viewport 1
  vtkSmartPointer<vtkDataSetMapper> meshMapper = vtkSmartPointer<vtkDataSetMapper>::New();
  meshMapper->SetInputConnection( reader->GetOutputPort() );
  vtkSmartPointer<vtkActor> meshActor = vtkSmartPointer<vtkActor>::New();
  meshActor->SetMapper( meshMapper );
  meshActor->GetProperty()->SetColor( .23, .37, .17 );
  meshActor->GetProperty()->SetRepresentationToWireframe();
  ren1->AddActor( meshActor );

  // Create multi-block mesh for linear extractor
  reader->Update();
  vtkUnstructuredGrid* mesh = reader->GetOutput();
  vtkSmartPointer<vtkMultiBlockDataSet> meshMB = vtkSmartPointer<vtkMultiBlockDataSet>::New();
  meshMB->SetNumberOfBlocks( 1 );
  meshMB->GetMetaData( static_cast<unsigned>( 0 ) )->Set( vtkCompositeDataSet::NAME(), "Mesh" );
  meshMB->SetBlock( 0, mesh );

  // Create broken line widget, attach it to input mesh
  vtkSmartPointer<vtkBrokenLineWidget> line = vtkSmartPointer<vtkBrokenLineWidget>::New();
  line->SetInteractor( iren );
  line->SetInputData( mesh );
  line->SetPriority( 1. );
  line->KeyPressActivationOff();
  line->PlaceWidget();
  line->ProjectToPlaneOff();
  line->On();
  line->SetHandleSizeFactor( 1.2 );

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
  lineMapper->SetInputData( linePD );
  vtkSmartPointer<vtkActor> lineActor = vtkSmartPointer<vtkActor>::New();
  lineActor->SetMapper( lineMapper );
  lineActor->GetProperty()->SetColor( 1., 0., 0. );
  lineActor->GetProperty()->SetLineWidth( 2. );
  ren2->AddActor( lineActor );

  // Create selection along broken line defined by list of points
  vtkSmartPointer<vtkLinearSelector> selector = vtkSmartPointer<vtkLinearSelector>::New();
  selector->SetInputData( meshMB );
  selector->SetPoints( points );
  selector->IncludeVerticesOff();
  selector->SetVertexEliminationTolerance( 1.e-12 );

  // Extract selection from mesh
  vtkSmartPointer<vtkExtractSelection> extractor = vtkSmartPointer<vtkExtractSelection>::New();
  extractor->SetInputData( 0, meshMB );
  extractor->SetInputConnection( 1, selector->GetOutputPort() );
  extractor->Update();
  vtkMultiBlockDataSet* outMB = vtkMultiBlockDataSet::SafeDownCast( extractor->GetOutput() );
  vtkUnstructuredGrid* selection = vtkUnstructuredGrid::SafeDownCast( outMB->GetBlock( 0 ) );

  // Create selection actor
  vtkSmartPointer<vtkDataSetMapper> selMapper = vtkSmartPointer<vtkDataSetMapper>::New();
  selMapper->SetInputData( selection );
  vtkSmartPointer<vtkActor> selActor = vtkSmartPointer<vtkActor>::New();
  selActor->SetMapper( selMapper );
  selActor->GetProperty()->SetColor( 0., 0., 0. );
  selActor->GetProperty()->SetRepresentationToWireframe();
  ren2->AddActor( selActor );

  // Annotate with number of elements
  vtkSmartPointer<vtkTextActor> txtActor = vtkSmartPointer<vtkTextActor>::New();
  vtksys_ios::ostringstream txt;
  txt << "Number of selected elements: " << ( selection ? selection->GetNumberOfCells() : 0 );
  txtActor->SetInput( txt.str().c_str() );
  txtActor->SetTextScaleModeToViewport();
  txtActor->SetNonLinearFontScale( .2, 18 );
  txtActor->GetTextProperty()->SetColor( 0., 0., 1. );
  txtActor->GetTextProperty()->SetFontSize( 18 );
  ren2->AddActor( txtActor );

  // Invoke callback on polygonal line to interactively select elements
  vtkSmartPointer<vtkBLWCallback> cb = vtkSmartPointer<vtkBLWCallback>::New();
  cb->Poly = linePD;
  cb->Selector = selector;
  cb->Extractor = extractor;
  cb->Mapper = selMapper;
  cb->Text = txtActor;
  line->AddObserver( vtkCommand::InteractionEvent, cb );

  // Render and test
  win->Render();
  int retVal = vtkRegressionTestImage( win );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  return !retVal;
}
