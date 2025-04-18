#include "vtkArrayCalculator.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkFloatArray.h"
#include "vtkGenerateIds.h"
#include "vtkMath.h"
#include "vtkMinimalStandardRandomSequence.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRandomAttributeGenerator.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"

#include <string>

static const std::string RANDOM_POINT_SCALARS_NAME = "RandomPointScalars";
static const std::string RANDOM_POINT_VECTORS_NAME = "RandomPointVectors";
static const std::string RANDOM_CELL_SCALARS_NAME = "RandomCellScalars";
static const std::string RANDOM_CELL_VECTORS_NAME = "RandomCellVectors";

//------------------------------------------------------------------------------
bool TestDataPresence(vtkRandomAttributeGenerator* randomFilter, const std::string& randomArrayName,
  const std::string& additionalArrayName, int attributeType)
{
  vtkPolyData* polyData = randomFilter->GetPolyDataOutput();

  vtkDataArray* randomDataArray =
    polyData->GetAttributes(attributeType)->GetArray(randomArrayName.c_str());
  vtkDataArray* additionalDataArray =
    polyData->GetAttributes(attributeType)->GetArray(additionalArrayName.c_str());

  if (!randomDataArray || !additionalDataArray)
  {
    std::cerr << "Unable to retrieve both " << randomArrayName << " and " << additionalArrayName;
    return false;
  }

  return true;
}

/**
 * Test if vtkRandomAttributeGenerator does not replace the active data array with the random data
 * array. This is achieved by appending dummy data before the Random Attribute filter and making
 * sure that both arrays exist in the end of the pipeline.
 */
bool TestRandomAttributesDataAppending(vtkAlgorithm* dataSource)
{
  vtkNew<vtkArrayCalculator> pointScalarsArrayCalculator;
  vtkNew<vtkArrayCalculator> pointVectorsArrayCalculator;
  vtkNew<vtkArrayCalculator> cellScalarsArrayCalculator;
  vtkNew<vtkArrayCalculator> cellVectorsArrayCalculator;
  vtkNew<vtkRandomAttributeGenerator> randomAttribFilter;

  pointScalarsArrayCalculator->SetInputConnection(dataSource->GetOutputPort());
  pointVectorsArrayCalculator->SetInputConnection(pointScalarsArrayCalculator->GetOutputPort());
  cellScalarsArrayCalculator->SetInputConnection(pointVectorsArrayCalculator->GetOutputPort());
  cellVectorsArrayCalculator->SetInputConnection(cellScalarsArrayCalculator->GetOutputPort());
  randomAttribFilter->SetInputConnection(cellVectorsArrayCalculator->GetOutputPort());

  std::string additionalPointScalarsName = "AdditionalPointScalars";
  std::string additionalPointVectorsName = "AdditionalPointVectors";
  std::string additionalCellScalarsName = "AdditionalCellScalars";
  std::string additionalCellVectorsName = "AdditionalCellVectors";
  pointScalarsArrayCalculator->SetAttributeTypeToPointData();
  pointScalarsArrayCalculator->SetResultArrayName(additionalPointScalarsName.c_str());
  pointScalarsArrayCalculator->SetFunction("1.0");
  pointVectorsArrayCalculator->SetAttributeTypeToPointData();
  pointVectorsArrayCalculator->SetResultArrayName(additionalPointVectorsName.c_str());
  pointVectorsArrayCalculator->SetFunction("iHat");
  cellScalarsArrayCalculator->SetAttributeTypeToCellData();
  cellScalarsArrayCalculator->SetResultArrayName(additionalCellScalarsName.c_str());
  cellScalarsArrayCalculator->SetFunction("2.0");
  cellVectorsArrayCalculator->SetAttributeTypeToCellData();
  cellVectorsArrayCalculator->SetResultArrayName(additionalCellVectorsName.c_str());
  cellVectorsArrayCalculator->SetFunction("jHat");
  randomAttribFilter->GenerateAllPointDataOn();
  randomAttribFilter->GenerateAllCellDataOn();

  randomAttribFilter->Update();

  bool success = true;
  success &= TestDataPresence(randomAttribFilter, RANDOM_POINT_SCALARS_NAME,
    additionalPointScalarsName, vtkDataObject::POINT);
  success &= TestDataPresence(randomAttribFilter, RANDOM_POINT_VECTORS_NAME,
    additionalPointVectorsName, vtkDataObject::POINT);
  success &= TestDataPresence(
    randomAttribFilter, RANDOM_CELL_SCALARS_NAME, additionalCellScalarsName, vtkDataObject::CELL);
  success &= TestDataPresence(
    randomAttribFilter, RANDOM_CELL_VECTORS_NAME, additionalCellVectorsName, vtkDataObject::CELL);

  return success;
}

//------------------------------------------------------------------------------
// Generates a sequence of random numbers starting from seed parameter.
// The range of the random number can be specified by randomMin and randomMAx
// The type and the number of component in the data array can be specified by the template
// parameters
template <typename DataArrayType, typename TupleType, int NumComponents>
vtkSmartPointer<vtkDataArray> GenerateRandomSequence(
  int seed, double randomMin, double randomMax, vtkIdType numberOfTuples)
{
  vtkNew<vtkMinimalStandardRandomSequence> randomSequence;

  // Important: this will set the seed without calling Next() 3 times
  randomSequence->SetSeedOnly(seed);

  vtkNew<DataArrayType> generatedScalars;
  generatedScalars->SetNumberOfComponents(NumComponents);
  generatedScalars->SetNumberOfTuples(numberOfTuples);
  // The usage of GetPointer() here is based on the implementation of vtkRandomAttributeFilter
  // internally. This is not the best way of doing it and should be reworked in the future.
  TupleType* buffer = generatedScalars->GetPointer(0);
  for (vtkIdType i = 0; i < numberOfTuples; i++)
  {
    for (vtkIdType compIdx = 0; compIdx < NumComponents; compIdx++)
    {
      randomSequence->Next();
      TupleType value = static_cast<TupleType>(randomSequence->GetRangeValue(randomMin, randomMax));
      buffer[i * NumComponents + compIdx] = value;
    }
  }

  return generatedScalars;
}

