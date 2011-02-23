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
#include "vtkObjectFactory.h"

#include "vtkCell.h"
#include "vtkEmptyCell.h"
#include "vtkPolygon.h"
#include "vtkConvexPointSet.h"
#include "vtkDataSet.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkDataArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkIdTypeArray.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkUnstructuredGrid.h"
#include "vtkDoubleArray.h"
#include "vtkMath.h"
#include "vtkIdList.h"

#include <vtkstd/vector>
#include <vtkstd/string> 
#include <vtkstd/map>
#include <vtkstd/algorithm>

#include <math.h>
#include <assert.h>

class vtkYoungsMaterialInterfaceCellCut
{
public:

  enum {
    MAX_CELL_POINTS = 128,
    MAX_CELL_TETRAS = 128
  };


  static void cellInterface3D( 
   
                              // Inputs
                              int ncoords,
                              double coords[][3],
                              int nedge,
                              int cellEdges[][2],
                              int ntetra,
                              int tetraPointIds[][4],
                              double fraction, double normal[3] , 
                              bool useFractionAsDistance,

                              // Outputs
                              int & np, int eids[], double weights[] ,
                              int & nInside, int inPoints[],
                              int & nOutside, int outPoints[] );


  static double findTetraSetCuttingPlane(
                                         const double normal[3],
                                         const double fraction,
                                         const int vertexCount,
                                         const double vertices[][3],
                                         const int tetraCount,
                                         const int tetras[][4] );

  static bool cellInterface2D( 

                              // Inputs
                              double points[][3],
                              int nPoints,
                              int triangles[][3], 
                              int nTriangles,
                              double fraction, double normal[3] ,
                              bool axisSymetric,
                              bool useFractionAsDistance,

                              // Outputs
                              int eids[4], double weights[2] ,
                              int &polygonPoints, int polygonIds[],
                              int &nRemPoints, int remPoints[] );


  static double findTriangleSetCuttingPlane(
                                            const double normal[3],
                                            const double fraction,
                                            const int vertexCount,
                                            const double vertices[][3],
                                            const int triangleCount,
                                            const int triangles[][3],
                                            bool axisSymetric=false );


} ;




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
vtkStandardNewMacro(vtkYoungsMaterialInterface);

#ifdef DEBUG
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

void vtkYoungsMaterialInterface::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "FillMaterial: " << this->FillMaterial << "\n";
  os << indent << "InverseNormal: " << this->InverseNormal << "\n";
  os << indent << "AxisSymetric: " << this->AxisSymetric << "\n";
  os << indent << "OnionPeel: " << this->OnionPeel << "\n";
  os << indent << "ReverseMaterialOrder: " << this->ReverseMaterialOrder << "\n";
  os << indent << "UseFractionAsDistance: " << this->UseFractionAsDistance << "\n";
  os << indent << "VolumeFractionRange: [" << this->VolumeFractionRange[0] << ";" << this->VolumeFractionRange[1] <<"]\n";
  os << indent << "TwoMaterialsOptimization: " << this->TwoMaterialsOptimization << "\n";
}

void vtkYoungsMaterialInterface::SetNumberOfMaterials(int n)
{
  vtkDebugMacro(<<"Resize Materials to "<<n<<"\n");
  this->Internals->Materials.resize(n);
}

