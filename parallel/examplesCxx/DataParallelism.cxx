#include <sys/stat.h>

#include "mpi.h"
#include "vtkMPIController.h"
#include "vtkRTAnalyticSource.h"
#include "vtkImageGaussianSmooth.h"
#include "vtkKitwareContourFilter.h"
#include "vtkImageGradientMagnitude.h"
#include "vtkProbeFilter.h"
#include "vtkDataSetMapper.h"
#include "vtkActor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"
#include "vtkWindowToImageFilter.h"
#include "vtkTIFFWriter.h"
#include "vtkTimerLog.h"
#include "vtkStructuredPointsWriter.h"
#include "vtkStructuredPointsReader.h"
#include "vtkRendererSource.h"
#include "vtkFloatArray.h"
#include "vtkCommand.h"
#include "vtkLargeInteger.h"
#include "vtkPipelineSize.h"

#include "composite.h"

// the whole extent is -EXTENT,EXTENT,-EXTENT,EXTENT,-EXTENT,EXTENT
static const int EXTENT=200;
// memory limit, in K
static const unsigned long MEM_LIMIT=50000;
// number of intermediate times an image will be stored in disk
static const int NUM_SAVE = 8;
// number of pieces requested for this run
static int NO_REQUESTED_PIECES=0;

static int START=0;

// number of processors per sub group
// when storing, the root node of each group will
// save a composited image
static const int NUM_PROC_PER_GROUP = 1;

static const int WINDOW_WIDTH = 400; 
static const int WINDOW_HEIGHT = 300; 

typedef struct {vtkTimerLog* timer; float time;} _TimerInfo;

_TimerInfo timerInfo1;
_TimerInfo timerInfo3;
_TimerInfo timerInfo4;
_TimerInfo timerInfo5;
_TimerInfo timerInfo6;
float totalElapsedTime=0;
int totalNumberOfPolygons=0;

// Start recording time
void startRecording(void* arg)
{
  _TimerInfo* timerInfo = (_TimerInfo *)arg;
  timerInfo->timer->StartTimer();
}

void stopRecording(void* arg)
{
  _TimerInfo* timerInfo = (_TimerInfo *)arg;
  timerInfo->timer->StopTimer();
  timerInfo->time += timerInfo->timer->GetElapsedTime();
}

void countPolygons(vtkObject* caller,
		   unsigned long vtkNotUsed(event), 
		   void* arg, void* vtkNotUsed(whatIsThis))
{
  vtkKitwareContourFilter* contour = (vtkKitwareContourFilter*)arg;
  totalNumberOfPolygons += contour->GetOutput()->GetNumberOfPolys();
}

void reduceAndPrintLogResult(float time, 
			     int myId, int numProcs,  int root, 
			     const char* title, ostream& os, 
			     MPI_Comm comm)
{
  float totalTime,  avgTime,  maxTime;

  MPI_Reduce(&time, &totalTime, 1, MPI_FLOAT, MPI_SUM, root, comm);
  MPI_Reduce(&time, &maxTime, 1, MPI_FLOAT, MPI_MAX, root, comm);
  avgTime = totalTime / numProcs;
  if (myId == root)
    {
    os << "---------------------------------------" << endl;
    os << title << endl;
    os << "Total: " << totalTime << endl;
    os << "Average: " << avgTime << endl;
    os << "Max: " << maxTime << endl;
    os << "---------------------------------------" << endl;
    }
  
}

int restoreInfo( const char* fileName )
{
  // If there is an input file read it
  struct stat sbuf;

  if (!stat(fileName,&sbuf))
    {
    int start=0;
    ifstream ifs(fileName, ios::in);
    ifs >> start;
    ifs >> timerInfo1.time;
    ifs >> timerInfo3.time;
    ifs >> timerInfo4.time;
    ifs >> timerInfo5.time;
    ifs >> timerInfo6.time;
    ifs >> totalElapsedTime;
    ifs >> totalNumberOfPolygons;
    ifs.close();

    START = start;
    return 1;
    }
  return 0;
}