enum TestMode
{
  TEST_GENERATE_POINT_SCALARS = 0,
  TEST_GENERATE_POINT_VECTORS,
  TEST_GENERATE_CELL_SCALARS,
  TEST_GENERATE_CELL_VECTORS
};

void UpdatePipeline(vtkRandomAttributeGenerator* randomFilter, TestMode testMode)
{
  vtkTypeBool pointScalarsGenOn = testMode == TEST_GENERATE_POINT_SCALARS ? 1 : 0;
  vtkTypeBool pointVectorsGenOn = testMode == TEST_GENERATE_POINT_VECTORS ? 1 : 0;
  vtkTypeBool cellScalarsGenOn = testMode == TEST_GENERATE_CELL_SCALARS ? 1 : 0;
  vtkTypeBool cellVectorsGenOn = testMode == TEST_GENERATE_CELL_VECTORS ? 1 : 0;

  // Parameters are activated one at the time to generate
  // a sequence of random numbers only for one attribute.
  randomFilter->SetGeneratePointScalars(pointScalarsGenOn);
  randomFilter->SetGeneratePointVectors(pointVectorsGenOn);
  randomFilter->SetGenerateCellScalars(cellScalarsGenOn);
  randomFilter->SetGenerateCellVectors(cellVectorsGenOn);

  randomFilter->Update();
}

bool TestFilterRandomData(vtkRandomAttributeGenerator* randomFilter, int attributeType,
  const std::string& arrayName, int seed)
{
  vtkPolyData* randomFilterPolyData =
    vtkPolyData::SafeDownCast(randomFilter->GetOutputDataObject(0));
  vtkIdType numberOfTuples = attributeType == vtkDataObject::POINT
    ? randomFilterPolyData->GetNumberOfPoints()
    : randomFilterPolyData->GetNumberOfCells();
  double randomMin = randomFilter->GetMinimumComponentValue();
  double randomMax = randomFilter->GetMaximumComponentValue();

  vtkDataArray* randomFilterData =
    randomFilterPolyData->GetAttributes(attributeType)->GetArray(arrayName.c_str());

  if (!randomFilterData)
  {
    std::cerr << "Data array " << arrayName << " not found";
    return false;
  }

  vtkSmartPointer<vtkDataArray> generatedRandomData;
  if (arrayName == RANDOM_POINT_SCALARS_NAME || arrayName == RANDOM_CELL_SCALARS_NAME)
  {
    generatedRandomData =
      GenerateRandomSequence<vtkFloatArray, float, 1>(seed, randomMin, randomMax, numberOfTuples);
  }
  else if (arrayName == RANDOM_POINT_VECTORS_NAME || arrayName == RANDOM_CELL_VECTORS_NAME)
  {
    generatedRandomData =
      GenerateRandomSequence<vtkFloatArray, float, 3>(seed, randomMin, randomMax, numberOfTuples);
  }

  if (!generatedRandomData)
  {
    std::cerr << "the array generation for this test has not been implemented yet";
    return false;
  }

  bool sameArray = vtkTestUtilities::CompareAbstractArray(randomFilterData, generatedRandomData);
  return sameArray;
}

/**
 * Test wether the vtkRandomAttributeGenerator generates a random sequence of number and if the
 * output data exists.
 */
bool TestFilterRandomGeneration(vtkAlgorithm* dataSource)
{
  bool success = true;

  vtkNew<vtkRandomAttributeGenerator> randomAttributeFilter;
  randomAttributeFilter->SetInputConnection(dataSource->GetOutputPort());

  // Point scalars test
  {
    // capture seed before generating random data to replecate them and compare
    int seed = vtkMath::GetSeed();
    UpdatePipeline(randomAttributeFilter, TEST_GENERATE_POINT_SCALARS);
    success &= TestFilterRandomData(
      randomAttributeFilter, vtkDataObject::POINT, RANDOM_POINT_SCALARS_NAME, seed);
  }
  // Point vectors test
  {
    int seed = vtkMath::GetSeed();
    UpdatePipeline(randomAttributeFilter, TEST_GENERATE_POINT_VECTORS);
    success &= TestFilterRandomData(
      randomAttributeFilter, vtkDataObject::POINT, RANDOM_POINT_VECTORS_NAME, seed);
  }
  // Cell scalars test
  {
    int seed = vtkMath::GetSeed();
    UpdatePipeline(randomAttributeFilter, TEST_GENERATE_CELL_SCALARS);
    success &= TestFilterRandomData(
      randomAttributeFilter, vtkDataObject::CELL, RANDOM_CELL_SCALARS_NAME, seed);
  }
  // Cell vectors test
  {
    int seed = vtkMath::GetSeed();
    UpdatePipeline(randomAttributeFilter, TEST_GENERATE_CELL_VECTORS);
    success &= TestFilterRandomData(
      randomAttributeFilter, vtkDataObject::CELL, RANDOM_CELL_VECTORS_NAME, seed);
  }

  return success;
}

//------------------------------------------------------------------------------
int TestRandomAttributeGeneratorFilter(int, char*[])
{
  vtkNew<vtkSphereSource> sphereSource;

  bool success = true;

  success &= TestFilterRandomGeneration(sphereSource);
  success &= TestRandomAttributesDataAppending(sphereSource);

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
