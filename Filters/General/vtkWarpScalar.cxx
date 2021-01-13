/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWarpScalar.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWarpScalar.h"

#include "vtkArrayDispatch.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSetAttributes.h"
#include "vtkImageData.h"
#include "vtkImageDataToPointSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkRectilinearGrid.h"
#include "vtkRectilinearGridToPointSet.h"

#include "vtkNew.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkWarpScalar);

//------------------------------------------------------------------------------
vtkWarpScalar::vtkWarpScalar()
{
  this->ScaleFactor = 1.0;
  this->UseNormal = 0;
  this->Normal[0] = 0.0;
  this->Normal[1] = 0.0;
  this->Normal[2] = 1.0;
  this->XYPlane = 0;
  this->OutputPointsPrecision = vtkAlgorithm::DEFAULT_PRECISION;

  // by default process active point scalars
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
}

//------------------------------------------------------------------------------
vtkWarpScalar::~vtkWarpScalar() = default;

//------------------------------------------------------------------------------
int vtkWarpScalar::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkRectilinearGrid");
  return 1;
}

//------------------------------------------------------------------------------
int vtkWarpScalar::RequestDataObject(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkImageData* inImage = vtkImageData::GetData(inputVector[0]);
  vtkRectilinearGrid* inRect = vtkRectilinearGrid::GetData(inputVector[0]);

  if (inImage || inRect)
  {
    vtkStructuredGrid* output = vtkStructuredGrid::GetData(outputVector);
    if (!output)
    {
      vtkNew<vtkStructuredGrid> newOutput;
      outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(), newOutput);
    }
    return 1;
  }
  else
  {
    return this->Superclass::RequestDataObject(request, inputVector, outputVector);
  }
}

// Core methods to scale points with scalars
namespace
{ // anonymous

struct ScaleWorker
{
  template <typename InPT, typename OutPT, typename ST>
  void operator()(InPT* inPts, OutPT* outPts, ST* scalars, vtkWarpScalar* self, double sf,
    bool XYPlane, vtkDataArray* inNormals, double* normal)

  {
    vtkIdType numPts = inPts->GetNumberOfTuples();
    const auto ipts = vtk::DataArrayTupleRange<3>(inPts);
    auto opts = vtk::DataArrayTupleRange<3>(outPts);
    const auto sRange = vtk::DataArrayTupleRange(scalars);

    // For smaller data sizes, serial processing is faster than spinning up
    // threads. The cutoff point between serial and threaded is empirical and
    // is likely to change.
    static constexpr int VTK_SMP_THRESHOLD = 750000;
    if (numPts >= VTK_SMP_THRESHOLD)
    {
      vtkSMPTools::For(0, numPts, [&](vtkIdType ptId, vtkIdType endPtId) {
        double s, *n = normal, inNormal[3];
        for (; ptId < endPtId; ++ptId)
        {
          const auto xi = ipts[ptId];
          auto xo = opts[ptId];

          if (XYPlane)
          {
            s = xi[2];
          }
          else
          {
            const auto sval = sRange[ptId];
            s = sval[0]; // 0th component of the tuple
          }

          if (inNormals)
          {
            inNormals->GetTuple(ptId, inNormal);
            n = inNormal;
          }

          xo[0] = xi[0] + sf * s * n[0];
          xo[1] = xi[1] + sf * s * n[1];
          xo[2] = xi[2] + sf * s * n[2];
        }
      }); // lambda
    }     // threaded

    else // serial
    {
      double s, *n = normal, inNormal[3];
      for (vtkIdType ptId = 0; ptId < numPts; ptId++)
      {
        if (!(ptId % 10000))
        {
          self->UpdateProgress((double)ptId / numPts);
          if (self->GetAbortExecute())
          {
            break;
          }
        }

        const auto xi = ipts[ptId];
        auto xo = opts[ptId];

        if (XYPlane)
        {
          s = xi[2];
        }
        else
        {
          const auto sval = sRange[ptId];
          s = sval[0]; // 0th component of the tuple
        }

        if (inNormals)
        {
          inNormals->GetTuple(ptId, inNormal);
          n = inNormal;
        }

        xo[0] = xi[0] + sf * s * n[0];
        xo[1] = xi[1] + sf * s * n[1];
        xo[2] = xi[2] + sf * s * n[2];
      } // over all points
    }   // serial processing
  }
};

} // anonymous namespace

