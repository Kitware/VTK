/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetGradientPrecompute.cxx

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

#include "vtkDataSetGradientPrecompute.h"

#include "vtkMath.h"
#include "vtkTriangle.h"
#include "vtkInformationVector.h"
#include "vtkInformation.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkCell.h"
#include "vtkCell3D.h"
#include "vtkTetra.h"
#include "vtkFieldData.h"
#include "vtkCellData.h"
#include "vtkObjectFactory.h"

#define VTK_DATASET_GRADIENT_TETRA_OPTIMIZATION
#define VTK_DATASET_GRADIENT_TRIANGLE_OPTIMIZATION
//#define DEBUG

vtkStandardNewMacro(vtkDataSetGradientPrecompute);

vtkDataSetGradientPrecompute::vtkDataSetGradientPrecompute()
{
}

vtkDataSetGradientPrecompute::~vtkDataSetGradientPrecompute()
{
}

void vtkDataSetGradientPrecompute::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

#define ADD_VEC(a,b) a[0]+=b[0];a[1]+=b[1];a[2]+=b[2]
#define SCALE_VEC(a,b) a[0]*=b;a[1]*=b;a[2]*=b
#define ZERO_VEC(a) a[0]=0;a[1]=0;a[2]=0
#define MAX_CELL_POINTS 128
#define MAX_FACE_POINTS 16
#define VTK_CQS_EPSILON 1e-12

static inline void TETRA_CQS_VECTOR( double v0[3], double v1[3], double v2[3], double p[3], double cqs[3] )
{
  double surface = fabs( vtkTriangle::TriangleArea (v0,v1,v2) );

  vtkTriangle::ComputeNormal( v0,v1,v2 , cqs );

  // inverse face normal if not toward opposite vertex
  double edge[3];
  edge[0] = p[0] - v0[0];
  edge[1] = p[1] - v0[1];
  edge[2] = p[2] - v0[2];
  if( vtkMath::Dot(edge,cqs) < 0 )
    {
    cqs[0] = - cqs[0];
    cqs[1] = - cqs[1];
    cqs[2] = - cqs[2];
    }

  SCALE_VEC( cqs , surface / 2.0 );
}

static inline void TRIANGLE_CQS_VECTOR( double v0[3], double v1[3], double p[3], double cqs[3] )
{
  double length = fabs( vtkMath::Distance2BetweenPoints(v0,v1) );
  double a[3], b[3], c[3];
  for(int i=0;i<3;i++)
    {
    a[i] = v1[i] - v0[i];
    b[i] = p[i] - v0[i];
    }
  vtkMath::Cross( a, b, c );
  vtkMath::Cross( c , a , cqs );
  double n = vtkMath::Norm( cqs );
  SCALE_VEC( cqs , length / (n*2.0) );
}

