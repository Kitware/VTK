/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkElevationFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkElevationFilter.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkPointSet.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkSMPTools.h"

vtkStandardNewMacro(vtkElevationFilter);

// The heart of the algorithm plus interface to the SMP tools. Double templated
// over point and scalar types.
template <class TP>
class vtkElevationAlgorithm
{
public:
  vtkIdType NumPts;
  double LowPoint[3];
  double HighPoint[3];
  double ScalarRange[2];
  const TP *Points;
  float *Scalars;
  const double *V;
  double L2;

  // Contructor
  vtkElevationAlgorithm();

  // Interface between VTK and templated functions
  static void Elevate(vtkElevationFilter *self, vtkIdType numPts,
                      double v[3], double l2, TP *points, float *scalars);

  // Interface implicit function computation to SMP tools.
  template <class T> class ElevationOp
  {
    public:
      ElevationOp(vtkElevationAlgorithm<T> *algo)
        { this->Algo = algo;}
      vtkElevationAlgorithm *Algo;
      void  operator() (vtkIdType k, vtkIdType end)
      {
        double ns, vec[3];
        const double *range = this->Algo->ScalarRange;
        const double diffScalar = range[1] - range[0];
        const double *v = this->Algo->V;
        const double l2 = this->Algo->L2;
        const double *lp = this->Algo->LowPoint;
        const TP *p = this->Algo->Points + 3*k;
        float *s = this->Algo->Scalars + k;
        for ( ; k < end; ++k)
        {
          vec[0] = p[0] - lp[0];
          vec[1] = p[1] - lp[1];
          vec[2] = p[2] - lp[2];
          ns = (vec[0]*v[0] + vec[1]*v[1] + vec[2]*v[2]) / l2;
          ns = (ns < 0.0 ? 0.0 : ns > 1.0 ? 1.0 : ns);

          // Store the resulting scalar value.
          *s = range[0] + ns*diffScalar;

          p+=3;
          ++s;
        }
      }
  };
};

//----------------------------------------------------------------------------
// Initialized mainly to eliminate compiler warnings.
template <class TP> vtkElevationAlgorithm<TP>::
vtkElevationAlgorithm():Points(NULL),Scalars(NULL)
{
  this->LowPoint[0] = this->LowPoint[1] = this->LowPoint[2] = 0.0;
  this->HighPoint[0] = this->HighPoint[1] = 0.0;
  this->HighPoint[2] = 1.0;
  this->ScalarRange[0] = 0.0;
  this->ScalarRange[1] = 1.0;
}

//----------------------------------------------------------------------------
// Templated class is glue between VTK and templated algorithms.
template <class TP> void vtkElevationAlgorithm<TP>::
Elevate(vtkElevationFilter *self, vtkIdType numPts,
        double *v, double l2, TP *points, float *scalars)
{
  // Populate data into local storage
  vtkElevationAlgorithm<TP> algo;
  algo.NumPts = numPts;
  self->GetLowPoint(algo.LowPoint);
  self->GetHighPoint(algo.HighPoint);
  self->GetScalarRange(algo.ScalarRange);
  algo.Points = points;
  algo.Scalars = scalars;
  algo.V = v;
  algo.L2 = l2;

  // Okay now generate samples using SMP tools
  ElevationOp<TP> values(&algo);
  vtkSMPTools::For(0,algo.NumPts, values);
}

//----------------------------------------------------------------------------
// Begin the class proper
vtkElevationFilter::vtkElevationFilter()
{
  this->LowPoint[0] = 0.0;
  this->LowPoint[1] = 0.0;
  this->LowPoint[2] = 0.0;

  this->HighPoint[0] = 0.0;
  this->HighPoint[1] = 0.0;
  this->HighPoint[2] = 1.0;

  this->ScalarRange[0] = 0.0;
  this->ScalarRange[1] = 1.0;
}

//----------------------------------------------------------------------------
vtkElevationFilter::~vtkElevationFilter()
{
}

