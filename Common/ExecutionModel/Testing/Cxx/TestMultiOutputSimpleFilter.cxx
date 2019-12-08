#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkFieldData.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPassInputTypeAlgorithm.h"
#include "vtkPolyData.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkUnsignedIntArray.h"
#include "vtkXMLGenericDataObjectReader.h"

#define VTK_SUCCESS 0
#define VTK_FAILURE 1
namespace
{

class vtkTestAlgorithm : public vtkPassInputTypeAlgorithm
{
public:
  static vtkTestAlgorithm* New();
  vtkTestAlgorithm(const vtkTestAlgorithm&) = delete;
  void operator=(const vtkTestAlgorithm&) = delete;

  vtkTypeMacro(vtkTestAlgorithm, vtkPassInputTypeAlgorithm);

protected:
  vtkTestAlgorithm()
    : Superclass()
  {
    this->SetNumberOfOutputPorts(2);
  }

  int FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info) override
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    return 1;
  }

  int FillOutputPortInformation(int port, vtkInformation* info) override
  {
    if (port == 0)
    {
      info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
      return 1;
    }
    else
    {
      return Superclass::FillOutputPortInformation(port, info);
    }
  }

  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override
  {
    int success = Superclass::RequestDataObject(request, inputVector, outputVector);
    vtkDataObject* output = vtkDataObject::GetData(outputVector, 0);
    if (!output || !vtkPolyData::SafeDownCast(output))
    {
      vtkNew<vtkPolyData> newOutput;
      outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(), newOutput);
    }
    return success;
  }

  int RequestData(vtkInformation* vtkNotUsed(request), vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override
  {
    vtkDataSet* input = vtkDataSet::GetData(inputVector[0], 0);
    double bounds[6];
    input->GetBounds(bounds);
    vtkNew<vtkSphereSource> sphere;
    sphere->SetCenter(bounds[0], bounds[2], bounds[4]);
    sphere->SetRadius(bounds[1] - bounds[0]);
    sphere->Update();

    vtkPolyData* polyOut = vtkPolyData::GetData(outputVector, 0);
    polyOut->ShallowCopy(sphere->GetOutput());

    polyOut->GetFieldData()->PassData(input->GetFieldData());

    vtkDataSet* output = vtkDataSet::GetData(outputVector, 1);
    output->ShallowCopy(input);
    return 1;
  }
};

vtkStandardNewMacro(vtkTestAlgorithm);

void AddPerBlockFieldData(vtkCompositeDataSet* data)
{
  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(data->NewIterator());
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkDataObject* currentData = iter->GetCurrentDataObject();
    if (vtkDataSet::SafeDownCast(currentData))
    {
      vtkFieldData* fd = currentData->GetFieldData();
      if (!fd)
      {
        vtkNew<vtkFieldData> fieldData;
        currentData->SetFieldData(fieldData);
        fd = fieldData;
      }
      vtkNew<vtkUnsignedIntArray> array;
      array->SetNumberOfComponents(1);
      array->SetNumberOfTuples(1);
      array->SetValue(0, iter->GetCurrentFlatIndex());
      array->SetName("compositeIndexBasedData");
      fd->AddArray(array);
      std::cout << "Assinging field data " << iter->GetCurrentFlatIndex() << std::endl;
    }
  }
}

bool CheckPerBlockFieldData(vtkCompositeDataSet* data)
{
  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(data->NewIterator());
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkDataObject* currentData = iter->GetCurrentDataObject();
    if (vtkDataSet::SafeDownCast(currentData))
    {
      vtkFieldData* fd = currentData->GetFieldData();
      if (!fd)
      {
        // field data didn't propagate
        std::cout << "No field data!" << std::endl;
        return false;
      }
      vtkUnsignedIntArray* array =
        vtkUnsignedIntArray::SafeDownCast(fd->GetArray("compositeIndexBasedData"));
      if (!array)
      {
        // array type changed
        std::cout << "Expected field data array not found!" << std::endl;
        return false;
      }
      unsigned value = array->GetValue(0);
      if (value != iter->GetCurrentFlatIndex())
      {
        // data changed
        std::cout << "Field data didn't match, should be " << iter->GetCurrentFlatIndex()
                  << " but was " << value << std::endl;
        return false;
      }
    }
  }
  return true;
}

int TestComposite(std::string& inputDataFile, bool isAMR)
{
  vtkNew<vtkXMLGenericDataObjectReader> reader;
  reader->SetFileName(inputDataFile.c_str());
  reader->Update();

  vtkCompositeDataSet* data = vtkCompositeDataSet::SafeDownCast(reader->GetOutput());

  AddPerBlockFieldData(data);

  vtkNew<vtkTestAlgorithm> testAlg;
  testAlg->SetInputData(data);
  testAlg->Update();

  int retVal = VTK_SUCCESS;

  vtkDataObject* data0 = testAlg->GetOutputDataObject(0);
  vtkDataObject* data1 = testAlg->GetOutputDataObject(1);
  if (!vtkMultiBlockDataSet::SafeDownCast(data0))
  {
    std::cout << "Error: output 0 is not multiblock after composite data pipeline run" << std::endl;
    std::cout << "instead it is " << data0->GetClassName() << std::endl;
    retVal = VTK_FAILURE;
  }
  if (!isAMR)
  {
    // This test doesn't work on AMR data... only the root block has field data and that field data
    // is copied to
    // all output blocks.
    if (retVal == VTK_SUCCESS && !CheckPerBlockFieldData(vtkCompositeDataSet::SafeDownCast(data0)))
    {
      std::cout << "Per block field data for the first output port changed" << std::endl;
      retVal = VTK_FAILURE;
    }
    if (!vtkMultiBlockDataSet::SafeDownCast(data1))
    {
      std::cout << "Error: output 1 is not multiblock after composite data pipeline run"
                << std::endl;
      std::cout << "instead it is " << data1->GetClassName() << std::endl;
      retVal = VTK_FAILURE;
    }
  }
  else
  {
    if (!vtkHierarchicalBoxDataSet::SafeDownCast(data1))
    {
      std::cout << "Error: output 1 is not an AMR dataset after composite data pipeline run"
                << std::endl;
      std::cout << "instead it is " << data1->GetClassName() << std::endl;
      retVal = VTK_FAILURE;
    }
  }
  if (retVal == VTK_SUCCESS && !CheckPerBlockFieldData(vtkCompositeDataSet::SafeDownCast(data1)))
  {
    std::cout << "Per block field data for the second output port changed" << std::endl;
    retVal = VTK_FAILURE;
  }

  // Exercise NewInstance for coverage.
  auto dummy = testAlg->NewInstance();
  dummy->Delete();

  return retVal;
}
}

int TestMultiOutputSimpleFilter(int argc, char* argv[])
{
  char const* tmp =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/AMR/HierarchicalBoxDataset.v1.1.vthb");
  std::string inputAMR = tmp;
  delete[] tmp;

  tmp = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/many_blocks/many_blocks.vtm");
  std::string inputMultiblock = tmp;
  delete[] tmp;

  int retVal = TestComposite(inputAMR, true);

  retVal |= TestComposite(inputMultiblock, false);

  return retVal;
}
