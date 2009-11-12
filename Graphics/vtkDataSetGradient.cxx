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
#include "vtkDataSet.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkCell.h"

// standard constructors and factory
vtkCxxRevisionMacro(vtkDataSetGradient, "1.4");
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

  vtkDataArray* cqsArray = _input->GetFieldData()->GetArray("GradientPrecomputation");

  if( cqsArray==0 )
  {
     vtkErrorMacro(<<"Couldn't find field array 'GradientPrecomputation'. Add a vtkDataSetGradientPrecompute filter to your pipeline.\n");
     return 0;
  }

 // we're just adding a scalar field
  _output->ShallowCopy( _input );

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
     double cqs[3];
     cqsArray->GetTuple( cellPoint++ , cqs );
     double scalar = inArray->GetTuple1( cell->GetPointId(p) );
     gradient[0] += scalar * cqs[0];
     gradient[1] += scalar * cqs[1];
     gradient[2] += scalar * cqs[2];
  }
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
     vtkIdType cellPoint = 0;
     for(vtkIdType i=0;i<nCells;i++)
     {
  vtkCell* cell = _input->GetCell(i);
  int np = cell->GetNumberOfPoints();
  double scalar = inArray->GetTuple1( i );
  for(int p=0;p<np;p++)
  {
     double cqs[3];
     cqsArray->GetTuple( cellPoint++ , cqs );
     vtkIdType pointId = cell->GetPointId(p);
     gradient[pointId*3+0] += scalar * cqs[0];
     gradient[pointId*3+1] += scalar * cqs[1];
     gradient[pointId*3+2] += scalar * cqs[2];
  }
     }
     _output->GetPointData()->AddArray( gradientArray );
    //_output->GetPointData()->SetVectors( gradientArray );
  }
  gradientArray->Delete();

  vtkDebugMacro(<<_output->GetClassName()<<" @ "<<_output<<" :\n");

  return 1;
}

