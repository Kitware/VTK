#include <vtkCellData.h>
#include <vtkCubeSource.h>
#include <vtkDataObjectWriter.h>
#include <vtkDelaunay3D.h>
#include <vtkDirectedGraph.h>
#include <vtkEdgeListIterator.h>
#include <vtkGenericDataObjectReader.h>
#include <vtkGenericDataObjectWriter.h>
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
#include <vtkUnstructuredGrid.h>
#include <vtkVariant.h>

void InitializeData(vtkDirectedGraph* Data)
{
  vtkRandomGraphSource* const source = vtkRandomGraphSource::New();
  source->SetNumberOfVertices(5);
  source->SetNumberOfEdges(10);
  source->IncludeEdgeWeightsOn();
  source->DirectedOn();
  source->UseEdgeProbabilityOff();
  source->StartWithTreeOff();
  source->AllowSelfLoopsOff();
  source->Update();
  
  Data->ShallowCopy(source->GetOutput());
  source->Delete();
}

void InitializeData(vtkUndirectedGraph* Data)
{
  vtkRandomGraphSource* const source = vtkRandomGraphSource::New();
  source->SetNumberOfVertices(5);
  source->SetNumberOfEdges(10);
  source->IncludeEdgeWeightsOn();
  source->DirectedOff();
  source->UseEdgeProbabilityOff();
  source->StartWithTreeOff();
  source->AllowSelfLoopsOff();
  source->Update();
  
  Data->ShallowCopy(source->GetOutput());
  source->Delete();
}

bool CompareData(vtkGraph* Output, vtkGraph* Input)
{
  bool inputDirected = (vtkDirectedGraph::SafeDownCast(Input) != 0);
  bool outputDirected = (vtkDirectedGraph::SafeDownCast(Output) != 0);
  if(inputDirected != outputDirected)
    return false;

  if(Input->GetNumberOfVertices() != Output->GetNumberOfVertices())
    return false;
    
  if(Input->GetNumberOfEdges() != Output->GetNumberOfEdges())
    return false;

  if(Input->GetVertexData()->GetNumberOfArrays() != Output->GetVertexData()->GetNumberOfArrays())
    return false;
    
  if(Input->GetEdgeData()->GetNumberOfArrays() != Output->GetEdgeData()->GetNumberOfArrays())
    return false;

  vtkEdgeListIterator *inputEdges = vtkEdgeListIterator::New();
  vtkEdgeListIterator *outputEdges = vtkEdgeListIterator::New();
  while(inputEdges->HasNext())
    {
    vtkEdgeType inputEdge = inputEdges->Next();
    vtkEdgeType outputEdge = outputEdges->Next();
    if(inputEdge.Source != outputEdge.Source)
      return false;
      
    if(inputEdge.Target != outputEdge.Target)
      return false;

    if(inputEdge.Id != outputEdge.Id)
      return false;
    }
  inputEdges->Delete();
  outputEdges->Delete();

  return true;
}

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

void InitializeData(vtkStructuredGrid* Data)
{
  Data->SetDimensions(2, 3, 4);
}

bool CompareData(vtkStructuredGrid* Output, vtkStructuredGrid* Input)
{
  if(memcmp(Input->GetDimensions(), Output->GetDimensions(), 3 * sizeof(int)))
    return false;
    
  return true;
}

void InitializeData(vtkTable* Data)
{
  vtkIntArray* const column1 = vtkIntArray::New();
  Data->AddColumn(column1);
  column1->Delete();
  column1->SetName("column1");
  
  vtkIntArray* const column2 = vtkIntArray::New();
  Data->AddColumn(column2);
  column2->Delete();
  column2->SetName("column2");
  
  Data->InsertNextBlankRow();
  Data->InsertNextBlankRow();
  Data->InsertNextBlankRow();
  
  Data->SetValue(0, 0, 1);
  Data->SetValue(0, 1, 2);
  Data->SetValue(1, 0, 3);
  Data->SetValue(1, 1, 4);
  Data->SetValue(2, 0, 5);
  Data->SetValue(2, 1, 6);
}

