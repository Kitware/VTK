/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVectorDot.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVectorDot.h"

#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSMPTools.h"

vtkStandardNewMacro(vtkVectorDot);

// These are created to support a float,double fast path. They work in
// tandem with vtkTemplate2Macro found in vtkSetGet.h.
#define vtkTemplate2MacroFP(call) \
  vtkTemplate2MacroCase1FP(VTK_DOUBLE, double, call);                           \
  vtkTemplate2MacroCase1FP(VTK_FLOAT, float, call);
#define vtkTemplate2MacroCase1FP(type1N, type1, call) \
  vtkTemplate2MacroCase2(type1N, type1, VTK_DOUBLE, double, call);                 \
  vtkTemplate2MacroCase2(type1N, type1, VTK_FLOAT, float, call);


// The heart of the algorithm plus interface to the SMP tools. Double templated
// over point and scalar types.
template <class TN,class TV>
class vtkVectorDotAlgorithm
{
public:
  vtkIdType NumPts;
  double Min, Max;
  double ScalarRange[2];
  const TN *Normals;
  const TV *Vectors;
  float *Scalars;

  // Constructor
  vtkVectorDotAlgorithm();

  // Interface between VTK and templated functions.
  static void Dot(vtkVectorDot *self, vtkIdType numPts, TN *normals,
                  TV *vectors, float *scalars,
                  double range[2], double actualRange[2]);

  // Interface dot product computation to SMP tools.
  template <class T1,class T2> class DotOp
  {
    public:
      vtkVectorDotAlgorithm *Algo;
      vtkSMPThreadLocal<double> Min;
      vtkSMPThreadLocal<double> Max;
      DotOp(vtkVectorDotAlgorithm<T1,T2> *algo) :
        Algo(algo), Min(VTK_DOUBLE_MAX), Max(VTK_DOUBLE_MIN) {}
      void  operator() (vtkIdType k, vtkIdType end)
      {
        double &min = this->Min.Local();
        double &max = this->Max.Local();
        const T1 *n = this->Algo->Normals + 3*k;
        const T2 *v = this->Algo->Vectors + 3*k;
        float *s = this->Algo->Scalars + k;
        for ( ; k < end; ++k)
        {
          *s = n[0]*v[0] + n[1]*v[1] + n[2]*v[2];
          min = ( *s < min ? *s : min );
          max = ( *s > max ? *s : max );
          s++;
          n += 3;
          v += 3;
        }
      }
  };

  // Interface normalize computation to SMP tools.
  template <class T1,class T2> class MapOp
  {
    public:
      vtkVectorDotAlgorithm *Algo;
      MapOp(vtkVectorDotAlgorithm<T1,T2> *algo)
        { this->Algo = algo; }
      void  operator() (vtkIdType k, vtkIdType end)
      {
        const double dR = this->Algo->ScalarRange[1]-this->Algo->ScalarRange[0];
        const double srMin = this->Algo->ScalarRange[0];
        const double dS = this->Algo->Max - this->Algo->Min;
        const double min = this->Algo->Min;
        float *s = this->Algo->Scalars + k;
        for ( ; k < end; ++k)
        {
          *s = srMin + ((*s - min)/dS)*dR;
          s++;
        }
      }
  };
};

//----------------------------------------------------------------------------
// Initialized mainly to eliminate compiler warnings.
template <class TN,class TV> vtkVectorDotAlgorithm<TN,TV>::
vtkVectorDotAlgorithm():Normals(NULL),Vectors(NULL),Scalars(NULL)
{
  this->NumPts = 0;
  this->Min = this->Max = 0.0;
  this->ScalarRange[0] = 0.0;
  this->ScalarRange[1] = 1.0;
}

//----------------------------------------------------------------------------
// Templated class is glue between VTK and templated algorithms.
template <class TN,class TV> void vtkVectorDotAlgorithm<TN,TV>::
Dot(vtkVectorDot *self, vtkIdType numPts, TN *normals, TV *vectors,
    float *scalars, double range[2], double actualRange[2])
{
  // Populate data into local storage
  vtkVectorDotAlgorithm<TN,TV> algo;

  algo.NumPts = numPts;
  algo.Normals = normals;
  algo.Vectors = vectors;
  algo.Scalars = scalars;
  algo.ScalarRange[0] = range[0];
  algo.ScalarRange[1] = range[1];

  // Okay now generate samples using SMP tools
  DotOp<TN,TV> dot(&algo);
  vtkSMPTools::For(0,algo.NumPts, dot);

  // Have to roll up the thread local storage and get the overall range
  double min = VTK_DOUBLE_MAX;
  double max = VTK_DOUBLE_MIN;
  vtkSMPThreadLocal<double>::iterator itr;
  for ( itr=dot.Min.begin(); itr != dot.Min.end(); ++itr )
  {
    if ( *itr < min )
    {
      min = *itr;
    }
  }
  for ( itr=dot.Max.begin(); itr != dot.Max.end(); ++itr )
  {
    if ( *itr > max )
    {
      max = *itr;
    }
  }

  // Return the global range
  actualRange[0] = algo.Min = min;
  actualRange[1] = algo.Max = max;

  if ( self->GetMapScalars() )
  {
    MapOp<TN,TV> mapValues(&algo);
    vtkSMPTools::For(0,algo.NumPts, mapValues);
  }
}


