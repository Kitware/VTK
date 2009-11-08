/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkYoungsMaterialInterface.cxx

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

#include "vtkYoungsMaterialInterface.h"

#include <vtkCell.h>
#include <vtkEmptyCell.h>
#include <vtkPolygon.h>
#include <vtkConvexPointSet.h>
#include <vtkDataSet.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkCellData.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>
#include <vtkUnsignedCharArray.h>
#include <vtkIdTypeArray.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkUnstructuredGrid.h>
#include <vtkDoubleArray.h>
#include <vtkMath.h>
#include <vtkIdList.h>

#include <vtkstd/vector>
#include <vtkstd/string> 
#include <vtkstd/map>
#include <vtkstd/algorithm>

#include "vtkYoungsMaterialInterfaceCellCut.h"

class vtkYoungsMaterialInterfaceInternals
{
  public: 
    struct MaterialDescription
    {
      vtkstd::string volume, normal, normalX, normalY, normalZ, ordering;
    };
    vtkstd::vector<MaterialDescription> Materials;
};

// standard constructors and factory
vtkCxxRevisionMacro(vtkYoungsMaterialInterface, "1.2");
vtkStandardNewMacro(vtkYoungsMaterialInterface);

#ifdef DEBUG
#include <assert.h>
#define DBG_ASSERT(cond) assert(cond)
#else
#define DBG_ASSERT(cond) (void)0
#endif

/*!
The default constructor
\sa ~vtkYoungsMaterialInterface()
*/
vtkYoungsMaterialInterface::vtkYoungsMaterialInterface()
{
   this->FillMaterial = 0;
   this->InverseNormal = 0;
   this->AxisSymetric = 0;
   this->OnionPeel = 0;
   this->ReverseMaterialOrder = 0;
   this->UseFractionAsDistance = 0;
   this->VolumeFractionRange[0] = 0.01;
   this->VolumeFractionRange[1] = 0.99;
   this->TwoMaterialsOptimization = 0;
   this->Internals = new vtkYoungsMaterialInterfaceInternals;
}

/*!
The destrcutor
\sa vtkYoungsMaterialInterface()
*/
vtkYoungsMaterialInterface::~vtkYoungsMaterialInterface()
{
    delete this->Internals;
}

void vtkYoungsMaterialInterface::SetNumberOfMaterials(int n)
{
  vtkDebugMacro(<<"Resize Materials to "<<n<<"\n");
  this->Internals->Materials.resize(n);
}

int vtkYoungsMaterialInterface::GetNumberOfMaterials()
{
  return this->Internals->Materials.size();
}

void vtkYoungsMaterialInterface::SetMaterialVolumeFractionArray( int M,  const char* volume )
{
   if( M<0 || M>=this->GetNumberOfMaterials() )
   {
  vtkErrorMacro(<<"Bad material index "<<M<<"\n");
  return;
   }
   this->Internals->Materials[M].volume = volume;
}

void vtkYoungsMaterialInterface::SetMaterialNormalArray( int M,  const char* normal )
{
   if( M<0 || M>=this->GetNumberOfMaterials() )
   {
  vtkErrorMacro(<<"Bad material index "<<M<<"\n");
  return;
   }
   this->Internals->Materials[M].normal = normal;
   this->Internals->Materials[M].normalX = "";
   this->Internals->Materials[M].normalY = "";
   this->Internals->Materials[M].normalZ = "";
}

void vtkYoungsMaterialInterface::SetMaterialOrderingArray( int M,  const char* ordering )
{
   if( M<0 || M>=this->GetNumberOfMaterials() )
   {
  vtkErrorMacro(<<"Bad material index "<<M<<"\n");
  return;
   }
   this->Internals->Materials[M].ordering = ordering;
}

/*
void vtkYoungsMaterialInterface::AddMaterial( const char* volume )
{
   MaterialDescription md;
   md.volume = volume;
   md.normal = "";
   md.normalX = "";
   md.normalY = "";
   md.normalZ = "";
   md.ordering = "";
   this->Materials.push_back(md);
}
*/

void vtkYoungsMaterialInterface::SetMaterialArrays( int M, const char* volume, const char* normal, const char* ordering )
{
   if( M<0 || M>=this->GetNumberOfMaterials() )
   {
  vtkErrorMacro(<<"Bad material index "<<M<<"\n");
  return;
   }
  vtkDebugMacro(<<"Set Material "<<M<<" : "<<volume<<","<<normal<<","<<ordering<<"\n");
   vtkYoungsMaterialInterfaceInternals::MaterialDescription md;
   md.volume = volume;
   md.normal = normal;
   md.normalX = "";
   md.normalY = "";
   md.normalZ = "";
   md.ordering = ordering;
   this->Internals->Materials[M] = md;
}

/*
void vtkYoungsMaterialInterface::AddMaterial( const char* volume, const char* normalX, const char* normalY, const char* normalZ, const char* ordering )
{
  vtkDebugMacro(<<"Added Material "<<volume<<","<<normalX<<","<<normalY<<","<<normalZ<<","<<ordering<<"\n");
   MaterialDescription md;
   md.volume = volume;
   md.normal = "";
   md.normalX = normalX;
   md.normalY = normalY;
   md.normalZ = normalZ;
   md.ordering = ordering;
   this->Materials.push_back( md );
}

void vtkYoungsMaterialInterface::SetMaterial( int Material,  const char* volume, const char* normalX, const char* normalY, const char* normalZ, const char* ordering )
{
   MaterialDescription md;
   md.volume = volume;
   md.normal = "";
   md.normalX = normalX;
   md.normalY = normalY;
   md.normalZ = normalZ;
   md.ordering = ordering;
   this->Materials.push_back( md );
}
*/

