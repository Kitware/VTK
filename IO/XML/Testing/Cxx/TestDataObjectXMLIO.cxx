#include <vtkCellData.h>
#include <vtkCubeSource.h>
#include <vtkDataObjectWriter.h>
#include <vtkDelaunay3D.h>
#include <vtkDirectedGraph.h>
#include <vtkEdgeListIterator.h>
#include <vtkXMLGenericDataObjectReader.h>
#include <vtkXMLDataSetWriter.h>
#include <vtkGraph.h>
#include <vtkImageData.h>
#include <vtkImageNoiseSource.h>
#include <vtkIntArray.h>
#include <vtkMutableDirectedGraph.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkRandomGraphSource.h>
#include <vtkRectilinearGrid.h>
#include <vtkSmartPointer.h>
#include <vtkStructuredGrid.h>
#include <vtkTable.h>
#include <vtkTree.h>
#include <vtkUndirectedGraph.h>
#include <vtkUniformGrid.h>
#include <vtkUnstructuredGrid.h>
#include <vtkVariant.h>

namespace
{
void InitializeData(vtkImageData* Data)
{
  vtkImageNoiseSource* const source = vtkImageNoiseSource::New();
  source->SetWholeExtent(0, 15, 0, 15, 0, 0);
  source->Update();

  Data->ShallowCopy(source->GetOutput());
  source->Delete();
}

bool CompareData(vtkImageData* Output, vtkImageData* Input)
{
  if(memcmp(Input->GetDimensions(), Output->GetDimensions(), 3 * sizeof(int)))
    return false;

  const int point_count = Input->GetDimensions()[0] * Input->GetDimensions()[1] * Input->GetDimensions()[2];
  for(int point = 0; point != point_count; ++point)
    {
    if(memcmp(Input->GetPoint(point), Output->GetPoint(point), 3 * sizeof(double)))
      return false;
    }

  return true;
}

void InitializeData(vtkPolyData* Data)
{
  vtkCubeSource* const source = vtkCubeSource::New();
  source->Update();

  Data->ShallowCopy(source->GetOutput());
  source->Delete();
}

bool CompareData(vtkPolyData* Output, vtkPolyData* Input)
{
  if(Input->GetNumberOfPoints() != Output->GetNumberOfPoints())
    return false;
  if(Input->GetNumberOfPolys() != Output->GetNumberOfPolys())
    return false;

  return true;
}

void InitializeData(vtkRectilinearGrid* Data)
{
  Data->SetDimensions(2, 3, 4);
}

bool CompareData(vtkRectilinearGrid* Output, vtkRectilinearGrid* Input)
{
  if(memcmp(Input->GetDimensions(), Output->GetDimensions(), 3 * sizeof(int)))
    return false;

  return true;
}

void InitializeData(vtkUniformGrid* Data)
{
  InitializeData(static_cast<vtkImageData*>(Data));
}

void InitializeData(vtkUnstructuredGrid* Data)
{
  vtkCubeSource* const source = vtkCubeSource::New();
  vtkDelaunay3D* const delaunay = vtkDelaunay3D::New();
  delaunay->AddInputConnection(source->GetOutputPort());
  delaunay->Update();

  Data->ShallowCopy(delaunay->GetOutput());

  delaunay->Delete();
  source->Delete();
}

bool CompareData(vtkUnstructuredGrid* Output, vtkUnstructuredGrid* Input)
{
  if(Input->GetNumberOfPoints() != Output->GetNumberOfPoints())
    return false;
  if(Input->GetNumberOfCells() != Output->GetNumberOfCells())
    return false;

  return true;
}

template<typename DataT>
bool TestDataObjectXMLSerialization()
{
  DataT* const output_data = DataT::New();
  InitializeData(output_data);

  const char* const filename = output_data->GetClassName();

  vtkXMLDataSetWriter* const writer =
    vtkXMLDataSetWriter::New();
  writer->SetInputData(output_data);
  writer->SetFileName(filename);
  writer->Write();
  writer->Delete();

  vtkXMLGenericDataObjectReader* const reader =
    vtkXMLGenericDataObjectReader::New();
  reader->SetFileName(filename);
  reader->Update();

  vtkDataObject *obj = reader->GetOutput();
  DataT* input_data = DataT::SafeDownCast(obj);
  if(!input_data)
    {
    reader->Delete();
    output_data->Delete();
    return false;
    }

  const bool result = CompareData(output_data, input_data);

  reader->Delete();
  output_data->Delete();

  return result;
}

bool TestUniformGridXMLSerialization()
{
  vtkUniformGrid* const output_data = vtkUniformGrid::New();
  InitializeData(output_data);

  const char* const filename = output_data->GetClassName();

  vtkXMLDataSetWriter* const writer =
    vtkXMLDataSetWriter::New();
  writer->SetInputData(output_data);
  writer->SetFileName(filename);
  writer->Write();
  writer->Delete();

  vtkXMLGenericDataObjectReader* const reader =
    vtkXMLGenericDataObjectReader::New();
  reader->SetFileName(filename);
  reader->Update();

  vtkDataObject *obj = reader->GetOutput();
  vtkImageData* input_data = vtkImageData::SafeDownCast(obj);
  if(!input_data)
    {
    reader->Delete();
    output_data->Delete();
    return false;
    }

  const bool result = CompareData(output_data, input_data);

  reader->Delete();
  output_data->Delete();

  return result;
}
}
int TestDataObjectXMLIO(int /*argc*/, char* /*argv*/[])
{
  int result = 0;

  if(!TestDataObjectXMLSerialization<vtkImageData>())
    {
    cerr << "Error: failure serializing vtkImageData" << endl;
    result = 1;
    }
  if(!TestUniformGridXMLSerialization())
    {
    // note that the current output from serializing a vtkUniformGrid
    // is a vtkImageData. this is the same as writing out a
    // vtkUniformGrid using vtkXMLImageDataWriter.
    cerr << "Error: failure serializing vtkUniformGrid" << endl;
    result = 1;
    }
  if(!TestDataObjectXMLSerialization<vtkPolyData>())
    {
    cerr << "Error: failure serializing vtkPolyData" << endl;
    result = 1;
    }
  if(!TestDataObjectXMLSerialization<vtkRectilinearGrid>())
    {
    cerr << "Error: failure serializing vtkRectilinearGrid" << endl;
    result = 1;
    }
//  if(!TestDataObjectXMLSerialization<vtkStructuredGrid>())
//    {
//    cerr << "Error: failure serializing vtkStructuredGrid" << endl;
//    result = 1;
//    }
  if(!TestDataObjectXMLSerialization<vtkUnstructuredGrid>())
    {
    cerr << "Error: failure serializaing vtkUnstructuredGrid" << endl;
    result = 1;
    }

  return result;
}