//=================================Begin class proper=========================
//----------------------------------------------------------------------------
// Construct object with scalar range (-1,1).
vtkVectorDot::vtkVectorDot()
{
  this->MapScalars = 1;

  this->ScalarRange[0] = -1.0;
  this->ScalarRange[1] =  1.0;

  this->ActualRange[0] = -1.0;
  this->ActualRange[1] =  1.0;
}

//----------------------------------------------------------------------------
// Compute dot product.
//
int vtkVectorDot::RequestData(
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

  vtkIdType numPts;
  vtkFloatArray *newScalars;
  vtkDataArray *inNormals;
  vtkDataArray *inVectors;
  vtkPointData *pd=input->GetPointData(), *outPD=output->GetPointData();

  // Initialize
  //
  vtkDebugMacro(<<"Generating vector/normal dot product!");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  if ( (numPts=input->GetNumberOfPoints()) < 1 )
  {
    vtkErrorMacro(<< "No points!");
    return 1;
  }
  if ( (inNormals=pd->GetNormals()) == NULL )
  {
    vtkErrorMacro(<< "No normals defined!");
    return 1;
  }
  if ( (inVectors=pd->GetVectors()) == NULL )
  {
    vtkErrorMacro(<< "No vectors defined!");
    return 1;
  }

  // Allocate
  //
  newScalars = vtkFloatArray::New();
  newScalars->SetNumberOfTuples(numPts);

  // This is potentiall a two pass algorithm. The first pass computes the dot
  // product and keeps track of min/max scalar values; and the second
  // (optional pass) maps the output into a specified range. Passes two and
  // three are optional.
  //
  float *scalars = static_cast<float*>(newScalars->GetVoidPointer(0));
  void *normals = inNormals->GetVoidPointer(0);
  void *vectors = inVectors->GetVoidPointer(0);

  int fastPath=1;
  double *range = this->ScalarRange;
  double *actualRange = this->ActualRange;
  switch (vtkTemplate2PackMacro(inNormals->GetDataType(),
                                inVectors->GetDataType()))
  {
    // Double explicit specification of multiple template arguments.
    // Supports combinations of float and double.
    vtkTemplate2MacroFP((vtkVectorDotAlgorithm<VTK_T1,VTK_T2>::
      Dot(this,numPts,(VTK_T1*)normals,(VTK_T2*)vectors,
          scalars,range,actualRange)));

    default:
      // Unknown input or output VTK type id.
      fastPath = 0;
      break;
  }

  // If we couldn't use the fast path, then take the scenic route
  if ( ! fastPath )
  {
    // Compute initial scalars
    //
    int abort=0;
    vtkIdType ptId;
    double s, n[3], v[3], min, max, dR, dS;
    vtkIdType progressInterval=numPts/20 + 1;
    for (min=VTK_DOUBLE_MAX,max=(-VTK_DOUBLE_MAX),ptId=0;
         ptId < numPts && !abort; ptId++)
    {
      if ( ! (ptId % progressInterval) )
      {
        this->UpdateProgress ((double)ptId/numPts);
        abort = this->GetAbortExecute();
      }
      inNormals->GetTuple(ptId, n);
      inVectors->GetTuple(ptId, v);
      s = vtkMath::Dot(n,v);
      if ( s < min )
      {
        min = s;
      }
      if ( s > max )
      {
        max = s;
      }
      newScalars->SetTuple(ptId,&s);
    }

    // Map scalars into scalar range
    //
    if ( (dR=this->ScalarRange[1]-this->ScalarRange[0]) == 0.0 )
    {
      dR = 1.0;
    }
    if ( (dS=max-min) == 0.0 )
    {
      dS = 1.0;
    }

    for ( ptId=0; ptId < numPts; ptId++ )
    {
      s = newScalars->GetComponent(ptId,0);
      s = ((s - min)/dS) * dR + this->ScalarRange[0];
      newScalars->SetTuple(ptId,&s);
    }
  }

  // Update self and release memory
  //
  outPD->PassData(input->GetPointData());

  int idx = outPD->AddArray(newScalars);
  outPD->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
  newScalars->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void vtkVectorDot::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "MapScalars: "
     << (this->MapScalars ? "On\n" : "Off\n");

  os << indent << "Scalar Range: (" << this->ScalarRange[0] << ", "
                                    << this->ScalarRange[1] << ")\n";

  os << indent << "Actual Range: (" << this->ActualRange[0] << ", "
                                    << this->ActualRange[1] << ")\n";
}