void vtkYoungsMaterialInterface::RemoveAllMaterials()
{
  vtkDebugMacro(<<"Remove All Materials\n");
  this->SetNumberOfMaterials(0);
}

int vtkYoungsMaterialInterface::FillInputPortInformation(int, vtkInformation *info)
{
   info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  //info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
   return 1;
}

// internal classes
struct vtkYoungsMaterialInterface_IndexedValue
{
      double value;
      int index;
      inline bool operator < ( const vtkYoungsMaterialInterface_IndexedValue& iv ) const { return value < iv.value; }
};

struct vtkYoungsMaterialInterface_Mat
{
     // input
      vtkDataArray* fractionArray;
      vtkDataArray* normalArray;
      vtkDataArray* normalXArray;
      vtkDataArray* normalYArray;
      vtkDataArray* normalZArray;
      vtkDataArray* orderingArray;      

     // temporary
      vtkIdType numberOfCells;
      vtkIdType numberOfPoints;
      vtkIdType cellCount;
      vtkIdType cellArrayCount;
      vtkIdType pointCount;
      vtkIdType* pointMap;

     // output
      vtkstd::vector<unsigned char> cellTypes;
      vtkstd::vector<vtkIdType> cells;
      vtkDataArray** outCellArrays;
      vtkDataArray** outPointArrays; // last point array is point coords
};


static inline void 
vtkYoungsMaterialInterface_GetPointData(
   int nPointData,
   vtkDataArray** inPointArrays,
   vtkDataSet * input,
   vtkstd::vector< vtkstd::pair<int,vtkIdType> > & prevPointsMap,
   int nmat,
   vtkYoungsMaterialInterface_Mat * Mats ,
   int a, vtkIdType i, double* t)
{
   if( (i) >= 0 )              
   {                  
      if(a<(nPointData-1))            
      {                  
   DBG_ASSERT( /*i>=0 &&*/ i<inPointArrays[a]->GetNumberOfTuples());   
   inPointArrays[a]->GetTuple( i , t );        
      }                  
      else                
      {
   DBG_ASSERT( a == (nPointData-1) );
   DBG_ASSERT( /*i>=0 &&*/ i<input->GetNumberOfPoints());              
   input->GetPoint( i , t );          
      }
   }
   else
   {
      int j=-i-1;              
      DBG_ASSERT(j>=0 && j<prevPointsMap.size());      
      int prev_m = prevPointsMap[j].first;        
      DBG_ASSERT(prev_m>=0 && prev_m<nmat);        
      vtkIdType prev_i = ( prevPointsMap[j].second );      
      DBG_ASSERT(prev_i>=0 && prev_i<Mats[prev_m].outPointArrays[a]->GetNumberOfTuples()); 
      Mats[prev_m].outPointArrays[a]->GetTuple( prev_i , t );    
   } 
}

#define GET_POINT_DATA(a,i,t) vtkYoungsMaterialInterface_GetPointData(nPointData,inPointArrays,input,prevPointsMap,nmat,Mats,a,i,t)

struct CellInfo
{
      double points[vtkYoungsMaterialInterface::MAX_CELL_POINTS][3];
      vtkIdType pointIds[vtkYoungsMaterialInterface::MAX_CELL_POINTS];
      int triangulation[vtkYoungsMaterialInterface::MAX_CELL_POINTS*4];
      int edges[vtkYoungsMaterialInterface::MAX_CELL_POINTS][2];

      int dim;
      int np;
      int nf;
      int ntri;
      int type;
      int nEdges;

      bool triangulationOk;
      bool needTriangulation;

      inline CellInfo() : dim(2), np(0), nf(0), ntri(0), type(VTK_EMPTY_CELL), nEdges(0), triangulationOk(false), needTriangulation(false) {}
};

int vtkYoungsMaterialInterface::CellProduceInterface( int dim, int np, double fraction, double minFrac, double maxFrac )
{
   return
      (
   (dim==3 && np>=4) || 
   (dim==2 && np>=3)
      ) &&
      (
   this->UseFractionAsDistance ||
   (
      ( fraction > minFrac ) &&
      ( fraction < maxFrac || this->FillMaterial )
   )
      ) ;
   
}

