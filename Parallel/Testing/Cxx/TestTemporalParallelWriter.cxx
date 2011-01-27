#include <iostream>
#include <mpi.h>

#include <vtkDataSetSurfaceFilter.h>
#include <vtkMPIController.h>
#include <vtkNetCDFPOPReader.h>
#include <vtkXMLPPolyDataWriter.h>

// read in a set of NetCDFPOP files in parallel where
// we partition both in space and time and write them out
// returns 0 for success

// this gets called by all processes
void MyMain(vtkMultiProcessController* controller, void* vtkNotUsed(arg) )
{
  vtkGenericWarningMacro(<< getpid() << " is rank " << controller->GetLocalProcessId() << " of " << controller->GetNumberOfProcesses());
  vtkNetCDFPOPReader* reader = vtkNetCDFPOPReader::New();
  reader->SetStride(4, 4, 4);
  reader->SetFileName("/home/acbauer/DATA/UVCDAT/t.x1_SAMOC_greenland.008910.bin.nc");

  vtkDataSetSurfaceFilter* surfaceFilter = vtkDataSetSurfaceFilter::New();
  surfaceFilter->SetInputConnection(reader->GetOutputPort());
  surfaceFilter->UseStripsOff();

  vtkXMLPPolyDataWriter* writer = vtkXMLPPolyDataWriter::New();
  writer->SetInputConnection(surfaceFilter->GetOutputPort());
  writer->SetFileName("/media/ssddrive/BUILDS/VTK/temporal-parallelism/test.pvtp");
  writer->SetNumberOfPieces(controller->GetNumberOfProcesses());
  writer->SetStartPiece(controller->GetLocalProcessId());
  writer->SetEndPiece(controller->GetLocalProcessId());
  writer->Write();
  writer->Delete();
  surfaceFilter->Delete();

  reader->Delete();
  cerr << "finished\n";
}

int main(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);

  vtkMPIController* controller = vtkMPIController::New();
  controller->Initialize(&argc, &argv, 1);
  vtkMultiProcessController::SetGlobalController(controller);

  controller->SetSingleMethod(MyMain, NULL); // second argument is for arguments to MyMain
  controller->SingleMethodExecute();

  controller->Finalize();
  controller->Delete();

  //MPI_Finalize(); it looks like this is not needed
  return 0;
}
