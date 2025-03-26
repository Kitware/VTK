// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkExtractTensorComponents.h"

#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSMPTools.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkExtractTensorComponents);

//------------------------------------------------------------------------------
namespace
{
vtkDataArray* CreateDataArray(int type, const char* name)
{
  vtkDataArray* da;
  if (type == VTK_DOUBLE)
  {
    da = vtkDoubleArray::New();
  }
  else
  {
    da = vtkFloatArray::New();
  }
  da->SetName(name);
  return da;
}
}

//------------------------------------------------------------------------------
// Construct object to extract nothing and to not pass tensor data
// through the pipeline.
vtkExtractTensorComponents::vtkExtractTensorComponents()
{
  this->PassTensorsToOutput = 0;

  this->ExtractScalars = 0;
  this->ExtractVectors = 0;
  this->ExtractNormals = 0;
  this->ExtractTCoords = 0;

  this->ScalarMode = VTK_EXTRACT_COMPONENT;
  this->ScalarComponents[0] = this->ScalarComponents[1] = 0;

  this->VectorComponents[0] = 0;
  this->VectorComponents[1] = 0;
  this->VectorComponents[2] = 1;
  this->VectorComponents[3] = 0;
  this->VectorComponents[4] = 2;
  this->VectorComponents[5] = 0;

  this->NormalizeNormals = 1;
  this->NormalComponents[0] = 0;
  this->NormalComponents[1] = 1;
  this->NormalComponents[2] = 1;
  this->NormalComponents[3] = 1;
  this->NormalComponents[4] = 2;
  this->NormalComponents[5] = 1;

  this->NumberOfTCoords = 2;
  this->TCoordComponents[0] = 0;
  this->TCoordComponents[1] = 2;
  this->TCoordComponents[2] = 1;
  this->TCoordComponents[3] = 2;
  this->TCoordComponents[4] = 2;
  this->TCoordComponents[5] = 2;

  this->OutputPrecision = vtkAlgorithm::DEFAULT_PRECISION;
}