int vtkYoungsMaterialInterface::RequestData(vtkInformation *request,
              vtkInformationVector **inputVector,
              vtkInformationVector *outputVector)
{
   vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
   vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get input
   vtkDataSet * input = vtkDataSet::SafeDownCast(
      inInfo->Get(vtkDataObject::DATA_OBJECT()) );

  // get typed output
   vtkMultiBlockDataSet * output = vtkMultiBlockDataSet::SafeDownCast(
      outInfo->Get(vtkDataObject::DATA_OBJECT()));

   if(input==0 || output==0)
   {
      vtkErrorMacro(<<"Invalid algorithm connection\n");
      return 0;
   }

  // variables visible by debugger

   int nmat = this->Internals->Materials.size();
   int nCellData = input->GetCellData()->GetNumberOfArrays();
   int nPointData = input->GetPointData()->GetNumberOfArrays();
   vtkIdType nCells = input->GetNumberOfCells();
   vtkIdType nPoints = input->GetNumberOfPoints();

  // -------------- temporary data initialization -------------------
   vtkDataArray** inCellArrays = new vtkDataArray* [nCellData] ;
   for(int i=0;i<nCellData;i++)
   {
      inCellArrays[i] = input->GetCellData()->GetArray(i);
   }

   vtkDataArray** inPointArrays = new vtkDataArray* [nPointData+1]; // last point array is point coords
   int* pointArrayOffset = new int [ nPointData+1 ];
   int pointDataComponents = 0;
   for(int i=0;i<nPointData;i++)
   {
      inPointArrays[i] = input->GetPointData()->GetArray(i);
      pointArrayOffset[i] = pointDataComponents;
      pointDataComponents += inPointArrays[i]->GetNumberOfComponents();
   }
  // we add another data array for point coords
   pointArrayOffset[nPointData] = pointDataComponents;
   pointDataComponents += 3;
   nPointData++;

   vtkYoungsMaterialInterface_Mat* Mats = new vtkYoungsMaterialInterface_Mat[nmat];
   {
      int m=0;
      for( vtkstd::vector<vtkYoungsMaterialInterfaceInternals::MaterialDescription>::iterator it=this->Internals->Materials.begin(); it!=this->Internals->Materials.end(); ++it , ++m )
      {
   Mats[m].fractionArray = input->GetCellData()->GetArray( (*it).volume.c_str() );
   Mats[m].normalArray = input->GetCellData()->GetArray( (*it).normal.c_str() );
   Mats[m].normalXArray = input->GetCellData()->GetArray( (*it).normalX.c_str() );
   Mats[m].normalYArray = input->GetCellData()->GetArray( (*it).normalY.c_str() );
   Mats[m].normalZArray = input->GetCellData()->GetArray( (*it).normalZ.c_str() );
   Mats[m].orderingArray = input->GetCellData()->GetArray( (*it).ordering.c_str() );

#ifdef DEBUG
   if( Mats[m].fractionArray == 0 )
   {
      vtkWarningMacro(<<"Material "<<m<<": volume fraction array '"<<(*it).volume<<"' not found\n");
   }
   if( Mats[m].orderingArray == 0 )    
   {
      vtkWarningMacro(<<"Material "<<m<<" material ordering array '"<<(*it).ordering<<"' not found\n");
   }
   if( Mats[m].normalArray==0 && Mats[m].normalXArray==0 && Mats[m].normalYArray==0 && Mats[m].normalZArray==0 )
   {
      vtkWarningMacro(<<"Material "<<m<<" normal  array '"<<(*it).normal<<"' not found\n");
   }
#endif

   Mats[m].numberOfCells = 0;
   Mats[m].cellCount = 0;
   Mats[m].cellArrayCount = 0;

   Mats[m].outCellArrays = new vtkDataArray* [ nCellData ];
   for(int i=0;i<nCellData;i++)
   {
      Mats[m].outCellArrays[i] = vtkDataArray::CreateDataArray( inCellArrays[i]->GetDataType() );
      Mats[m].outCellArrays[i]->SetName( inCellArrays[i]->GetName() );
      Mats[m].outCellArrays[i]->SetNumberOfComponents( inCellArrays[i]->GetNumberOfComponents() );
   }

   Mats[m].numberOfPoints = 0;
   Mats[m].pointCount = 0;
   Mats[m].outPointArrays = new vtkDataArray* [ nPointData ];

   for(int i=0;i<(nPointData-1);i++)
   {
      Mats[m].outPointArrays[i] = vtkDataArray::CreateDataArray( inPointArrays[i]->GetDataType() );
      Mats[m].outPointArrays[i]->SetName( inPointArrays[i]->GetName() );
      Mats[m].outPointArrays[i]->SetNumberOfComponents( inPointArrays[i]->GetNumberOfComponents() );
   }
   Mats[m].outPointArrays[nPointData-1] = vtkDoubleArray::New();
   Mats[m].outPointArrays[nPointData-1]->SetName("Points");
   Mats[m].outPointArrays[nPointData-1]->SetNumberOfComponents(3);
      }
   }

  // --------------- per material number of interfaces estimation ------------
   for(vtkIdType c=0;c<nCells;c++)
   {
      vtkCell* vtkcell = input->GetCell(c);
      int cellDim = vtkcell->GetCellDimension();
      int np = vtkcell->GetNumberOfPoints();
      int nf = vtkcell->GetNumberOfFaces();

      for(int m=0;m<nmat;m++)
      {
   double fraction = ( Mats[m].fractionArray != 0 ) ? Mats[m].fractionArray->GetTuple1(c) : 0 ;
   if( this->CellProduceInterface(cellDim,np,fraction,this->VolumeFractionRange[0],this->VolumeFractionRange[1]) )
   {
      if( cellDim == 2 )
      {
         Mats[m].numberOfPoints += 2;
      }
      else
      {
         Mats[m].numberOfPoints += nf;
      }
      if( this->FillMaterial )
      {
         Mats[m].numberOfPoints += np-1;
      }
      Mats[m].numberOfCells ++;
   }
      }
   }

  // allocation of output arrays
   for(int m=0;m<nmat;m++)
   {
      vtkDebugMacro(<<"Mat #"<<m<<" : cells="<<Mats[m].numberOfCells<<", points="<<Mats[m].numberOfPoints<<", FillMaterial="<<this->FillMaterial<<"\n");
      for(int i=0;i<nCellData;i++)
      {
   Mats[m].outCellArrays[i]->Allocate( Mats[m].numberOfCells * Mats[m].outCellArrays[i]->GetNumberOfComponents() );
      }
      for(int i=0;i<nPointData;i++)
      {
   Mats[m].outPointArrays[i]->Allocate( Mats[m].numberOfPoints * Mats[m].outPointArrays[i]->GetNumberOfComponents() );
      }
      Mats[m].cellTypes.reserve( Mats[m].numberOfCells );
      Mats[m].cells.reserve( Mats[m].numberOfCells + Mats[m].numberOfPoints );
      Mats[m].pointMap = new vtkIdType[ nPoints ];
      for(vtkIdType i=0;i<nPoints;i++) { Mats[m].pointMap[i] = -1; }
   }

  // --------------------------- core computation --------------------------
   vtkIdList *ptIds = vtkIdList::New();
   vtkPoints *pts = vtkPoints::New();
   vtkConvexPointSet* cpsCell = vtkConvexPointSet::New();

   double* interpolatedValues = new double[ MAX_CELL_POINTS * pointDataComponents ];
   vtkYoungsMaterialInterface_IndexedValue * matOrdering = new vtkYoungsMaterialInterface_IndexedValue[nmat];

   vtkstd::vector< vtkstd::pair<int,vtkIdType> > prevPointsMap; 
   prevPointsMap.reserve( MAX_CELL_POINTS*nmat );

   for(vtkIdType ci=0;ci<nCells;ci++)
   {
      int interfaceEdges[MAX_CELL_POINTS*2];
      double interfaceWeights[MAX_CELL_POINTS];
      int nInterfaceEdges;

      int insidePointIds[MAX_CELL_POINTS];
      int nInsidePoints;

      int outsidePointIds[MAX_CELL_POINTS];
      int nOutsidePoints;

      int outCellPointIds[MAX_CELL_POINTS];
      int nOutCellPoints;

      double referenceVolume = 1.0;
      double normal[3];
      bool normaleNulle = false;


      prevPointsMap.clear();

     // sort materials
      int nEffectiveMat = 0;
      for(int mi=0;mi<nmat;mi++)
      {
   matOrdering[mi].index = mi;
   matOrdering[mi].value = ( Mats[mi].orderingArray != 0 ) ? Mats[mi].orderingArray->GetTuple1(ci) : 0.0;

   double fraction = ( Mats[mi].fractionArray != 0 ) ? Mats[mi].fractionArray->GetTuple1(ci) : 0 ;
   if( this->UseFractionAsDistance || fraction>this->VolumeFractionRange[0] ) nEffectiveMat++;
      }
      vtkstd::stable_sort( matOrdering , matOrdering+nmat );

      bool twoMaterialOptimization =  !this->UseFractionAsDistance && this->TwoMaterialsOptimization && (nEffectiveMat==2);
      if( twoMaterialOptimization )
      {
   vtkDebugMacro(<<"2 material optimization triggered for cell #"<<ci<<"\n");
      }

     // read cell information for the first iteration
     // a temporary cell will then be generated after each iteration for the next one.
      vtkCell* vtkcell = input->GetCell(ci);
      CellInfo cell;
      cell.dim = vtkcell->GetCellDimension();
      cell.np = vtkcell->GetNumberOfPoints();
      cell.nf = vtkcell->GetNumberOfFaces();
      cell.type = vtkcell->GetCellType();
      
     /* copy points and point ids to lacal arrays.
     IMPORTANT NOTE : A negative point id refers to a point in the previous material.
     the material number and real point id can be found through the prevPointsMap. */
      for(int p=0;p<cell.np;p++)
      {
   cell.pointIds[p] = vtkcell->GetPointId(p);
   DBG_ASSERT( cell.pointIds[p]>=0 && cell.pointIds[p]<nPoints );
   vtkcell->GetPoints()->GetPoint( p , cell.points[p] );
      }

     /* Triangulate cell.
     IMPORTANT NOTE: triangulation is given with mesh point ids (not local cell ids) 
     and are translated to cell local point ids. */
      cell.needTriangulation = false;
      cell.triangulationOk = ( vtkcell->Triangulate(ci,ptIds,pts) != 0 );
      cell.ntri = 0;
      if( cell.triangulationOk )
      {
   cell.ntri = ptIds->GetNumberOfIds() / (cell.dim+1);
   for(int i=0;i<(cell.ntri*(cell.dim+1));i++)
   {
      vtkIdType j = vtkstd::find( cell.pointIds , cell.pointIds+cell.np , ptIds->GetId(i) ) - cell.pointIds;
      DBG_ASSERT( j>=0 && j<cell.np );
      cell.triangulation[i] = j;
   }
      }
      else
      {
   vtkWarningMacro(<<"Triangulation failed on primary cell\n");
      }

     // get 3D cell edges.
      if( cell.dim == 3 )
      {
   vtkCell3D* cell3D = vtkCell3D::SafeDownCast( vtkcell );
   cell.nEdges = vtkcell->GetNumberOfEdges();
   for(int i=0;i<cell.nEdges;i++)
   {
      int tmp[4];
      int * edgePoints = tmp; 
      cell3D->GetEdgePoints(i,edgePoints);
      cell.edges[i][0] = edgePoints[0];
      DBG_ASSERT( cell.edges[i][0]>=0 && cell.edges[i][0]<cell.np );
      cell.edges[i][1] = edgePoints[1];
      DBG_ASSERT( cell.edges[i][1]>=0 && cell.edges[i][1]<cell.np );
   }
      }

     // for debugging : ensures that we don't read anything from cell, but only from previously filled arrays
      vtkcell = 0; 
      
      int processedEfectiveMat = 0;

     // Loop for each material. Current cell is iteratively cut.
      for(int mi=0;mi<nmat;mi++)
      {
   int m = this->ReverseMaterialOrder ? matOrdering[nmat-1-mi].index : matOrdering[mi].index;

  // get volume fraction and interface plane normal from input arrays
   double fraction = ( Mats[m].fractionArray != 0 ) ? Mats[m].fractionArray->GetTuple1(ci) : 0 ;
  if( !twoMaterialOptimization )
  {
     fraction = (referenceVolume>0) ? (fraction/referenceVolume) : 0.0 ;
  }

   if( this->CellProduceInterface(cell.dim,cell.np,fraction,this->VolumeFractionRange[0],this->VolumeFractionRange[1]) )
   {
      CellInfo nextCell; // empty cell by default
      int interfaceCellType = VTK_EMPTY_CELL;

      if( mi==0 || ( !this->OnionPeel && !twoMaterialOptimization) )
      {
         normal[0]=0; normal[1]=0; normal[2]=0;

         if( Mats[m].normalArray != 0 ) Mats[m].normalArray->GetTuple(ci,normal);
         if( Mats[m].normalXArray != 0 ) normal[0] = Mats[m].normalXArray->GetTuple1(ci);
         if( Mats[m].normalYArray != 0 ) normal[1] = Mats[m].normalYArray->GetTuple1(ci);
         if( Mats[m].normalZArray != 0 ) normal[2] = Mats[m].normalZArray->GetTuple1(ci);

        // work-around for degenerated normals
         if( vtkMath::Norm(normal) == 0.0 ) // should it be <EPSILON ?
         {
#ifdef DEBUG
      vtkWarningMacro(<<"Nul normal\n");
#endif
      normaleNulle=true;
      normal[0]=1.0;
      normal[1]=0.0;
      normal[2]=0.0;
         }
         else
         {
      vtkMath::Normalize( normal );
         }
         if( this->InverseNormal )
         {
      normal[0] = -normal[0];
      normal[1] = -normal[1];
      normal[2] = -normal[2];
         }
      }

     // 2 material case optimization
      if( twoMaterialOptimization && processedEfectiveMat>0 )
      {
      normal[0] = -normal[0];
      normal[1] = -normal[1];
      normal[2] = -normal[2];         
        if( fraction > this->VolumeFractionRange[0] )
        {
             processedEfectiveMat ++;
        }
      }

     // -= case where the entire input cell is passed through =-
      if( ( !this->UseFractionAsDistance && fraction>this->VolumeFractionRange[1] && this->FillMaterial ) || ( this->UseFractionAsDistance && normaleNulle ) )
      {
         interfaceCellType = cell.type;
        //Mats[m].cellTypes.push_back( cell.type );
         nOutCellPoints = nInsidePoints = cell.np;
         nInterfaceEdges = 0;
         nOutsidePoints = 0;
         for(int p=0;p<cell.np;p++) { outCellPointIds[p] = insidePointIds[p] = p; } 
        // remaining volume is an empty cell (nextCell is left as is)
      }

     // -= case where the entire cell is ignored =-
      else if ( !this->UseFractionAsDistance && ( fraction<this->VolumeFractionRange[0] || (fraction>this->VolumeFractionRange[1] && !this->FillMaterial) || !cell.triangulationOk ) )
      {
         interfaceCellType = VTK_EMPTY_CELL;
        //Mats[m].cellTypes.push_back( VTK_EMPTY_CELL );

         nOutCellPoints = 0;
         nInterfaceEdges = 0;
         nInsidePoints = 0;
         nOutsidePoints = 0;

        // remaining volume is the same cell
         nextCell = cell;

         if( !cell.triangulationOk )
         {
      vtkWarningMacro(<<"Cell triangulation failed\n");
         }
      }

     // -= 2D case =-
      else if( cell.dim == 2 ) 
      {
         int nRemCellPoints;
         int remCellPointIds[MAX_CELL_POINTS];

         int triangles[MAX_CELL_POINTS][3];
         for(int i=0;i<cell.ntri;i++) for(int j=0;j<3;j++)
         {
      triangles[i][j] = cell.triangulation[i*3+j];
      DBG_ASSERT( triangles[i][j]>=0 && triangles[i][j]<cell.np );
         }

         bool interfaceFound = vtkYoungsMaterialInterfaceCellCut::cellInterface2D(
      cell.points, cell.np,
      triangles, cell.ntri,
      fraction, normal,
      this->AxisSymetric != 0,
      this->UseFractionAsDistance != 0,
      interfaceEdges, interfaceWeights,
      nOutCellPoints, outCellPointIds,
      nRemCellPoints, remCellPointIds ) ;

         if( interfaceFound )
         {
      nInterfaceEdges = 2;
      interfaceCellType = this->FillMaterial ? VTK_POLYGON : VTK_LINE;
     //Mats[m].cellTypes.push_back( this->FillMaterial ? VTK_POLYGON : VTK_LINE );

     // remaining volume is a polygon
      nextCell.dim = 2;
      nextCell.np = nRemCellPoints;
      nextCell.nf = nRemCellPoints;
      nextCell.type = VTK_POLYGON;

     // build polygon triangulation for next iteration
      nextCell.ntri = nextCell.np-2;
      for(int i=0;i<nextCell.ntri;i++)
      {
         nextCell.triangulation[i*3+0] = 0;
         nextCell.triangulation[i*3+1] = i+1;
         nextCell.triangulation[i*3+2] = i+2;
      }
      nextCell.triangulationOk = true;
      nextCell.needTriangulation = false;

     // populate prevPointsMap and next iteration cell point ids
      int ni = 0;
      for(int i=0;i<nRemCellPoints;i++)
      {
         vtkIdType id = remCellPointIds[i];
         if( id < 0 )
         {
      id = - (int)( prevPointsMap.size() + 1 );
      DBG_ASSERT( (-id-1) == prevPointsMap.size() );
      prevPointsMap.push_back( vtkstd::make_pair( m , Mats[m].pointCount+ni ) ); // intersection points will be added first
      ni++;
         }
         else
         {
      DBG_ASSERT( id>=0 && id<cell.np );
      id = cell.pointIds[ id ];
         }
         nextCell.pointIds[i] = id;
      }
      DBG_ASSERT( ni == nInterfaceEdges );

     // filter out points inside material volume
      nInsidePoints = 0;
      for(int i=0;i<nOutCellPoints;i++)
      {
         if( outCellPointIds[i] >= 0 ) insidePointIds[nInsidePoints++] = outCellPointIds[i];
      }

      if( ! this->FillMaterial ) // keep only interface points
      {
         int n = 0;
         for(int i=0;i<nOutCellPoints;i++)
         {
      if( outCellPointIds[i] < 0 ) outCellPointIds[n++] = outCellPointIds[i];
         }
         nOutCellPoints = n;
      }
         }
         else
         {
     //vtkWarningMacro(<<"no interface found for cell "<<ci<<", mi="<<mi<<", m="<<m<<", frac="<<fraction<<"\n");
      nInterfaceEdges = 0;
      nOutCellPoints = 0;
      nInsidePoints = 0;
      nOutsidePoints = 0;
      interfaceCellType = VTK_EMPTY_CELL;
     //Mats[m].cellTypes.push_back( VTK_EMPTY_CELL );
     // remaining volume is the original cell left unmodified
      nextCell = cell;
         }

      }

     // -= 3D case =-
      else
      {
         int tetras[MAX_CELL_POINTS][4];
         for(int i=0;i<cell.ntri;i++) for(int j=0;j<4;j++)
         {
      tetras[i][j] = cell.triangulation[i*4+j];
         }

        // compute innterface polygon
         vtkYoungsMaterialInterfaceCellCut::cellInterface3D(
      cell.np, cell.points,
      cell.nEdges, cell.edges,
      cell.ntri, tetras,
      fraction, normal,
      this->UseFractionAsDistance != 0,
      nInterfaceEdges, interfaceEdges, interfaceWeights,
      nInsidePoints, insidePointIds,
      nOutsidePoints, outsidePointIds );

         if( nInterfaceEdges>cell.nf || nInterfaceEdges<3 ) // degenerated case, considered as null interface
         {
      vtkWarningMacro(<<"no interface found for cell "<<ci<<", mi="<<mi<<", m="<<m<<", frac="<<fraction<<"\n");
      nInterfaceEdges = 0;
      nOutCellPoints = 0;
      nInsidePoints = 0;
      nOutsidePoints = 0;
      interfaceCellType = VTK_EMPTY_CELL;
     //Mats[m].cellTypes.push_back( VTK_EMPTY_CELL );

     // in this case, next iteration cell is the same
      nextCell = cell;
         }
         else
         {
      nOutCellPoints = 0;

      for(int e=0;e<nInterfaceEdges;e++) 
      { 
         outCellPointIds[nOutCellPoints++] = -e -1; 
      }

      if(this->FillMaterial) 
      {
         interfaceCellType = VTK_CONVEX_POINT_SET;
        //Mats[m].cellTypes.push_back( VTK_CONVEX_POINT_SET );
         for(int p=0;p<nInsidePoints;p++)
         {
      outCellPointIds[nOutCellPoints++] = insidePointIds[p];
         }
      }
      else
      {
         interfaceCellType = VTK_POLYGON;
        //Mats[m].cellTypes.push_back( VTK_POLYGON );
      }

     /* remaining volume is a convex point set
     IMPORTANT NOTE: next iteration cell cannot be entirely built right now. 
     in this particular case we'll finish it at the end of the material loop */
      if( mi<(nmat-1) && !twoMaterialOptimization )
      {
         nextCell.type = VTK_CONVEX_POINT_SET;     
         nextCell.np = nInterfaceEdges + nOutsidePoints;
         vtkcell = cpsCell;
         vtkcell->Points->Reset();
         vtkcell->PointIds->Reset();
         vtkcell->Points->SetNumberOfPoints( nextCell.np );
         vtkcell->PointIds->SetNumberOfIds( nextCell.np );
         for(int i=0;i<nextCell.np;i++)
         {
      vtkcell->PointIds->SetId( i, i );
         }
        // nf, ntri and triangulation have to be computed later on, when point coords are computed
         nextCell.needTriangulation = true;
      }

      for(int i=0;i<nInterfaceEdges;i++)
      {
         vtkIdType id = - (int) ( prevPointsMap.size() + 1 );
         DBG_ASSERT( (-id-1) == prevPointsMap.size() );
         prevPointsMap.push_back( vtkstd::make_pair( m , Mats[m].pointCount+i ) ); // we know that interpolated points will be added consecutively
         nextCell.pointIds[i] = id;
      }
      for(int i=0;i<nOutsidePoints;i++)
      {
         nextCell.pointIds[nInterfaceEdges+i] = cell.pointIds[ outsidePointIds[i] ];
      }
         }

        // check correctness of next cell's point ids
         for(int i=0;i<nextCell.np;i++)
         {
      DBG_ASSERT( ( nextCell.pointIds[i]<0 && (-nextCell.pointIds[i]-1)<prevPointsMap.size() ) || ( nextCell.pointIds[i]>=0 && nextCell.pointIds[i]<nPoints ) );
         }

      } // End 3D case


     //  create output cell 
      if( interfaceCellType != VTK_EMPTY_CELL )
      {

        // set type of cell
         Mats[m].cellTypes.push_back( interfaceCellType );

        // interpolate point values for cut edges
         for(int e=0;e<nInterfaceEdges;e++)
         {
      double t = interfaceWeights[e];
      for(int p=0;p<nPointData;p++)
      {
         double v0[16];
         double v1[16];
         int nc = Mats[m].outPointArrays[p]->GetNumberOfComponents();
         int ep0 = cell.pointIds[ interfaceEdges[e*2+0] ];
         int ep1 = cell.pointIds[ interfaceEdges[e*2+1] ];
         GET_POINT_DATA( p , ep0 , v0 );
         GET_POINT_DATA( p , ep1 , v1 );
         for(int c=0;c<nc;c++)
         {
      interpolatedValues[ e*pointDataComponents + pointArrayOffset[p] + c ] = v0[c] + t * ( v1[c] - v0[c] );
         }
      }
         }

        // copy point values
         for(int e=0;e<nInterfaceEdges;e++)
         {
      vtkIdType nptId = Mats[m].pointCount + e;
      for(int a=0;a<nPointData;a++) 
      {
         DBG_ASSERT( nptId == Mats[m].outPointArrays[a]->GetNumberOfTuples() );
         Mats[m].outPointArrays[a]->InsertNextTuple( interpolatedValues + e*pointDataComponents + pointArrayOffset[a] );
      }
         }
         int pointsCopied = 0;
         int prevMatInterfToBeAdded = 0;
         if( this->FillMaterial )
         {
      for(int p=0;p<nInsidePoints;p++)
      {
         vtkIdType ptId = cell.pointIds[ insidePointIds[p] ]; 
         if( ptId>=0 )
         {
      if( Mats[m].pointMap[ptId] == -1 )
      {
         vtkIdType nptId = Mats[m].pointCount + nInterfaceEdges + pointsCopied;
         Mats[m].pointMap[ptId] = nptId;
         pointsCopied++;
         for(int a=0;a<nPointData;a++)
         {
            DBG_ASSERT( nptId == Mats[m].outPointArrays[a]->GetNumberOfTuples() );
            double tuple[16];
            GET_POINT_DATA( a, ptId, tuple );
            Mats[m].outPointArrays[a]->InsertNextTuple( tuple );
         }
      }
         }
         else
         {
      prevMatInterfToBeAdded++;
         }
      }
         }

        // populate connectivity array 
        // and add extra points from previous edge intersections that are used but not inserted yet
         int prevMatInterfAdded = 0;
         Mats[m].cells.push_back( nOutCellPoints ); Mats[m].cellArrayCount++;
         for(int p=0;p<nOutCellPoints;p++)
         {
      int nptId;
      int pointIndex = outCellPointIds[p];
      if( pointIndex >=0 ) // an original point (not an edge intersection)
      {
         DBG_ASSERT( pointIndex>=0 && pointIndex<cell.np ); 
         int ptId = cell.pointIds[ pointIndex ]; 
         if( ptId>=0 )
         {
      DBG_ASSERT( ptId>=0 && ptId<nPoints ); // OUI, car interface d'une iteration precedente. que faire ...
      nptId = Mats[m].pointMap[ptId];
         }
         else
         {
      nptId = Mats[m].pointCount + nInterfaceEdges + pointsCopied + prevMatInterfAdded;
      prevMatInterfAdded++;
      for(int a=0;a<nPointData;a++)
      {
         DBG_ASSERT( nptId == Mats[m].outPointArrays[a]->GetNumberOfTuples() );
         double tuple[16];
         GET_POINT_DATA( a, ptId, tuple );
         Mats[m].outPointArrays[a]->InsertNextTuple( tuple );
      }
         }
      }
      else
      {
         int interfaceIndex = -pointIndex - 1;
         DBG_ASSERT( interfaceIndex>=0 && interfaceIndex<nInterfaceEdges );
         nptId = Mats[m].pointCount + interfaceIndex ;
      }
      DBG_ASSERT( nptId>=0 && nptId<(Mats[m].pointCount+nInterfaceEdges+pointsCopied+prevMatInterfToBeAdded) );
      Mats[m].cells.push_back( nptId ); Mats[m].cellArrayCount++;
         }

         Mats[m].pointCount += nInterfaceEdges + pointsCopied + prevMatInterfAdded;

        // copy cell arrays
         for(int a=0;a<nCellData;a++)
         {
      Mats[m].outCellArrays[a]->InsertNextTuple( inCellArrays[a]->GetTuple(ci) );
         }
         Mats[m].cellCount ++;

        // check for equivalence between counters and container sizes
         DBG_ASSERT( Mats[m].cellCount == Mats[m].cellTypes.size() );
         DBG_ASSERT( Mats[m].cellArrayCount == Mats[m].cells.size() );
      
        // populate next iteration cell's point coords
         for(int i=0;i<nextCell.np;i++)
         {
      DBG_ASSERT( ( nextCell.pointIds[i]<0 && (-nextCell.pointIds[i]-1)<prevPointsMap.size() ) || ( nextCell.pointIds[i]>=0 && nextCell.pointIds[i]<nPoints ) );
      GET_POINT_DATA( (nPointData-1) , nextCell.pointIds[i] , nextCell.points[i] );
         }

        // for the convex point set, we need to first compute point coords before triangulation (no fixed topology)
         if( nextCell.needTriangulation  && mi<(nmat-1) )
         {
      vtkcell->Initialize();
      nextCell.nf = vtkcell->GetNumberOfFaces();
      if( nextCell.dim == 3 )
      {
         vtkCell3D* cell3D = vtkCell3D::SafeDownCast( vtkcell );
         nextCell.nEdges = vtkcell->GetNumberOfEdges();
         for(int i=0;i<nextCell.nEdges;i++)
         {
      int tmp[4];
      int * edgePoints = tmp; 
      cell3D->GetEdgePoints(i,edgePoints);
      nextCell.edges[i][0] = edgePoints[0];
      DBG_ASSERT( nextCell.edges[i][0]>=0 && nextCell.edges[i][0]<nextCell.np );
      nextCell.edges[i][1] = edgePoints[1];
      DBG_ASSERT( nextCell.edges[i][1]>=0 && nextCell.edges[i][1]<nextCell.np );
         }
      }
      nextCell.triangulationOk = ( vtkcell->Triangulate(ci,ptIds,pts) != 0 );
      nextCell.ntri = 0;
      if( nextCell.triangulationOk )
      {
         nextCell.ntri = ptIds->GetNumberOfIds() / (nextCell.dim+1);
         for(int i=0;i<(nextCell.ntri*(nextCell.dim+1));i++)
         {
      vtkIdType j = ptIds->GetId(i); // cell ids have been set with local ids
      DBG_ASSERT( j>=0 && j<nextCell.np );
      nextCell.triangulation[i] = j;
         }
      }
      else
      {
        vtkWarningMacro(<<"Triangulation failed. Info: cell "<<ci<<", material "<<mi<<", np="<<nextCell.np<<", nf="<<nextCell.nf<<", ne="<<nextCell.nEdges<<"\n");
      }
      nextCell.needTriangulation = false;
      vtkcell = 0;
         }

        // switch to next cell
         if( !twoMaterialOptimization )
         {
      cell = nextCell;
         }

      } // end of 'interface was found'
      else
      {
         vtkcell = 0;
      }

   } // end of 'cell is ok'

   else // cell is ignored
   {
     //vtkWarningMacro(<<"ignoring cell #"<<ci<<", m="<<m<<", mi="<<mi<<", frac="<<fraction<<"\n");
   }

  // update reference volume
   if( !twoMaterialOptimization )
  {
     referenceVolume -= fraction;
  }

      } // for materials

   } // for cells
   delete [] pointArrayOffset;
   delete [] inPointArrays;
   delete [] inCellArrays;

   ptIds->Delete();
   pts->Delete();
   cpsCell->Delete();
   delete [] interpolatedValues;
   delete [] matOrdering;

  // finish output creation
   output->SetNumberOfBlocks( nmat );
   for(int m=0;m<nmat;m++)
   {
     //if( Mats[m].cellCount > Mats[m].numberOfCells )
      {
   vtkDebugMacro(<<"Mat #"<<m<<" : cellCount="<<Mats[m].cellCount<<", numberOfCells="<<Mats[m].numberOfCells<<"\n");
      }
     //if( Mats[m].pointCount > Mats[m].numberOfPoints )
      {
   vtkDebugMacro(<<"Mat #"<<m<<" : pointCount="<<Mats[m].pointCount<<", numberOfPoints="<<Mats[m].numberOfPoints<<"\n");
      }

      delete [] Mats[m].pointMap;

      vtkUnstructuredGrid* ugOutput = vtkUnstructuredGrid::New();

     // set points
      Mats[m].outPointArrays[nPointData-1]->Squeeze();
      vtkPoints* points = vtkPoints::New();
      points->SetDataTypeToDouble();
      points->SetNumberOfPoints( Mats[m].pointCount );
      points->SetData( Mats[m].outPointArrays[nPointData-1] );
      Mats[m].outPointArrays[nPointData-1]->Delete();
      ugOutput->SetPoints( points );
      points->Delete();

     // set cell connectivity
      vtkIdTypeArray* cellArrayData = vtkIdTypeArray::New();
      cellArrayData->SetNumberOfValues( Mats[m].cellArrayCount );
      vtkIdType* cellArrayDataPtr = cellArrayData->WritePointer(0,Mats[m].cellArrayCount);
      for(vtkIdType i=0;i<Mats[m].cellArrayCount;i++) cellArrayDataPtr[i] = Mats[m].cells[i];

      vtkCellArray* cellArray = vtkCellArray::New();
      cellArray->SetCells( Mats[m].cellCount , cellArrayData );
      cellArrayData->Delete();

     // set cell types
      vtkUnsignedCharArray *cellTypes = vtkUnsignedCharArray::New();
      cellTypes->SetNumberOfValues( Mats[m].cellCount );
      unsigned char* cellTypesPtr = cellTypes->WritePointer(0,Mats[m].cellCount);
      for(vtkIdType i=0;i<Mats[m].cellCount;i++) cellTypesPtr[i] = Mats[m].cellTypes[i];

     // set cell locations
      vtkIdTypeArray* cellLocations = vtkIdTypeArray::New();
      cellLocations->SetNumberOfValues( Mats[m].cellCount );
      vtkIdType counter = 0;
      for(vtkIdType i=0;i<Mats[m].cellCount;i++)
      {
   cellLocations->SetValue(i,counter);
   counter += Mats[m].cells[counter] + 1;
      }

     // attach conectivity arrays to data set
      ugOutput->SetCells( cellTypes, cellLocations, cellArray );
      cellArray->Delete();
      cellTypes->Delete();
      cellLocations->Delete();

     // attach point arrays
      for(int i=0;i<nPointData-1;i++)
      {
   Mats[m].outPointArrays[i]->Squeeze();
   ugOutput->GetPointData()->AddArray( Mats[m].outPointArrays[i] );
   Mats[m].outPointArrays[i]->Delete();
      }

     // attach cell arrays
      for(int i=0;i<nCellData;i++)
      {
   Mats[m].outCellArrays[i]->Squeeze();
   ugOutput->GetCellData()->AddArray( Mats[m].outCellArrays[i] );
   Mats[m].outCellArrays[i]->Delete();
      }

      delete [] Mats[m].outCellArrays;
      delete [] Mats[m].outPointArrays;

     // activate attributes similarily to input
      for(int i=0;i<vtkDataSetAttributes::NUM_ATTRIBUTES;i++)
      {
   vtkDataArray* attr = input->GetCellData()->GetAttribute(i);
   if( attr!=0 )
   {
      ugOutput->GetCellData()->SetActiveAttribute(attr->GetName(),i);
   }
      }
      for(int i=0;i<vtkDataSetAttributes::NUM_ATTRIBUTES;i++)
      {
   vtkDataArray* attr = input->GetPointData()->GetAttribute(i);
   if( attr!=0 )
   {
      ugOutput->GetPointData()->SetActiveAttribute(attr->GetName(),i);
   }
      }

     // add material data set to multiblock output
#if VTK_MINOR_VERSION > 2
      output->SetBlock(m,ugOutput);
#else
      output->SetNumberOfDataSets(m,1);
      output->SetDataSet (m,0,ugOutput);
#endif
      ugOutput->Delete();
   }
   delete [] Mats;

   return 1;
}

#undef GET_POINT_DATA

