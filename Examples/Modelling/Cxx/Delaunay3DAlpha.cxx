#include <vtkSmartPointer.h>
#include <vtkCleanPolyData.h>
#include <vtkDelaunay3D.h>
#include <vtkXMLDataSetWriter.h>
#include <vtkXMLPolyDataReader.h>


int main ( int argc, char *argv[] )
{
  if (argc != 4)
    {
    cout << "Usage: " << argv[0]
         << " Alpha InputPolyDataFile OutputDataSetFile" << endl;
    return EXIT_FAILURE;
    }

  //Read a file
  vtkSmartPointer<vtkXMLPolyDataReader> reader =
    vtkSmartPointer<vtkXMLPolyDataReader>::New();
  reader->SetFileName (argv[2]);

  // Clean the polydata. This will remove duplicate points that may be
  // present in the input data.
  vtkSmartPointer<vtkCleanPolyData> cleaner =
    vtkSmartPointer<vtkCleanPolyData>::New();
  cleaner->SetInputConnection (reader->GetOutputPort());

  // Generate a mesh from the input points. If Alpha is non-zero, then
  // tetrahedra, triangles, edges and vertices that lie within the
  // alpha radius are output.
  vtkSmartPointer<vtkDelaunay3D> delaunay3D =
    vtkSmartPointer<vtkDelaunay3D>::New();
  delaunay3D->SetInputConnection (cleaner->GetOutputPort());
  delaunay3D->SetAlpha(atof(argv[1]));

  // Output the mesh
  vtkSmartPointer<vtkXMLDataSetWriter> writer =
    vtkSmartPointer<vtkXMLDataSetWriter>::New();
  writer->SetFileName ( argv[3] );
  writer->SetInputConnection ( delaunay3D->GetOutputPort() );
  writer->Write();

  return EXIT_SUCCESS;
}