//------------------------------------------------------------------------------
// Extract data from tensors.
//
int vtkExtractTensorComponents::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataArray* inTensors;
  vtkPointData* pd = input->GetPointData();
  vtkPointData* outPD = output->GetPointData();
  vtkDataArray* newScalars = nullptr;
  vtkDataArray* newVectors = nullptr;
  vtkDataArray* newNormals = nullptr;
  vtkDataArray* newTCoords = nullptr;
  vtkIdType numPts;

  // Initialize
  //
  vtkDebugMacro(<< "Extracting tensor components!");

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  inTensors = pd->GetTensors();
  numPts = input->GetNumberOfPoints();

  if (!inTensors || numPts < 1)
  {
    vtkErrorMacro(<< "No data to extract!");
    return 1;
  }

  if (!this->ExtractScalars && !this->ExtractVectors && !this->ExtractNormals &&
    !this->ExtractTCoords)
  {
    vtkWarningMacro(<< "No data is being extracted");
  }

  int precisionType = (this->OutputPrecision == vtkAlgorithm::DEFAULT_PRECISION
      ? inTensors->GetDataType()
      : (this->OutputPrecision == vtkAlgorithm::SINGLE_PRECISION ? VTK_FLOAT : VTK_DOUBLE));

  outPD->CopyAllOn();
  if (!this->PassTensorsToOutput)
  {
    outPD->CopyTensorsOff();
  }
  if (this->ExtractScalars)
  {
    outPD->CopyScalarsOff();
    newScalars = CreateDataArray(precisionType,
      this->ScalarMode == VTK_EXTRACT_COMPONENT
        ? "Tensor Component"
        : (this->ScalarMode == VTK_EXTRACT_DETERMINANT
              ? "Tensor Determinant"
              : (this->ScalarMode == VTK_EXTRACT_NONNEGATIVE_DETERMINANT
                    ? "NonNegative Tensor Determinant"
                    : "Tensor Effective Stress")));
    newScalars->SetNumberOfTuples(numPts);
  }
  if (this->ExtractVectors)
  {
    outPD->CopyVectorsOff();
    newVectors = CreateDataArray(precisionType, "TensorVectors");
    newVectors->SetNumberOfComponents(3);
    newVectors->SetNumberOfTuples(numPts);
  }
  if (this->ExtractNormals)
  {
    outPD->CopyNormalsOff();
    newNormals = CreateDataArray(precisionType, "TensorNormals");
    newNormals->SetNumberOfComponents(3);
    newNormals->SetNumberOfTuples(numPts);
  }
  if (this->ExtractTCoords)
  {
    outPD->CopyTCoordsOff();
    newTCoords = CreateDataArray(precisionType, "TensorTCoords");
    newTCoords->SetNumberOfComponents(2);
    newTCoords->SetNumberOfTuples(numPts);
  }
  outPD->PassData(pd);

  // Loop over all points extracting components of tensor as directed. Use a
  // lambda function.
  //
  vtkSMPTools::For(0, numPts, [&](vtkIdType ptId, vtkIdType endPtId) {
    double tensor[9];
    double s = 0.0;
    double v[3];
    double sx, sy, sz, txy, tyz, txz;
    bool isFirst = vtkSMPTools::GetSingleThread();
    vtkIdType checkAbortInterval = std::min((endPtId - ptId) / 10 + 1, (vtkIdType)1000);

    for (; ptId < endPtId; ++ptId)
    {
      if (ptId % checkAbortInterval == 0)
      {
        if (isFirst)
        {
          this->CheckAbort();
        }
        if (this->GetAbortOutput())
        {
          break;
        }
      }

      inTensors->GetTuple(ptId, tensor);
      if (inTensors->GetNumberOfComponents() == 6)
      {
        vtkMath::TensorFromSymmetricTensor(tensor);
      }

      if (this->ExtractScalars)
      {
        if (this->ScalarMode == VTK_EXTRACT_EFFECTIVE_STRESS)
        {
          sx = tensor[0];
          sy = tensor[4];
          sz = tensor[8];
          txy = tensor[3];
          tyz = tensor[7];
          txz = tensor[6];

          s = sqrt(0.16666667 *
            ((sx - sy) * (sx - sy) + (sy - sz) * (sy - sz) + (sz - sx) * (sz - sx) +
              6.0 * (txy * txy + tyz * tyz + txz * txz)));
        }

        else if (this->ScalarMode == VTK_EXTRACT_COMPONENT)
        {
          s = tensor[this->ScalarComponents[0] + 3 * this->ScalarComponents[1]];
        }

        else if (this->ScalarMode == VTK_EXTRACT_DETERMINANT)
        {
          s = tensor[0] * tensor[4] * tensor[8] - tensor[0] * tensor[5] * tensor[7] -
            tensor[1] * tensor[3] * tensor[8] + tensor[1] * tensor[5] * tensor[6] +
            tensor[2] * tensor[3] * tensor[7] - tensor[2] * tensor[4] * tensor[6];
        }
        else if (this->ScalarMode == VTK_EXTRACT_NONNEGATIVE_DETERMINANT)
        {
          s = fabs(tensor[0] * tensor[4] * tensor[8] - tensor[0] * tensor[5] * tensor[7] -
            tensor[1] * tensor[3] * tensor[8] + tensor[1] * tensor[5] * tensor[6] +
            tensor[2] * tensor[3] * tensor[7] - tensor[2] * tensor[4] * tensor[6]);
        }
        else // if ( this->ScalarMode == VTK_EXTRACT_TRACE )
        {
          s = tensor[0] + tensor[4] + tensor[8];
        }
        newScalars->SetTuple(ptId, &s);
      } // if extract scalars

      if (this->ExtractVectors)
      {
        v[0] = tensor[this->VectorComponents[0] + 3 * this->VectorComponents[1]];
        v[1] = tensor[this->VectorComponents[2] + 3 * this->VectorComponents[3]];
        v[2] = tensor[this->VectorComponents[4] + 3 * this->VectorComponents[5]];
        newVectors->SetTuple(ptId, v);
      }

      if (this->ExtractNormals)
      {
        v[0] = tensor[this->NormalComponents[0] + 3 * this->NormalComponents[1]];
        v[1] = tensor[this->NormalComponents[2] + 3 * this->NormalComponents[3]];
        v[2] = tensor[this->NormalComponents[4] + 3 * this->NormalComponents[5]];
        newNormals->SetTuple(ptId, v);
      }

      if (this->ExtractTCoords)
      {
        for (int i = 0; i < this->NumberOfTCoords; i++)
        {
          v[i] = tensor[this->TCoordComponents[2 * i] + 3 * this->TCoordComponents[2 * i + 1]];
        }
        newTCoords->SetTuple(ptId, v);
      }
    } // for all points in this batch
  }); // end lambda

  // Send data to output
  //
  if (this->ExtractScalars)
  {
    int idx = outPD->AddArray(newScalars);
    outPD->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
    newScalars->Delete();
  }
  if (this->ExtractVectors)
  {
    outPD->SetVectors(newVectors);
    newVectors->Delete();
  }
  if (this->ExtractNormals)
  {
    outPD->SetNormals(newNormals);
    newNormals->Delete();
  }
  if (this->ExtractTCoords)
  {
    outPD->SetTCoords(newTCoords);
    newTCoords->Delete();
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkExtractTensorComponents::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Pass Tensors To Output: " << (this->PassTensorsToOutput ? "On\n" : "Off\n");

  os << indent << "Extract Scalars: " << (this->ExtractScalars ? "On\n" : "Off\n");

  os << indent << "Scalar Extraction Mode: ";

  if (this->ScalarMode == VTK_EXTRACT_COMPONENT)
  {
    os << "VTK_EXTRACT_COMPONENT\n";
  }
  else if (this->ScalarMode == VTK_EXTRACT_EFFECTIVE_STRESS)
  {
    os << "VTK_EXTRACT_EFFECTIVE_STRESS\n";
  }
  else if (this->ScalarMode == VTK_EXTRACT_DETERMINANT)
  {
    os << "VTK_EXTRACT_DETERMINANT\n";
  }
  else if (this->ScalarMode == VTK_EXTRACT_NONNEGATIVE_DETERMINANT)
  {
    os << "VTK_EXTRACT_NONNEGATIVE_DETERMINANT\n";
  }
  else // if (this->ScalarMode == VTK_EXTRACT_TRACE)
  {
    os << "VTK_EXTRACT_TRACE\n";
  }

  os << indent << "Scalar Components: \n";
  os << indent << "  (row,column): (" << this->ScalarComponents[0] << ", "
     << this->ScalarComponents[1] << ")\n";

  os << indent << "Extract Vectors: " << (this->ExtractVectors ? "On\n" : "Off\n");
  os << indent << "Vector Components: \n";
  os << indent << "  (row,column)0: (" << this->VectorComponents[0] << ", "
     << this->VectorComponents[1] << ")\n";
  os << indent << "  (row,column)1: (" << this->VectorComponents[2] << ", "
     << this->VectorComponents[3] << ")\n";
  os << indent << "  (row,column)2: (" << this->VectorComponents[4] << ", "
     << this->VectorComponents[5] << ")\n";

  os << indent << "Extract Normals: " << (this->ExtractNormals ? "On\n" : "Off\n");
  os << indent << "Normalize Normals: " << (this->NormalizeNormals ? "On\n" : "Off\n");
  os << indent << "Normal Components: \n";
  os << indent << "  (row,column)0: (" << this->NormalComponents[0] << ", "
     << this->NormalComponents[1] << ")\n";
  os << indent << "  (row,column)1: (" << this->NormalComponents[2] << ", "
     << this->NormalComponents[3] << ")\n";
  os << indent << "  (row,column)2: (" << this->NormalComponents[4] << ", "
     << this->NormalComponents[5] << ")\n";

  os << indent << "Extract TCoords: " << (this->ExtractTCoords ? "On\n" : "Off\n");
  os << indent << "Number Of TCoords: (" << this->NumberOfTCoords << ")\n";
  os << indent << "TCoord Components: \n";
  os << indent << "  (row,column)0: (" << this->TCoordComponents[0] << ", "
     << this->TCoordComponents[1] << ")\n";
  os << indent << "  (row,column)1: (" << this->TCoordComponents[2] << ", "
     << this->TCoordComponents[3] << ")\n";
  os << indent << "  (row,column)2: (" << this->TCoordComponents[4] << ", "
     << this->TCoordComponents[5] << ")\n";

  os << indent << "Output Precision: " << this->OutputPrecision << "\n";
}
VTK_ABI_NAMESPACE_END