int vtkDataSetGradientPrecompute::GradientPrecompute(vtkDataSet* ds)
{
  vtkIdType nCells = ds->GetNumberOfCells();
  vtkIdType nCellNodes = 0;
  for(vtkIdType i=0;i<nCells;i++)
    {
    nCellNodes += ds->GetCell(i)->GetNumberOfPoints();
    }

  vtkDoubleArray* cqs = vtkDoubleArray::New();
  cqs->SetName("GradientPrecomputation");
  cqs->SetNumberOfComponents(3);
  cqs->SetNumberOfTuples(nCellNodes);
  cqs->FillComponent(0, 0.0);
  cqs->FillComponent(1, 0.0);
  cqs->FillComponent(2, 0.0);

  vtkDoubleArray* cellVolume = vtkDoubleArray::New();
  cellVolume->SetName("CellVolume");
  cellVolume->SetNumberOfTuples(nCells);

  vtkIdType curPoint = 0;
  for(vtkIdType c=0;c<nCells;c++)
    {
    vtkCell* cell = ds->GetCell(c);
    int np = cell->GetNumberOfPoints();

    double cellCenter[3] = {0,0,0};
    double cellPoints[MAX_CELL_POINTS][3];
    double cellVectors[MAX_CELL_POINTS][3];
    double tmp[3];
    double volume = 0.0;

    for(int p=0;p<np;p++)
      {
      ds->GetPoint( cell->GetPointId(p), cellPoints[p] );
      ADD_VEC( cellCenter , cellPoints[p] );
      ZERO_VEC( cellVectors[p] );
      }
    SCALE_VEC(cellCenter,1.0/np);

    // -= 3 D =-
    if( cell->GetCellDimension() == 3 )
      {
#ifdef VTK_DATASET_GRADIENT_TETRA_OPTIMIZATION
      if( np == 4 ) // cell is a tetrahedra
        {
        //vtkWarningMacro(<<"Tetra detected\n");
        volume = fabs( vtkTetra::ComputeVolume(cellPoints[0], cellPoints[1], cellPoints[2], cellPoints[3]) ) *1.5 ; 
        
        TETRA_CQS_VECTOR( cellPoints[0], cellPoints[1], cellPoints[2], cellPoints[3] , tmp );    
        ADD_VEC(cellVectors[3],tmp);
        
        TETRA_CQS_VECTOR( cellPoints[1], cellPoints[2], cellPoints[3], cellPoints[0] , tmp );    
        ADD_VEC(cellVectors[0],tmp);
        
        TETRA_CQS_VECTOR( cellPoints[2], cellPoints[3], cellPoints[0], cellPoints[1] , tmp );    
        ADD_VEC(cellVectors[1],tmp);
        
        TETRA_CQS_VECTOR( cellPoints[3], cellPoints[0], cellPoints[1], cellPoints[2] , tmp );    
        ADD_VEC(cellVectors[2],tmp);
        }
      else if( np > 4 )
#endif
        {
        vtkCell3D* cell3d = static_cast<vtkCell3D*>( cell ); 
        int nf = cell->GetNumberOfFaces();
        for(int f=0;f<nf;f++)
          {
          int* faceIds = 0;
          int nfp = cell->GetFace(f)->GetNumberOfPoints();
          cell3d->GetFacePoints(f,faceIds);
#ifdef VTK_DATASET_GRADIENT_TRIANGLE_OPTIMIZATION
          if( nfp == 3 ) // face is a triangle
            {
            //vtkWarningMacro(<<"triangular face detected\n");
            volume+=fabs(vtkTetra::ComputeVolume(cellCenter,cellPoints[faceIds[0]],cellPoints[faceIds[1]],cellPoints[faceIds[2]]))*1.5;
 
            TETRA_CQS_VECTOR( cellCenter, cellPoints[faceIds[0]], cellPoints[faceIds[1]], cellPoints[faceIds[2]] , tmp );    
            ADD_VEC(cellVectors[faceIds[2]],tmp);
            
            TETRA_CQS_VECTOR( cellCenter, cellPoints[faceIds[1]], cellPoints[faceIds[2]], cellPoints[faceIds[0]] , tmp );    
            ADD_VEC(cellVectors[faceIds[0]],tmp);
            
            TETRA_CQS_VECTOR( cellCenter, cellPoints[faceIds[2]], cellPoints[faceIds[0]], cellPoints[faceIds[1]] , tmp );    
            ADD_VEC(cellVectors[faceIds[1]],tmp);
            }
          else if( nfp > 3 ) // generic case
#endif
            {
            double faceCenter[3] = {0,0,0};
            for(int p=0;p<nfp;p++)
              {
              ADD_VEC( faceCenter , cellPoints[faceIds[p]] );
              }
            SCALE_VEC( faceCenter, 1.0/nfp );
            for(int p=0;p<nfp;p++)
              {
              int p2 = (p+1) % nfp ;
              volume += fabs( vtkTetra::ComputeVolume(cellCenter, faceCenter, cellPoints[faceIds[p]] , cellPoints[faceIds[p2]]) ) ;

              TETRA_CQS_VECTOR( cellCenter, faceCenter, cellPoints[faceIds[p]] , cellPoints[faceIds[p2]] , tmp );    
              ADD_VEC( cellVectors[faceIds[p2]] , tmp );
              
              TETRA_CQS_VECTOR( cellCenter, faceCenter, cellPoints[faceIds[p2]] , cellPoints[faceIds[p]] , tmp );    
              ADD_VEC( cellVectors[faceIds[p]] , tmp );
              }
            }
          }
        }
      }

    // -= 2 D =-
    else
      {
      if( np == 3 ) // cell is a triangle
        {
        TRIANGLE_CQS_VECTOR( cellPoints[0] , cellPoints[1] , cellPoints[2] , tmp );
        ADD_VEC( cellVectors[2] , tmp );

        TRIANGLE_CQS_VECTOR( cellPoints[1] , cellPoints[2] , cellPoints[0] , tmp );
        ADD_VEC( cellVectors[0] , tmp );

        TRIANGLE_CQS_VECTOR( cellPoints[2] , cellPoints[0] , cellPoints[1] , tmp );
        ADD_VEC( cellVectors[1] , tmp );
        }
      else if( np > 3) // generic case
        {
        for(int f=0;f<np;f++)
          {
          const int e0 = f;
          const int e1 = (f+1)%np;
          TRIANGLE_CQS_VECTOR( cellCenter , cellPoints[e0] , cellPoints[e1] , tmp );
          ADD_VEC( cellVectors[e1] , tmp );
        
          TRIANGLE_CQS_VECTOR( cellCenter , cellPoints[e1] , cellPoints[e0] , tmp );
          ADD_VEC( cellVectors[e0] , tmp );
          }
        }
      else
        {
        //vtkWarningMacro(<<"Can't process 2D cells with less than 3 points.");
        //return 0;
        }
      }

    cellVolume->SetTuple1(c,volume);

    // check cqs consistency
    double checkZero[3] = {0,0,0};
    double checkVolume = 0;
    for(int p=0;p<np;p++)
      {
      checkVolume += vtkMath::Dot( cellPoints[p] , cellVectors[p] );
      ADD_VEC(checkZero,cellVectors[p]);
      cqs->SetTuple( curPoint + p , cellVectors[p] );
      }
    checkVolume /= (double) cell->GetCellDimension();

#ifdef DEBUG
    if( vtkMath::Norm(checkZero)>VTK_CQS_EPSILON || fabs(volume-checkVolume)>VTK_CQS_EPSILON )
      {
      cout<<"Bad CQS sum at cell #"<<c<<", Sum="<<vtkMath::Norm(checkZero)<<", volume="<<volume<<", ratio Vol="<<volume/checkVolume<<"\n";
      }
#endif

    curPoint += np;
    }

  ds->GetFieldData()->AddArray( cqs );
  ds->GetCellData()->AddArray( cellVolume );
  cqs->Delete();
  cellVolume->Delete();

  return 1;
}

int vtkDataSetGradientPrecompute::RequestData(vtkInformation *vtkNotUsed(request),
                                              vtkInformationVector **inputVector,
                                              vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get connected input & output
  vtkDataSet* _output = vtkDataSet::SafeDownCast( outInfo->Get(vtkDataObject::DATA_OBJECT()) );
  vtkDataSet* _input = vtkDataSet::SafeDownCast( inInfo->Get(vtkDataObject::DATA_OBJECT()) );

  if( _input==0 || _output==0 )
    {
    vtkErrorMacro(<<"missing input/output connection\n");
    return 0;
    }
      
  _output->ShallowCopy(_input);
  return vtkDataSetGradientPrecompute::GradientPrecompute(_output);
}

