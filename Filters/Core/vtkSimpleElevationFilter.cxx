/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSimpleElevationFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSimpleElevationFilter.h"

#include "vtkArrayDispatch.h"
#include "vtkCellData.h"
#include "vtkDataArrayRange.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkSMPTools.h"

vtkStandardNewMacro(vtkSimpleElevationFilter);

namespace {

// The heart of the algorithm plus interface to the SMP tools.
template <class PointArrayT>
class vtkSimpleElevationAlgorithm
{
public:
  vtkIdType NumPts;
  double Vector[3];
  PointArrayT *PointArray;
  float *Scalars;

  vtkSimpleElevationAlgorithm(PointArrayT* pointArray,
                              vtkSimpleElevationFilter* filter,
                              float* scalars)
    : NumPts{pointArray->GetNumberOfTuples()}
    , PointArray{pointArray}
    , Scalars{scalars}
  {
    filter->GetVector(this->Vector);
  }

  // Interface implicit function computation to SMP tools.
  void operator()(vtkIdType begin, vtkIdType end)
  {
    const double* v = this->Vector;
    float* s = this->Scalars + begin;

    const auto pointRange = vtk::DataArrayTupleRange<3>(this->PointArray,
                                                        begin, end);

    for (const auto p : pointRange)
    {
      *s = v[0]*p[0] + v[1]*p[1] + v[2]*p[2];
      ++s;
    }
  }
};

//----------------------------------------------------------------------------
// Templated class is glue between VTK and templated algorithms.
struct Elevate
{
  template <typename PointArrayT>
  void operator()(PointArrayT* pointArray,
                  vtkSimpleElevationFilter* filter,
                  float* scalars)
  {
    vtkSimpleElevationAlgorithm<PointArrayT> algo{pointArray, filter, scalars};
    vtkSMPTools::For(0, pointArray->GetNumberOfTuples(), algo);
  }
};

} // end anon namespace

// Okay begin the class proper
//----------------------------------------------------------------------------
// Construct object with Vector=(0,0,1).
vtkSimpleElevationFilter::vtkSimpleElevationFilter()
{
  this->Vector[0] = 0.0;
  this->Vector[1] = 0.0;
  this->Vector[2] = 1.0;
}

//----------------------------------------------------------------------------
// Convert position along the ray into scalar value.  Example use includes
// coloring terrain by elevation.
//
int vtkSimpleElevationFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType i, numPts;
  vtkFloatArray* newScalars;
  double s, x[3];

  // Initialize
  //
  vtkDebugMacro(<< "Generating elevation scalars!");

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  if (((numPts = input->GetNumberOfPoints()) < 1))
  {
    vtkDebugMacro(<< "No input!");
    return 1;
  }

  // Allocate
  //
  newScalars = vtkFloatArray::New();
  newScalars->SetNumberOfTuples(numPts);

  // Ensure that there is a valid vector
  //
  if (vtkMath::Dot(this->Vector, this->Vector) == 0.0)
  {
    vtkErrorMacro(<< "Bad vector, using (0,0,1)");
    this->Vector[0] = this->Vector[1] = 0.0;
    this->Vector[2] = 1.0;
  }

  // Create a fast path for point set input
  //
  vtkPointSet* ps = vtkPointSet::SafeDownCast(input);
  if (ps)
  {
    float* scalars = newScalars->GetPointer(0);
    vtkPoints* points = ps->GetPoints();
    vtkDataArray* pointsArray = points->GetData();

    Elevate worker; // Entry point to vtkSimpleElevationAlgorithm

    // Generate an optimized fast-path for float/double
    using FastValueTypes = vtkArrayDispatch::Reals;
    using Dispatcher = vtkArrayDispatch::DispatchByValueType<FastValueTypes>;
    if (!Dispatcher::Execute(pointsArray, worker, this, scalars))
    { // fallback for unknown arrays and integral value types:
      worker(pointsArray, this, scalars);
    }
  } // fast path

  else
  {
    // Too bad, got to take the scenic route.
    // Compute dot product.
    //
    int abort = 0;
    vtkIdType progressInterval = numPts / 20 + 1;
    for (i = 0; i < numPts && !abort; i++)
    {
      if (!(i % progressInterval))
      {
        this->UpdateProgress((double)i / numPts);
        abort = this->GetAbortExecute();
      }

      input->GetPoint(i, x);
      s = vtkMath::Dot(this->Vector, x);
      newScalars->SetComponent(i, 0, s);
    }
  }

  // Update self
  //
  output->GetPointData()->CopyScalarsOff();
  output->GetPointData()->PassData(input->GetPointData());

  output->GetCellData()->PassData(input->GetCellData());

  newScalars->SetName("Elevation");
  output->GetPointData()->AddArray(newScalars);
  output->GetPointData()->SetActiveScalars(newScalars->GetName());
  newScalars->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void vtkSimpleElevationFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Vector: (" << this->Vector[0] << ", " << this->Vector[1] << ", "
     << this->Vector[2] << ")\n";
}
