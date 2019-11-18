#include <vtkDataSetAttributes.h>
#include <vtkFloatArray.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkNew.h>
#include <vtkTable.h>
#include <vtkTesting.h>
#include <vtkXMLMultiBlockDataReader.h>
#include <vtkXMLMultiBlockDataWriter.h>

int TestMultiBlockXMLIOWithPartialArraysTable(int argc, char* argv[])
{
  // Create a table with some points in it...
  vtkNew<vtkTable> table;
  vtkNew<vtkFloatArray> arrX;
  arrX->SetName("X Axis");
  table->AddColumn(arrX);
  vtkNew<vtkFloatArray> arrC;
  arrC->SetName("Cosine");
  table->AddColumn(arrC);
  vtkNew<vtkFloatArray> arrS;
  arrS->SetName("Sine");
  table->AddColumn(arrS);
  int numPoints = 69;
  float inc = 7.5 / (numPoints - 1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
  {
    table->SetValue(i, 0, i * inc);
    table->SetValue(i, 1, cos(i * inc) + 0.0);
    table->SetValue(i, 2, sin(i * inc) + 0.0);
  }

  vtkNew<vtkTable> table1;
  table1->DeepCopy(table);
  table1->GetRowData()->GetArray("Sine")->SetName("NewSine");

  vtkNew<vtkMultiBlockDataSet> outMB;
  outMB->SetBlock(0, table);
  outMB->SetBlock(1, table1);

  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, argv);

  std::ostringstream filename_stream;
  filename_stream << testing->GetTempDirectory()
                  << "/TestMultiBlockXMLIOWithPartialArraysTable.vtm";

  vtkNew<vtkXMLMultiBlockDataWriter> writer;
  writer->SetFileName(filename_stream.str().c_str());
  writer->SetInputDataObject(outMB);
  writer->Write();

  vtkNew<vtkXMLMultiBlockDataReader> reader;
  reader->SetFileName(filename_stream.str().c_str());
  reader->Update();

  auto inMB = vtkMultiBlockDataSet::SafeDownCast(reader->GetOutputDataObject(0));
  if (inMB->GetNumberOfBlocks() != 2 || vtkTable::SafeDownCast(inMB->GetBlock(0)) == nullptr ||
    vtkTable::SafeDownCast(inMB->GetBlock(0))->GetRowData()->GetArray("Sine") == nullptr ||
    vtkTable::SafeDownCast(inMB->GetBlock(0))->GetRowData()->GetArray("NewSine") != nullptr ||
    vtkTable::SafeDownCast(inMB->GetBlock(1)) == nullptr ||
    vtkTable::SafeDownCast(inMB->GetBlock(1))->GetRowData()->GetArray("Sine") != nullptr ||
    vtkTable::SafeDownCast(inMB->GetBlock(1))->GetRowData()->GetArray("NewSine") == nullptr)
  {
    cerr << "ERROR: In/out data mismatched!" << endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