//------------------------------------------------------------------------------
int vtkWarpScalar::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkSmartPointer<vtkPointSet> input = vtkPointSet::GetData(inputVector[0]);
  vtkPointSet* output = vtkPointSet::GetData(outputVector);

  if (!input)
  {
    // Try converting image data.
    vtkImageData* inImage = vtkImageData::GetData(inputVector[0]);
    if (inImage)
    {
      vtkNew<vtkImageDataToPointSet> image2points;
      image2points->SetInputData(inImage);
      image2points->Update();
      input = image2points->GetOutput();
    }
  }

  if (!input)
  {
    // Try converting rectilinear grid.
    vtkRectilinearGrid* inRect = vtkRectilinearGrid::GetData(inputVector[0]);
    if (inRect)
    {
      vtkNew<vtkRectilinearGridToPointSet> rect2points;
      rect2points->SetInputData(inRect);
      rect2points->Update();
      input = rect2points->GetOutput();
    }
  }

  if (!input)
  {
    vtkErrorMacro(<< "Invalid or missing input");
    return 0;
  }

  vtkPoints* inPts;
  vtkDataArray* inNormals;
  vtkDataArray* inScalars;
  vtkIdType numPts;

  vtkDebugMacro(<< "Warping data with scalars");

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  inPts = input->GetPoints();
  inNormals = input->GetPointData()->GetNormals();
  inScalars = this->GetInputArrayToProcess(0, inputVector);
  if (!inPts || !inScalars)
  {
    vtkDebugMacro(<< "No data to warp");
    return 1;
  }

  numPts = inPts->GetNumberOfPoints();

  // Create the output points. Backward compatibility requires the
  // output type to be float - this can be overridden.
  vtkNew<vtkPoints> newPts;
  if (this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION ||
    this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
  {
    newPts->SetDataType(VTK_FLOAT);
  }
  else
  {
    newPts->SetDataType(VTK_DOUBLE);
  }
  newPts->SetNumberOfPoints(numPts);
  output->SetPoints(newPts);

  // Figure out what normal to use
  double normal[3] = { 0.0, 0.0, 0.0 };
  if (inNormals && !this->UseNormal)
  {
    vtkDebugMacro(<< "Using data normals");
  }
  else if (this->XYPlane)
  {
    inNormals = nullptr;
    normal[2] = 1.0;
    vtkDebugMacro(<< "Using x-y plane normal");
  }
  else
  {
    inNormals = nullptr;
    normal[0] = this->Normal[0];
    normal[1] = this->Normal[1];
    normal[2] = this->Normal[2];
    vtkDebugMacro(<< "Using Normal instance variable");
  }

  // Dispatch over point and scalar types
  using vtkArrayDispatch::Reals;
  using ScaleDispatch = vtkArrayDispatch::Dispatch3ByValueType<Reals, Reals, Reals>;
  ScaleWorker scaleWorker;
  if (!ScaleDispatch::Execute(inPts->GetData(), newPts->GetData(), inScalars, scaleWorker, this,
        this->ScaleFactor, this->XYPlane, inNormals, normal))
  { // fallback to slowpath
    scaleWorker(inPts->GetData(), newPts->GetData(), inScalars, this, this->ScaleFactor,
      this->XYPlane, inNormals, normal);
  }

  // Update ourselves and release memory
  //
  output->GetPointData()->CopyNormalsOff(); // distorted geometry
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->CopyNormalsOff(); // distorted geometry
  output->GetCellData()->PassData(input->GetCellData());

  return 1;
}

//------------------------------------------------------------------------------
void vtkWarpScalar::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
  os << indent << "Use Normal: " << (this->UseNormal ? "On\n" : "Off\n");
  os << indent << "Normal: (" << this->Normal[0] << ", " << this->Normal[1] << ", "
     << this->Normal[2] << ")\n";
  os << indent << "XY Plane: " << (this->XYPlane ? "On\n" : "Off\n");
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
