#include <vtkDataArray.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkSphereSource.h>
#include <vtkTesting.h>
#include <vtkXMLMultiBlockDataReader.h>
#include <vtkXMLMultiBlockDataWriter.h>

int TestMultiBlockXMLIOWithPartialArrays(int argc, char* argv[])
{
  vtkNew<vtkSphereSource> sp;
  sp->Update();

  vtkNew<vtkPolyData> pd0;
  pd0->DeepCopy(sp->GetOutput());

  vtkNew<vtkPolyData> pd1;
  pd1->DeepCopy(sp->GetOutput());
  pd1->GetPointData()->GetArray("Normals")->SetName("NewNormals");

  vtkNew<vtkMultiBlockDataSet> outMB;
  outMB->SetBlock(0, pd0);
  outMB->SetBlock(1, pd1);

  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, argv);

  std::ostringstream filename_stream;
  filename_stream << testing->GetTempDirectory() << "/TestMultiBlockXMLIOWithPartialArrays.vtm";

  vtkNew<vtkXMLMultiBlockDataWriter> writer;
  writer->SetFileName(filename_stream.str().c_str());
  writer->SetInputDataObject(outMB);
  writer->Write();

  vtkNew<vtkXMLMultiBlockDataReader> reader;
  reader->SetFileName(filename_stream.str().c_str());
  reader->Update();

  auto inMB = vtkMultiBlockDataSet::SafeDownCast(reader->GetOutputDataObject(0));
  if (inMB->GetNumberOfBlocks() != 2 || vtkPolyData::SafeDownCast(inMB->GetBlock(0)) == nullptr ||
    vtkPolyData::SafeDownCast(inMB->GetBlock(0))->GetPointData()->GetArray("Normals") == nullptr ||
    vtkPolyData::SafeDownCast(inMB->GetBlock(0))->GetPointData()->GetArray("NewNormals") !=
      nullptr ||
    vtkPolyData::SafeDownCast(inMB->GetBlock(1)) == nullptr ||
    vtkPolyData::SafeDownCast(inMB->GetBlock(1))->GetPointData()->GetArray("Normals") != nullptr ||
    vtkPolyData::SafeDownCast(inMB->GetBlock(1))->GetPointData()->GetArray("NewNormals") == nullptr)
  {
    cerr << "ERROR: In/out data mismatched!" << endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