//----------------------------------------------------------------------------
void vtkElevationFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Low Point: ("
     << this->LowPoint[0] << ", "
     << this->LowPoint[1] << ", "
     << this->LowPoint[2] << ")\n";
  os << indent << "High Point: ("
     << this->HighPoint[0] << ", "
     << this->HighPoint[1] << ", "
     << this->HighPoint[2] << ")\n";
  os << indent << "Scalar Range: ("
     << this->ScalarRange[0] << ", "
     << this->ScalarRange[1] << ")\n";
}

//----------------------------------------------------------------------------
int vtkElevationFilter::RequestData(vtkInformation*,
                                    vtkInformationVector** inputVector,
                                    vtkInformationVector* outputVector)
{
  // Get the input and output data objects.
  vtkDataSet* input = vtkDataSet::GetData(inputVector[0]);
  vtkDataSet* output = vtkDataSet::GetData(outputVector);

  // Check the size of the input.
  vtkIdType numPts = input->GetNumberOfPoints();
  if(numPts < 1)
  {
    vtkDebugMacro("No input!");
    return 1;
  }

  // Allocate space for the elevation scalar data.
  vtkSmartPointer<vtkFloatArray> newScalars =
    vtkSmartPointer<vtkFloatArray>::New();
  newScalars->SetNumberOfTuples(numPts);

  // Set up 1D parametric system and make sure it is valid.
  double diffVector[3] =
    { this->HighPoint[0] - this->LowPoint[0],
      this->HighPoint[1] - this->LowPoint[1],
      this->HighPoint[2] - this->LowPoint[2] };
  double length2 = vtkMath::Dot(diffVector, diffVector);
  if(length2 <= 0)
  {
    vtkErrorMacro("Bad vector, using (0,0,1).");
    diffVector[0] = 0;
    diffVector[1] = 0;
    diffVector[2] = 1;
    length2 = 1.0;
  }

  vtkDebugMacro("Generating elevation scalars!");

  // Create a fast path for point set input
  //
  vtkPointSet *ps = vtkPointSet::SafeDownCast(input);
  if ( ps )
  {
    float *scalars =
      static_cast<float*>(newScalars->GetVoidPointer(0));
    vtkPoints *points = ps->GetPoints();
    void *pts = points->GetData()->GetVoidPointer(0);
    switch ( points->GetDataType() )
    {
      vtkTemplateMacro(
        vtkElevationAlgorithm<VTK_TT>::Elevate(this,numPts,diffVector,length2,
                                               (VTK_TT *)pts,scalars));
    }
  }//fast path

  else
  {
    // Too bad, got to take the scenic route.
    // Support progress and abort.
    vtkIdType tenth = (numPts >= 10? numPts/10 : 1);
    double numPtsInv = 1.0/numPts;
    int abort = 0;

    // Compute parametric coordinate and map into scalar range.
    double diffScalar = this->ScalarRange[1] - this->ScalarRange[0];
    for(vtkIdType i=0; i < numPts && !abort; ++i)
    {
      // Periodically update progress and check for an abort request.
      if(i % tenth == 0)
      {
        this->UpdateProgress((i+1)*numPtsInv);
        abort = this->GetAbortExecute();
      }

      // Project this input point into the 1D system.
      double x[3];
      input->GetPoint(i, x);
      double v[3] = { x[0] - this->LowPoint[0],
                      x[1] - this->LowPoint[1],
                      x[2] - this->LowPoint[2] };
      double s = vtkMath::Dot(v, diffVector) / length2;
      s = (s < 0.0 ? 0.0 : s > 1.0 ? 1.0 : s);

      // Store the resulting scalar value.
      newScalars->SetValue(i, this->ScalarRange[0] + s*diffScalar);
    }
  }

  // Copy all the input geometry and data to the output.
  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  // Add the new scalars array to the output.
  newScalars->SetName("Elevation");
  output->GetPointData()->AddArray(newScalars);
  output->GetPointData()->SetActiveScalars("Elevation");

  return 1;
}