int restoreData(unsigned char*& prevPixels, float*& prevZBuffer, 
		int myId, vtkStructuredPointsReader*& reader)
{
  
  // Read the stored bitmap and z buffer
  reader = vtkStructuredPointsReader::New();
  ostrstream fname;
  fname << "zbuffer" << myId << ".vtk" << '\0';
  reader->SetFileName(fname.str());
  reader->Update();
  
  prevPixels = 
    ((vtkUnsignedCharArray*)
     reader->GetOutput()->GetPointData()->GetScalars()
     ->GetData())->GetPointer(0);
  prevZBuffer = ((vtkFloatArray *)(reader->GetOutput()->GetPointData()
				   ->GetFieldData()
				   ->GetArray(0)))->GetPointer(0);
  if (prevPixels && prevZBuffer)
    return 1;

  return 0;
}

void saveInfo(const char* fileName, const char* message)
{
  ofstream ofs(fileName, ios::out);
  ofs << message;
  ofs.close();
}

void saveData(vtkRenderWindow* renWin, 
	      float* zdata, unsigned char* pixels,
	      float* prevZBuffer, unsigned char* prevPixels,  
	      int myId, int do_composite)

{
  int* winSize = renWin->GetSize();
  int numPixels = winSize[0]*winSize[1];

  if (do_composite)
    {
    // Composite with  previous stored bitmap
    vtkUCCompositeImagePair(zdata, pixels, prevZBuffer, prevPixels,
			    numPixels);
    }
    
  // Copy the data over a structured points
  vtkStructuredPoints* pts = vtkStructuredPoints::New();
  pts->SetDimensions(winSize[0], winSize[1], 1);
  pts->SetSpacing(1,1,1);
  pts->SetOrigin(0,0,0);
  vtkScalars *outScalars = vtkScalars::New(VTK_UNSIGNED_CHAR,3);
  unsigned char* ptr = ((vtkUnsignedCharArray *)outScalars->GetData())
    ->WritePointer(0,numPixels*3);
  memcpy(ptr,pixels,3*numPixels);
  pts->GetPointData()->SetScalars(outScalars);
  outScalars->Delete();
      
  vtkFloatArray *zArray = vtkFloatArray::New();
  zArray->Allocate(numPixels);
  zArray->SetNumberOfTuples(numPixels);
  zArray->SetName("ZBuffer");
  float* zPtr = zArray->WritePointer(0, numPixels);
  memcpy(zPtr,zdata,numPixels*sizeof(float));
      
  // z buffer goes into field data
  pts->GetPointData()->AddArray(zArray);
  zArray->Delete();
      
  // write it out using a structured points writer
  vtkStructuredPointsWriter* writer = vtkStructuredPointsWriter::New();
  writer->SetInput(pts);
  ostrstream fname;
  fname << "zbuffer" << myId << ".vtk" << '\0';
  writer->SetFileName(fname.str());
  writer->Write();
  writer->Delete();
  pts->Delete();
}      