int vtkYoungsMaterialInterface::GetNumberOfMaterials()
{
  return static_cast<int>( this->Internals->Materials.size() );
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
                                        int vtkNotUsed(nmat),
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

int vtkYoungsMaterialInterface::RequestData(vtkInformation *vtkNotUsed(request),
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

  int nmat = static_cast<int>( this->Internals->Materials.size() );
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
              vtkDebugMacro(<<"no interface found for cell "<<ci<<", mi="<<mi<<", m="<<m<<", frac="<<fraction<<"\n");
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
#ifdef DEBUG
              vtkIdType nptId = Mats[m].pointCount + e;
#endif
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





/* ------------------------------------------------------------------------------------------
   --- Low level computations including interface placement and intersection line/polygon ---
   ------------------------------------------------------------------------------------------ */

// here after the low-level functions that compute placement of the interface given a normal vector and a set of simplices
namespace vtkYoungsMaterialInterfaceCellCutInternals
{
#define REAL_PRECISION 64 // use double precision
#define REAL_COORD REAL3

// par defaut, on est en double
#ifndef REAL_PRECISION
#define REAL_PRECISION 64
#endif

// float = precision la plus basse
#if ( REAL_PRECISION == 32 )

#define REAL  float
#define REAL2 float2
#define REAL3 float3
#define REAL4 float4

#define make_REAL1 make_float1
#define make_REAL2 make_float2
#define make_REAL3 make_float3
#define make_REAL4 make_float4

#define SQRT sqrtf
#define FABS fabsf
#define REAL_CONST(x) ((float)(x)) //( x##f )


// long double = highest precision
#elif ( REAL_PRECISION > 64 )

#define REAL  long double
#define REAL2 ldouble2
#define REAL3 ldouble3
#define REAL4 ldouble4

#define make_REAL1 make_ldouble1
#define make_REAL2 make_ldouble2
#define make_REAL3 make_ldouble3
#define make_REAL4 make_ldouble4

#define SQRT  sqrtl
#define FABS  fabsl

#define REAL_CONST(x) ((long double)(x)) //( x##l )


// double = default precision
#else

#define REAL  double
#define REAL2 double2
#define REAL3 double3
#define REAL4 double4

#define make_REAL1 make_double1
#define make_REAL2 make_double2
#define make_REAL3 make_double3
#define make_REAL4 make_double4

#define SQRT  sqrt
#define FABS  fabs

#define REAL_CONST(x) x

#endif


#ifndef __CUDACC__ /* compiling with host compiler (gcc, icc, etc.) */

#ifndef FUNC_DECL
#define FUNC_DECL static inline
#endif

#ifndef KERNEL_DECL
#define KERNEL_DECL /* exported function */
#endif

#ifndef CONSTANT_DECL
#define CONSTANT_DECL static const
#endif

#ifndef REAL_PRECISION
#define REAL_PRECISION 64 /* defaults to 64 bits floating point */
#endif

#else /* compiling with cuda */

#ifndef FUNC_DECL
#define FUNC_DECL __device__
#endif

#ifndef KERNEL_DECL
#define KERNEL_DECL __global__
#endif

#ifndef CONSTANT_DECL
#define CONSTANT_DECL __constant__
#endif

#ifndef REAL_PRECISION
#define REAL_PRECISION 32 /* defaults to 32 bits floating point */
#endif

#endif /* __CUDACC__ */



/*
  Some of the vector functions where found in the file vector_operators.h from the NVIDIA's CUDA Toolkit.
  Please read the above notice.
*/

/*
 * Copyright 1993-2007 NVIDIA Corporation.  All rights reserved.
 *
 * NOTICE TO USER:   
 *
 * This source code is subject to NVIDIA ownership rights under U.S. and 
 * international Copyright laws.  Users and possessors of this source code 
 * are hereby granted a nonexclusive, royalty-free license to use this code 
 * in individual and commercial software.
 *
 * NVIDIA MAKES NO REPRESENTATION ABOUT THE SUITABILITY OF THIS SOURCE 
 * CODE FOR ANY PURPOSE.  IT IS PROVIDED "AS IS" WITHOUT EXPRESS OR 
 * IMPLIED WARRANTY OF ANY KIND.  NVIDIA DISCLAIMS ALL WARRANTIES WITH 
 * REGARD TO THIS SOURCE CODE, INCLUDING ALL IMPLIED WARRANTIES OF 
 * MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE.
 * IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL, 
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS 
 * OF USE, DATA OR PROFITS,  WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE 
 * OR OTHER TORTIOUS ACTION,  ARISING OUT OF OR IN CONNECTION WITH THE USE 
 * OR PERFORMANCE OF THIS SOURCE CODE.  
 *
 * U.S. Government End Users.   This source code is a "commercial item" as 
 * that term is defined at  48 C.F.R. 2.101 (OCT 1995), consisting  of 
 * "commercial computer  software"  and "commercial computer software 
 * documentation" as such terms are  used in 48 C.F.R. 12.212 (SEPT 1995) 
 * and is provided to the U.S. Government only as a commercial end item.  
 * Consistent with 48 C.F.R.12.212 and 48 C.F.R. 227.7202-1 through 
 * 227.7202-4 (JUNE 1995), all U.S. Government End Users acquire the 
 * source code with only those rights set forth herein. 
 *
 * Any use of this source code in individual and commercial software must 
 * include, in the user documentation and internal comments to the code,
 * the above Disclaimer and U.S. Government End Users Notice.
 */

// define base vector types and operators or use those provided by CUDA
#ifndef __CUDACC__
  struct float2 { float x,y; };
  struct float3 { float x,y,z; };
  struct float4 { float x,y,z,w; };
  struct double2 { double x,y; };
  struct uint3 {unsigned int x,y,z; };
  struct uint4 {unsigned int x,y,z,w; };
  struct uchar4 {unsigned char x,y,z,w; };
  struct uchar3 {unsigned char x,y,z; };
  FUNC_DECL float2 make_float2(float x,float y)
  {
    float2 v = {x,y};
    return v;
  }
  FUNC_DECL float3 make_float3(float x,float y,float z)
  {
    float3 v = {x,y,z};
    return v;
  }
  FUNC_DECL float4 make_float4(float x,float y,float z,float w)
  {
    float4 v = {x,y,z,w};
    return v;
  }

  FUNC_DECL float min(float a, float b){ return (a<b)?a:b; }
  FUNC_DECL float max(float a, float b){ return (a>b)?a:b; }

#else
#include <vector_types.h>
#include <vector_functions.h>
#endif

#ifndef FUNC_DECL
#define FUNC_DECL static inline
#endif



/* -------------------------------------------------------- */
/* -----------  FLOAT ------------------------------------- */
/* -------------------------------------------------------- */
#if REAL_PRECISION <= 32

  FUNC_DECL  float3 operator *(float3 a, float3 b)
  {
    return make_float3(a.x*b.x, a.y*b.y, a.z*b.z);
  }

  FUNC_DECL float3 operator *(float f, float3 v)
  {
    return make_float3(v.x*f, v.y*f, v.z*f);
  }

  FUNC_DECL float2 operator *(float f, float2 v)
  {
    return make_float2(v.x*f, v.y*f);
  }

  FUNC_DECL float3 operator *(float3 v, float f)
  {
    return make_float3(v.x*f, v.y*f, v.z*f);
  }

  FUNC_DECL float2 operator *(float2 v,float f)
  {
    return make_float2(v.x*f, v.y*f);
  }

  FUNC_DECL float4 operator *(float4 v, float f)
  {
    return make_float4(v.x*f, v.y*f, v.z*f, v.w*f);
  }
  FUNC_DECL float4 operator *(float f, float4 v)
  {
    return make_float4(v.x*f, v.y*f, v.z*f, v.w*f);
  }


  FUNC_DECL float2 operator +(float2 a, float2 b)
  {
    return make_float2(a.x+b.x, a.y+b.y);
  }


  FUNC_DECL float3 operator +(float3 a, float3 b)
  {
    return make_float3(a.x+b.x, a.y+b.y, a.z+b.z);
  }

  FUNC_DECL void operator +=(float3 & b, float3 a)
  {
    b.x += a.x;
    b.y += a.y;
    b.z += a.z;
  }
  FUNC_DECL void operator +=(float2 & b, float2 a)
  {
    b.x += a.x;
    b.y += a.y;
  }


  FUNC_DECL void operator +=(float4 & b, float4 a)
  {
    b.x += a.x;
    b.y += a.y;
    b.z += a.z;
    b.w += a.w;
  }

  FUNC_DECL float3 operator -(float3 a, float3 b)
  {
    return make_float3(a.x-b.x, a.y-b.y, a.z-b.z);
  }

  FUNC_DECL float2 operator -(float2 a, float2 b)
  {
    return make_float2(a.x-b.x, a.y-b.y);
  }

  FUNC_DECL void operator -=(float3 & b, float3 a)
  {
    b.x -= a.x;
    b.y -= a.y;
    b.z -= a.z;
  }

  FUNC_DECL float3 operator /(float3 v, float f)
  {
    float inv = 1.0f / f;
    return v * inv;
  }

  FUNC_DECL void operator /=(float2 & b, float f)
  {
    float inv = 1.0f / f;
    b.x *= inv;
    b.y *= inv;
  }

  FUNC_DECL void operator /=(float3 & b, float f)
  {
    float inv = 1.0f / f;
    b.x *= inv;
    b.y *= inv;
    b.z *= inv;
  }

  FUNC_DECL float dot(float2 a, float2 b)
  {
    return a.x * b.x + a.y * b.y;
  }

  FUNC_DECL float dot(float3 a, float3 b)
  {
    return a.x * b.x + a.y * b.y + a.z * b.z;
  }

  FUNC_DECL float dot(float4 a, float4 b)
  {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
  }

  FUNC_DECL float clamp(float f, float a, float b)
  {
    return max(a, min(f, b));
  }

  FUNC_DECL float3 clamp(float3 v, float a, float b)
  {
    return make_float3(clamp(v.x, a, b), clamp(v.y, a, b), clamp(v.z, a, b));
  }

  FUNC_DECL float3 clamp(float3 v, float3 a, float3 b)
  {
    return make_float3(clamp(v.x, a.x, b.x), clamp(v.y, a.y, b.y), clamp(v.z, a.z, b.z));
  }

  FUNC_DECL float2 normalize(float2 v)
  {
    float len = 1.0f / sqrtf(dot(v, v));
    return make_float2(v.x * len, v.y * len);
  }

  FUNC_DECL float3 normalize(float3 v)
  {
    float len = 1.0f / sqrtf(dot(v, v));
    return make_float3(v.x * len, v.y * len, v.z * len);
  }

  FUNC_DECL float3 cross( float3 A, float3 B)
  {
    return make_float3( A.y * B.z - A.z * B.y ,
                        A.z * B.x - A.x * B.z ,
                        A.x * B.y - A.y * B.x );
  }

#endif /* REAL_PRECISION <= 32 */

#ifndef __CUDACC__



/* -------------------------------------------------------- */
/* ----------- DOUBLE ------------------------------------- */
/* -------------------------------------------------------- */
#if REAL_PRECISION == 64

  struct double3 { double x,y,z; };
  struct double4 { double x,y,z,w; };

  FUNC_DECL double min(double a, double b){ return (a<b)?a:b; }
  FUNC_DECL double max(double a, double b){ return (a>b)?a:b; }

  FUNC_DECL double2 make_double2(double x,double y)
  {
    double2 v = {x,y};
    return v;
  }

  FUNC_DECL double3 make_double3(double x,double y,double z)
  {
    double3 v = {x,y,z};
    return v;
  }

  FUNC_DECL double4 make_double4(double x,double y,double z,double w)
  {
    double4 v = {x,y,z,w};
    return v;
  }

  FUNC_DECL  double3 operator *(double3 a, double3 b)
  {
    return make_double3(a.x*b.x, a.y*b.y, a.z*b.z);
  }

  FUNC_DECL double3 operator *(double f, double3 v)
  {
    return make_double3(v.x*f, v.y*f, v.z*f);
  }

  FUNC_DECL double3 operator *(double3 v, double f)
  {
    return make_double3(v.x*f, v.y*f, v.z*f);
  }

  FUNC_DECL double2 operator *(double2 v, double f)
  {
    return make_double2(v.x*f, v.y*f);
  }

  FUNC_DECL double2 operator *(double f, double2 v)
  {
    return make_double2(v.x*f, v.y*f);
  }

  FUNC_DECL double4 operator *(double4 v, double f)
  {
    return make_double4(v.x*f, v.y*f, v.z*f, v.w*f);
  }
  FUNC_DECL double4 operator *(double f, double4 v)
  {
    return make_double4(v.x*f, v.y*f, v.z*f, v.w*f);
  }


  FUNC_DECL double3 operator +(double3 a, double3 b)
  {
    return make_double3(a.x+b.x, a.y+b.y, a.z+b.z);
  }

  FUNC_DECL double2 operator +(double2 a, double2 b)
  {
    return make_double2(a.x+b.x, a.y+b.y);
  }

  FUNC_DECL void operator +=(double3 & b, double3 a)
  {
    b.x += a.x;
    b.y += a.y;
    b.z += a.z;
  }
  FUNC_DECL void operator +=(double2 & b, double2 a)
  {
    b.x += a.x;
    b.y += a.y;
  }


  FUNC_DECL void operator +=(double4 & b, double4 a)
  {
    b.x += a.x;
    b.y += a.y;
    b.z += a.z;
    b.w += a.w;
  }

  FUNC_DECL double3 operator - (double3 a, double3 b)
  {
    return make_double3(a.x-b.x, a.y-b.y, a.z-b.z);
  }

  FUNC_DECL double2 operator - (double2 a, double2 b)
  {
    return make_double2(a.x-b.x, a.y-b.y);
  }

  FUNC_DECL void operator -= (double3 & b, double3 a)
  {
    b.x -= a.x;
    b.y -= a.y;
    b.z -= a.z;
  }

  FUNC_DECL double3 operator / (double3 v, double f)
  {
    return make_double3( v.x/f, v.y/f, v.z/f );
  }

  FUNC_DECL void operator /=(double2 & b, double f)
  {
    b.x /= f;
    b.y /= f;
  }

  FUNC_DECL void operator /=(double3 & b, double f)
  {
    b.x /= f;
    b.y /= f;
    b.z /= f;
  }

  FUNC_DECL double dot(double2 a, double2 b)
  {
    return a.x * b.x + a.y * b.y ;
  }

  FUNC_DECL double dot(double3 a, double3 b)
  {
    return a.x * b.x + a.y * b.y + a.z * b.z;
  }

  FUNC_DECL double dot(double4 a, double4 b)
  {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
  }

  FUNC_DECL double clamp(double f, double a, double b)
  {
    return max(a, min(f, b));
  }

  FUNC_DECL double3 clamp(double3 v, double a, double b)
  {
    return make_double3(clamp(v.x, a, b), clamp(v.y, a, b), clamp(v.z, a, b));
  }

  FUNC_DECL double3 clamp(double3 v, double3 a, double3 b)
  {
    return make_double3(clamp(v.x, a.x, b.x), clamp(v.y, a.y, b.y), clamp(v.z, a.z, b.z));
  }

  FUNC_DECL double3 normalize(double3 v)
  {
    double len = sqrt(dot(v, v));
    return make_double3(v.x / len, v.y / len, v.z / len);
  }

  FUNC_DECL double2 normalize(double2 v)
  {
    double len = sqrt( dot(v,v) );
    return make_double2(v.x / len, v.y / len);
  }

  FUNC_DECL double3 cross( double3 A, double3 B)
  {
    return make_double3( A.y * B.z - A.z * B.y ,
                         A.z * B.x - A.x * B.z ,
                         A.x * B.y - A.y * B.x );
  }
#endif /* REAL_PRECISION == 64 */



/* -------------------------------------------------------- */
/* ----------- LONG DOUBLE -------------------------------- */
/* -------------------------------------------------------- */
#if REAL_PRECISION > 64

  struct ldouble2 { long double x,y; };
  struct ldouble3 { long double x,y,z; };
  struct ldouble4 { long double x,y,z,w; };

  FUNC_DECL long double min(long double a, long double b){ return (a<b)?a:b; }
  FUNC_DECL long double max(long double a, long double b){ return (a>b)?a:b; }

  FUNC_DECL ldouble2 make_ldouble2(long double x,long double y)
  {
    ldouble2 v = {x,y};
    return v;
  }


  FUNC_DECL ldouble3 make_ldouble3(long double x,long double y,long double z)
  {
    ldouble3 v = {x,y,z};
    return v;
  }

  FUNC_DECL ldouble4 make_ldouble4(long double x,long double y,long double z,long double w)
  {
    ldouble4 v = {x,y,z,w};
    return v;
  }

  FUNC_DECL  ldouble3 operator *(ldouble3 a, ldouble3 b)
  {
    return make_ldouble3(a.x*b.x, a.y*b.y, a.z*b.z);
  }

  FUNC_DECL ldouble2 operator * (long double f, ldouble2 v)
  {
    return make_ldouble2(v.x*f, v.y*f);
  }

  FUNC_DECL ldouble3 operator *(long double f, ldouble3 v)
  {
    return make_ldouble3(v.x*f, v.y*f, v.z*f);
  }

  FUNC_DECL ldouble2 operator * (ldouble2 v, long double f)
  {
    return make_ldouble2(v.x*f, v.y*f);
  }

  FUNC_DECL ldouble3 operator *(ldouble3 v, long double f)
  {
    return make_ldouble3(v.x*f, v.y*f, v.z*f);
  }

  FUNC_DECL ldouble4 operator *(ldouble4 v, long double f)
  {
    return make_ldouble4(v.x*f, v.y*f, v.z*f, v.w*f);
  }
  FUNC_DECL ldouble4 operator *(long double f, ldouble4 v)
  {
    return make_ldouble4(v.x*f, v.y*f, v.z*f, v.w*f);
  }


  FUNC_DECL ldouble2 operator +(ldouble2 a, ldouble2 b)
  {
    return make_ldouble2(a.x+b.x, a.y+b.y);
  }

  FUNC_DECL ldouble3 operator +(ldouble3 a, ldouble3 b)
  {
    return make_ldouble3(a.x+b.x, a.y+b.y, a.z+b.z);
  }

  FUNC_DECL void operator += (ldouble3 & b, ldouble3 a)
  {
    b.x += a.x;
    b.y += a.y;
    b.z += a.z;
  }

  FUNC_DECL void operator += (ldouble2 & b, ldouble2 a)
  {
    b.x += a.x;
    b.y += a.y;
  }


  FUNC_DECL void operator += (ldouble4 & b, ldouble4 a)
  {
    b.x += a.x;
    b.y += a.y;
    b.z += a.z;
    b.w += a.w;
  }

  FUNC_DECL ldouble2 operator - (ldouble2 a, ldouble2 b)
  {
    return make_ldouble2(a.x-b.x, a.y-b.y);
  }

  FUNC_DECL ldouble3 operator - (ldouble3 a, ldouble3 b)
  {
    return make_ldouble3(a.x-b.x, a.y-b.y, a.z-b.z);
  }

  FUNC_DECL void operator -= (ldouble3 & b, ldouble3 a)
  {
    b.x -= a.x;
    b.y -= a.y;
    b.z -= a.z;
  }

  FUNC_DECL ldouble3 operator / (ldouble3 v, long double f)
  {
    return make_ldouble3( v.x/f, v.y/f, v.z/f );
  }

  FUNC_DECL void operator /= (ldouble3 & b, long double f)
  {
    b.x /= f;
    b.y /= f;
    b.z /= f;
  }

  FUNC_DECL long double dot(ldouble2 a, ldouble2 b)
  {
    return a.x * b.x + a.y * b.y ;
  }

  FUNC_DECL long double dot(ldouble3 a, ldouble3 b)
  {
    return a.x * b.x + a.y * b.y + a.z * b.z;
  }

  FUNC_DECL long double dot(ldouble4 a, ldouble4 b)
  {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
  }

  FUNC_DECL long double clamp(long double f, long double a, long double b)
  {
    return max(a, min(f, b));
  }

  FUNC_DECL ldouble3 clamp(ldouble3 v, long double a, long double b)
  {
    return make_ldouble3(clamp(v.x, a, b), clamp(v.y, a, b), clamp(v.z, a, b));
  }

  FUNC_DECL ldouble3 clamp(ldouble3 v, ldouble3 a, ldouble3 b)
  {
    return make_ldouble3(clamp(v.x, a.x, b.x), clamp(v.y, a.y, b.y), clamp(v.z, a.z, b.z));
  }

  FUNC_DECL ldouble2 normalize(ldouble2 v)
  {
    long double len = sqrtl( dot(v,v) );
    return make_ldouble2(v.x / len, v.y / len);
  }

  FUNC_DECL ldouble3 normalize(ldouble3 v)
  {
    long double len = sqrtl( dot(v,v) );
    return make_ldouble3(v.x / len, v.y / len, v.z / len);
  }

  FUNC_DECL ldouble3 cross( ldouble3 A, ldouble3 B)
  {
    return make_ldouble3( A.y * B.z - A.z * B.y ,
                          A.z * B.x - A.x * B.z ,
                          A.x * B.y - A.y * B.x );
  }
#endif /* REAL_PRECISION > 64 */

#endif /* __CUDACC__ */


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/**************************************
 *** Precision dependant constants   ***
 ***************************************/

// float
#if ( REAL_PRECISION <= 32 )
#define EPSILON 1e-7
#define NEWTON_NITER 16

// long double
#elif ( REAL_PRECISION > 64 )
#define EPSILON 1e-31
#define NEWTON_NITER 64

// double ( default )
#else
#define EPSILON 1e-15 
#define NEWTON_NITER 32

#endif


/**************************************
 ***       Debugging                 ***
 ***************************************/
#define DBG_MESG(m) (void)0


/**************************************
 ***          Macros                 ***
 ***************************************/

// assure un alignement maximum des tableaux
#define ROUND_SIZE(n) (n)
//( (n+sizeof(REAL)-1) & ~(sizeof(REAL)-1) )

// local arrays allocation
#ifdef __CUDACC__

#define ALLOC_LOCAL_ARRAY(name,type,n)          \
  type * name = (type*)sdata;                   \
  sdata += ROUND_SIZE( sizeof(type)*(n) )
#define FREE_LOCAL_ARRAY(name,type,n) sdata -= ROUND_SIZE( sizeof(type)*(n) )

#elif defined(__GNUC__) // Warning, this is a gcc extension, not all compiler accept it
#define ALLOC_LOCAL_ARRAY(name,type,n) type name[(n)]
#define FREE_LOCAL_ARRAY(name,type,n) 
#else
#include <malloc.h>
#define ALLOC_LOCAL_ARRAY(name,type,n) type* name = (type*) malloc( sizeof(type) * (n) )
#define FREE_LOCAL_ARRAY(name,type,n) free(name)
#endif

#ifdef __GNUC__
#define LOCAL_ARRAY_SIZE(n) n
#else
#define LOCAL_ARRAY_SIZE(n) 128
#endif


/*********************
 *** Triangle area ***
 *********************/
/*
  Formula from VTK in vtkTriangle.cxx, method TriangleArea
*/
  FUNC_DECL
  REAL triangleSurf( REAL3 p1, REAL3 p2, REAL3 p3 )
  {
    const REAL3 e1 = p2-p1;
    const REAL3 e2 = p3-p2;
    const REAL3 e3 = p1-p3;

    const REAL a = dot(e1,e1);
    const REAL b = dot(e2,e2);
    const REAL c = dot(e3,e3);

    return
      REAL_CONST(0.25) *
      SQRT( FABS( 4*a*c - (a-b+c)*(a-b+c) ) )
      ;
  }
  FUNC_DECL
  REAL triangleSurf( REAL2 p1, REAL2 p2, REAL2 p3 )
  {
    const REAL2 e1 = p2-p1;
    const REAL2 e2 = p3-p2;
    const REAL2 e3 = p1-p3;

    const REAL a = dot(e1,e1);
    const REAL b = dot(e2,e2);
    const REAL c = dot(e3,e3);

    return
      REAL_CONST(0.25) *
      SQRT( FABS( 4*a*c - (a-b+c)*(a-b+c) ) )
      ;
  }


/*************************
 *** Tetrahedra volume ***
 *************************/

  FUNC_DECL
  REAL tetraVolume( REAL3 p0, REAL3 p1, REAL3 p2, REAL3 p3 )
  {
    REAL3 A = p1 - p0;
    REAL3 B = p2 - p0;
    REAL3 C = p3 - p0;
    REAL3 BC = cross(B,C);
    return FABS( dot(A,BC) / REAL_CONST(6.0) );
  }

  FUNC_DECL
  REAL tetraVolume( const uchar4 tetra, const REAL3* vertices )
  {
    return tetraVolume( vertices[tetra.x], vertices[tetra.y], vertices[tetra.z], vertices[tetra.w] );
  }


/*******************************************
 *** Evaluation of a polynomial function ***
 *******************************************/
  FUNC_DECL
  REAL evalPolynomialFunc(const REAL2 F, const REAL x)
  {
    return F.x * x + F.y ;
  }

  FUNC_DECL
  REAL evalPolynomialFunc(const REAL3 F, const REAL x)
  {
    REAL y = ( F.x * x + F.y ) * x ;
    return y + F.z;
  }

  FUNC_DECL
  REAL evalPolynomialFunc(const REAL4 F, const REAL x)
  {
    REAL y = ( ( F.x * x + F.y ) * x + F.z ) * x;
    return y + F.w; // this increases numerical stability when compiled with -ffloat-store
  }


/*****************************************
 *** Intergal of a polynomial function ***
 *****************************************/
  FUNC_DECL
  REAL3 integratePolynomialFunc( REAL2 linearFunc )
  {
    return make_REAL3( linearFunc.x/2 , linearFunc.y, 0 );
  }

  FUNC_DECL
  REAL4 integratePolynomialFunc( REAL3 quadFunc )
  {
    return make_REAL4( quadFunc.x/3, quadFunc.y/2, quadFunc.z, 0 );
  }

/*******************************************
 *** Derivative of a polynomial function ***
 *******************************************/
  FUNC_DECL
  REAL2 derivatePolynomialFunc( REAL3 F )
  {
    REAL2 dF = make_REAL2( 2*F.x, F.y );
    return dF;
  }

  FUNC_DECL
  REAL3 derivatePolynomialFunc( REAL4 F )
  {
    REAL3 dF = make_REAL3( 3*F.x, 2*F.y, F.z );
    return dF;
  }

/****************************
 *** Linear interpolation ***
 ****************************/
  FUNC_DECL
  REAL3 linearInterp( REAL t0, REAL3 x0, REAL t1, REAL3 x1, REAL t )
  {
    REAL f = (t1!=t0) ? (t-t0)/(t1-t0) : 0 ;
    return x0 + f * (x1-x0) ;
  }

  FUNC_DECL
  REAL2 linearInterp( REAL t0, REAL2 x0, REAL t1, REAL2 x1, REAL t )
  {
    REAL f = (t1!=t0) ? (t-t0)/(t1-t0) : REAL_CONST(0.0) ;
    return x0 + f * (x1-x0) ;
  }

  FUNC_DECL
  REAL linearInterp( REAL t0, REAL x0, REAL t1, REAL x1, REAL t )
  {
    REAL f = (t1!=t0) ? (t-t0)/(t1-t0) : REAL_CONST(0.0) ;
    return x0 + f * (x1-x0) ;
  }


/****************************************
 *** Quadratic interpolation function ***
 ****************************************/
  FUNC_DECL
  REAL3 quadraticInterpFunc( REAL x0, REAL y0, REAL x1, REAL y1, REAL x2, REAL y2 )
  {
    // Formula from the book 'Maillages', page 409

    // non-degenerated case (really a quadratic function)
    if( x1>x0 && x2>x1 )
      {
      // denominators
      const REAL d0 = ( x0 - x1 ) * ( x0 - x2 );
      const REAL d1 = ( x1 - x0 ) * ( x1 - x2 );
      const REAL d2 = ( x2 - x0 ) * ( x2 - x1 );

      // coefficients for the quadratic interpolation of (x0,y0) , (x1,y1) and p2(x2,y2)
      return make_REAL3(
                        ( y0          / d0 ) + ( y1          / d1 ) + ( y2          / d2 ) ,  // x^2 term
                        ( y0*(-x1-x2) / d0 ) + ( y1*(-x0-x2) / d1 ) + ( y2*(-x0-x1) / d2 ) ,  // x term
                        ( y0*(x1*x2)  / d0 ) + ( y1*(x0*x2)  / d1 ) + ( y2*(x0*x1)  / d2 ) ); // constant term
      }

    // linear case : 2 out of the 3 points are the same
    else if( x2 > x0 )
      {
      return make_REAL3(
                        0                         ,  // x^2 term
                        ( y2 - y0 ) / ( x2 - x0 ) ,  // x term
                        y0                        ); // constant term
      }

    // degenerated case
    return make_REAL3(0,0,0);
  }


/**************************************
 *** Analytic solver for ax²+bx+c=0 ***
 **************************************/
  FUNC_DECL
  REAL quadraticFunctionSolve( REAL3 F, const REAL value, const REAL xmin, const REAL xmax )
  {
// resolution analytique de ax²+bx+c=0
// (!) numeriquement hazardeux, donc on prefere le newton qui est pourtant BEAUCOUP plus lent

    F.z -= value;

    REAL delta = ( F.y * F.y ) - (4 * F.x * F.z);
    REAL sqrt_delta = SQRT(delta);
    REAL x = ( -F.y - sqrt_delta ) / ( 2 * F.x );
    DBG_MESG("delta="<<delta<<", sqrt(delta)="<<sqrt_delta<<", x1="<<x<<", xmin="<<xmin<<", xmax="<<xmax);
    if( x < xmin || x > xmax ) // choose a solution inside the bounds [xmin;xmax]
      {
      x = ( -F.y + sqrt_delta ) / ( 2 * F.x );
      DBG_MESG("x2="<<x);
      }

    if( F.x == REAL_CONST(0.0) ) // < EPSILON ?
      {
      x = (F.y!=0) ? ( - F.z / F.y ) : xmin /* or nan or 0 ? */;
      DBG_MESG("xlin="<<x);
      }

    x = clamp( x , xmin , xmax ); // numerical safety
    DBG_MESG("clamp(x)="<<x);
    return x;
  }

/****************************
 *** Newton search method ***
 ****************************/
  FUNC_DECL
  REAL newtonSearchPolynomialFunc( REAL3 F, REAL2 dF, const REAL value, const REAL xmin, const REAL xmax )
  {
    // translation de F, car le newton cherche le zero de la derivee
    F.z -= value;

    // on demarre du x le plus proche entre xmin, xmilieu et xmax
    const REAL ymin = evalPolynomialFunc( F, xmin );
    const REAL ymax = evalPolynomialFunc( F, xmax );

    REAL x = ( xmin + xmax ) * REAL_CONST(0.5);
    REAL y = evalPolynomialFunc(F,x);

    // cherche x tel que F(x) = 0
#ifdef __CUDACC__
#pragma unroll
#endif
    for(int i=0;i<NEWTON_NITER;i++)
      {
      DBG_MESG("F("<<x<<")="<<y);
      // Xi+1 = Xi - F'(x)/F''(x)
      REAL d = evalPolynomialFunc(dF,x);
      if( d==0 ) { d=1; y=0; }
      x = x - ( y / d );
      y = evalPolynomialFunc(F,x);
      }

    // on verifie que la solution n'est pas moins bonne que si on prend une des deux bornes
    DBG_MESG("F("<<xmin<<")="<<ymin<<", "<<"F("<<x<<")="<<y<<", "<<"F("<<xmax<<")="<<ymax);
    y = FABS( y );
    if( FABS(ymin) < y ) { x = xmin; }
    if( FABS(ymax) < y ) { x = xmax; }

    DBG_MESG("F("<<x<<")="<<y);
    return x;
  }

  FUNC_DECL
  REAL newtonSearchPolynomialFunc( REAL4 F,  REAL3 dF, const REAL value, const REAL xmin, const REAL xmax )
  {
    // translation de F, car le newton cherche le zero de la derivee
    F.w -= value;

    // on demarre du x le plus proche entre xmin, xmilieu et xmax
    const REAL ymin = evalPolynomialFunc( F, xmin );
    const REAL ymax = evalPolynomialFunc( F, xmax );

    REAL x = ( xmin + xmax ) * REAL_CONST(0.5);
    REAL y = evalPolynomialFunc(F,x);

    // cherche x tel que F(x) = 0
#ifdef __CUDACC__
#pragma unroll
#endif
    for(int i=0;i<NEWTON_NITER;i++)
      {
      DBG_MESG("F("<<x<<")="<<y);
      // Xi+1 = Xi - F'(x)/F''(x)
      REAL d = evalPolynomialFunc(dF,x);
      if( d==0 ) { d=1; y=0; }
      x = x - ( y / d );
      y = evalPolynomialFunc(F,x);
      }

    // on verifie que la solution n'est pas moins bonne que si on prend une des deux bornes
    DBG_MESG("F("<<xmin<<")="<<ymin<<", "<<"F("<<x<<")="<<y<<", "<<"F("<<xmax<<")="<<ymax);
    y = FABS( y );
    if( FABS(ymin) < y ) { x = xmin; }
    if( FABS(ymax) < y ) { x = xmax; }

    DBG_MESG("F("<<x<<")="<<y);
    return x;
  }


/***********************
 *** Sorting methods ***
 ***********************/
  FUNC_DECL
  uint3 sortTriangle( uint3 t , unsigned int* i )
  {
#define SWAP(a,b) { unsigned int tmp=a; a=b; b=tmp; }
    if( i[t.y] < i[t.x] ) SWAP(t.x,t.y);
    if( i[t.z] < i[t.y] ) SWAP(t.y,t.z);
    if( i[t.y] < i[t.x] ) SWAP(t.x,t.y);
#undef SWAP
    return t;
  }

  FUNC_DECL
  uchar3 sortTriangle( uchar3 t , unsigned char* i )
  {
#define SWAP(a,b) { unsigned char tmp=a; a=b; b=tmp; }
    if( i[t.y] < i[t.x] ) SWAP(t.x,t.y);
    if( i[t.z] < i[t.y] ) SWAP(t.y,t.z);
    if( i[t.y] < i[t.x] ) SWAP(t.x,t.y);
#undef SWAP
    return t;
  }



  typedef unsigned char IntType;
/***********************
 *** Sorting methods ***
 ***********************/
  FUNC_DECL
  void sortVertices( const int n, const REAL* dist, IntType* indices )
  {
// insertion sort : slow but symetrical across all instances
#define SWAP(a,b) { IntType t = indices[a]; indices[a] = indices[b]; indices[b] = t; }
    for(int i=0;i<n;i++)
      {
      int imin = i;
      for(int j=i+1;j<n;j++)
        {
        imin = ( dist[indices[j]] < dist[indices[imin]] ) ? j : imin;
        }
      SWAP( i, imin );
      }
#undef SWAP
  }


  FUNC_DECL
  void sortVertices( const int n, const REAL3* vertices, const REAL3 normal, IntType* indices )
  {
// insertion sort : slow but symetrical across all instances
#define SWAP(a,b) { IntType t = indices[a]; indices[a] = indices[b]; indices[b] = t; }
    for(int i=0;i<n;i++)
      {
      int imin = i;
      REAL dmin = dot(vertices[indices[i]],normal);
      for(int j=i+1;j<n;j++)
        {
        REAL d = dot(vertices[indices[j]],normal);
        imin = ( d < dmin ) ? j : imin;
        dmin = min( dmin , d );
        }
      SWAP( i, imin );
      }
#undef SWAP
  }

  FUNC_DECL
  void sortVertices( const int n, const REAL2* vertices, const REAL2 normal, IntType* indices )
  {
// insertion sort : slow but symetrical across all instances
#define SWAP(a,b) { IntType t = indices[a]; indices[a] = indices[b]; indices[b] = t; }
    for(int i=0;i<n;i++)
      {
      int imin = i;
      REAL dmin = dot(vertices[indices[i]],normal);
      for(int j=i+1;j<n;j++)
        {
        REAL d = dot(vertices[indices[j]],normal);
        imin = ( d < dmin ) ? j : imin;
        dmin = min( dmin , d );
        }
      SWAP( i, imin );
      }
#undef SWAP
  }

  FUNC_DECL
  uchar4 sortTetra( uchar4 t , IntType* i )
  {
#define SWAP(a,b) { IntType tmp=a; a=b; b=tmp; }
    if( i[t.y] < i[t.x] ) SWAP(t.x,t.y);
    if( i[t.w] < i[t.z] ) SWAP(t.z,t.w);
    if( i[t.z] < i[t.y] ) SWAP(t.y,t.z);
    if( i[t.y] < i[t.x] ) SWAP(t.x,t.y);
    if( i[t.w] < i[t.z] ) SWAP(t.z,t.w);
    if( i[t.z] < i[t.y] ) SWAP(t.y,t.z);
#undef SWAP
    return t;
  }



  FUNC_DECL
  REAL makeTriangleSurfaceFunctions(
                                    const uchar3 triangle,
                                    const REAL_COORD* vertices,
                                    const REAL_COORD normal,
                                    REAL2 func[2]
                                    )
  {

    // 1. load the data
    const REAL_COORD v0 = vertices[ triangle.x ];
    const REAL_COORD v1 = vertices[ triangle.y ];
    const REAL_COORD v2 = vertices[ triangle.z ];

    const REAL d0 = dot( v0 , normal );
    const REAL d1 = dot( v1 , normal );
    const REAL d2 = dot( v2 , normal );
   

    DBG_MESG("v0 = "<<v0.x<<','<<v0.y<<" d0="<<d0);
    DBG_MESG("v1 = "<<v1.x<<','<<v1.y<<" d1="<<d1);
    DBG_MESG("v2 = "<<v2.x<<','<<v2.y<<" d2="<<d2);
   

    // 2. compute 
   
    // compute vector from point on v0-v2 that has distance d1 from Plane0
    REAL_COORD I = linearInterp( d0, v0, d2, v2 , d1 );
    DBG_MESG("I = "<<I.x<<','<<I.y);
    REAL_COORD vec = v1 - I;
    REAL length = sqrt( dot(vec,vec) );
    DBG_MESG("length = "<<length);
   
    // side length function = (x-d0) * length / (d1-d0) = (length/(d1-d0)) * x - length * d0 / (d1-d0)
    REAL2 linearFunc01 = make_REAL2( length/(d1-d0) , - length * d0 / (d1-d0) );
    // surface function = integral of distance function starting at d0
    func[0] = make_REAL2(0,0);
    if( d1 > d0 )
      {
      func[0]  = linearFunc01;
      }
   
    // side length function = (d2-x) * length / (d2-d1) = (-length/(d2-d1)) * x + d2*length / (d2-d1)
    REAL2 linearFunc12 = make_REAL2( -length/(d2-d1) , d2*length/(d2-d1) );
    // surface function = integral of distance function starting at d1
    func[1] = make_REAL2(0,0);
    if( d2 > d1 )
      {
      func[1] = linearFunc12;
      }

    return triangleSurf( v0, v1, v2 );
  }

  FUNC_DECL
  REAL findTriangleSetCuttingPlane(
                                   const REAL_COORD normal,    // IN  , normal vector
                                   const REAL fraction,   // IN  , volume fraction
                                   const int nv,          // IN  , number of vertices
                                   const int nt,          // IN  , number of triangles
                                   const uchar3* tv,       // IN  , triangles connectivity, size=nt
                                   const REAL_COORD* vertices // IN  , vertex coordinates, size=nv
#ifdef __CUDACC__
                                   ,char* sdata           // TEMP Storage
#endif
                                   )
  {
    ALLOC_LOCAL_ARRAY( derivatives, REAL2, nv-1 );
    ALLOC_LOCAL_ARRAY( index, unsigned char, nv );
    ALLOC_LOCAL_ARRAY( rindex, unsigned char, nv );

    // initialization
    for(int i=0;i<nv;i++)
      {
      index[i] = i;
      }

    for(int i=0;i<(nv-1);i++)
      {
      derivatives[i] = make_REAL2(0,0);
      }

    // sort vertices in the normal vector direction
    sortVertices( nv, vertices, normal, index );

    // reverse indirection table
    for(int i=0;i<nv;i++)
      {
      rindex[ index[i] ] = i;
      }

    // total area
    REAL surface = 0;

    // construction of the truncated volume piecewise cubic function
    for(int i=0;i<nt;i++)
      {
      // area of the interface-tetra intersection at points P1 and P2
      uchar3 triangle = sortTriangle( tv[i] , rindex );
      DBG_MESG( "\ntriangle "<<i<<" : "<<tv[i].x<<','<<tv[i].y<<','<<tv[i].z<<" -> "<<triangle.x<<','<<triangle.y<<','<<triangle.z );

      // compute the volume function derivative pieces 
      REAL2 triangleSurfFunc[2];
      surface += makeTriangleSurfaceFunctions( triangle, vertices, normal, triangleSurfFunc );      

#ifdef DEBUG
      for(int k=0;k<2;k++)
        {
        DBG_MESG( "surf'["<<k<<"] = "<<triangleSurfFunc[k].x<<','<<triangleSurfFunc[k].y );
        }
#endif

      // surface function bounds
      unsigned int i0 = rindex[ triangle.x ];
      unsigned int i1 = rindex[ triangle.y ];
      unsigned int i2 = rindex[ triangle.z ];

      DBG_MESG( "surf(x) steps = "<<i0<<','<<i1<<','<<i2 );

      DBG_MESG( "ajout surfFunc sur ["<<i0<<';'<<i1<<"]" );
      for(unsigned int j=i0;j<i1;j++)
        {
        derivatives[j] += triangleSurfFunc[0];
        }

      DBG_MESG( "ajout surfFunc sur ["<<i1<<';'<<i2<<"]" );
      for(unsigned int j=i1;j<i2;j++) 
        {
        derivatives[j] += triangleSurfFunc[1];
        }
      }

    // target volume fraction we're looking for
    REAL y = surface*fraction;
    DBG_MESG( "surface = "<<surface<<", surface*fraction = "<<y );

    // integrate area function pieces to obtain volume function pieces
    REAL sum = 0;
    REAL3 surfaceFunction = make_REAL3(0,0,0);
    REAL xmin = 0;
    REAL xmax = dot( vertices[index[0]], normal ) ;
    int s = -1;
    while( sum<y && s<(nv-2) )
      {
      xmin = xmax;
      y -= sum;
      ++ s;
      REAL3 F = integratePolynomialFunc( derivatives[s] );
      F.z = - evalPolynomialFunc( F , xmin );
      surfaceFunction = F;
      xmax = dot( vertices[index[s+1]] , normal );
      sum = evalPolynomialFunc( F, xmax );
      }
    if( s<0) s=0;

    DBG_MESG( "step="<<s<<", x in ["<<xmin<<';'<<xmax<<']' );
    DBG_MESG( "surface reminder = "<< y );

    //REAL x = quadraticFunctionSolve( funcs[s], surface, xmin, xmax ); // analytic solution is highly unsteady
    // newton search
    REAL x = newtonSearchPolynomialFunc( surfaceFunction, derivatives[s], y, xmin, xmax );

    DBG_MESG( "final x = "<< x );
    return x ;
  }


/*
  compute the derivatives of the piecewise cubic function of the volume behind the cutting cone ( axis symetric 2D plane)
*/
  FUNC_DECL
  void makeConeVolumeDerivatives(
                                 const uchar3 triangle,
                                 const REAL2* vertices,
                                 const REAL2 normal,
                                 REAL3 deriv[2]
                                 )
  {

    // 1. load the data
    const REAL2 v0 = vertices[ triangle.x ];
    const REAL2 v1 = vertices[ triangle.y ];
    const REAL2 v2 = vertices[ triangle.z ];

    // 2. compute 
    const REAL d0 = dot( v0 , normal );
    const REAL d1 = dot( v1 , normal );
    const REAL d2 = dot( v2 , normal );

    DBG_MESG("v0 = "<<v0.x<<','<<v0.y<<" d0="<<d0);
    DBG_MESG("v1 = "<<v1.x<<','<<v1.y<<" d1="<<d1);
    DBG_MESG("v2 = "<<v2.x<<','<<v2.y<<" d2="<<d2);
      
    // compute vector from point on v0-v2 that has distance d1 from Plane0
    REAL2 I = linearInterp( d0, v0, d2, v2 , d1 );
    DBG_MESG("I = "<<I.x<<','<<I.y);
    REAL2 vec = v1 - I;
    REAL length = sqrt( dot(vec,vec) );
    DBG_MESG("length = "<<length);

    // compute truncated cone surface at d1
    REAL Isurf = REAL_CONST(M_PI) * FABS(I.y+v1.y) * length; // 2 * REAL_CONST(M_PI) * ( (I.y+v1.y) * 0.5 ) * length ;
    REAL coef;

    // build cubic volume functions derivatives
    coef = ( d1 > d0 )  ?  ( Isurf / ((d1-d0)*(d1-d0)) ) : REAL_CONST(0.0) ;
    deriv[0] = coef * make_REAL3( 1 , -2*d0 , d0*d0 ) ;

    coef = ( d2 > d1 )  ?  ( Isurf / ((d2-d1)*(d2-d1)) ) : REAL_CONST(0.0) ;
    deriv[1] = coef * make_REAL3( 1 , -2*d2 , d2*d2 ) ;
  }


  FUNC_DECL
  REAL findTriangleSetCuttingCone(
                                  const REAL2 normal,    // IN  , normal vector
                                  const REAL fraction,   // IN  , volume fraction
                                  const int nv,          // IN  , number of vertices
                                  const int nt,          // IN  , number of triangles
                                  const uchar3* tv,       // IN  , triangles connectivity, size=nt
                                  const REAL2* vertices // IN  , vertex coordinates, size=nv
#ifdef __CUDACC__
                                  ,char* sdata           // TEMP Storage
#endif
                                  )
  {
    ALLOC_LOCAL_ARRAY( derivatives, REAL3, nv-1 );
    ALLOC_LOCAL_ARRAY( index, unsigned char, nv );
    ALLOC_LOCAL_ARRAY( rindex, unsigned char, nv );

    // initialization
    for(int i=0;i<nv;i++)
      {
      index[i] = i;
      }

    for(int i=0;i<(nv-1);i++)
      {
      derivatives[i] = make_REAL3(0,0,0);
      }

    // sort vertices along normal vector 
    sortVertices( nv, vertices, normal, index );

    // reverse indirection table
    for(int i=0;i<nv;i++)
      {
      rindex[ index[i] ] = i;
      }

    // construction of the truncated volume piecewise cubic function
    for(int i=0;i<nt;i++)
      {
      // area of the interface-tetra intersection at points P1 and P2
      uchar3 triangle = sortTriangle( tv[i] , rindex );
      DBG_MESG( "\ntriangle "<<i<<" : "<<tv[i].x<<','<<tv[i].y<<','<<tv[i].z<<" -> "<<triangle.x<<','<<triangle.y<<','<<triangle.z );

      // compute the volume function derivatives pieces 
      REAL3 coneVolDeriv[2];
      makeConeVolumeDerivatives( triangle, vertices, normal, coneVolDeriv );      

      // area function bounds
      unsigned int i0 = rindex[ triangle.x ];
      unsigned int i1 = rindex[ triangle.y ];
      unsigned int i2 = rindex[ triangle.z ];

      DBG_MESG( "surf(x) steps = "<<i0<<','<<i1<<','<<i2 );

      DBG_MESG( "ajout surfFunc sur ["<<i0<<';'<<i1<<"]" );
      for(unsigned int j=i0;j<i1;j++)
        {
        derivatives[j] += coneVolDeriv[0];
        }

      DBG_MESG( "ajout surfFunc sur ["<<i1<<';'<<i2<<"]" );
      for(unsigned int j=i1;j<i2;j++) 
        {
        derivatives[j] += coneVolDeriv[1];
        }
      }

    REAL surface = 0;
    REAL xmin = 0;
    REAL xmax = dot( vertices[index[0]], normal ) ;
    for(int i=0;i<(nv-1);i++)
      {
      xmin = xmax;
      REAL4 F = integratePolynomialFunc( derivatives[i] );
      F.w = - evalPolynomialFunc( F , xmin );
      xmax = dot( vertices[index[i+1]] , normal );
      surface += evalPolynomialFunc( F, xmax );
      }

    REAL y = surface*fraction;
    DBG_MESG( "surface = "<<surface<<", surface*fraction = "<<y );

    // integrate area function pieces to obtain volume function pieces
    REAL sum = 0;
    REAL4 volumeFunction = make_REAL4(0,0,0,0);
    xmax = dot( vertices[index[0]], normal ) ;
    int s = -1;
    while( sum<y && s<(nv-2) )
      {
      xmin = xmax;
      y -= sum;
      ++ s;
      REAL4 F = integratePolynomialFunc( derivatives[s] );
      F.w = - evalPolynomialFunc( F , xmin );
      volumeFunction = F;
      xmax = dot( vertices[index[s+1]] , normal );
      sum = evalPolynomialFunc( F, xmax );
      }
    if( s<0) s=0;

    // look for the function piece that contain the target volume
    DBG_MESG( "step="<<s<<", x in ["<<xmin<<';'<<xmax<<']' );
    DBG_MESG( "surface reminder = "<< y );

    //REAL x = quadraticFunctionSolve( funcs[s], surface, xmin, xmax ); // analytical solution is highly unstable
    // newton search method
    REAL x = newtonSearchPolynomialFunc( volumeFunction, derivatives[s], y, xmin, xmax );

    DBG_MESG( "final x = "<< x );

    FREE_LOCAL_ARRAY( derivatives, REAL3        , nv-1 );
    FREE_LOCAL_ARRAY( index      , unsigned char, nv   );
    FREE_LOCAL_ARRAY( rindex     , unsigned char, nv   );

    return x ;
  }


/*
  Computes the area of the intersection between the plane, orthognal to the 'normal' vector,
  that passes through P1 (resp. P2), and the given tetrahedron.
  the resulting area function, is a function of the intersection area given the distance of the cutting plane to the origin.
*/
  FUNC_DECL
  REAL tetraPlaneSurfFunc(
                          const uchar4 tetra,
                          const REAL3* vertices,
                          const REAL3 normal,
                          REAL3 func[3]
                          )
  {
    // 1. load the data

    const REAL3 v0 = vertices[ tetra.x ];
    const REAL3 v1 = vertices[ tetra.y ];
    const REAL3 v2 = vertices[ tetra.z ];
    const REAL3 v3 = vertices[ tetra.w ];

    const REAL d0 = dot( v0 , normal );
    const REAL d1 = dot( v1 , normal );
    const REAL d2 = dot( v2 , normal );
    const REAL d3 = dot( v3 , normal );

#ifdef DEBUG
    bool ok = (d0<=d1 && d1<=d2 && d2<=d3);
    if( !ok )
      {
      DBG_MESG( "d0="<<d0<<", d1="<<d1<<", d2="<<d2<<", d3="<<d3 );
      }
    assert( d0<=d1 && d1<=d2 && d2<=d3 );
#endif

    // 2. compute

    // surface de l'intersection en p1
    const REAL surf1 = triangleSurf(
                                    v1,
                                    linearInterp( d0, v0, d2, v2, d1 ),
                                    linearInterp( d0, v0, d3, v3, d1 )
                                    );

    // calcul de la surface d'intersection au milieu de p1 et p2
    // l'intersection est un quadrangle de sommets a,b,c,d
    const REAL d12 = (d1+d2) * REAL_CONST(0.5) ;
    const REAL3 a = linearInterp( d0, v0, d2, v2, d12);
    const REAL3 b = linearInterp( d0, v0, d3, v3, d12);
    const REAL3 c = linearInterp( d1, v1, d3, v3, d12);
    const REAL3 d = linearInterp( d1, v1, d2, v2, d12);

    const REAL surf12 = triangleSurf( a,b,d ) + triangleSurf( b,c,d );

    // surface de l'intersection en p2
    const REAL surf2 = triangleSurf(
                                    v2,
                                    linearInterp( d0, v0, d3, v3, d2 ) ,
                                    linearInterp( d1, v1, d3, v3, d2 ) );


    // construction des fonctions de surface
    REAL coef;

    // recherche S0(x) = coef * (x-d0)²
    coef = ( d1 > d0 )  ?  ( surf1 / ((d1-d0)*(d1-d0)) ) : REAL_CONST(0.0) ;
    func[0] = coef * make_REAL3( 1 , -2*d0 , d0*d0 ) ;

    // recherche S1(x) = interp quadric de surf1, surf12, surf2 aux points d1, d12, d2
    func[1] = quadraticInterpFunc( d1, surf1, d12, surf12, d2, surf2 );

    // de la forme S(x) = coef * (d3-x)²
    coef = ( d3 > d2 )  ?  ( surf2 / ((d3-d2)*(d3-d2)) ) : REAL_CONST(0.0) ;
    func[2] = coef * make_REAL3( 1 , -2*d3 , d3*d3 ) ;

    return tetraVolume( v0, v1, v2, v3 );
  }


  FUNC_DECL
  REAL findTetraSetCuttingPlane(
                                const REAL3 normal,    // IN  , normal vector
                                const REAL fraction,   // IN  , volume fraction
                                const int nv,          // IN  , number of vertices
                                const int nt,          // IN  , number of tetras
                                const uchar4* tv,       // IN  , tetras connectivity, size=nt
                                const REAL3* vertices // IN  , vertex coordinates, size=nv
#ifdef __CUDACC__
                                ,char* sdata           // TEMP Storage
#endif
                                )
  {
    ALLOC_LOCAL_ARRAY( rindex, unsigned char, nv );
    ALLOC_LOCAL_ARRAY( index, unsigned char, nv );
    ALLOC_LOCAL_ARRAY( derivatives, REAL3, nv-1 );

    // initialisation
    for(int i=0;i<nv;i++)
      {
      index[i] = i;
      }

    // tri des sommets dans le sens de la normale
    sortVertices( nv,  vertices, normal, index );

    // table d'indirection inverse
    for(int i=0;i<nv;i++)
      {
      rindex[ index[i] ] = i;
      }

#ifdef DEBUG
    for(int i=0;i<nv;i++)
      {
      DBG_MESG("index["<<i<<"]="<<index[i]<<", rindex["<<i<<"]="<<rindex[i]);
      }
#endif

    for(int i=0;i<(nv-1);i++)
      {
      derivatives[i] = make_REAL3(0,0,0);
      }

    REAL volume = 0;

    // construction de la fonction cubique par morceau du volume tronqué
    for(int i=0;i<nt;i++)
      {
      // calcul de la surface de l'intersection plan/tetra aux point P1 et P2
      uchar4 tetra = sortTetra( tv[i] , rindex );
      DBG_MESG( "\ntetra "<<i<<" : "<<tv[i].x<<','<<tv[i].y<<','<<tv[i].z<<','<<tv[i].w<<" -> "<<tetra.x<<','<<tetra.y<<','<<tetra.z<<','<<tetra.w );

      // calcul des sous fonctions cubiques du volume derriere le plan en fonction de la distance
      REAL3 tetraSurfFunc[3];
      volume += tetraPlaneSurfFunc( tetra, vertices, normal, tetraSurfFunc );      

#ifdef DEBUG
      for(int k=0;k<3;k++)
        {
        DBG_MESG( "surf["<<k<<"] = "<<tetraSurfFunc[k].x<<','<<tetraSurfFunc[k].y<<','<<tetraSurfFunc[k].z );
        }
#endif

      // surface function bounds
      unsigned int i0 = rindex[ tetra.x ];
      unsigned int i1 = rindex[ tetra.y ];
      unsigned int i2 = rindex[ tetra.z ];
      unsigned int i3 = rindex[ tetra.w ];

      DBG_MESG( "surf(x) steps = "<<i0<<','<<i1<<','<<i2<<','<<i3 );

      DBG_MESG( "ajout surfFunc sur ["<<i0<<';'<<i1<<"]" );
      for(unsigned int j=i0;j<i1;j++) derivatives[j] += tetraSurfFunc[0] ;

      DBG_MESG( "ajout surfFunc sur ["<<i1<<';'<<i2<<"]" );
      for(unsigned int j=i1;j<i2;j++) derivatives[j] += tetraSurfFunc[1] ;

      DBG_MESG( "ajout surfFunc sur ["<<i2<<';'<<i3<<"]" );
      for(unsigned int j=i2;j<i3;j++) derivatives[j] += tetraSurfFunc[2] ;
      }
 
    // calcul du volume recherche
    REAL y = volume*fraction;
    DBG_MESG( "volume = "<<volume<<", volume*fraction = "<<y );

    // integration des fonctions de surface en fonctions de volume
    REAL sum = 0;
    REAL4 volumeFunction = make_REAL4(0,0,0,0);
    REAL xmin = 0;
    REAL xmax = dot( vertices[index[0]], normal ) ;
    int s = -1;
    while( sum<y && s<(nv-2) )
      {
      xmin = xmax;
      y -= sum;
      ++ s;
      REAL4 F = integratePolynomialFunc( derivatives[s] );
      F.w = - evalPolynomialFunc( F , xmin );
      volumeFunction = F;
      xmax = dot( vertices[index[s+1]] , normal );
      sum = evalPolynomialFunc( F, xmax );
      }
    if( s<0) s=0;
    // F, F' : free derivatives

    // recherche de la portion de fonction qui contient la valeur
    DBG_MESG( "step="<<s<<", x in ["<<xmin<<';'<<xmax<<']' );

    /* chaque portion de fonction redemarre de 0,
       on calcul donc le volume recherché dans cette portion de fonction
    */
    //y -= sum;
    DBG_MESG( "volume reminder = "<< y );

    // recherche par newton
    REAL x = newtonSearchPolynomialFunc( volumeFunction, derivatives[s], y, xmin, xmax );
   
    DBG_MESG( "final x = "<< x );
    return x ;
  }





  typedef REAL Real;
  typedef REAL2 Real2;
  typedef REAL3 Real3;
  typedef REAL4 Real4;

#undef REAL_PRECISION 
#undef REAL_COORD

  struct VertexInfo
  {
    double coord[3];
    double weight;
    int eid[2];
  };

  struct CWVertex
  {
    double angle;
    double coord[3];
    double weight;
    int eid[2];
    inline bool operator < (const CWVertex& v) const { return angle < v.angle; }
  };

} /* namespace vtkYoungsMaterialInterfaceCellCutInternals */


// useful to avoid numerical errors
#define Clamp(x,min,max) if(x<min) x=min; else if(x>max) x=max

// ------------------------------------
//         ####     ####
//             #    #   #
//          ###     #   #
//             #    #   #
//         ####     ####
// ------------------------------------
void vtkYoungsMaterialInterfaceCellCut::cellInterface3D( 
                                                        int ncoords,
                                                        double coords[][3],
                                                        int nedge,
                                                        int cellEdges[][2],
                                                        int ntetra,
                                                        int tetraPointIds[][4],
                                                        double fraction, double normal[3] , 
                                                        bool useFractionAsDistance,
                                                        int & np, int eids[], double weights[] ,
                                                        int & nInside, int inPoints[],
                                                        int & nOutside, int outPoints[] )
{
  // normalisation du vecteur normal si la norme >0
  double nlen2 = normal[0]*normal[0] + normal[1]*normal[1] + normal[2]*normal[2];
  if( nlen2 > 0 )
    {
    double nlen = sqrt(nlen2);
    normal[0] /= nlen;
    normal[1] /= nlen;
    normal[2] /= nlen;
    }
  else
    {
    normal[0] = 1;
    normal[1] = 0;
    normal[2] = 0;
    }

  double dmin, dmax;
  dmin = dmax = coords[0][0]*normal[0] + coords[0][1]*normal[1] + coords[0][2]*normal[2];
  for(int i=0;i<ncoords;i++)
    {
    double d = coords[i][0]*normal[0] + coords[i][1]*normal[1] + coords[i][2]*normal[2];
    if( d<dmin ) dmin=d;
    else if( d>dmax ) dmax=d;
    }

  // compute plane's offset ( D parameter in Ax+By+Cz+D=0 )
  double d = useFractionAsDistance ? fraction : findTetraSetCuttingPlane( normal, fraction, ncoords, coords, ntetra, tetraPointIds );

  // compute vertex distances to interface plane
  double dist[MAX_CELL_POINTS];
  for(int i=0;i<ncoords;i++)
    {
    dist[i] = coords[i][0]*normal[0] + coords[i][1]*normal[1] + coords[i][2]*normal[2] + d;
    }

  // get in/out points
  nInside=0;
  nOutside=0;
  for(int i=0;i<ncoords;i++)
    {
    if( dist[i] <= 0.0 ) 
      {
      inPoints[nInside++] = i;
      }
    else
      {
      outPoints[nOutside++] = i;
      }
    }   

  double center[3] = {0,0,0};
  double polygon[MAX_CELL_POINTS][3];

  // compute intersections between edges and interface plane
  np = 0;
  for(int i=0;i<nedge;i++)
    {
    int e0 = cellEdges[i][0];
    int e1 = cellEdges[i][1];
    if( dist[e0]*dist[e1] < 0 )
      {
      double edist = dist[e1] - dist[e0];
      double t;
      if(edist!=0)
        {
        t = ( 0 - dist[e0] ) / edist ;
        Clamp(t,0,1);
        }
      else
        {
        t = 0;
        }
   
      for(int c=0;c<3;c++)
        {
        polygon[np][c] = coords[e0][c] + t * ( coords[e1][c] - coords[e0][c] ) ;
        center[c] += polygon[np][c];
        }
      eids[np*2+0] = e0;
      eids[np*2+1] = e1;
      weights[np] = t;
      np++;
      }
    }

  // tri des points
  if(np>3)
    {
    // calcul du centre du polygone
    for(int comp=0;comp<3;comp++) { center[comp] /= np; }

    // calcul de la direction dominante, pour retomber sur un cas 2D
    int maxDim = 0;
    if( fabs(normal[1]) > fabs(normal[maxDim]) ) maxDim=1;
    if( fabs(normal[2]) > fabs(normal[maxDim]) ) maxDim=2;
    int xd=0, yd=1;
    switch(maxDim)
      {
      case 0: xd=1; yd=2; break;
      case 1: xd=0; yd=2; break;
      case 2: xd=0; yd=1; break;
      }

    // calcul des angles des points du polygone
    vtkYoungsMaterialInterfaceCellCutInternals::CWVertex pts[MAX_CELL_POINTS];
    for(int i=0;i<np;i++)
      {
      double vec[3];
      for(int comp=0;comp<3;comp++)
        {
        pts[i].coord[comp] = polygon[i][comp];
        vec[comp] = polygon[i][comp]-center[comp];
        }
   
      pts[i].weight = weights[i];
      pts[i].eid[0] = eids[i*2+0];
      pts[i].eid[1] = eids[i*2+1];
      pts[i].angle = atan2( vec[yd], vec[xd] );
      }
    vtkstd::sort( pts , pts+np );
    for(int i=0;i<np;i++)
      {
      weights[i] = pts[i].weight;
      eids[i*2+0] = pts[i].eid[0];
      eids[i*2+1] = pts[i].eid[1];
      }
    }
}

double vtkYoungsMaterialInterfaceCellCut::findTetraSetCuttingPlane(
                                                                   const double normal[3],
                                                                   const double fraction,
                                                                   const int vertexCount,
                                                                   const double vertices[][3],
                                                                   const int tetraCount,
                                                                   const int tetras[][4]
                                                                   )
{
  vtkYoungsMaterialInterfaceCellCutInternals::Real3 N = { normal[0], normal[1], normal[2] };
  vtkYoungsMaterialInterfaceCellCutInternals::Real3 V[LOCAL_ARRAY_SIZE(vertexCount)];
  vtkYoungsMaterialInterfaceCellCutInternals::uchar4 tet[LOCAL_ARRAY_SIZE(tetraCount)];

  for(int i=0;i<vertexCount;i++)
    {
    V[i].x = vertices[i][0] - vertices[0][0] ;
    V[i].y = vertices[i][1] - vertices[0][1] ;
    V[i].z = vertices[i][2] - vertices[0][2] ;
    }

  vtkYoungsMaterialInterfaceCellCutInternals::Real3 vmin,vmax;
  vtkYoungsMaterialInterfaceCellCutInternals::Real scale;
  vmin = vmax = V[0];
  for(int i=1;i<vertexCount;i++)
    {
    if( V[i].x < vmin.x ) vmin.x = V[i].x;
    if( V[i].x > vmax.x ) vmax.x = V[i].x;
    if( V[i].y < vmin.y ) vmin.y = V[i].y;
    if( V[i].y > vmax.y ) vmax.y = V[i].y;
    if( V[i].z < vmin.z ) vmin.z = V[i].z;
    if( V[i].z > vmax.z ) vmax.z = V[i].z;
    }
  scale = vmax.x - vmin.x;
  if( (vmax.y-vmin.y) > scale ) scale = vmax.y-vmin.y;
  if( (vmax.z-vmin.z) > scale ) scale = vmax.z-vmin.z;
  for(int i=0;i<vertexCount;i++) V[i] /= scale;

  for(int i=0;i<tetraCount;i++)
    {
    tet[i].x = tetras[i][0];
    tet[i].y = tetras[i][1];
    tet[i].z = tetras[i][2];
    tet[i].w = tetras[i][3];
    }
   
  double dist0 = vertices[0][0]*normal[0] + vertices[0][1]*normal[1] + vertices[0][2]*normal[2];
  double d = dist0 + vtkYoungsMaterialInterfaceCellCutInternals::findTetraSetCuttingPlane(N, fraction, vertexCount, tetraCount, tet, V ) * scale;

  return - d;
}


// ------------------------------------
//         ####     ####
//             #    #   #
//          ###     #   #
//         #        #   #
//        #####     ####
// ------------------------------------

bool vtkYoungsMaterialInterfaceCellCut::cellInterface2D( 
                                                        double points[][3],
                                                        int nPoints,
                                                        int triangles[][3], // TODO: int [] pour plus d'integration au niveau du dessus
                                                        int nTriangles,
                                                        double fraction, double normal[3] ,
                                                        bool axisSymetric,
                                                        bool useFractionAsDistance,
                                                        int eids[4], double weights[2] ,
                                                        int &polygonPoints, int polygonIds[],
                                                        int &nRemPoints, int remPoints[]
                                                         )
{
  double d = useFractionAsDistance ? fraction : findTriangleSetCuttingPlane( normal, fraction, nPoints, points, nTriangles, triangles , axisSymetric );

  // compute vertex distances to interface plane
  double dist[LOCAL_ARRAY_SIZE(nPoints)];
  for(int i=0;i<nPoints;i++)
    {
    dist[i] = points[i][0]*normal[0] + points[i][1]*normal[1] + points[i][2]*normal[2] + d;
    }

  // compute intersections between edges and interface line
  int np = 0;
  nRemPoints = 0;
  polygonPoints = 0;
  for(int i=0;i<nPoints;i++)
    {
    int edge[2];
    edge[0] = i;
    edge[1] = (i+1)%nPoints; 
    if( dist[i] <= 0.0 ) 
      {
      polygonIds[polygonPoints++] = i;
      }
    else
      {
      remPoints[nRemPoints++] = i;
      }
    if( np < 2 )
      {
      if( dist[edge[0]]*dist[edge[1]] < 0.0 )
        {
        double t = ( 0 - dist[edge[0]] ) / ( dist[edge[1]] - dist[edge[0]] );
        Clamp(t,0,1);
        eids[np*2+0] = edge[0];
        eids[np*2+1] = edge[1];
        weights[np] = t;
        np++;
        polygonIds[polygonPoints++] = -np;
        remPoints[nRemPoints++] = -np;
        }
      }
    }

  return (np==2);
}


double vtkYoungsMaterialInterfaceCellCut::findTriangleSetCuttingPlane(
                                                                      const double normal[3],
                                                                      const double fraction,
                                                                      const int vertexCount,
                                                                      const double vertices[][3],
                                                                      const int triangleCount,
                                                                      const int triangles[][3],
                                                                      bool axisSymetric
                                                                      )
{
  double d;

  vtkYoungsMaterialInterfaceCellCutInternals::uchar3 tri[LOCAL_ARRAY_SIZE(triangleCount)];
  for(int i=0;i<triangleCount;i++)
    {
    tri[i].x = triangles[i][0];
    tri[i].y = triangles[i][1];
    tri[i].z = triangles[i][2];
    }

  if( axisSymetric )
    {
    vtkYoungsMaterialInterfaceCellCutInternals::Real2 N = { normal[0], normal[1] };
    vtkYoungsMaterialInterfaceCellCutInternals::Real2 V[LOCAL_ARRAY_SIZE(vertexCount)];
    for(int i=0;i<vertexCount;i++)
      {
      V[i].x = vertices[i][0] - vertices[0][0] ;
      V[i].y = vertices[i][1] - vertices[0][1] ;
      }
    vtkYoungsMaterialInterfaceCellCutInternals::Real2 vmin,vmax;
    vtkYoungsMaterialInterfaceCellCutInternals::Real scale;
    vmin = vmax = V[0];
    for(int i=1;i<vertexCount;i++)
      {
      if( V[i].x < vmin.x ) vmin.x = V[i].x;
      if( V[i].x > vmax.x ) vmax.x = V[i].x;
      if( V[i].y < vmin.y ) vmin.y = V[i].y;
      if( V[i].y > vmax.y ) vmax.y = V[i].y;
      }
    scale = vmax.x - vmin.x;
    if( (vmax.y-vmin.y) > scale ) scale = vmax.y-vmin.y;
    for(int i=0;i<vertexCount;i++) V[i] /= scale;
    double dist0 = vertices[0][0]*normal[0] + vertices[0][1]*normal[1] ;
    d = dist0 + vtkYoungsMaterialInterfaceCellCutInternals::findTriangleSetCuttingCone(N, fraction, vertexCount, triangleCount, tri, V ) * scale;  
    }
  else
    {
    vtkYoungsMaterialInterfaceCellCutInternals::Real3 N = { normal[0], normal[1], normal[2] };
    vtkYoungsMaterialInterfaceCellCutInternals::Real3 V[LOCAL_ARRAY_SIZE(vertexCount)];
    for(int i=0;i<vertexCount;i++)
      {
      V[i].x = vertices[i][0] - vertices[0][0] ;
      V[i].y = vertices[i][1] - vertices[0][1] ;
      V[i].z = vertices[i][2] - vertices[0][2] ;
      }
    vtkYoungsMaterialInterfaceCellCutInternals::Real3 vmin,vmax;
    vtkYoungsMaterialInterfaceCellCutInternals::Real scale;
    vmin = vmax = V[0];
    for(int i=1;i<vertexCount;i++)
      {
      if( V[i].x < vmin.x ) vmin.x = V[i].x;
      if( V[i].x > vmax.x ) vmax.x = V[i].x;
      if( V[i].y < vmin.y ) vmin.y = V[i].y;
      if( V[i].y > vmax.y ) vmax.y = V[i].y;
      if( V[i].z < vmin.z ) vmin.z = V[i].z;
      if( V[i].z > vmax.z ) vmax.z = V[i].z;
      }
    scale = vmax.x - vmin.x;
    if( (vmax.y-vmin.y) > scale ) scale = vmax.y-vmin.y;
    if( (vmax.z-vmin.z) > scale ) scale = vmax.z-vmin.z;
    for(int i=0;i<vertexCount;i++) V[i] /= scale;
    double dist0 = vertices[0][0]*normal[0] + vertices[0][1]*normal[1] + vertices[0][2]*normal[2];
    d = dist0 + vtkYoungsMaterialInterfaceCellCutInternals::findTriangleSetCuttingPlane(N, fraction, vertexCount, triangleCount, tri, V ) * scale;
    }

  return - d;
}





