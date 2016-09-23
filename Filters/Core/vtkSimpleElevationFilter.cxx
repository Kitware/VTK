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

#include "vtkCellData.h"
#include "vtkPointSet.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSMPTools.h"

vtkStandardNewMacro(vtkSimpleElevationFilter);

// The heart of the algorithm plus interface to the SMP tools. Double templated
// over point and scalar types.
template <class TP>
class vtkSimpleElevationAlgorithm
{
public:
  vtkIdType NumPts;
  double Vector[3];
  const TP *Points;
  float *Scalars;

  // Contructor
  vtkSimpleElevationAlgorithm();

  // Interface between VTK and templated functions
  static void Elevate(vtkSimpleElevationFilter *self, vtkIdType numPts,
                      const TP *points, float *scalars);

  // Interface implicit function computation to SMP tools.
  template <class T> class ElevationOp
  {
    public:
      ElevationOp(vtkSimpleElevationAlgorithm<T> *algo)
        { this->Algo = algo;}
      vtkSimpleElevationAlgorithm *Algo;
      void  operator() (vtkIdType k, vtkIdType end)
      {
        const double *v = this->Algo->Vector;
        const TP *p = this->Algo->Points + 3*k;
        float *s = this->Algo->Scalars + k;
        for ( ; k < end; ++k)
        {
          *s = v[0]*p[0] + v[1]*p[1] + v[2]*p[2];
          p+=3;
          ++s;
        }
      }
  };
};

//----------------------------------------------------------------------------
// Initialized mainly to eliminate compiler warnings.
template <class TP> vtkSimpleElevationAlgorithm<TP>::
vtkSimpleElevationAlgorithm():Points(NULL),Scalars(NULL)
{
  this->Vector[0] = this->Vector[1] = this->Vector[2] = 0.0;
}

//----------------------------------------------------------------------------
// Templated class is glue between VTK and templated algorithms.
template <class TP> void vtkSimpleElevationAlgorithm<TP>::
Elevate(vtkSimpleElevationFilter *self, vtkIdType numPts,
        const TP *points, float *scalars)
{
  // Populate data into local storage
  vtkSimpleElevationAlgorithm<TP> algo;
  algo.NumPts = numPts;
  self->GetVector(algo.Vector);
  algo.Points = points;
  algo.Scalars = scalars;

  // Okay now generate samples using SMP tools
  ElevationOp<TP> values(&algo);
  vtkSMPTools::For(0,algo.NumPts, values);
}


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
int vtkSimpleElevationFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType i, numPts;
  vtkFloatArray *newScalars;
  double s, x[3];

  // Initialize
  //
  vtkDebugMacro(<<"Generating elevation scalars!");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  if ( ((numPts=input->GetNumberOfPoints()) < 1) )
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
  if ( vtkMath::Dot(this->Vector,this->Vector) == 0.0)
  {
    vtkErrorMacro(<< "Bad vector, using (0,0,1)");
    this->Vector[0] = this->Vector[1] = 0.0; this->Vector[2] = 1.0;
  }

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
      vtkTemplateMacro(vtkSimpleElevationAlgorithm<VTK_TT>::
                       Elevate(this,numPts,(VTK_TT *)pts,scalars));
    }
  }//fast path

  else
  {
    // Too bad, got to take the scenic route.
    // Compute dot product.
    //
    int abort=0;
    vtkIdType progressInterval=numPts/20 + 1;
    for (i=0; i<numPts && !abort; i++)
    {
      if ( ! (i % progressInterval) )
      {
        this->UpdateProgress ((double)i/numPts);
        abort = this->GetAbortExecute();
      }

      input->GetPoint(i,x);
      s = vtkMath::Dot(this->Vector,x);
      newScalars->SetComponent(i,0,s);
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
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Vector: (" << this->Vector[0] << ", "
     << this->Vector[1] << ", " << this->Vector[2] << ")\n";
}
