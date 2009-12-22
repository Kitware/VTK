//
// DumpXMLFile - report on the contents of an XML or legacy vtk file
//  Usage: DumpXMLFile XMLFile1 XMLFile2 ...
//         where
//         XMLFile is a vtk XML file of type .vtu, .vtp, .vts, .vtr,
//         .vti, .vto 
//
#include <vtkSmartPointer.h>
#include <vtkXMLReader.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkXMLStructuredGridReader.h>
#include <vtkXMLRectilinearGridReader.h>
#include <vtkXMLHyperOctreeReader.h>
#include <vtkXMLCompositeDataReader.h>
#include <vtkXMLStructuredGridReader.h>
#include <vtkXMLImageDataReader.h>
#include <vtkDataSetReader.h>
#include <vtkDataSet.h>
#include <vtkUnstructuredGrid.h>
#include <vtkRectilinearGrid.h>
#include <vtkHyperOctree.h>
#include <vtkImageData.h>
#include <vtkPolyData.h>
#include <vtkStructuredGrid.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkFieldData.h>
#include <vtkCellTypes.h>
#include <vtksys/SystemTools.hxx>

#include <vtkstd/map>

template<class TReader> vtkDataSet *ReadAnXMLFile(const char*fileName)
{
  vtkSmartPointer<TReader> reader =
    vtkSmartPointer<TReader>::New();
  reader->SetFileName(fileName);
  reader->Update();
  reader->GetOutput()->Register(reader);
  return vtkDataSet::SafeDownCast(reader->GetOutput());
}

int main (int argc, char *argv[])
{
  if (argc < 2)
    {
    cerr << "Usage: " << argv[0] << " XMLFile1 XMLFile2 ..." << endl;
    }

  // Process each file on the command line
  int f = 1;
  while (f < argc)
    {
    vtkDataSet *dataSet;
    vtkstd::string extension =
      vtksys::SystemTools::GetFilenameLastExtension(argv[f]);
    // Dispatch based on the file extension
    if (extension == ".vtu")
      {
      dataSet = ReadAnXMLFile<vtkXMLUnstructuredGridReader> (argv[f]);
      }
    else if (extension == ".vtp")
      {
      dataSet = ReadAnXMLFile<vtkXMLPolyDataReader> (argv[f]);
      }
    else if (extension == ".vts")
      {
      dataSet = ReadAnXMLFile<vtkXMLStructuredGridReader> (argv[f]);
      }
    else if (extension == ".vtr")
      {
      dataSet = ReadAnXMLFile<vtkXMLRectilinearGridReader> (argv[f]);
      }
    else if (extension == ".vti")
      {
      dataSet = ReadAnXMLFile<vtkXMLImageDataReader> (argv[f]);
      }
    else if (extension == ".vto")
      {
      dataSet = ReadAnXMLFile<vtkXMLHyperOctreeReader> (argv[f]);
      }
    else if (extension == ".vtk")
      {
      dataSet = ReadAnXMLFile<vtkDataSetReader> (argv[f]);
      }
    else
      {
      cerr << argv[0] << " Unknown extenstion: " << extension << endl;
      return EXIT_FAILURE;
      }

    int numberOfCells = dataSet->GetNumberOfCells();
    int numberOfPoints = dataSet->GetNumberOfPoints();

    // Generate a report
    cout << "------------------------" << endl;
    cout << argv[f] << endl 
         << " contains a " << endl
         << dataSet->GetClassName() 
         <<  " that has " << numberOfCells << " cells"
         << " and " << numberOfPoints << " points." << endl;
    typedef vtkstd::map<int,int> CellContainer;
    CellContainer cellMap;
    for (int i = 0; i < numberOfCells; i++)
      {
      cellMap[dataSet->GetCellType(i)]++;
      }

    CellContainer::const_iterator it = cellMap.begin();
    while (it != cellMap.end())
      {
      cout << "\tCell type " 
           << vtkCellTypes::GetClassNameFromTypeId(it->first)
           << " occurs " << it->second << " times." << endl;
      ++it;
      }

    // Now check for point data
    vtkPointData *pd = dataSet->GetPointData();
    if (pd)
      {
      cout << " contains point data with " 
           << pd->GetNumberOfArrays() 
           << " arrays." << endl;
      for (int i = 0; i < pd->GetNumberOfArrays(); i++)
        {
        cout << "\tArray " << i 
             << " is named "
             << (pd->GetArrayName(i) ? pd->GetArrayName(i) : "NULL")
             << endl;
        }
      }
    // Now check for cell data
    vtkCellData *cd = dataSet->GetCellData();
    if (cd)
      {
      cout << " contains cell data with " 
           << cd->GetNumberOfArrays() 
           << " arrays." << endl;
      for (int i = 0; i < cd->GetNumberOfArrays(); i++)
        {
        cout << "\tArray " << i 
             << " is named "
             << (cd->GetArrayName(i) ? cd->GetArrayName(i) : "NULL")
             << endl;
        }
      }
    // Now check for field data
    if (dataSet->GetFieldData())
      {
      cout << " contains field data with " 
           << dataSet->GetFieldData()->GetNumberOfArrays() 
           << " arrays." << endl;
      for (int i = 0; i < dataSet->GetFieldData()->GetNumberOfArrays(); i++)
        {
        cout << "\tArray " << i 
             << " is named " << dataSet->GetFieldData()->GetArray(i)->GetName() 
             << endl;
        }
      }
    dataSet->Delete();
    f++;
    }
  return EXIT_SUCCESS;
}