void process(vtkMultiProcessController* controller, void* arg)
{
  int i;
  unsigned char* pixels=0;
  float* zBuffer=0;

  int myId = controller->GetLocalProcessId();
  int numProcs = controller->GetNumberOfProcesses();
  
// We will separate the problem into sub-problems
// Each with NUM_PROC_PER_GROUP processors
// There will be one composite image for each group
// and the root node for that sub-problem group will
// save a composited image
// See the documentation of vtkMPICommunicator and vtkMPIGroup
// for information on how this can be done.
  vtkMPICommunicator* localComm = vtkMPICommunicator::New();
  vtkMPIGroup* localGroup= vtkMPIGroup::New();
  vtkMPIController* localController = vtkMPIController::New();
  vtkMPICommunicator* worldComm = vtkMPICommunicator::GetWorldCommunicator();

  int numGroups = numProcs / NUM_PROC_PER_GROUP;
  int currentGroup = myId / NUM_PROC_PER_GROUP;

  localGroup->Initialize((vtkMPIController*)controller);
  for(i=0; i<NUM_PROC_PER_GROUP; i++)
    {
    localGroup->AddProcessId(currentGroup*NUM_PROC_PER_GROUP + i);
    }
  localComm->Initialize(worldComm, localGroup);
  localGroup->Delete();

  // Create a local controller (for the sub-group)
  localController->SetCommunicator(localComm);
  localComm->Delete();

  // Create the communicator which contains root processes
  // This is used to do the final compositing
  vtkMPICommunicator* rootsComm;
  vtkMPIGroup* rootsGroup;
  vtkMPIController* rootsController;

  rootsGroup = vtkMPIGroup::New();
  rootsGroup->Initialize((vtkMPIController*)controller);

  rootsComm = vtkMPICommunicator::New();
  for(int n=0; n < numGroups; n++)
    {
    rootsGroup->AddProcessId(n*NUM_PROC_PER_GROUP);
    }

  rootsComm->Initialize(worldComm, rootsGroup);
  rootsGroup->Delete();

  if ( myId % NUM_PROC_PER_GROUP == 0 )
    {
    rootsController = vtkMPIController::New();
    rootsController->SetCommunicator(rootsComm);
    }
  rootsComm->Delete();
    
// Read the data from the previous run
  vtkStructuredPointsReader* reader = 0;
  unsigned char* prevPixels;
  float* prevZBuffer;
  ostrstream fname;
  fname << "dataPar" << myId << ".cfg" << '\0';
  int do_composite = restoreInfo( fname.str());
  if (do_composite && (myId % NUM_PROC_PER_GROUP == 0))
    {
    restoreData(prevPixels, prevZBuffer, myId/NUM_PROC_PER_GROUP,  reader);
    }

  vtkTimerLog* log;
  log = vtkTimerLog::New();
  log->StartTimer();

// The pipeline

//  source
  vtkRTAnalyticSource* source1 = vtkRTAnalyticSource::New();
  source1->SetWholeExtent (-1*EXTENT, EXTENT, -1*EXTENT, EXTENT, 
			   -1*EXTENT ,EXTENT );
  source1->SetCenter(0, 0, 0);
  source1->SetStandardDeviation( 0.5 );
  source1->SetMaximum( 255.0 );
  source1->SetXFreq( 60 );
  source1->SetXMag( 10 );
  source1->SetYFreq( 30 );
  source1->SetYMag( 18 );
  source1->SetZFreq( 40 );
  source1->SetZMag( 5 );
  source1->GetOutput()->SetSpacing(2.0/EXTENT,2.0/EXTENT,2.0/EXTENT);

  vtkTimerLog* timer1 = vtkTimerLog::New();
  timerInfo1.timer = timer1;
  if (!do_composite)
    timerInfo1.time = 0;
  source1->SetStartMethod(startRecording, &timerInfo1);
  source1->SetEndMethod(stopRecording, &timerInfo1);

// Iso-surfacing
  vtkKitwareContourFilter* contour = vtkKitwareContourFilter::New();
  contour->SetInput(source1->GetOutput());
  contour->SetNumberOfContours(1);
  contour->SetValue(0, 220);
// Reduces memory use
  contour->GetOutput()->ReleaseDataFlagOn();

  vtkTimerLog* timer3 = vtkTimerLog::New();
  timerInfo3.timer = timer3;
  if (!do_composite)
    timerInfo3.time = 0;
  contour->SetStartMethod(startRecording, &timerInfo3);
  contour->SetEndMethod(stopRecording, &timerInfo3);
 
  vtkCallbackCommand *cbc = new vtkCallbackCommand;
  cbc->SetCallback(countPolygons);
  cbc->SetClientData((void*)contour);
  // contour will delete the cbc when the observer is removed.
  contour->AddObserver(vtkCommand::EndEvent,cbc);
 
// Magnitude of the gradient vector
  vtkImageGradientMagnitude* magn = vtkImageGradientMagnitude::New();
  magn->SetDimensionality(3);
  magn->SetInput(source1->GetOutput());
// Reduces memory use
  magn->GetOutput()->ReleaseDataFlagOn();

  vtkTimerLog* timer4 = vtkTimerLog::New();
  timerInfo4.timer = timer4;
  if (!do_composite)
    timerInfo4.time = 0;
  magn->SetStartMethod(startRecording, &timerInfo4);
  magn->SetEndMethod(stopRecording, &timerInfo4);

// Probe the magnitude with the iso-surface
  vtkProbeFilter* probe = vtkProbeFilter::New();
  probe->SetInput(contour->GetOutput());
  probe->SetSource(magn->GetOutput());
// To avoid the use of the whole extent in gradient magnitude computation
  probe->SpatialMatchOn();

  vtkTimerLog* timer5 = vtkTimerLog::New();
  timerInfo5.timer = timer5;
  if (!do_composite)
    timerInfo5.time = 0;
  probe->SetStartMethod(startRecording, &timerInfo5);
  probe->SetEndMethod(stopRecording, &timerInfo5);

  vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
  mapper->SetInput(probe->GetPolyDataOutput());

// Compute the number of pieces which will be processed by this
// processor using streaming.
// The total number of points is approximately 8*EXTENT*EXTENT*EXTENT.
// The number of points which will be processed in one pass is numPts.

// Use vtkPipelineSize to estimate the number of sub-pieces
// which will satisfy the memory limit
  vtkPipelineSize* psize = vtkPipelineSize::New();
  mapper->SetNumberOfPieces(numProcs*NUM_SAVE);
  mapper->SetPiece(myId*NUM_SAVE);
  unsigned long numPieces = psize->GetNumberOfSubPieces(MEM_LIMIT, mapper);
  psize->Delete();
  unsigned long maxPieces;
// Since numPieces can vary from piece to piece, use the maximum
// one to make sure that we don't go over the memory limit
  MPI_Allreduce(&numPieces, &maxPieces, 1, MPI_UNSIGNED_LONG, MPI_MAX,
		MPI_COMM_WORLD);
  numPieces = maxPieces;

  if (numPieces == 0)
    numPieces = 1;
  if (!myId)
    {
    cout << "Number of pieces / processor: " << NUM_SAVE*numPieces << endl;
    }

// Set the total number of pieces 
  mapper->SetNumberOfSubPieces(numPieces);
  mapper->SetScalarRange(50, 180);
// Reduces memory use
  mapper->ImmediateModeRenderingOn();

  vtkActor* actor = vtkActor::New();
  actor->SetMapper(mapper);

  vtkRenderWindow* renWin = vtkRenderWindow::New();
  vtkRenderer* ren = vtkRenderer::New();
  renWin->AddRenderer(ren);
  renWin->SetOffScreenRendering(1);
  ren->SetBackground( 0.5, 0.5, 0.5 );
  ren->AddActor(actor);

  renWin->SetSize( WINDOW_WIDTH, WINDOW_HEIGHT );
  int* winSize = renWin->GetSize();
  int numPixels = (winSize[0]*winSize[1]);

  vtkCamera* cam = vtkCamera::New();
  cam->SetPosition( -0.6105, 1.467, -6.879 );
  cam->SetFocalPoint( -0.0617558, 0.127043, 0 );
  cam->SetViewUp( -0.02, 0.98, 0.193 );
  cam->SetClippingRange( 3.36, 11.67);
  ren->SetActiveCamera( cam );
  
  _TimerInfo timerInfo6;
  vtkTimerLog* timer6 = vtkTimerLog::New();
  timerInfo6.timer = timer6;
  if (!do_composite)
    timerInfo6.time = 0;
  ren->SetStartRenderMethod(startRecording, &timerInfo6);
  ren->SetEndRenderMethod(stopRecording, &timerInfo6);

// Loop over each piece and render without erasing previously rendered
// image
  int count=0;
  for(i=START; (count<NO_REQUESTED_PIECES) && (i<NUM_SAVE); i++, count++)
    {
    if (myId == 0)
      {
      cout << "Current piece: " << myId*NUM_SAVE+i << endl;
      }
    mapper->SetPiece(myId*NUM_SAVE+i);
    renWin->Render();
    renWin->EraseOff();

    // Save state
    ostrstream message;
    message << i+1 << endl;
    message << timerInfo1.time << endl;
    message << timerInfo3.time << endl;
    message << timerInfo4.time << endl;
    message << timerInfo5.time << endl;
    message << timerInfo6.time << endl;
    log->StopTimer();
    totalElapsedTime += log->GetElapsedTime();
    log->StartTimer();
    message << totalElapsedTime << endl;
    message << totalNumberOfPolygons << endl;
    message << '\0';
    saveInfo(fname.str(),message.str());
    
    // Composite in each group
    delete[] zBuffer;
    delete[] pixels;
    zBuffer = renWin->GetZbufferData(0,0,winSize[0]-1, winSize[1]-1);
    pixels = renWin->GetPixelData(0,0,winSize[0]-1,winSize[1]-1,1);
    vtkTreeComposite(localController, numPixels, zBuffer, pixels);
    
    // Only the root nodes store the composited image
    if (myId % NUM_PROC_PER_GROUP == 0)
      {
      saveData(renWin, zBuffer, pixels, prevZBuffer, prevPixels, 
 	       myId/NUM_PROC_PER_GROUP, do_composite);
      }
    }

// For timing the wait of all processors
  vtkTimerLog* waitLog = vtkTimerLog::New();  
  waitLog->StartTimer();
// Sync
  MPI_Barrier(MPI_COMM_WORLD);
  waitLog->StopTimer();
  
// If at the end of the problem
  if ( (i == NUM_SAVE)  && (START < NUM_SAVE))
    {
// Composite the root nodes
    if ( myId % NUM_PROC_PER_GROUP == 0 )
      {
      vtkTreeComposite(rootsController, numPixels, zBuffer, pixels);
      }
    
// Report and image generation
    log->StopTimer();
    totalElapsedTime += log->GetElapsedTime();
    if (myId == 0)
      {
      cout << "Number of processors: " << numProcs << endl;
      cout << "Problem size: 8 " << EXTENT << "^3" << endl;
      cout << "Number of pieces per processor: " <<  NUM_SAVE*numPieces 
	   << endl;
      cout << "Total elapsed time is: " << totalElapsedTime << endl;

      renWin->SwapBuffersOff();
      renWin->SetPixelData(0, 0,  winSize[0]-1, winSize[1]-1, pixels,0);
      renWin->SwapBuffersOn();
      renWin->Frame();

      ostrstream fname;
      fname << "fractal" << myId/NUM_PROC_PER_GROUP << ".tif" << '\0';
      vtkWindowToImageFilter* w2if = vtkWindowToImageFilter::New();
      vtkTIFFWriter *tw = vtkTIFFWriter::New();
      w2if->SetInput(renWin);
      tw->SetInput(w2if->GetOutput());
      tw->SetFileName(fname.str());
      tw->Write();
      tw->Delete();
      w2if->Delete();
      }
    reduceAndPrintLogResult(waitLog->GetElapsedTime(), myId, numProcs, 0, 
			    "Wait:", cout, MPI_COMM_WORLD);
    reduceAndPrintLogResult(timerInfo1.time, myId, numProcs, 0, 
			    "Source:", cout, MPI_COMM_WORLD);
    reduceAndPrintLogResult(timerInfo3.time, myId, numProcs, 0, 
			    "Contour:", cout, MPI_COMM_WORLD);
    reduceAndPrintLogResult(timerInfo4.time, myId, numProcs, 0, 
			    "Image gradient magn:", cout, MPI_COMM_WORLD);
    reduceAndPrintLogResult(timerInfo5.time, myId, numProcs, 0, 
			    "Probe:", cout, MPI_COMM_WORLD);
    reduceAndPrintLogResult(timerInfo6.time, myId, numProcs, 0, 
			    "Renderer:", cout, MPI_COMM_WORLD);
    reduceAndPrintLogResult(totalNumberOfPolygons, myId, numProcs, 0, 
			    "Number of polygons:", cout, MPI_COMM_WORLD);
    }

// Sync
  MPI_Barrier(MPI_COMM_WORLD);

  log->Delete();
  waitLog->Delete();
  timer1->Delete();
  timer3->Delete();
  timer4->Delete();
  timer5->Delete();
  timer6->Delete();

// Cleanup

  delete[] pixels;
  delete[] zBuffer;

  if(reader)
    reader->Delete();

  if ( myId % NUM_PROC_PER_GROUP == 0 )
    {
    rootsController->Delete();
    }

  localController->Delete();
  source1->Delete();
  contour->Delete();
  magn->Delete();
  probe->Delete();
  mapper->Delete();
  actor->Delete();
  ren->Delete();
  renWin->Delete();
  cam->Delete();

}

int main( int argc, char* argv[] )
{
  vtkMPIController* controller = vtkMPIController::New();
  controller->Initialize(&argc, &argv);
  controller->CreateOutputWindow();

  controller->SetSingleMethod(process, 0);

  if (argc > 1)
    {
    NO_REQUESTED_PIECES = atoi(argv[1]);
    controller->SingleMethodExecute();
    }
  else
    {
    if (controller->GetLocalProcessId() == 0)
      {
      cerr << "\nNo pieces requested. \n"
	   << "Usage:  mpirun -np NUM_PROCS DataParallelism NUM_PIECES\n"
	   << "  NUM_PROCS is the number of processesor to assign.\n"
	   << "  NUM_PIECES is the number of pieces to process on this run.\n\n"
	   << "If the simulation is not finished after NUM_PIECES has been processed,\n"
	   << "the final state is saved in files and used to start the next run.\n\n";
      }
    }
  
  
  controller->Finalize();
  controller->Delete();
  return 1;
}


