//
// Delaunay3D
// Usage: Delaunay3D InputFile(.vtp) OutputFile(.vtu)
//        where
//        InputFile is an XML PolyData file with extension .vtp
//        OutputFile is an XML Unstructured Grid file with extension .vtu
//
#include <vtkSmartPointer.h>
#include <vtkCleanPolyData.h>
#include <vtkDelaunay3D.h>
#include <vtkXMLDataSetWriter.h>
#include <vtkXMLPolyDataReader.h>

int main ( int argc, char *argv[] )
{
  if (argc != 3)
    {
    cout << "Usage: " << argv[0]
         << " InputPolyDataFile OutputDataSetFile" << endl;
    return EXIT_FAILURE;
    }

  //Read the file
  vtkSmartPointer<vtkXMLPolyDataReader> reader =
    vtkSmartPointer<vtkXMLPolyDataReader>::New();
  reader->SetFileName (argv[1]);

  // Clean the polydata. This will remove duplicate points that may be
  // present in the input data.
  vtkSmartPointer<vtkCleanPolyData> cleaner =
    vtkSmartPointer<vtkCleanPolyData>::New();
  cleaner->SetInputConnection (reader->GetOutputPort());

  // Generate a tetrahedral mesh from the input points. By
  // default, the generated volume is the convex hull of the points.
  vtkSmartPointer<vtkDelaunay3D> delaunay3D =
    vtkSmartPointer<vtkDelaunay3D>::New();
  delaunay3D->SetInputConnection (cleaner->GetOutputPort());

  // Write the mesh as an unstructured grid
  vtkSmartPointer<vtkXMLDataSetWriter> writer =
    vtkSmartPointer<vtkXMLDataSetWriter>::New();
  writer->SetFileName ( argv[2] );
  writer->SetInputConnection ( delaunay3D->GetOutputPort() );
  writer->Write();

  return EXIT_SUCCESS;
}
