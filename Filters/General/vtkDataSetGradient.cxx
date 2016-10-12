/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetGradient.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
// .SECTION Thanks
// This file is part of the generalized Youngs material interface reconstruction algorithm contributed by
// CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France <br>
// BP12, F-91297 Arpajon, France. <br>
// Implementation by Thierry Carrard (CEA)

#include "vtkDataSetGradient.h"
#include "vtkDataSetGradientPrecompute.h"
#include "vtkDataSet.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkCell.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"


// utility macros
#define ADD_VEC(a,b) a[0]+=b[0];a[1]+=b[1];a[2]+=b[2]
#define SCALE_VEC(a,b) a[0]*=b;a[1]*=b;a[2]*=b
#define ZERO_VEC(a) a[0]=0;a[1]=0;a[2]=0
#define COPY_VEC(a,b) a[0]=b[0];a[1]=b[1];a[2]=b[2];
#define MAX_CELL_POINTS 128
#define VTK_CQS_EPSILON 1e-12


// standard constructors and factory
vtkStandardNewMacro(vtkDataSetGradient);

/*!
  The default constructor
  \sa ~vtkDataSetGradient()
*/
vtkDataSetGradient::vtkDataSetGradient()
  : ResultArrayName(0)
{
  this->SetResultArrayName("gradient");
}

/*!
  The destrcutor
  \sa vtkDataSetGradient()
*/
vtkDataSetGradient::~vtkDataSetGradient()
{
  this->SetResultArrayName( 0 );
}

void vtkDataSetGradient::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Result array name: " << this->ResultArrayName << "\n";
}

int vtkDataSetGradient::RequestData(vtkInformation * vtkNotUsed(request),
            vtkInformationVector **inputVector,
            vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get connected input & output
  vtkDataSet *_output = vtkDataSet::SafeDownCast( outInfo->Get(vtkDataObject::DATA_OBJECT()) );
  vtkDataSet* _input = vtkDataSet::SafeDownCast( inInfo->Get(vtkDataObject::DATA_OBJECT()) );

  if( _input==0 || _output==0 )
  {
    vtkErrorMacro(<<"Missing input or output \n");
    return 0;
  }

  // get array to compute gradient from
  vtkDataArray* inArray = this->GetInputArrayToProcess( 0, _input );
  if( inArray==0 )
  {
    inArray = _input->GetPointData()->GetScalars();
  }
  if( inArray==0 )
  {
    inArray = _input->GetCellData()->GetScalars();
  }

  if( inArray==0 )
  {
    vtkErrorMacro(<<"no  input array to process\n");
    return 0;
  }

  vtkDebugMacro(<<"Input array to process : "<<inArray->GetName()<<"\n");

  bool pointData;
  if( _input->GetCellData()->GetArray(inArray->GetName()) == inArray )
  {
    pointData = false;
    vtkDebugMacro(<<"cell data to point gradient\n");
  }
  else if( _input->GetPointData()->GetArray(inArray->GetName()) == inArray )
  {
    pointData = true;
    vtkDebugMacro(<<"point data to cell gradient\n");
  }
  else
  {
    vtkErrorMacro(<<"input array must be cell or point data\n");
    return 0;
  }

  // we're just adding a scalar field
  _output->ShallowCopy( _input );

  vtkDataArray* cqsArray = _output->GetFieldData()->GetArray("GradientPrecomputation");
  vtkDataArray* sizeArray = _output->GetCellData()->GetArray("CellSize");
  if( cqsArray==0 || sizeArray==0 )
  {
    vtkDebugMacro(<<"Couldn't find field array 'GradientPrecomputation', computing it right now.\n");
    vtkDataSetGradientPrecompute::GradientPrecompute(_output);
    cqsArray = _output->GetFieldData()->GetArray("GradientPrecomputation");
    sizeArray = _output->GetCellData()->GetArray("CellSize");
    if( cqsArray==0 || sizeArray==0 )
    {
      vtkErrorMacro(<<"Computation of field array 'GradientPrecomputation' or 'CellSize' failed.\n");
      return 0;
    }
  }

  vtkIdType nCells = _input->GetNumberOfCells();
  vtkIdType nPoints = _input->GetNumberOfPoints();

  vtkDoubleArray* gradientArray = vtkDoubleArray::New();
  gradientArray->SetName( this->ResultArrayName );
  gradientArray->SetNumberOfComponents(3);

  if( pointData ) // compute cell gradient from point data
  {
    gradientArray->SetNumberOfTuples( nCells );
    vtkIdType cellPoint = 0;
    for(vtkIdType i=0;i<nCells;i++)
    {
      vtkCell* cell = _input->GetCell(i);
      int np = cell->GetNumberOfPoints();
      double gradient[3] = {0,0,0};
      for(int p=0;p<np;p++)
      {
        double cqs[3], scalar;
        cqsArray->GetTuple( cellPoint++ , cqs );
        scalar = inArray->GetTuple1( cell->GetPointId(p) );
        SCALE_VEC( cqs , scalar );
        ADD_VEC( gradient , cqs );
      }
      SCALE_VEC( gradient , ( 1.0 / sizeArray->GetTuple1(i) ) );
      gradientArray->SetTuple( i , gradient );
    }

    _output->GetCellData()->AddArray( gradientArray );
    //_output->GetCellData()->SetVectors( gradientArray );
  }
  else // compute point gradient from cell data
  {
    gradientArray->SetNumberOfTuples( nPoints );
    gradientArray->FillComponent(0, 0.0);
    gradientArray->FillComponent(1, 0.0);
    gradientArray->FillComponent(2, 0.0);
    double * gradient = gradientArray->WritePointer(0,nPoints*3);
    double * gradientDivisor = new double [nPoints];
    for(vtkIdType i=0;i<nPoints;i++)
    {
      gradientDivisor[i] = 0.0;
    }
    vtkIdType cellPoint = 0;
    for(vtkIdType i=0;i<nCells;i++)
    {
      vtkCell* cell = _input->GetCell(i);
      int np = cell->GetNumberOfPoints();
      double scalar = inArray->GetTuple1( i );
      for(int p=0;p<np;p++)
      {
        double cqs[3];
        double pointCoord[3];
        vtkIdType pointId = cell->GetPointId(p);
        cqsArray->GetTuple( cellPoint++ , cqs );
        _input->GetPoint( cell->GetPointId(p), pointCoord );
        scalar *= cell->GetCellDimension();
        SCALE_VEC( (gradient+pointId*3) , scalar );
        gradientDivisor[pointId] += vtkMath::Dot(cqs,pointCoord);
      }
    }
    for(vtkIdType i=0;i<nPoints;i++)
    {
      SCALE_VEC( (gradient+i*3) , (1.0/gradientDivisor[i]) );
    }
    delete [] gradientDivisor;
    _output->GetPointData()->AddArray( gradientArray );
    //_output->GetPointData()->SetVectors( gradientArray );
  }
  gradientArray->Delete();

  vtkDebugMacro(<<_output->GetClassName()<<" @ "<<_output<<" :\n");

  return 1;
}

