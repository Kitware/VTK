/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVectorNorm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVectorNorm.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSMPTools.h"

#include <cmath>

vtkStandardNewMacro(vtkVectorNorm);


// The heart of the algorithm plus interface to the SMP tools. Double templated
// over point and scalar types.
template <class TV>
class vtkVectorNormAlgorithm
{
public:
  vtkIdType Num;
  double Max;
  const TV *Vectors;
  float *Scalars;

  // Constructor
  vtkVectorNormAlgorithm();

  // Interface between VTK and templated functions.
  static void Norm(vtkVectorNorm *self, vtkIdType num, TV *vectors,
                   float *scalars);

  // Interface dot product computation to SMP tools.
  template <class T> class NormOp
  {
    public:
      vtkVectorNormAlgorithm *Algo;
      vtkSMPThreadLocal<double> Max;
      NormOp(vtkVectorNormAlgorithm<T> *algo) :
        Algo(algo), Max(VTK_DOUBLE_MIN) {}
      void  operator() (vtkIdType k, vtkIdType end)
      {
        double &max = this->Max.Local();
        const T *v = this->Algo->Vectors + 3*k;
        float *s = this->Algo->Scalars + k;
        for ( ; k < end; ++k)
        {
          *s = static_cast<float>(
            sqrt( static_cast<double>(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]) ) );
          max = ( *s > max ? *s : max );
          s++;
          v += 3;
        }
      }
  };

  // Interface normalize computation to SMP tools.
  template <class T> class MapOp
  {
    public:
      vtkVectorNormAlgorithm *Algo;
      MapOp(vtkVectorNormAlgorithm<T> *algo)
        { this->Algo = algo; }
      void  operator() (vtkIdType k, vtkIdType end)
      {
        const double max = this->Algo->Max;
        float *s = this->Algo->Scalars + k;
        for ( ; k < end; ++k)
        {
          *s++ /= max;
        }
      }
  };
};

//----------------------------------------------------------------------------
// Initialized mainly to eliminate compiler warnings.
template <class TV> vtkVectorNormAlgorithm<TV>::
vtkVectorNormAlgorithm():Vectors(NULL),Scalars(NULL)
{
  this->Num = 0;
  this->Max = 0.0;
}

//----------------------------------------------------------------------------
// Templated class is glue between VTK and templated algorithms.
template <class TV> void vtkVectorNormAlgorithm<TV>::
Norm(vtkVectorNorm *self, vtkIdType num, TV *vectors, float *scalars)
{
  // Populate data into local storage
  vtkVectorNormAlgorithm<TV> algo;

  algo.Num = num;
  algo.Vectors = vectors;
  algo.Scalars = scalars;

  // Okay now generate samples using SMP tools
  NormOp<TV> norm(&algo);
  vtkSMPTools::For(0,algo.Num, norm);

  // Have to roll up the thread local storage and get the overall range
  double max = VTK_DOUBLE_MIN;
  vtkSMPThreadLocal<double>::iterator itr;
  for ( itr=norm.Max.begin(); itr != norm.Max.end(); ++itr )
  {
    if ( *itr > max )
    {
      *itr = max;
    }
  }
  algo.Max = max;

  if ( max > 0.0 && self->GetNormalize() )
  {
    MapOp<TV> mapValues(&algo);
    vtkSMPTools::For(0,algo.Num, mapValues);
  }
}


//=================================Begin class proper=========================
//----------------------------------------------------------------------------
// Construct with normalize flag off.
vtkVectorNorm::vtkVectorNorm()
{
  this->Normalize = 0;
  this->AttributeMode = VTK_ATTRIBUTE_MODE_DEFAULT;
}

//----------------------------------------------------------------------------
int vtkVectorNorm::RequestData(
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

  vtkIdType numVectors;
  int computePtScalars=1, computeCellScalars=1;
  vtkFloatArray *newScalars;
  vtkDataArray *ptVectors, *cellVectors;
  vtkPointData *pd=input->GetPointData(), *outPD=output->GetPointData();
  vtkCellData *cd=input->GetCellData(), *outCD=output->GetCellData();

  // Initialize
  vtkDebugMacro(<<"Computing norm of vectors!");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  ptVectors = pd->GetVectors();
  cellVectors = cd->GetVectors();
  if (!ptVectors || this->AttributeMode == VTK_ATTRIBUTE_MODE_USE_CELL_DATA)
  {
    computePtScalars = 0;
  }

  if (!cellVectors || this->AttributeMode == VTK_ATTRIBUTE_MODE_USE_POINT_DATA)
  {
    computeCellScalars = 0;
  }

  if ( !computeCellScalars && !computePtScalars )
  {
    vtkErrorMacro(<< "No vector norm to compute!");
    return 1;
  }

  // Allocate / operate on point data
  if ( computePtScalars )
  {
    numVectors = ptVectors->GetNumberOfTuples();
    newScalars = vtkFloatArray::New();
    newScalars->SetNumberOfTuples(numVectors);

    this->GenerateScalars(numVectors,ptVectors,newScalars);

    int idx = outPD->AddArray(newScalars);
    outPD->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
    newScalars->Delete();
    outPD->CopyScalarsOff();
  }//if computing point scalars

  this->UpdateProgress (0.50);

  // Allocate / operate on cell data
  if ( computeCellScalars )
  {
    numVectors = cellVectors->GetNumberOfTuples();
    newScalars = vtkFloatArray::New();
    newScalars->SetNumberOfTuples(numVectors);

    this->GenerateScalars(numVectors,cellVectors,newScalars);

    int idx = outCD->AddArray(newScalars);
    outCD->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
    newScalars->Delete();
    outCD->CopyScalarsOff();
  }//if computing cell scalars

  // Pass appropriate data through to output
  outPD->PassData(pd);
  outCD->PassData(cd);

  return 1;
}


//----------------------------------------------------------------------------
// All this does it wrap up templated code.
void vtkVectorNorm::
GenerateScalars(vtkIdType num, vtkDataArray *v, vtkFloatArray *s)
{
  float *scalars = static_cast<float*>(s->GetVoidPointer(0));
  void *vectors = v->GetVoidPointer(0);
  switch ( v->GetDataType() )
  {
    vtkTemplateMacro(vtkVectorNormAlgorithm<VTK_TT>::
                     Norm(this,num,(VTK_TT*)vectors,scalars));

    default:
      break;
  }
}

//----------------------------------------------------------------------------
// Return the method for generating scalar data as a string.
const char *vtkVectorNorm::GetAttributeModeAsString(void)
{
  if ( this->AttributeMode == VTK_ATTRIBUTE_MODE_DEFAULT )
  {
    return "Default";
  }
  else if ( this->AttributeMode == VTK_ATTRIBUTE_MODE_USE_POINT_DATA )
  {
    return "UsePointData";
  }
  else
  {
    return "UseCellData";
  }
}

//----------------------------------------------------------------------------
void vtkVectorNorm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Normalize: " << (this->Normalize ? "On\n" : "Off\n");
  os << indent << "Attribute Mode: " << this->GetAttributeModeAsString()
     << endl;
}