bool CompareData(vtkTable* Output, vtkTable* Input)
{
  if(Input->GetNumberOfColumns() != Output->GetNumberOfColumns())
    return false;
  if(Input->GetNumberOfRows() != Output->GetNumberOfRows())
    return false;
    
  for(int column = 0; column != Input->GetNumberOfColumns(); ++column)
    {
    for(int row = 0; row != Input->GetNumberOfRows(); ++row)
      {
      if(Input->GetValue(row, column).ToDouble() != Output->GetValue(row, column).ToDouble())
        {
        return false;
        }
      }
    }

  return true;
}

void InitializeData(vtkTree* Data)
{
  vtkMutableDirectedGraph *g = vtkMutableDirectedGraph::New();
  for (vtkIdType i = 0; i < 5; ++i)
    {
    g->AddVertex();
    }
  g->AddEdge(2, 0);
  g->AddEdge(0, 1);
  g->AddEdge(0, 3);
  g->AddEdge(0, 4);

  if (!Data->CheckedShallowCopy(g))
    {
    cerr << "Invalid tree structure." << endl;
    }

  g->Delete();
}

bool CompareData(vtkTree* Output, vtkTree* Input)
{
  if(Input->GetNumberOfVertices() != Output->GetNumberOfVertices())
    return false;
    
  if(Input->GetNumberOfEdges() != Output->GetNumberOfEdges())
    return false;

  if(Input->GetVertexData()->GetNumberOfArrays() != Output->GetVertexData()->GetNumberOfArrays())
    return false;
    
  if(Input->GetEdgeData()->GetNumberOfArrays() != Output->GetEdgeData()->GetNumberOfArrays())
    return false;
  
  if(Input->GetRoot() != Output->GetRoot())
    return false;
  
  for(vtkIdType child = 0; child != Input->GetNumberOfVertices(); ++child)
    {
    if(Input->GetParent(child) != Output->GetParent(child))
      return false;
    }
  
  return true;
}

void InitializeData(vtkUnstructuredGrid* Data)
{
  vtkCubeSource* const source = vtkCubeSource::New();
  vtkDelaunay3D* const delaunay = vtkDelaunay3D::New();
  delaunay->AddInput(source->GetOutput());
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
bool TestDataObjectSerialization()
{
  DataT* const output_data = DataT::New();
  InitializeData(output_data);

  const char* const filename = output_data->GetClassName();
  
  vtkGenericDataObjectWriter* const writer = vtkGenericDataObjectWriter::New();
  writer->SetInput(output_data);
  writer->SetFileName(filename);
  writer->Write();
  writer->Delete();
  
  vtkGenericDataObjectReader* const reader = vtkGenericDataObjectReader::New();
  reader->SetFileName(filename);
  reader->Update();
  
  vtkDataObject *obj = reader->GetOutput();
  DataT* const input_data = DataT::SafeDownCast(obj);
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

int TestDataObjectIO(int /*argc*/, char* /*argv*/[])
{
  int result = 0;
  
  if(!TestDataObjectSerialization<vtkDirectedGraph>())
    {
    cerr << "Error: failure serializing vtkDirectedGraph" << endl;
    result = 1;
    }
  if(!TestDataObjectSerialization<vtkUndirectedGraph>())
    {
    cerr << "Error: failure serializing vtkUndirectedGraph" << endl;
    result = 1;
    }
  if(!TestDataObjectSerialization<vtkImageData>())
    {
    cerr << "Error: failure serializing vtkImageData" << endl;
    result = 1;
    }
  if(!TestDataObjectSerialization<vtkPolyData>())
    {
    cerr << "Error: failure serializing vtkPolyData" << endl;
    result = 1;
    }
  if(!TestDataObjectSerialization<vtkRectilinearGrid>())
    {
    cerr << "Error: failure serializing vtkRectilinearGrid" << endl;
    result = 1;
    }
  if(!TestDataObjectSerialization<vtkStructuredGrid>())
    {
    cerr << "Error: failure serializing vtkStructuredGrid" << endl;
    result = 1;
    }
  if(!TestDataObjectSerialization<vtkTable>())
    {
    cerr << "Error: failure serializing vtkTable" << endl;
    result = 1;
    }
  if(!TestDataObjectSerialization<vtkTree>())
    {
    cerr << "Error: failure serializing vtkTree" << endl;
    result = 1;
    }
  if(!TestDataObjectSerialization<vtkUnstructuredGrid>())
    {
    cerr << "Error: failure serializaing vtkUnstructuredGrid" << endl;
    result = 1;
    }

  return result;
}
