/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTableBasedClipDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/


/*****************************************************************************
*
* Copyright (c) 2000 - 2009, Lawrence Livermore National Security, LLC
* Produced at the Lawrence Livermore National Laboratory
* LLNL-CODE-400124
* All rights reserved.
*
* This file was adapted from the VisIt clipper (vtkVisItClipper). For  details,
* see https://visit.llnl.gov/.  The full copyright notice is contained in the
* file COPYRIGHT located at the root of the VisIt distribution or at 
* http://www.llnl.gov/visit/copyright.html.
*
*****************************************************************************/


#include "vtkTableBasedClipDataSet.h"

#include "vtkExecutive.h"
#include "vtkInformation.h"
#include "vtkSmartPointer.h"
#include "vtkObjectFactory.h"
#include "vtkCallbackCommand.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkMergePoints.h"
#include "vtkIncrementalPointLocator.h"

#include "vtkPlane.h"
#include "vtkClipDataSet.h"
#include "vtkImplicitFunction.h"

#include "vtkPolyData.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkCellArray.h"
#include "vtkImageData.h"
#include "vtkDoubleArray.h"
#include "vtkAppendFilter.h"
#include "vtkStructuredGrid.h"
#include "vtkRectilinearGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkGenericCell.h"

#include "vtkTableBasedClipCases.h"

vtkStandardNewMacro( vtkTableBasedClipDataSet );
vtkCxxSetObjectMacro( vtkTableBasedClipDataSet, ClipFunction, vtkImplicitFunction );


// ============================================================================
// ============== vtkTableBasedClipperDataSetFromVolume (begin) ===============
// ============================================================================


struct TableBasedClipperPointEntry
{
  int      ptIds[2];
  double   percent;
};


// ---- vtkTableBasedClipperPointList (bein)
class vtkTableBasedClipperPointList
{
public:
  vtkTableBasedClipperPointList();
  virtual ~vtkTableBasedClipperPointList();

  int      AddPoint( int, int, double );
  int      GetTotalNumberOfPoints() const;
  int      GetNumberOfLists() const;
  int      GetList( int, const TableBasedClipperPointEntry *& ) const;

protected:
  int      currentList;
  int      currentPoint;
  int      listSize;
  int      pointsPerList;
  TableBasedClipperPointEntry ** list;
};
// ---- vtkTableBasedClipperPointList (end)


// ---- vtkTableBasedClipperEdgeHashEntry (begin)
class vtkTableBasedClipperEdgeHashEntry
{
public:
  vtkTableBasedClipperEdgeHashEntry();
  virtual ~vtkTableBasedClipperEdgeHashEntry() { }

  int      GetPointId(void) { return ptId; };
  void     SetInfo( int, int, int );
  void     SetNext( vtkTableBasedClipperEdgeHashEntry * n ) { next = n; };
  bool     IsMatch( int i1, int i2 )
           { return ( i1 == id1 && i2 == id2 ? true : false ); };
  
  vtkTableBasedClipperEdgeHashEntry * GetNext() { return next; }

protected:
  int             id1, id2;
  int             ptId;
  vtkTableBasedClipperEdgeHashEntry * next;

};
// ---- vtkTableBasedClipperEdgeHashEntry (end)


// ---- vtkTableBasedClipperEdgeHashEntryMemoryManager (begin)
#define FREE_ENTRY_LIST_SIZE 16384
#define POOL_SIZE 256
class   vtkTableBasedClipperEdgeHashEntryMemoryManager
{
public:
  vtkTableBasedClipperEdgeHashEntryMemoryManager();
  virtual ~vtkTableBasedClipperEdgeHashEntryMemoryManager();

  inline vtkTableBasedClipperEdgeHashEntry * GetFreeEdgeHashEntry()
  {
    if ( freeEntryindex <= 0 )
      {
      AllocateEdgeHashEntryPool();
      }
    freeEntryindex --;
    return freeEntrylist[ freeEntryindex ];
  }

  inline void ReRegisterEdgeHashEntry( vtkTableBasedClipperEdgeHashEntry * q )
  {
    if ( freeEntryindex >= FREE_ENTRY_LIST_SIZE - 1 )
      {
      // We've got plenty, so ignore this one.
      return;
      }
    freeEntrylist[ freeEntryindex ] = q;
    freeEntryindex ++;
  }

protected:
  int             freeEntryindex;
  vtkTableBasedClipperEdgeHashEntry * freeEntrylist[ FREE_ENTRY_LIST_SIZE ];
  std::vector< vtkTableBasedClipperEdgeHashEntry * > edgeHashEntrypool;

  void  AllocateEdgeHashEntryPool();
};
// ---- vtkTableBasedClipperEdgeHashEntryMemoryManager (end)


// ---- vtkTableBasedClipperEdgeHashTable (begin)
class vtkTableBasedClipperEdgeHashTable
{
public:
  vtkTableBasedClipperEdgeHashTable( int, vtkTableBasedClipperPointList & );
  virtual ~vtkTableBasedClipperEdgeHashTable();

  int         AddPoint( int, int, double );
  vtkTableBasedClipperPointList & GetPointList( );

protected:
  int         nHashes;
  vtkTableBasedClipperPointList       & pointlist;
  vtkTableBasedClipperEdgeHashEntry  ** hashes;
  vtkTableBasedClipperEdgeHashEntryMemoryManager  emm;
  
  int GetKey( int, int );
  
  private:
  vtkTableBasedClipperEdgeHashTable
                  ( const vtkTableBasedClipperEdgeHashTable & ); // Not implemented.
  void operator = ( const vtkTableBasedClipperEdgeHashTable & ); // Not implemented.
};
// ---- vtkTableBasedClipperEdgeHashTable (end)
  
  
class  vtkTableBasedClipperDataSetFromVolume
{
  public:
    vtkTableBasedClipperDataSetFromVolume( int ptSizeGuess );
    vtkTableBasedClipperDataSetFromVolume( int nPts, int ptSizeGuess );
    virtual ~vtkTableBasedClipperDataSetFromVolume() { }
    
    int  AddPoint( int p1, int p2, double percent )
         { return numPrevPts + edges.AddPoint( p1, p2, percent ); }

  protected:
    int           numPrevPts;
    vtkTableBasedClipperPointList      pt_list;
    vtkTableBasedClipperEdgeHashTable  edges;
    
  private:
  vtkTableBasedClipperDataSetFromVolume
    ( const vtkTableBasedClipperDataSetFromVolume & ); // Not implemented.
  void operator = 
    ( const vtkTableBasedClipperDataSetFromVolume & ); // Not implemented.
};

vtkTableBasedClipperPointList::vtkTableBasedClipperPointList()
{
  listSize      = 4096;
  pointsPerList = 1024;
 
  list = new TableBasedClipperPointEntry * [ listSize ];
  list[0] = new TableBasedClipperPointEntry[pointsPerList];
  for ( int i = 1; i < listSize; i ++ )
    {
    list[i] = NULL;
    }
 
  currentList  = 0;
  currentPoint = 0;
}
 
vtkTableBasedClipperPointList::~vtkTableBasedClipperPointList()
{
  for ( int i = 0; i < listSize; i ++ )
    {
    if ( list[i] != NULL )
      {
      delete [] list[i];
      }
    else
      {
      break;
      }
    }
    
  delete [] list;
}

int vtkTableBasedClipperPointList::GetList
  ( int listId, const TableBasedClipperPointEntry *& outlist) const
{
  if ( listId < 0 || listId > currentList )
    {
    outlist = NULL;
    return 0;
    }
 
  outlist = list[ listId ];
  return ( listId == currentList ? currentPoint : pointsPerList );
}
 
int vtkTableBasedClipperPointList::GetNumberOfLists() const
{
  return currentList + 1;
}
 
int vtkTableBasedClipperPointList::GetTotalNumberOfPoints() const
{
  int numFullLists = currentList;   // actually currentList-1+1
  int numExtra     = currentPoint;  // again, currentPoint-1+1
 
  return numFullLists * pointsPerList + numExtra;
}

int vtkTableBasedClipperPointList::AddPoint( int pt0, int pt1, double percent )
{
  if ( currentPoint >= pointsPerList )
    {
    if (  ( currentList + 1 )  >=  listSize  )
      {
      TableBasedClipperPointEntry ** tmpList = new 
      TableBasedClipperPointEntry  * [ 2 * listSize ];
      for ( int i = 0; i < listSize; i ++ )
        {
        tmpList[i] = list[i];
        }
        
      for ( int i = listSize; i < listSize * 2; i ++ )
        {
        tmpList[i] = NULL;
        }

      listSize *= 2;
      delete [] list;
      list = tmpList;
      }
 
    currentList ++;
    list[ currentList ] = new TableBasedClipperPointEntry[ pointsPerList ];
    currentPoint = 0;
    }
 
  list[ currentList ][ currentPoint ].ptIds[0] = pt0;
  list[ currentList ][ currentPoint ].ptIds[1] = pt1;
  list[ currentList ][ currentPoint ].percent  = percent;
  currentPoint ++;
 
  return ( GetTotalNumberOfPoints() - 1 );
}

vtkTableBasedClipperEdgeHashEntry::vtkTableBasedClipperEdgeHashEntry()
{
    id1  = -1;
    id2  = -1;
    ptId = -1;
    next = NULL;
}
  
void vtkTableBasedClipperEdgeHashEntry::SetInfo( int i1, int i2, int pId )
{
    id1  = i1;
    id2  = i2;
    ptId = pId;
    next = NULL;
}

vtkTableBasedClipperEdgeHashEntryMemoryManager::
vtkTableBasedClipperEdgeHashEntryMemoryManager()
{
    freeEntryindex = 0;
}
 
vtkTableBasedClipperEdgeHashEntryMemoryManager::~
vtkTableBasedClipperEdgeHashEntryMemoryManager()
{
  int npools = static_cast<int> ( edgeHashEntrypool.size() );
  for ( int i = 0; i < npools; i ++ )
    {
    vtkTableBasedClipperEdgeHashEntry * pool = edgeHashEntrypool[i];
    delete [] pool;
    }
}
 
void vtkTableBasedClipperEdgeHashEntryMemoryManager::AllocateEdgeHashEntryPool()
{
  if ( freeEntryindex == 0 )
    {
    vtkTableBasedClipperEdgeHashEntry * newlist = new 
    vtkTableBasedClipperEdgeHashEntry[ POOL_SIZE ];
    edgeHashEntrypool.push_back( newlist );
    
    for ( int i = 0; i < POOL_SIZE; i ++ )
      {
      freeEntrylist[i] = &( newlist[i] );
      }
      
    freeEntryindex = POOL_SIZE;
    }
}

vtkTableBasedClipperEdgeHashTable::
vtkTableBasedClipperEdgeHashTable( int nh, vtkTableBasedClipperPointList & p ) 
 : pointlist( p )
{
  nHashes = nh;
  hashes  = new vtkTableBasedClipperEdgeHashEntry * [ nHashes ];
  for ( int i = 0; i < nHashes; i ++ )
    {
    hashes[i] = NULL;
    }
}
 
vtkTableBasedClipperEdgeHashTable::~vtkTableBasedClipperEdgeHashTable()
{
    delete [] hashes;
}
 
int vtkTableBasedClipperEdgeHashTable::GetKey( int p1, int p2 )
{
  int rv = (  ( p1 * 18457 + p2 * 234749 ) % nHashes  );
 
  // In case of overflows and modulo with negative numbers.
  if ( rv < 0 )
    {
    rv += nHashes;
    }
 
  return rv;
}

int vtkTableBasedClipperEdgeHashTable::AddPoint
  ( int ap1, int ap2, double apercent )
{
  int    p1, p2;
  double percent;
  
  if ( ap2 < ap1 )
    {
    p1      = ap2;
    p2      = ap1;
    percent = 1.0 - apercent;
    }
  else
    {
    p1      = ap1;
    p2      = ap2;
    percent = apercent;
    }

  int key = GetKey( p1, p2 );
 
  //
  // See if we have any matches in the current hashes.
  //
  vtkTableBasedClipperEdgeHashEntry * cur = hashes[ key ];
  while ( cur != NULL )
    {
    if (  cur->IsMatch( p1, p2 )  )
      {
      //
      // We found a match.
      //
      return cur->GetPointId();
      }
      
    cur = cur->GetNext();
    }
 
  //
  // There was no match.  We will have to add a new entry.
  //
  vtkTableBasedClipperEdgeHashEntry * new_one = emm.GetFreeEdgeHashEntry();
 
  int newPt = pointlist.AddPoint( p1, p2, percent );
  new_one->SetInfo( p1, p2, newPt );
  new_one->SetNext(  hashes[ key ]  );
  hashes[key] = new_one;
 
  return newPt;
}

vtkTableBasedClipperDataSetFromVolume::
vtkTableBasedClipperDataSetFromVolume( int ptSizeGuess )
   : numPrevPts( 0 ), pt_list(), edges( ptSizeGuess, pt_list )
{
}
      
vtkTableBasedClipperDataSetFromVolume::
vtkTableBasedClipperDataSetFromVolume( int nPts, int ptSizeGuess )
   : numPrevPts( nPts ), pt_list(), edges( ptSizeGuess, pt_list )
{
}
// ============================================================================
// ============== vtkTableBasedClipperDataSetFromVolume ( end ) ===============
// ============================================================================


// ============================================================================
// =============== vtkTableBasedClipperVolumeFromVolume (begin) ===============
// ============================================================================


class vtkTableBasedClipperShapeList
{
  public:
                   vtkTableBasedClipperShapeList( int size );
    virtual       ~vtkTableBasedClipperShapeList();
    virtual int    GetVTKType() const = 0;
    int            GetShapeSize() const { return shapeSize; }
    int            GetTotalNumberOfShapes() const;
    int            GetNumberOfLists() const;
    int            GetList(int, const int *& ) const;
  protected:
    int         ** list;
    int            currentList;
    int            currentShape;
    int            listSize;
    int            shapesPerList;
    int            shapeSize;
};


class vtkTableBasedClipperHexList : public vtkTableBasedClipperShapeList
{
  public:
                   vtkTableBasedClipperHexList();
    virtual       ~vtkTableBasedClipperHexList();
    virtual int    GetVTKType() const { return VTK_HEXAHEDRON; }
    void           AddHex( int, int, int, int, int, int, int, int, int );
};


class vtkTableBasedClipperWedgeList : public vtkTableBasedClipperShapeList
{
  public:
                   vtkTableBasedClipperWedgeList();
    virtual       ~vtkTableBasedClipperWedgeList();
    virtual int    GetVTKType() const { return VTK_WEDGE; }
    void           AddWedge( int, int, int, int, int, int, int );
};


class vtkTableBasedClipperPyramidList : public vtkTableBasedClipperShapeList
{
  public:
                   vtkTableBasedClipperPyramidList();
    virtual       ~vtkTableBasedClipperPyramidList();
    virtual int    GetVTKType() const { return VTK_PYRAMID; }
    void           AddPyramid( int, int, int, int, int, int );
};


class vtkTableBasedClipperTetList : public vtkTableBasedClipperShapeList
{
  public:
                   vtkTableBasedClipperTetList();
    virtual       ~vtkTableBasedClipperTetList();
    virtual int    GetVTKType() const { return VTK_TETRA; }
    void           AddTet( int, int, int, int, int );
};


class vtkTableBasedClipperQuadList : public vtkTableBasedClipperShapeList
{
  public:
                   vtkTableBasedClipperQuadList();
    virtual       ~vtkTableBasedClipperQuadList();
    virtual int    GetVTKType() const { return VTK_QUAD; }
    void           AddQuad( int, int, int, int, int );
};


class vtkTableBasedClipperTriList : public vtkTableBasedClipperShapeList
{
  public:
                   vtkTableBasedClipperTriList();
    virtual       ~vtkTableBasedClipperTriList();
    virtual int    GetVTKType() const { return VTK_TRIANGLE; }
    void           AddTri( int, int, int, int );
};


class vtkTableBasedClipperLineList : public vtkTableBasedClipperShapeList
{
  public:
                   vtkTableBasedClipperLineList();
    virtual       ~vtkTableBasedClipperLineList();
    virtual int    GetVTKType() const { return VTK_LINE; }
    void           AddLine( int, int, int );
};


class vtkTableBasedClipperVertexList : public vtkTableBasedClipperShapeList
{
  public:
                   vtkTableBasedClipperVertexList();
    virtual       ~vtkTableBasedClipperVertexList();
    virtual int    GetVTKType() const { return VTK_VERTEX; }
    void           AddVertex( int, int );
};


struct TableBasedClipperCentroidPointEntry
{
  int  nPts;
  int  ptIds[8];
};


class vtkTableBasedClipperCentroidPointList
{
  public:
                   vtkTableBasedClipperCentroidPointList();
    virtual       ~vtkTableBasedClipperCentroidPointList();
 
    int            AddPoint(int, int*);
 
    int            GetTotalNumberOfPoints() const;
    int            GetNumberOfLists() const;
    int            GetList( int, 
                   const TableBasedClipperCentroidPointEntry *& ) const;
 
  protected:
    TableBasedClipperCentroidPointEntry ** list;
    int            currentList;
    int            currentPoint;
    int            listSize;
    int            pointsPerList;
};


struct TableBasedClipperCommonPointsStructure
{
  bool        hasPtsList;
  double    * pts_ptr;
  int       * dims;
  double    * X;
  double    * Y;
  double    * Z;
};


class  vtkTableBasedClipperVolumeFromVolume : 
       public vtkTableBasedClipperDataSetFromVolume
{
  public:
              vtkTableBasedClipperVolumeFromVolume
              ( int nPts, int ptSizeGuess );
    virtual  ~vtkTableBasedClipperVolumeFromVolume() { }

    void      ConstructDataSet( vtkPointData *, vtkCellData *,
                                vtkUnstructuredGrid *, double * );
    void      ConstructDataSet( vtkPointData *, vtkCellData *,
                                vtkUnstructuredGrid *, int *, double *,
                                double *, double * );

    int      AddCentroidPoint( int n, int * p )
             { return -1 - centroid_list.AddPoint( n, p ); }

    void     AddHex( int z,  int v0, int v1, int v2, int v3,
                     int v4, int v5, int v6, int v7 )
             { this->hexes.AddHex( z, v0, v1, v2, v3, v4, v5, v6, v7 ); }
        
    void     AddWedge( int z,int v0,int v1,int v2,int v3,int v4,int v5 )
             { this->wedges.AddWedge( z, v0, v1, v2, v3, v4, v5 ); }
    void     AddPyramid( int z, int v0, int v1, int v2, int v3, int v4 )
             { this->pyramids.AddPyramid( z, v0, v1, v2, v3, v4 ); }
    void     AddTet( int z, int v0, int v1, int v2, int v3 )
             { this->tets.AddTet( z, v0, v1, v2, v3 ); }
    void     AddQuad( int z, int v0, int v1, int v2, int v3 )
             { this->quads.AddQuad( z, v0, v1, v2, v3 ); }
    void     AddTri( int z, int v0, int v1, int v2 )
             { this->tris.AddTri( z, v0, v1, v2 ); }
    void     AddLine( int z, int v0, int v1 )
             { this->lines.AddLine( z, v0, v1 ); }
    void     AddVertex(int z, int v0)
             { this->vertices.AddVertex( z, v0 ); }

  protected:
    vtkTableBasedClipperCentroidPointList centroid_list;
    vtkTableBasedClipperHexList     hexes;
    vtkTableBasedClipperWedgeList   wedges;
    vtkTableBasedClipperPyramidList pyramids;
    vtkTableBasedClipperTetList     tets;
    vtkTableBasedClipperQuadList    quads;
    vtkTableBasedClipperTriList     tris;
    vtkTableBasedClipperLineList    lines;
    vtkTableBasedClipperVertexList  vertices;

    vtkTableBasedClipperShapeList * shapes[8];
    const int    nshapes;

    void         ConstructDataSet
                 ( vtkPointData *, vtkCellData *, vtkUnstructuredGrid *, 
                   TableBasedClipperCommonPointsStructure & );
};


vtkTableBasedClipperVolumeFromVolume::
vtkTableBasedClipperVolumeFromVolume( int nPts, int ptSizeGuess )
    : vtkTableBasedClipperDataSetFromVolume( nPts, ptSizeGuess ), nshapes( 8 )
{
  shapes[0] = &tets;
  shapes[1] = &pyramids;
  shapes[2] = &wedges;
  shapes[3] = &hexes;
  shapes[4] = &quads;
  shapes[5] = &tris;
  shapes[6] = &lines;
  shapes[7] = &vertices;
}

vtkTableBasedClipperCentroidPointList::vtkTableBasedClipperCentroidPointList()
{
  listSize      = 4096;
  pointsPerList = 1024;
 
  list    = new TableBasedClipperCentroidPointEntry * [ listSize ];
  list[0] = new TableBasedClipperCentroidPointEntry[ pointsPerList ];
  for (int i = 1; i < listSize; i ++ )
    {
    list[i] = NULL;
    }
 
  currentList  = 0;
  currentPoint = 0;
}
 
vtkTableBasedClipperCentroidPointList::~vtkTableBasedClipperCentroidPointList()
{
  for ( int i = 0; i < listSize; i ++ )
    {
    if ( list[i] != NULL )
      {
      delete [] list[i];
      }
    else
      {
      break;
      }
    }
    
  delete [] list;
}

int  vtkTableBasedClipperCentroidPointList::GetList
   ( int listId, const TableBasedClipperCentroidPointEntry *& outlist ) const
{
  if ( listId < 0 || listId > currentList )
    {
    outlist = NULL;
    return 0;
    }
 
  outlist = list[ listId ];
  return ( listId == currentList ? currentPoint : pointsPerList );
}
 
int  vtkTableBasedClipperCentroidPointList::GetNumberOfLists() const
{
    return currentList + 1;
}
 
int  vtkTableBasedClipperCentroidPointList::GetTotalNumberOfPoints() const
{
  int numFullLists = currentList;  // actually currentList-1+1
  int numExtra     = currentPoint; // again, currentPoint-1+1
 
  return numFullLists * pointsPerList + numExtra;
}

int  vtkTableBasedClipperCentroidPointList::AddPoint( int npts, int * pts )
{
  if ( currentPoint >= pointsPerList )
    {
    if (  ( currentList + 1 )  >=  listSize  )
      {
      TableBasedClipperCentroidPointEntry ** tmpList = new 
      TableBasedClipperCentroidPointEntry * [ 2 * listSize ];
      
      for ( int i = 0; i < listSize; i ++ )
        {
        tmpList[i] = list[i];
        }
        
      for (int i = listSize; i < listSize * 2; i ++ )
        {
        tmpList[i] = NULL;
        }
        
      listSize *= 2;
      delete [] list;
      list = tmpList;
      }
 
    currentList ++;
    list[ currentList ] = new TableBasedClipperCentroidPointEntry
                              [ pointsPerList ];
    currentPoint = 0;
    }
 
  list[ currentList ][ currentPoint ].nPts = npts;
  for ( int i = 0; i < npts; i ++ )
    {
    list[ currentList ][ currentPoint ].ptIds[i] = pts[i];
    }
  currentPoint ++;
 
  return ( GetTotalNumberOfPoints() - 1 );
}

vtkTableBasedClipperShapeList::vtkTableBasedClipperShapeList( int size )
{
  shapeSize     = size;
  listSize      = 4096;
  shapesPerList = 1024;
 
  list    = new int * [ listSize ];
  list[0] = new int[  ( shapeSize + 1 ) * shapesPerList  ];
  
  for ( int i = 1; i < listSize; i ++ )
    {
    list[i] = NULL;
    }
 
  currentList  = 0;
  currentShape = 0;
}
 
vtkTableBasedClipperShapeList::~vtkTableBasedClipperShapeList()
{
  for (int i = 0; i < listSize; i ++ )
    {
    if ( list[i] != NULL )
      {
      delete [] list[i];
      }
    else
      {
      break;
      }
    }
    
  delete [] list;
}
 
int  vtkTableBasedClipperShapeList::GetList
   ( int listId, const int *& outlist ) const
{
  if ( listId < 0 || listId > currentList )
    {
    outlist = NULL;
    return 0;
    }
 
  outlist = list[ listId ];
  return ( listId == currentList ? currentShape : shapesPerList );
}
 
int  vtkTableBasedClipperShapeList::GetNumberOfLists() const
{
    return currentList + 1;
}

int  vtkTableBasedClipperShapeList::GetTotalNumberOfShapes() const
{
  int numFullLists = currentList;  // actually currentList-1+1
  int numExtra     = currentShape; // again, currentShape-1+1
 
  return numFullLists * shapesPerList + numExtra;
}

vtkTableBasedClipperHexList::vtkTableBasedClipperHexList()
    : vtkTableBasedClipperShapeList( 8 )
{
}
 
vtkTableBasedClipperHexList::~vtkTableBasedClipperHexList()
{
}

void vtkTableBasedClipperHexList::AddHex( int cellId, 
     int v1, int v2, int v3, int v4, int v5, int v6, int v7, int v8 )
{
  if ( currentShape >= shapesPerList )
    {
    if (  ( currentList + 1) >= listSize  )
      {
      int ** tmpList = new int * [ 2 * listSize ];
      
      for ( int i = 0; i < listSize; i ++ )
        {
        tmpList[i] = list[i];
        }
        
      for ( int i = listSize; i < listSize * 2; i ++ )
        {
        tmpList[i] = NULL;
        }

      listSize *= 2;
      delete [] list;
      list = tmpList;
      }
 
    currentList ++;
    list[ currentList ] = new int[  ( shapeSize + 1 ) * shapesPerList  ];
    currentShape = 0;
    }
 
  int idx = ( shapeSize + 1 )* currentShape;
  list[ currentList ][ idx + 0 ] = cellId;
  list[ currentList ][ idx + 1 ] = v1;
  list[ currentList ][ idx + 2 ] = v2;
  list[ currentList ][ idx + 3 ] = v3;
  list[ currentList ][ idx + 4 ] = v4;
  list[ currentList ][ idx + 5 ] = v5;
  list[ currentList ][ idx + 6 ] = v6;
  list[ currentList ][ idx + 7 ] = v7;
  list[ currentList ][ idx + 8 ] = v8;
  currentShape ++;
}

vtkTableBasedClipperWedgeList::vtkTableBasedClipperWedgeList()
    : vtkTableBasedClipperShapeList( 6 )
{
}
 
vtkTableBasedClipperWedgeList::~vtkTableBasedClipperWedgeList()
{
}

void vtkTableBasedClipperWedgeList::AddWedge
   ( int cellId, int v1, int v2, int v3, int v4, int v5, int v6 )
{
  if (currentShape >= shapesPerList)
    {
    if ((currentList+1) >= listSize)
      {
      int ** tmpList = new int * [ 2 * listSize ];
      for ( int i = 0; i < listSize; i ++ )
        {
        tmpList[i] = list[i];
        }
        
      for ( int i = listSize; i < listSize * 2; i ++ )
        {
        tmpList[i] = NULL;
        }
        
      listSize *= 2;
      delete [] list;
      list = tmpList;
      }
 
    currentList ++;
    list[ currentList ] = new int[  ( shapeSize + 1 ) * shapesPerList  ];
    currentShape = 0;
    }
 
  int idx = ( shapeSize + 1 ) * currentShape;
  list[ currentList ][ idx + 0 ] = cellId;
  list[ currentList ][ idx + 1 ] = v1;
  list[ currentList ][ idx + 2 ] = v2;
  list[ currentList ][ idx + 3 ] = v3;
  list[ currentList ][ idx + 4 ] = v4;
  list[ currentList ][ idx + 5 ] = v5;
  list[ currentList ][ idx + 6 ] = v6;
  currentShape ++;
}

vtkTableBasedClipperPyramidList::vtkTableBasedClipperPyramidList()
    : vtkTableBasedClipperShapeList( 5 )
{
}
 
vtkTableBasedClipperPyramidList::~vtkTableBasedClipperPyramidList()
{
}

void vtkTableBasedClipperPyramidList::AddPyramid
   ( int cellId, int v1, int v2, int v3, int v4, int v5 )
{
  if (currentShape >= shapesPerList)
    {
    if (  ( currentList + 1 ) >= listSize  )
      {
      int ** tmpList = new int * [ 2 * listSize ];
      
      for ( int i = 0; i < listSize; i ++ )
        {
        tmpList[i] = list[i];
        }
        
      for ( int i = listSize; i < listSize * 2; i ++ )
        {
        tmpList[i] = NULL;
        }
        
      listSize *= 2;
      delete [] list;
      list = tmpList;
      }
 
    currentList ++;
    list[ currentList ] = new int[  ( shapeSize + 1 ) * shapesPerList  ];
    currentShape = 0;
    }
 
  int idx = ( shapeSize + 1 ) * currentShape;
  list[ currentList ][ idx + 0 ] = cellId;
  list[ currentList ][ idx + 1 ] = v1;
  list[ currentList ][ idx + 2 ] = v2;
  list[ currentList ][ idx + 3 ] = v3;
  list[ currentList ][ idx + 4 ] = v4;
  list[ currentList ][ idx + 5 ] = v5;
  currentShape ++;
}

vtkTableBasedClipperTetList::vtkTableBasedClipperTetList()
    : vtkTableBasedClipperShapeList( 4 )
{
}
 
vtkTableBasedClipperTetList::~vtkTableBasedClipperTetList()
{
}

void vtkTableBasedClipperTetList::AddTet
   ( int cellId, int v1,int v2,int v3,int v4 )
{
  if ( currentShape >= shapesPerList )
    {
    if (  ( currentList + 1 )  >=  listSize  )
      {
      int ** tmpList = new int * [ 2 * listSize ];
      
      for ( int i = 0; i < listSize; i ++ )
        {
        tmpList[i] = list[i];
        }
        
      for ( int i = listSize; i < listSize * 2; i ++ )
        {
        tmpList[i] = NULL;
        }
        
      listSize *= 2;
      delete [] list;
      list = tmpList;
      }
 
    currentList ++;
    list[ currentList ] = new int[  ( shapeSize + 1 ) * shapesPerList  ];
    currentShape = 0;
    }
 
  int idx = ( shapeSize + 1 ) * currentShape;
  list[ currentList ][ idx + 0 ] = cellId;
  list[ currentList ][ idx + 1 ] = v1;
  list[ currentList ][ idx + 2 ] = v2;
  list[ currentList ][ idx + 3 ] = v3;
  list[ currentList ][ idx + 4 ] = v4;
  currentShape ++;
}

vtkTableBasedClipperQuadList::vtkTableBasedClipperQuadList()
    : vtkTableBasedClipperShapeList( 4 )
{
}
 
vtkTableBasedClipperQuadList::~vtkTableBasedClipperQuadList()
{
}

void vtkTableBasedClipperQuadList::AddQuad
   ( int cellId, int v1,int v2,int v3,int v4 )
{
  if ( currentShape >= shapesPerList )
    {
    if (  ( currentList + 1 ) >= listSize  )
      {
      int ** tmpList = new int * [ 2 * listSize ];
      for ( int i = 0; i < listSize; i ++ )
        {
        tmpList[i] = list[i];
        }
          
      for ( int i = listSize; i < listSize * 2; i ++ )
        {
        tmpList[i] = NULL;
        }
          
      listSize *= 2;
      delete [] list;
      list = tmpList;
      }
 
    currentList ++;
    list[ currentList ] = new int[  ( shapeSize + 1 ) * shapesPerList  ];
    currentShape = 0;
    }
 
  int idx = ( shapeSize + 1 ) * currentShape;
  list[ currentList ][ idx + 0 ] = cellId;
  list[ currentList ][ idx + 1 ] = v1;
  list[ currentList ][ idx + 2 ] = v2;
  list[ currentList ][ idx + 3 ] = v3;
  list[ currentList ][ idx + 4 ] = v4;
  currentShape ++;
}

vtkTableBasedClipperTriList::vtkTableBasedClipperTriList()
    : vtkTableBasedClipperShapeList( 3 )
{
}
 
vtkTableBasedClipperTriList::~vtkTableBasedClipperTriList()
{
}

void vtkTableBasedClipperTriList::AddTri
   ( int cellId, int v1,int v2,int v3 )
{
  if ( currentShape >= shapesPerList )
    {
    if (  ( currentList + 1 ) >= listSize  )
      {
      int ** tmpList = new int * [ 2 * listSize ];
      for ( int i = 0; i < listSize; i ++ )
        {
        tmpList[i] = list[i];
        }
          
      for ( int i = listSize; i < listSize * 2; i ++ )
        {
        tmpList[i] = NULL;
        }
          
      listSize *= 2;
      delete [] list;
      list = tmpList;
      }
 
    currentList ++;
    list[ currentList ] = new int[  ( shapeSize + 1 ) * shapesPerList  ];
    currentShape = 0;
    }
 
  int idx = ( shapeSize + 1 ) * currentShape;
  list[ currentList ][ idx + 0 ] = cellId;
  list[ currentList ][ idx + 1 ] = v1;
  list[ currentList ][ idx + 2 ] = v2;
  list[ currentList ][ idx + 3 ] = v3;
  currentShape ++;
}

vtkTableBasedClipperLineList::vtkTableBasedClipperLineList()
    : vtkTableBasedClipperShapeList( 2 )
{
}
 
vtkTableBasedClipperLineList::~vtkTableBasedClipperLineList()
{
}
 
void vtkTableBasedClipperLineList::AddLine( int cellId, int v1,int v2 )
{
  if ( currentShape >= shapesPerList )
    {
    if (  ( currentList + 1 )  >=  listSize  )
      {
      int ** tmpList = new int * [ 2 * listSize ];
      for ( int i = 0; i < listSize; i ++ )
        {
        tmpList[i] = list[i];
        }
          
      for ( int i = listSize; i < listSize * 2; i ++ )
        {
        tmpList[i] = NULL;
        }
          
      listSize *= 2;
      delete [] list;
      list = tmpList;
      }
 
    currentList ++;
    list[ currentList ] = new int[  ( shapeSize + 1 ) * shapesPerList  ];
    currentShape = 0;
    }
 
  int idx = ( shapeSize + 1 ) * currentShape;
  list[ currentList ][ idx + 0 ] = cellId;
  list[ currentList ][ idx + 1 ] = v1;
  list[ currentList ][ idx + 2 ] = v2;
  currentShape ++;
}

vtkTableBasedClipperVertexList::vtkTableBasedClipperVertexList()
    : vtkTableBasedClipperShapeList( 1 )
{
}
 
vtkTableBasedClipperVertexList::~vtkTableBasedClipperVertexList()
{
}
 
void vtkTableBasedClipperVertexList::AddVertex( int cellId, int v1 )
{
  if ( currentShape >= shapesPerList )
    {
    if (  ( currentList + 1 ) >= listSize  )
      {
      int ** tmpList = new int * [ 2 * listSize ];
      for ( int i = 0; i < listSize; i ++ )
        {
        tmpList[i] = list[i];
        }
          
      for ( int i = listSize; i < listSize * 2; i ++ )
        {
        tmpList[i] = NULL;
        }
          
      listSize *= 2;
      delete [] list;
      list = tmpList;
      }
 
    currentList ++;
    list[ currentList ] = new int[  ( shapeSize + 1 ) * shapesPerList  ];
    currentShape = 0;
    }
 
  int idx = ( shapeSize + 1) * currentShape;
  list[ currentList ][ idx + 0 ] = cellId;
  list[ currentList ][ idx + 1 ] = v1;
  currentShape ++;
}

void vtkTableBasedClipperVolumeFromVolume::
     ConstructDataSet( vtkPointData * inPD, vtkCellData * inCD, 
                       vtkUnstructuredGrid * output, double * pts_ptr )
{
  TableBasedClipperCommonPointsStructure cps;
  cps.hasPtsList = true;
  cps.pts_ptr    = pts_ptr;
  ConstructDataSet( inPD, inCD, output, cps );
}

void vtkTableBasedClipperVolumeFromVolume::
     ConstructDataSet( vtkPointData * inPD, vtkCellData * inCD, 
                       vtkUnstructuredGrid * output,
                       int * dims, double * X, double * Y, double * Z )
{
  TableBasedClipperCommonPointsStructure cps;
  cps.hasPtsList = false;
  cps.dims       = dims;
  cps.X          = X;
  cps.Y          = Y;
  cps.Z          = Z;
  ConstructDataSet( inPD, inCD, output, cps );
}

void vtkTableBasedClipperVolumeFromVolume::
     ConstructDataSet( vtkPointData * inPD, vtkCellData * inCD, 
                       vtkUnstructuredGrid * output,
                       TableBasedClipperCommonPointsStructure & cps )
{
  int   i, j, k, l;

  vtkPointData * outPD = output->GetPointData();
  vtkCellData  * outCD = output->GetCellData();

  vtkIntArray * newOrigNodes = NULL;
  vtkIntArray * origNodes = vtkIntArray::SafeDownCast
                (  inPD->GetArray( "avtOriginalNodeNumbers" )  );
  //
  // If the isovolume only affects a small part of the dataset, we can save
  // on memory by only bringing over the points from the original dataset
  // that are used with the output.  Determine which points those are here.
  //
  int * ptLookup = new int[ numPrevPts ];
  for ( i = 0; i < numPrevPts; i ++ )
    {
    ptLookup[i] = -1;
    }
    
  int numUsed = 0;
  for ( i = 0; i < nshapes; i ++ )
    {
    int nlists = shapes[i]->GetNumberOfLists();
    int npts_per_shape = shapes[i]->GetShapeSize();
    
    for ( j = 0; j < nlists; j ++ )
      {
      const int * list;
      int listSize = shapes[i]->GetList( j, list );
      
      for ( k = 0; k < listSize; k ++ )
        {
        list ++; // skip the cell id entry
        
        for ( l = 0; l < npts_per_shape; l ++ )
          {
          int pt = *list;
          list ++;
          
          if ( pt >= 0 && pt < numPrevPts )
            {
            if ( ptLookup[pt] == -1 )
              {
              ptLookup[pt] = numUsed ++;
              }
            }
          }
        }
      }
    }

  //
  // Set up the output points and its point data.
  //
  vtkPoints * outPts = vtkPoints::New();
  int centroidStart  = numUsed + pt_list.GetTotalNumberOfPoints();
  int nOutPts        = centroidStart + centroid_list.GetTotalNumberOfPoints();
  outPts->SetNumberOfPoints( nOutPts );
  outPD->CopyAllocate( inPD, nOutPts );
  
  if ( origNodes != NULL )
    {
    newOrigNodes = vtkIntArray::New();
    newOrigNodes->SetNumberOfComponents( origNodes->GetNumberOfComponents() );
    newOrigNodes->SetNumberOfTuples( nOutPts );
    newOrigNodes->SetName( origNodes->GetName() );
    }

  //
  // Copy over all the points from the input that are actually used in the
  // output.
  //
  for ( i = 0; i < numPrevPts; i ++ )
    {
    if ( ptLookup[i] == -1 )
      {
      continue;
      }

    if ( cps.hasPtsList )
      {
      outPts->SetPoint( ptLookup[i], cps.pts_ptr + 3 * i );
      }
    else
      {
      int I = i % cps.dims[0];
      int J = (i / cps.dims[0]) % cps.dims[1];
      int K = i / ( cps.dims[0] * cps.dims[1] );
      outPts->SetPoint( ptLookup[i], cps.X[I], cps.Y[J], cps.Z[K] );
      }

    outPD->CopyData( inPD, i, ptLookup[i] );
    if ( newOrigNodes )
      {
      newOrigNodes->SetTuple(  ptLookup[i], origNodes->GetTuple( i )  );
      }
    }
    
  int ptIdx = numUsed;

  //
  // Now construct all the points that are along edges and new and add 
  // them to the points list.
  //
  int nLists = pt_list.GetNumberOfLists();
  for ( i = 0; i < nLists; i ++ )
    {
    const TableBasedClipperPointEntry * pe_list = NULL;
    int nPts = pt_list.GetList( i, pe_list );
    for ( j = 0; j < nPts; j ++ )
      {
      const TableBasedClipperPointEntry & pe = pe_list[j];
      double pt[3];
      int idx1 = pe.ptIds[0];
      int idx2 = pe.ptIds[1];

      // Construct the original points -- this will depend on whether
      // or not we started with a rectilinear grid or a point set.
      double * pt1 = NULL;
      double * pt2 = NULL;
      double pt1_storage[3];
      double pt2_storage[3];
      if ( cps.hasPtsList )
        {
        pt1 = cps.pts_ptr + 3 * idx1;
        pt2 = cps.pts_ptr + 3 * idx2;
        }
      else
        {
        pt1 = pt1_storage;
        pt2 = pt2_storage;
        int I = idx1 % cps.dims[0];
        int J = ( idx1 / cps.dims[0] ) % cps.dims[1];
        int K = idx1 / ( cps.dims[0] * cps.dims[1] );
        pt1[0] = cps.X[I];
        pt1[1] = cps.Y[J];
        pt1[2] = cps.Z[K];
        I = idx2 % cps.dims[0];
        J = ( idx2 / cps.dims[0] ) % cps.dims[1];
        K = idx2 / ( cps.dims[0] * cps.dims[1] );
        pt2[0] = cps.X[I];
        pt2[1] = cps.Y[J];
        pt2[2] = cps.Z[K];
        }

      // Now that we have the original points, calculate the new one.
      double p  = pe.percent;
      double bp = 1.0 - p;
      pt[0] = pt1[0] * p + pt2[0] * bp;
      pt[1] = pt1[1] * p + pt2[1] * bp;
      pt[2] = pt1[2] * p + pt2[2] * bp;
      outPts->SetPoint( ptIdx, pt );
      outPD->InterpolateEdge( inPD, ptIdx, pe.ptIds[0], pe.ptIds[1], bp );
      
      if ( newOrigNodes )
        {
        int id = ( bp <= 0.5 ? pe.ptIds[0] : pe.ptIds[1] );
        newOrigNodes->SetTuple(  ptIdx, origNodes->GetTuple( id )  );
        }
      ptIdx ++;
      }
    }

  // 
  // Now construct the new "centroid" points and add them to the points list.
  //
  nLists = centroid_list.GetNumberOfLists();
  vtkIdList * idList = vtkIdList::New();
  for ( i = 0; i < nLists; i ++ )
    {
    const TableBasedClipperCentroidPointEntry * ce_list = NULL;
    int nPts = centroid_list.GetList( i, ce_list );
    for ( j = 0; j < nPts; j ++ )
      {
      const TableBasedClipperCentroidPointEntry & ce = ce_list[j];
      idList->SetNumberOfIds( ce.nPts );
      double pts[8][3];
      double weights[8];
      double pt[3] = { 0.0, 0.0, 0.0 };
      double weight_factor = 1.0 / ce.nPts;
      for ( k = 0; k < ce.nPts; k ++ )
        {
        weights[k] = 1.0 * weight_factor;
        int id = 0;
        
        if ( ce.ptIds[k] < 0 )
          {
          id = centroidStart - 1 - ce.ptIds[k];
          }
        else
        if ( ce.ptIds[k] >= numPrevPts )
          {
          id = numUsed + ( ce.ptIds[k] - numPrevPts );
          }
        else
          {
          id = ptLookup[ ce.ptIds[k] ];
          }
          
        idList->SetId( k, id );
        outPts->GetPoint( id, pts[k] );
        pt[0] += pts[k][0];
        pt[1] += pts[k][1];
        pt[2] += pts[k][2];
        }
      pt[0] *= weight_factor;
      pt[1] *= weight_factor;
      pt[2] *= weight_factor;

      outPts->SetPoint( ptIdx, pt );
      outPD->InterpolatePoint( outPD, ptIdx, idList, weights );
      if ( newOrigNodes )
        {
        // these 'created' nodes have no original designation
        for ( int z = 0; z < newOrigNodes->GetNumberOfComponents(); z ++ )
          {
          newOrigNodes->SetComponent( ptIdx, z, -1 );
          }
        }
      ptIdx ++;
      }
    }
  idList->Delete();

  //
  // We are finally done constructing the points list.  Set it with our
  // output and clean up memory.
  //
  output->SetPoints( outPts );
  outPts->Delete();

  if ( newOrigNodes )
    {
    // AddArray will overwrite an already existing array with 
    // the same name, exactly what we want here.
    outPD->AddArray( newOrigNodes );
    newOrigNodes->Delete();
    }

  //
  // Now set up the shapes and the cell data.
  //
  int cellId = 0;
  int nlists;

  int ncells    = 0;
  int conn_size = 0;
  for ( i = 0; i < nshapes; i ++ )
    {
    int ns     = shapes[i]->GetTotalNumberOfShapes();
    ncells    += ns;
    conn_size += ( shapes[i]->GetShapeSize() + 1 ) * ns;
    }

  outCD->CopyAllocate( inCD, ncells );

  vtkIdTypeArray * nlist = vtkIdTypeArray::New();
  nlist->SetNumberOfValues( conn_size );
  vtkIdType * nl = nlist->GetPointer( 0 );

  vtkUnsignedCharArray * cellTypes = vtkUnsignedCharArray::New();
  cellTypes->SetNumberOfValues( ncells );
  unsigned char * ct = cellTypes->GetPointer( 0 );

  vtkIdTypeArray * cellLocations = vtkIdTypeArray::New();
  cellLocations->SetNumberOfValues( ncells );
  vtkIdType * cl = cellLocations->GetPointer( 0 );

  vtkIdType ids[1024]; // 8 (for hex) should be max, but...
  int current_index = 0;
  for ( i = 0; i < nshapes; i ++ )
    {
    const int * list;
    nlists = shapes[i]->GetNumberOfLists();
    int shapesize = shapes[i]->GetShapeSize();
    int vtk_type = shapes[i]->GetVTKType();
    
    for ( j = 0; j < nlists; j ++ )
      {
      int listSize = shapes[i]->GetList( j, list );
      
      for ( k = 0; k < listSize; k ++ )
        {
        outCD->CopyData( inCD, list[0], cellId );
        
        for ( l = 0; l < shapesize; l ++ )
          {
          if (  list[ l + 1 ] < 0  )
            {
            ids[l] = centroidStart - 1 - list[ l + 1 ];
            }
          else 
          if (  list[ l + 1 ] >= numPrevPts  )
            {
            ids[l] = numUsed + (  list[ l + 1 ] - numPrevPts  );
            }
          else
            {
            ids[l] = ptLookup[  list[ l + 1 ]  ];
            }
          }
        list += shapesize + 1;
        *nl ++ = shapesize;
        *cl ++ = current_index;
        *ct ++ = vtk_type;
        for ( l = 0; l < shapesize; l ++ )
          {
          *nl ++ = ids[l];
          }
          
        current_index += shapesize + 1;
        cellId ++;
        }
      }
    }

  vtkCellArray * cells = vtkCellArray::New();
  cells->SetCells( ncells, nlist );
  nlist->Delete();

  output->SetCells( cellTypes, cellLocations, cells );
  cellTypes->Delete();
  cellLocations->Delete();
  cells->Delete();

  delete [] ptLookup;
}

inline void GetPoint( double * pt, const double * X, const double * Y,
                      const double * Z, const int * dims, const int & index )
{
  int cellI = index % dims[0];
  int cellJ = ( index / dims[0] ) % dims[1];
  int cellK = index / ( dims[0] * dims[1] );
  pt[0] = X[ cellI ];
  pt[1] = Y[ cellJ ];
  pt[2] = Z[ cellK ];
}
// ============================================================================
// =============== vtkTableBasedClipperVolumeFromVolume ( end ) ===============
// ============================================================================


//-----------------------------------------------------------------------------
// Construct with user-specified implicit function; InsideOut turned off; value
// set to 0.0; and generate clip scalars turned off.
vtkTableBasedClipDataSet::vtkTableBasedClipDataSet( vtkImplicitFunction * cf )
{
  this->Locator      = NULL;
  this->ClipFunction = cf;
  
  // setup a callback to report progress
  this->InternalProgressObserver = vtkCallbackCommand::New();
  this->InternalProgressObserver->SetCallback
        ( &vtkTableBasedClipDataSet::InternalProgressCallbackFunction );
  this->InternalProgressObserver->SetClientData( this );
  
  this->Value     = 0.0;
  this->InsideOut = 0;
  this->MergeTolerance        = 0.01;
  this->UseValueAsOffset      = true;
  this->GenerateClipScalars   = 0;
  this->GenerateClippedOutput = 0;

  this->SetNumberOfOutputPorts( 2 );
  vtkUnstructuredGrid * output2 = vtkUnstructuredGrid::New();
  this->GetExecutive()->SetOutputData( 1, output2 );
  output2->Delete();
  output2 = NULL;

  // process active point scalars by default
  this->SetInputArrayToProcess
        ( 0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS,
          vtkDataSetAttributes::SCALARS );

  this->GetInformation()->Set( vtkAlgorithm::PRESERVES_RANGES(), 1 );
  this->GetInformation()->Set( vtkAlgorithm::PRESERVES_BOUNDS(), 1 );
}

//-----------------------------------------------------------------------------
vtkTableBasedClipDataSet::~vtkTableBasedClipDataSet()
{
  if ( this->Locator )
    {
    this->Locator->UnRegister( this );
    this->Locator = NULL;
    }
  this->SetClipFunction( NULL );
  this->InternalProgressObserver->Delete();
  this->InternalProgressObserver = NULL;
}

//-----------------------------------------------------------------------------
void vtkTableBasedClipDataSet::InternalProgressCallbackFunction
   ( vtkObject * arg, unsigned long, void * clientdata, void * )
{
  reinterpret_cast < vtkTableBasedClipDataSet * > ( clientdata )
    ->InternalProgressCallback(  static_cast < vtkAlgorithm * > ( arg )  );
}

//-----------------------------------------------------------------------------
void vtkTableBasedClipDataSet::InternalProgressCallback
   ( vtkAlgorithm * algorithm )
{
  double progress = algorithm->GetProgress();
  this->UpdateProgress( progress );
  
  if ( this->AbortExecute )
    {
    algorithm->SetAbortExecute( 1 );
    }
}

//-----------------------------------------------------------------------------
unsigned long vtkTableBasedClipDataSet::GetMTime()
{
  unsigned long time;
  unsigned long mTime = this->Superclass::GetMTime();

  if ( this->ClipFunction != NULL )
    {
    time  = this->ClipFunction->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }
    
  if ( this->Locator != NULL )
    {
    time  = this->Locator->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}

vtkUnstructuredGrid *vtkTableBasedClipDataSet::GetClippedOutput()
{
  if ( !this->GenerateClippedOutput )
    {
    return NULL;
    }
    
  return vtkUnstructuredGrid::SafeDownCast
        (  this->GetExecutive()->GetOutputData( 1 )  );
}

//-----------------------------------------------------------------------------
void vtkTableBasedClipDataSet::SetLocator
   ( vtkIncrementalPointLocator * locator )
{
  if ( this->Locator == locator)
    {
    return;
    }
  
  if ( this->Locator )
    {
    this->Locator->UnRegister( this );
    this->Locator = NULL;
    }

  if ( locator )
    {
    locator->Register( this );
    }

  this->Locator = locator;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkTableBasedClipDataSet::CreateDefaultLocator()
{
  if ( this->Locator == NULL )
    {
    this->Locator = vtkMergePoints::New();
    this->Locator->Register( this );
    this->Locator->Delete();
    }
}

//-----------------------------------------------------------------------------
int vtkTableBasedClipDataSet::FillInputPortInformation
  ( int, vtkInformation * info )
{
  info->Set( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet" );
  return 1;
}

//------------------------------------------------------------------------------
int vtkTableBasedClipDataSet::ProcessRequest( vtkInformation* request,
    vtkInformationVector ** inputVector, vtkInformationVector * outputVector )
{
  if (   request->Has(  vtkStreamingDemandDrivenPipeline::
                        REQUEST_UPDATE_EXTENT_INFORMATION()  )   
     )
    {
    // compute the priority for this UpdateExtent
    double           priorVal = 1;
    vtkInformation * inputInf = inputVector[0]->GetInformationObject( 0 );
    vtkInformation * outInfor = outputVector->GetInformationObject( 0 );
    
    if (   inputInf->Has(  vtkStreamingDemandDrivenPipeline::PRIORITY()  )   )
      {
      priorVal = inputInf->Get( vtkStreamingDemandDrivenPipeline::PRIORITY() );
      }
      
    if ( !priorVal )
      {
      inputInf = NULL;
      outInfor = NULL;
      return 1;
      }
      
    // Get bounds and evaluate implicit function. If all bounds
    // evaluate to a value smaller than input value, this piece
    // has priority set to 0.

    double        priority   = 1;
    double      * wholeBox   = NULL;
    static double boundBox[] = { -1.0, 1.0, -1.0, 1.0, -1.0, 1.0 };

    // determine geometric bounds of this piece
    wholeBox = inputInf->Get
               ( vtkStreamingDemandDrivenPipeline::PIECE_BOUNDING_BOX() );
    if ( !wholeBox )
      {
      wholeBox = inputInf->Get
                 ( vtkStreamingDemandDrivenPipeline::WHOLE_BOUNDING_BOX() );
      }
      
    if ( wholeBox )
      {
      boundBox[0] = wholeBox[0];
      boundBox[1] = wholeBox[1];
      boundBox[2] = wholeBox[2];
      boundBox[3] = wholeBox[3];
      boundBox[4] = wholeBox[4];
      boundBox[5] = wholeBox[5];
      wholeBox    = NULL;
      }
    else
      {
      //try to figure out geometric bounds
      double * originPt = inputInf->Get( vtkDataObject::ORIGIN() );
      double * spacings = inputInf->Get( vtkDataObject::SPACING() );
      int    * subXtent = inputInf->Get
                          ( vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT() );
                           
      if ( originPt && spacings && subXtent )
        {
        boundBox[0] = originPt[0] + subXtent[0] * spacings[0];
        boundBox[1] = originPt[0] + subXtent[1] * spacings[0];
        boundBox[2] = originPt[1] + subXtent[2] * spacings[1];
        boundBox[3] = originPt[1] + subXtent[3] * spacings[1];
        boundBox[4] = originPt[2] + subXtent[4] * spacings[2];
        boundBox[5] = originPt[2] + subXtent[5] * spacings[2];
        originPt    = NULL;
        spacings    = NULL;
        subXtent    = NULL;
        }
      else
        {
        outInfor->Set( vtkStreamingDemandDrivenPipeline::PRIORITY(), priorVal );
        inputInf = NULL;
        outInfor = NULL;
        originPt = NULL;
        spacings = NULL;
        subXtent = NULL;
        return 1;
        }
      }

    vtkPlane * clipFunc = vtkPlane::SafeDownCast( this->GetClipFunction() );
    if ( !clipFunc )
      {
      outInfor->Set( vtkStreamingDemandDrivenPipeline::PRIORITY(), priorVal );
      inputInf = NULL;
      outInfor = NULL;
      return 1;
      }

    static double boxValue[8];
    boxValue[0] = clipFunc->EvaluateFunction
                            ( boundBox[0], boundBox[2], boundBox[4] );
    boxValue[1] = clipFunc->EvaluateFunction
                            ( boundBox[0], boundBox[2], boundBox[5] );
    boxValue[2] = clipFunc->EvaluateFunction
                            ( boundBox[0], boundBox[3], boundBox[4] );
    boxValue[3] = clipFunc->EvaluateFunction
                            ( boundBox[0], boundBox[3], boundBox[5] );
    boxValue[4] = clipFunc->EvaluateFunction
                            ( boundBox[1], boundBox[2], boundBox[4] );
    boxValue[5] = clipFunc->EvaluateFunction
                            ( boundBox[1], boundBox[2], boundBox[5] );
    boxValue[6] = clipFunc->EvaluateFunction
                            ( boundBox[1], boundBox[3], boundBox[4] );
    boxValue[7] = clipFunc->EvaluateFunction
                            ( boundBox[1], boundBox[3], boundBox[5] );
    clipFunc    = NULL;

    priority = 0;
    for ( int i = 0; i < 8; i ++ )
      {
      if ( boxValue[i] > this->Value )
        {
        priority = priorVal;
        break;
        }
      }
    outInfor->Set( vtkStreamingDemandDrivenPipeline::PRIORITY(), priority );
    
    inputInf = NULL;
    outInfor = NULL;
    return 1;
    }

  //all other requests handled by superclass
  return this->Superclass::ProcessRequest( request, inputVector, outputVector );
}

//-----------------------------------------------------------------------------
int vtkTableBasedClipDataSet::RequestData( vtkInformation * vtkNotUsed( request ),
    vtkInformationVector ** inputVector, vtkInformationVector * outputVector )
{
  // input and output information objects
  vtkInformation * inputInf = inputVector[0]->GetInformationObject( 0 );
  vtkInformation * outInfor = outputVector->GetInformationObject( 0 );

  // Get the input of which we have to create a copy since the clipper requires
  // that InterpolateAllocate() be invoked for the output based on its input in
  // terms of the point data. If the input and output arrays are different, 
  // vtkCell3D's Clip will fail. The last argument of InterpolateAllocate makes
  // sure that arrays are shallow-copied from theInput to cpyInput.
  vtkDataSet * theInput = vtkDataSet::SafeDownCast
                          (  inputInf->Get( vtkDataObject::DATA_OBJECT() )  );
  vtkSmartPointer< vtkDataSet > cpyInput;
  cpyInput.TakeReference( theInput->NewInstance() );
  cpyInput->CopyStructure( theInput  );
  cpyInput->GetCellData()->PassData( theInput->GetCellData() );
  cpyInput->GetPointData()
          ->InterpolateAllocate( theInput->GetPointData(), 0, 0, 1 );

  // get the output (the remaining and the clipped parts)
  vtkUnstructuredGrid * outputUG = vtkUnstructuredGrid::SafeDownCast
                        (  outInfor->Get( vtkDataObject::DATA_OBJECT() )  );
  
  inputInf = NULL;
  outInfor = NULL;
  theInput = NULL;
  vtkDebugMacro( << "Clipping dataset" << endl );

  
  int  i;
  vtkIdType  numbPnts = cpyInput->GetNumberOfPoints();

  // handling exceptions
  if ( numbPnts < 1 )
    {
    vtkDebugMacro( << "No data to clip" << endl );
    outputUG = NULL;
    return 1;
    }

  if ( !this->ClipFunction && this->GenerateClipScalars )
    {
    vtkErrorMacro( << "Cannot generate clip scalars "
                   << "if no clip function defined" << endl );
    outputUG = NULL;
    return 1;
    }


  vtkDataArray   * clipAray = NULL;
  vtkDoubleArray * pScalars = NULL;

  // check whether the cells are clipped with input scalars or a clip function
  if ( this->ClipFunction )
    {
    pScalars = vtkDoubleArray::New();
    pScalars->SetNumberOfTuples( numbPnts );
    pScalars->SetName( "ClipDataSetScalars" );
    
    // enable clipDataSetScalars to be passed to the output
    if ( this->GenerateClipScalars )
      {
      cpyInput->GetPointData()->SetScalars( pScalars );
      }
      
    for ( i = 0; i < numbPnts; i ++ )
      {
      double s = this->ClipFunction->FunctionValue(  cpyInput->GetPoint( i )  );
      pScalars->SetTuple1( i, s );
      }
      
    clipAray = pScalars;
    }
  else //using input scalars
    {
    clipAray = this->GetInputArrayToProcess( 0, inputVector );
    if ( !clipAray ) 
      {
      vtkErrorMacro( << "no input scalars." << endl );
      return 1;
      }
    }
    
  
  int    gridType = cpyInput->GetDataObjectType(); 
  double isoValue = ( !this->ClipFunction || this->UseValueAsOffset ) 
                    ?  this->Value  :  0.0;               
  if ( gridType == VTK_IMAGE_DATA || gridType == VTK_STRUCTURED_POINTS )
    {
    int   numbDims;
    int * dataDims = vtkImageData::SafeDownCast( cpyInput )->GetDimensions();
    for ( numbDims = 3, i = 0; i < 3; i ++ )
      {
      numbDims -= (  ( dataDims[i] <= 1 ) ? 1 : 0  );
      }
    dataDims = NULL;
      
    if ( numbDims == 3 )
      {
      this->ClipImageData( cpyInput.GetPointer(), clipAray, isoValue, outputUG );
      }
    }
  else              
  if ( gridType == VTK_POLY_DATA )
    {
    this->ClipPolyData( cpyInput.GetPointer(), clipAray, isoValue, outputUG );
    }
  else
  if ( gridType == VTK_RECTILINEAR_GRID )
    {
    this->ClipRectilinearGridData( cpyInput.GetPointer(), clipAray, 
                                   isoValue, outputUG );
    }
  else
  if ( gridType == VTK_STRUCTURED_GRID )
    {
    this->ClipStructuredGridData( cpyInput.GetPointer(), clipAray, 
                                  isoValue, outputUG );
    }
  else
  if ( gridType == VTK_UNSTRUCTURED_GRID )
    {
    this->ClipUnstructuredGridData( cpyInput.GetPointer(), clipAray, 
                                    isoValue, outputUG );
    }
  else
    {
    this->ClipDataSet( cpyInput.GetPointer(), clipAray, outputUG );
    }
    
  outputUG->Squeeze();
  
  if ( pScalars )
    {
    pScalars->Delete();
    }
  pScalars = NULL;
  outputUG = NULL;
  clipAray = NULL;
  
  return 1;
}

//-----------------------------------------------------------------------------
void vtkTableBasedClipDataSet::ClipDataSet( vtkDataSet * pDataSet, 
     vtkDataArray * clipAray, vtkUnstructuredGrid * unstruct )
{
  vtkClipDataSet * clipData = vtkClipDataSet::New();
  clipData->SetInput( pDataSet );
  clipData->SetValue( this->Value );
  clipData->SetInsideOut( this->InsideOut );
  clipData->SetClipFunction( this->ClipFunction );
  clipData->SetUseValueAsOffset( this->UseValueAsOffset );
  clipData->SetGenerateClipScalars( this->GenerateClipScalars );
  
  if ( !this->ClipFunction )
    {
    pDataSet->GetPointData()->SetScalars( clipAray );
    }
    
  clipData->Update();
  unstruct->ShallowCopy( clipData->GetOutput() );
  
  clipData->Delete();
  clipData = NULL;
}

//-----------------------------------------------------------------------------
void vtkTableBasedClipDataSet::ClipImageData( vtkDataSet * inputGrd, 
     vtkDataArray * clipAray, double isoValue, vtkUnstructuredGrid * outputUG )
{
  int                  i, j;
  int                  dataDims[3];
  double               spacings[3];
  double               tmpValue = 0.0;
  double             * dataBBox = NULL;
  vtkImageData       * volImage = NULL;
  vtkDoubleArray     * pxCoords = NULL;
  vtkDoubleArray     * pyCoords = NULL;
  vtkDoubleArray     * pzCoords = NULL;
  vtkRectilinearGrid * rectGrid = NULL;
  
  volImage = vtkImageData::SafeDownCast( inputGrd );
  volImage->GetDimensions( dataDims );
  volImage->GetSpacing( spacings );
  dataBBox = volImage->GetBounds();
  
  pxCoords = vtkDoubleArray::New();
  pyCoords = vtkDoubleArray::New();
  pzCoords = vtkDoubleArray::New();
  vtkDoubleArray * tmpArays[3] = { pxCoords, pyCoords, pzCoords };
  for ( j = 0; j < 3; j ++ )
    {
    tmpArays[j]->SetNumberOfComponents( 1 );
    tmpArays[j]->SetNumberOfTuples( dataDims[j] );
    for ( tmpValue  = dataBBox[ j << 1 ], i = 0; i < dataDims[j]; i ++, 
          tmpValue += spacings[j] )
      {
      tmpArays[j]->SetComponent( i, 0, tmpValue );
      }
    tmpArays[j] = NULL;
    }
    
  rectGrid = vtkRectilinearGrid::New();
  rectGrid->SetDimensions( dataDims );
  rectGrid->SetXCoordinates( pxCoords );
  rectGrid->SetYCoordinates( pyCoords );
  rectGrid->SetZCoordinates( pzCoords );
  rectGrid->GetPointData()->ShallowCopy( volImage->GetPointData() );
  rectGrid->GetCellData()->ShallowCopy( volImage->GetCellData() );
  
  this->ClipRectilinearGridData( rectGrid, clipAray, isoValue, outputUG );
  
  pxCoords->Delete();
  pyCoords->Delete();
  pzCoords->Delete();
  rectGrid->Delete();
  pxCoords = NULL;
  pyCoords = NULL;
  pzCoords = NULL;
  rectGrid = NULL;
  volImage = NULL;
  dataBBox = NULL;
}

//-----------------------------------------------------------------------------
void vtkTableBasedClipDataSet::ClipPolyData( vtkDataSet * inputGrd, 
     vtkDataArray * clipAray, double isoValue, vtkUnstructuredGrid * outputUG )
{
  vtkPolyData * polyData = vtkPolyData::SafeDownCast( inputGrd );
  int           numCells = polyData->GetNumberOfCells();

  vtkTableBasedClipperVolumeFromVolume   * visItVFV = new
  vtkTableBasedClipperVolumeFromVolume(    polyData->GetNumberOfPoints(),
     int(   pow(  double( numCells ),  double( 0.6667f )  )   ) * 5 + 100    );

  vtkUnstructuredGrid * specials = vtkUnstructuredGrid::New();
  specials->SetPoints( polyData->GetPoints() );
  specials->GetPointData()->ShallowCopy( polyData->GetPointData() );
  specials->Allocate( numCells );

  vtkIdType   i, j;
  vtkIdType   numbPnts = 0;
  int         numCants = 0;  // number of cells not clipped by this filter
  
  for ( i = 0; i < numCells; i ++ )
    {
    int         cellType = polyData->GetCellType( i );
    bool        bCanClip = false;
    vtkIdType * pntIndxs = NULL;
    polyData->GetCellPoints( i, numbPnts, pntIndxs );
    
    switch ( cellType )
      {
      case VTK_TETRA:
      case VTK_PYRAMID:
      case VTK_WEDGE:
      case VTK_HEXAHEDRON:
      case VTK_TRIANGLE:
      case VTK_QUAD:
      case VTK_LINE:
      case VTK_VERTEX:
           bCanClip = true;
           break;
                       
      default:
           bCanClip = false;
           break;
      }
 
    if ( bCanClip )
      {
      double    grdDiffs[8];
      int       caseIndx = 0;
      
      for ( j = numbPnts - 1; j >= 0; j -- )
        {   
        grdDiffs[j] = clipAray->GetComponent( pntIndxs[j], 0 ) - isoValue;
        caseIndx   += (  ( grdDiffs[j] >= 0.0 ) ? 1 : 0  );
        caseIndx  <<= (  1 - ( !j )  );
        }

      int             startIdx = 0;
      int             nOutputs = 0;
      typedef int     EDGEIDXS[2];
      EDGEIDXS      * edgeVtxs = NULL;
      unsigned char * thisCase = NULL;

      switch ( cellType )
        {
        case VTK_TETRA:
          startIdx = vtkTableBasedClipperClipTables::StartClipShapesTet[ caseIndx ];
          thisCase =&vtkTableBasedClipperClipTables::ClipShapesTet[ startIdx ];
          nOutputs = vtkTableBasedClipperClipTables::NumClipShapesTet[ caseIndx ];
          edgeVtxs = ( EDGEIDXS * ) 
                     vtkTableBasedClipperTriangulationTables::TetVerticesFromEdges;
          break;
          
        case VTK_PYRAMID:
          startIdx = vtkTableBasedClipperClipTables::StartClipShapesPyr[ caseIndx ];
          thisCase =&vtkTableBasedClipperClipTables::ClipShapesPyr[ startIdx ];
          nOutputs = vtkTableBasedClipperClipTables::NumClipShapesPyr[ caseIndx ];
          edgeVtxs = ( EDGEIDXS * ) 
                     vtkTableBasedClipperTriangulationTables::PyramidVerticesFromEdges;
          break;
          
        case VTK_WEDGE:
          startIdx = vtkTableBasedClipperClipTables::StartClipShapesWdg[ caseIndx ];
          thisCase =&vtkTableBasedClipperClipTables::ClipShapesWdg[ startIdx ];
          nOutputs = vtkTableBasedClipperClipTables::NumClipShapesWdg[ caseIndx ];
          edgeVtxs = ( EDGEIDXS * ) 
                     vtkTableBasedClipperTriangulationTables::WedgeVerticesFromEdges;
          break;
          
        case VTK_HEXAHEDRON:
          startIdx = vtkTableBasedClipperClipTables::StartClipShapesHex[ caseIndx ];
          thisCase =&vtkTableBasedClipperClipTables::ClipShapesHex[ startIdx ];
          nOutputs = vtkTableBasedClipperClipTables::NumClipShapesHex[ caseIndx ];
          edgeVtxs = ( EDGEIDXS * ) 
                     vtkTableBasedClipperTriangulationTables::HexVerticesFromEdges;
          break;
          
        case VTK_TRIANGLE:
          startIdx = vtkTableBasedClipperClipTables::StartClipShapesTri[ caseIndx ];
          thisCase =&vtkTableBasedClipperClipTables::ClipShapesTri[ startIdx ];
          nOutputs = vtkTableBasedClipperClipTables::NumClipShapesTri[ caseIndx ];
          edgeVtxs = ( EDGEIDXS * ) 
                     vtkTableBasedClipperTriangulationTables::TriVerticesFromEdges;
          break;
          
        case VTK_QUAD:
          startIdx = vtkTableBasedClipperClipTables::StartClipShapesQua[ caseIndx ];
          thisCase =&vtkTableBasedClipperClipTables::ClipShapesQua[ startIdx ];
          nOutputs = vtkTableBasedClipperClipTables::NumClipShapesQua[ caseIndx ];
          edgeVtxs = ( EDGEIDXS * ) 
                     vtkTableBasedClipperTriangulationTables::QuadVerticesFromEdges;
          break;
          
        case VTK_LINE:
          startIdx = vtkTableBasedClipperClipTables::StartClipShapesLin[ caseIndx ];
          thisCase =&vtkTableBasedClipperClipTables::ClipShapesLin[ startIdx ];
          nOutputs = vtkTableBasedClipperClipTables::NumClipShapesLin[ caseIndx ];
          edgeVtxs = ( EDGEIDXS * ) 
                     vtkTableBasedClipperTriangulationTables::LineVerticesFromEdges;
          break;
          
        case VTK_VERTEX:
          startIdx = vtkTableBasedClipperClipTables::StartClipShapesVtx[ caseIndx ];
          thisCase =&vtkTableBasedClipperClipTables::ClipShapesVtx[ startIdx ];
          nOutputs = vtkTableBasedClipperClipTables::NumClipShapesVtx[ caseIndx ];
          edgeVtxs = NULL;
          break;
        }

      int  intrpIds[4];
      for ( j = 0; j < nOutputs; j ++ )
        {
        int      nCellPts = 0;
        int      intrpIdx = -1;
        int      theColor = -1;
        unsigned char theShape = *thisCase ++;
        
        switch ( theShape )
          {
          case ST_HEX:
            nCellPts = 8;
            theColor = *thisCase ++;
            break;
            
          case ST_WDG:
            nCellPts = 6;
            theColor = *thisCase ++;
            break;
            
          case ST_PYR:
            nCellPts = 5;
            theColor = *thisCase ++;
            break;
          case ST_TET:
            nCellPts = 4;
            theColor = *thisCase ++;
            break;
            
          case ST_QUA:
            nCellPts = 4;
            theColor = *thisCase ++;
            break;
            
          case ST_TRI:
            nCellPts = 3;
            theColor = *thisCase ++;
            break;
            
          case ST_LIN:
            nCellPts = 2;
            theColor = *thisCase ++;
            break;
            
          case ST_VTX:
            nCellPts = 1;
            theColor = *thisCase ++;
            break;
            
          case ST_PNT:
            intrpIdx = *thisCase ++;
            theColor = *thisCase ++;
            nCellPts = *thisCase ++;
            break;
            
          default:
            vtkErrorMacro( << "An invalid output shape was found in "
                           << "the ClipCases." << endl );
          }

        if ( (!this->InsideOut && theColor == COLOR0 ) ||
             ( this->InsideOut && theColor == COLOR1 )
           )
          {
          // We don't want this one; it's the wrong side.
          thisCase += nCellPts;
          continue;
          }

        int   shapeIds[8];
        for ( int p = 0; p < nCellPts; p ++ )
          {
          unsigned char pntIndex = *thisCase ++;
          
          if ( pntIndex <= P7 )
            {
            shapeIds[p] = pntIndxs[ pntIndex ];
            }
          else 
          if ( pntIndex >= EA && pntIndex <= EL )
            {
            int pt1Index = edgeVtxs[ pntIndex - EA ][0];
            int pt2Index = edgeVtxs[ pntIndex - EA ][1];
            if ( pt2Index < pt1Index )
              {
              int temp = pt2Index;
              pt2Index = pt1Index;
              pt1Index = temp;
              }
            double pt1ToPt2 = grdDiffs[ pt2Index ] - grdDiffs[ pt1Index ];
            double pt1ToIso = 0.0 - grdDiffs[ pt1Index ];
            double p1Weight = 1.0 - pt1ToIso / pt1ToPt2;

            int    pntIndx1 = pntIndxs[ pt1Index ];
            int    pntIndx2 = pntIndxs[ pt2Index ];
            
            shapeIds[p] = visItVFV->AddPoint( pntIndx1, pntIndx2, p1Weight );
            }
          else 
          if ( pntIndex >= N0 && pntIndex <= N3 )
            {
            shapeIds[p] = intrpIds[ pntIndex - N0 ];
            }
          else
            {
            vtkErrorMacro( << "An invalid output point value "
                           << "was found in the ClipCases." << endl );
            }
          }

        switch ( theShape )
          {
          case ST_HEX:
            visItVFV->AddHex( i, shapeIds[0], shapeIds[1], 
                                 shapeIds[2], shapeIds[3], shapeIds[4], 
                                 shapeIds[5], shapeIds[6], shapeIds[7] );
            break;
            
          case ST_WDG:
            visItVFV->AddWedge( i, shapeIds[0], shapeIds[1], shapeIds[2],
                                   shapeIds[3], shapeIds[4], shapeIds[5] );
            break;
            
          case ST_PYR:
            visItVFV->AddPyramid( i, shapeIds[0], shapeIds[1],
                                     shapeIds[2], shapeIds[3], shapeIds[4] );
            break;
            
          case ST_TET:
            visItVFV->AddTet( i, shapeIds[0], shapeIds[1], 
                                 shapeIds[2], shapeIds[3] );
            break;
            
          case ST_QUA:
            visItVFV->AddQuad( i, shapeIds[0], shapeIds[1], 
                                  shapeIds[2], shapeIds[3] );
            break;
            
          case ST_TRI:
            visItVFV->AddTri( i, shapeIds[0], shapeIds[1], shapeIds[2] );
            break;
            
          case ST_LIN:
            visItVFV->AddLine( i, shapeIds[0], shapeIds[1] );
            break;
            
          case ST_VTX:
            visItVFV->AddVertex( i, shapeIds[0] );
            break;
            
          case ST_PNT:
            intrpIds[intrpIdx] = visItVFV->AddCentroidPoint( nCellPts, shapeIds );
            break;
          }
        }
        
      edgeVtxs = NULL;
      thisCase = NULL;
      }
    else
      {
      if ( numCants == 0 )
        {
        specials->GetCellData()
                ->CopyAllocate( polyData->GetCellData(), numCells );
        }

      specials->InsertNextCell( cellType, numbPnts, pntIndxs );
      specials->GetCellData()
              ->CopyData( polyData->GetCellData(), i, numCants );
      numCants ++;
      }
      
    pntIndxs = NULL;
    }
  

  int         toDelete = 0;
  double    * theCords = NULL;
  vtkPoints * inputPts = polyData->GetPoints();
  if ( inputPts->GetDataType() == VTK_DOUBLE )
    {
    theCords = static_cast < double * > (  inputPts->GetVoidPointer( 0 )  );
    }
  else
    {
    toDelete = 1;
    numbPnts = inputPts->GetNumberOfPoints();
    theCords = new double [ numbPnts * 3 ];
    for ( i = 0; i < numbPnts; i ++ )
      {
      inputPts->GetPoint( i, theCords + ( i << 1 ) + i );
      }
    }
  inputPts = NULL;
  

  if ( numCants > 0 )
    {
    vtkUnstructuredGrid * vtkUGrid  = vtkUnstructuredGrid::New();
    this->ClipDataSet( specials, clipAray, vtkUGrid );
    
    vtkUnstructuredGrid * visItGrd = vtkUnstructuredGrid::New();
    visItVFV->ConstructDataSet( polyData->GetPointData(), 
                                polyData->GetCellData(), visItGrd, theCords );

    vtkAppendFilter * appender = vtkAppendFilter::New();
    appender->AddInput( vtkUGrid );
    appender->AddInput( visItGrd );
    appender->Update();

    outputUG->ShallowCopy( appender->GetOutput() );
    
    appender->Delete();
    vtkUGrid->Delete();
    visItGrd->Delete();
    appender = NULL;
    vtkUGrid = NULL;
    visItGrd = NULL;
    }
  else
    {
    visItVFV->ConstructDataSet( polyData->GetPointData(), 
                                polyData->GetCellData(), outputUG, theCords );
    }


  specials->Delete();
  delete visItVFV;
  if ( toDelete )
    {
    delete [] theCords;
    }
  specials = NULL;
  visItVFV = NULL;
  theCords = NULL;
  polyData = NULL;
}

//-----------------------------------------------------------------------------
void vtkTableBasedClipDataSet::ClipRectilinearGridData( vtkDataSet * inputGrd, 
     vtkDataArray * clipAray, double isoValue, vtkUnstructuredGrid * outputUG )
{
  vtkRectilinearGrid * rectGrid = vtkRectilinearGrid::SafeDownCast( inputGrd );
  
  int   i, j;
  int   numCells = 0;
  int   isTwoDim = 0;
  int   rectDims[3];
  rectGrid->GetDimensions( rectDims );
  isTwoDim = int( rectDims[2] <= 1 );
  numCells = rectGrid->GetNumberOfCells();

  vtkTableBasedClipperVolumeFromVolume   * visItVFV = new
  vtkTableBasedClipperVolumeFromVolume(    rectGrid->GetNumberOfPoints(),
      int(   pow(  double( numCells ), double( 0.6667f )  )   ) * 5 + 100    );

  int   shiftLUT[3][8] = { 
                           { 0, 1, 1, 0, 0, 1, 1, 0 },
                           { 0, 0, 1, 1, 0, 0, 1, 1 },
                           { 0, 0, 0, 0, 1, 1, 1, 1 }
                         };
  int   cellDims[3] = { rectDims[0] - 1, rectDims[1] - 1, rectDims[2] - 1 };
  int   cyStride    = cellDims[0];
  int   czStride    = cellDims[0] * cellDims[1];
  int   pyStride    = rectDims[0];
  int   pzStride    = rectDims[0] * rectDims[1];
  
  for ( i = 0; i < numCells; i ++ )
    {     
    int    caseIndx = 0;   
    int    nCellPts = isTwoDim ? 4 : 8;
    int    theCellI =   i % cellDims[0];
    int    theCellJ = ( i / cyStride ) % cellDims[1];
    int    theCellK = ( i / czStride );
    double grdDiffs[8];
    
    for ( j = nCellPts - 1; j >= 0; j -- )
      {   
      grdDiffs[j] = clipAray->GetComponent
                              (  ( theCellK + shiftLUT[2][j] ) * pzStride + 
                                 ( theCellJ + shiftLUT[1][j] ) * pyStride + 
                                 ( theCellI + shiftLUT[0][j] ),  0 
                              ) - isoValue;
      caseIndx   += (  ( grdDiffs[j] >= 0.0 ) ? 1 : 0  );
      caseIndx  <<= (  1 - ( !j )  );
      }

    int             nOutputs;
    int             intrpIds[4];
    unsigned char * thisCase = NULL;
    
    if ( isTwoDim )
      {
      thisCase = &vtkTableBasedClipperClipTables::ClipShapesQua
               [  vtkTableBasedClipperClipTables::StartClipShapesQua[ caseIndx ]  ];
      nOutputs = vtkTableBasedClipperClipTables::NumClipShapesQua[ caseIndx ];
      }
    else
      {
      thisCase = &vtkTableBasedClipperClipTables::ClipShapesHex
               [  vtkTableBasedClipperClipTables::StartClipShapesHex[ caseIndx ]  ];
      nOutputs = vtkTableBasedClipperClipTables::NumClipShapesHex[ caseIndx ];
      }

    for ( j = 0; j < nOutputs; j++ )
      {
      int      intrpIdx = -1;
      int      theColor = -1;
      unsigned char theShape = *thisCase ++;
      
      nCellPts = 0;
      switch ( theShape )
        {
        case ST_HEX:
          nCellPts = 8;
          theColor = *thisCase ++;
          break;
          
        case ST_WDG:
          nCellPts = 6;
          theColor = *thisCase ++;
          break;
          
        case ST_PYR:
          nCellPts = 5;
          theColor = *thisCase ++;
          break;
          
        case ST_TET:
          nCellPts = 4;
          theColor = *thisCase ++;
          break;
          
        case ST_QUA:
          nCellPts = 4;
          theColor = *thisCase ++;
          break;
          
        case ST_TRI:
          nCellPts = 3;
          theColor = *thisCase ++;
          break;
          
        case ST_LIN:
          nCellPts = 2;
          theColor = *thisCase ++;
          break;
          
        case ST_VTX:
          nCellPts = 1;
          theColor = *thisCase ++;
          break;
          
        case ST_PNT:
          intrpIdx = *thisCase ++;
          theColor = *thisCase ++;
          nCellPts = *thisCase ++;
          break;
          
        default:
          vtkErrorMacro( << "An invalid output shape was found in "
                         << "the ClipCases." << endl );
        }

      if ( (!this->InsideOut && theColor == COLOR0 ) ||
           ( this->InsideOut && theColor == COLOR1 )
         )
        {
        // We don't want this one; it's the wrong side.
        thisCase += nCellPts;
        continue;
        }

      int   shapeIds[8];
      for ( int p = 0; p < nCellPts; p ++ )
        {
        unsigned char pntIndex = *thisCase ++;
        
        if ( pntIndex <= P7 )
          {
          // We know pt P0 must be >P0 since we already
          // assume P0 == 0.  This is why we do not
          // bother subtracting P0 from pt here.
          shapeIds[p] = 
                      (   (  theCellI + shiftLUT[0][ pntIndex ]  ) +
                          (  theCellJ + shiftLUT[1][ pntIndex ]  ) * pyStride +
                          (  theCellK + shiftLUT[2][ pntIndex ]  ) * pzStride
                      );
          }
        else 
        if ( pntIndex >= EA && pntIndex <= EL )
          {
          int pt1Index = vtkTableBasedClipperTriangulationTables::
                         HexVerticesFromEdges[ pntIndex - EA ][0];
          int pt2Index = vtkTableBasedClipperTriangulationTables::
                         HexVerticesFromEdges[ pntIndex - EA ][1];
          
          if ( pt2Index < pt1Index )
            {
            int temp = pt2Index;
            pt2Index = pt1Index;
            pt1Index = temp;
            }
            
          double pt1ToPt2 = grdDiffs[ pt2Index ] - grdDiffs[ pt1Index ];
          double pt1ToIso = 0.0 - grdDiffs[ pt1Index ];
          double p1Weight = 1.0 - pt1ToIso / pt1ToPt2;

          int    pntIndx1 = 
                 (   (  theCellI + shiftLUT[0][ pt1Index ]  ) +
                     (  theCellJ + shiftLUT[1][ pt1Index ]  ) * pyStride +
                     (  theCellK + shiftLUT[2][ pt1Index ]  ) * pzStride
                 );
          int    pntIndx2 = 
                 (   (  theCellI + shiftLUT[0][ pt2Index ]  ) +
                     (  theCellJ + shiftLUT[1][ pt2Index ]  ) * pyStride +
                     (  theCellK + shiftLUT[2][ pt2Index ]  ) * pzStride
                 );

          /* We may have physically (though not logically) degenerate cells
          // if p1Weight == 0 or p1Weight == 1. We could pretty easily and 
          // mostly safely clamp percent to the range [1e-4, 1 - 1e-4].
          if( p1Weight == 1.0) 
            {
            shapeIds[p] = pntIndx1;
            }
          else 
          if( p1Weight == 0.0 ) 
            {
            shapeIds[p] = pntIndx2;
            }
          else
          
            {
            shapeIds[p] = visItVFV->AddPoint( pntIndx1, pntIndx2, p1Weight );
            }
          */
          
          // Turning on the above code segment, the alternative, would cause
          // a bug with a synthetic Wavelet dataset (vtkImageData) when the
          // the clipping plane (x/y/z axis) is positioned exactly at (0,0,0).
          // The problem occurs in the form of an open 'box', as opposed to an
          // expected closed one. This is due to the use of hash instead of a
          // point-locator based detection of duplicate points.
          shapeIds[p] = visItVFV->AddPoint( pntIndx1, pntIndx2, p1Weight );
          }
        else 
        if ( pntIndex >= N0 && pntIndex <= N3 )
          {
          shapeIds[p] = intrpIds[ pntIndex - N0 ];
          }
        else
          {
          vtkErrorMacro( << "An invalid output point value "
                         << "was found in the ClipCases." << endl );
          }
        }

      switch ( theShape )
        {
        case ST_HEX:
          visItVFV->AddHex( i, shapeIds[0], shapeIds[1], 
                               shapeIds[2], shapeIds[3], shapeIds[4], 
                               shapeIds[5], shapeIds[6], shapeIds[7] );
          break;
          
        case ST_WDG:
          visItVFV->AddWedge( i, shapeIds[0], shapeIds[1], shapeIds[2],
                                 shapeIds[3], shapeIds[4], shapeIds[5] );
          break;
          
        case ST_PYR:
          visItVFV->AddPyramid( i, shapeIds[0], shapeIds[1],
                                   shapeIds[2], shapeIds[3], shapeIds[4] );
          break;
          
        case ST_TET:
          visItVFV->AddTet( i, shapeIds[0], shapeIds[1], 
                               shapeIds[2], shapeIds[3] );
          break;
          
        case ST_QUA:
          visItVFV->AddQuad( i, shapeIds[0], shapeIds[1], 
                                shapeIds[2], shapeIds[3] );
          break;
          
        case ST_TRI:
          visItVFV->AddTri( i, shapeIds[0], shapeIds[1], shapeIds[2] );
          break;
          
        case ST_LIN:
          visItVFV->AddLine( i, shapeIds[0], shapeIds[1] );
          break;
          
        case ST_VTX:
          visItVFV->AddVertex( i, shapeIds[0] );
          break;
          
        case ST_PNT:
          intrpIds[ intrpIdx ] = visItVFV->AddCentroidPoint
                                           ( nCellPts, shapeIds );
          break;
        }
      }
      
    thisCase = NULL;
    }
  
  
  int            toDelete    = 0;
  double       * theCords[3] = { NULL, NULL, NULL };
  vtkDataArray * theArays[3] = { NULL, NULL, NULL };
  
  if ( rectGrid->GetXCoordinates()->GetDataType() == VTK_DOUBLE &&
       rectGrid->GetYCoordinates()->GetDataType() == VTK_DOUBLE &&
       rectGrid->GetZCoordinates()->GetDataType() == VTK_DOUBLE
     )
    {
    theCords[0] = static_cast < double * > 
                  (  rectGrid->GetXCoordinates()->GetVoidPointer( 0 )  );
    theCords[1] = static_cast < double * > 
                  (  rectGrid->GetYCoordinates()->GetVoidPointer( 0 )  );
    theCords[2] = static_cast < double * >
                  (  rectGrid->GetZCoordinates()->GetVoidPointer( 0 )  );
    }
  else
    {
    toDelete    = 1;
    theArays[0] = rectGrid->GetXCoordinates();
    theArays[1] = rectGrid->GetYCoordinates();
    theArays[2] = rectGrid->GetZCoordinates();
    for ( j = 0; j < 3; j ++ )
      {
      theCords[j] = new double [ rectDims[j] ];
      for ( i = 0; i < rectDims[j]; i ++ )
        {
        theCords[j][i] = theArays[j]->GetComponent( i, 0 );
        }
      theArays[j] = NULL;
      }
    }

  visItVFV->ConstructDataSet
            ( rectGrid->GetPointData(), rectGrid->GetCellData(), 
              outputUG, rectDims, theCords[0], theCords[1], theCords[2] );
              
  delete visItVFV;
  visItVFV = NULL;
  rectGrid = NULL;
  
  for ( i = 0; i < 3; i ++ )
    {
    if ( toDelete )
      {
      delete [] theCords[i];
      }
    theCords[i] = NULL;
    }
}

//-----------------------------------------------------------------------------
void vtkTableBasedClipDataSet::ClipStructuredGridData( vtkDataSet * inputGrd, 
     vtkDataArray * clipAray, double isoValue, vtkUnstructuredGrid * outputUG )
{
  vtkStructuredGrid * strcGrid = vtkStructuredGrid::SafeDownCast( inputGrd );
  
  int   i, j;
  int   isTwoDim    = 0;
  int   numCells    = 0;
  int   gridDims[3] = { 0, 0, 0 };
  strcGrid->GetDimensions( gridDims );
  isTwoDim = int( gridDims[2] <= 1 );
  numCells = strcGrid->GetNumberOfCells();

  vtkTableBasedClipperVolumeFromVolume  *  visItVFV = new
  vtkTableBasedClipperVolumeFromVolume(    strcGrid->GetNumberOfPoints(),
      int(   pow(  double( numCells ), double( 0.6667f )  )   ) * 5 + 100    );

  
  int   shiftLUT[3][8] = { 
                           { 0, 1, 1, 0, 0, 1, 1, 0 },
                           { 0, 0, 1, 1, 0, 0, 1, 1 },
                           { 0, 0, 0, 0, 1, 1, 1, 1 }
                         };
  int   numbPnts    = 0;
  int   cellDims[3] = { gridDims[0] - 1, gridDims[1] - 1, gridDims[2] - 1 };
  int   cyStride    = cellDims[0];
  int   czStride    = cellDims[0] * cellDims[1];
  int   pyStride    = gridDims[0];
  int   pzStride    = gridDims[0] * gridDims[1];
  
  for ( i = 0; i < numCells; i ++ )
    {
    int    caseIndx = 0;
    int    theCellI = i % cellDims[0];
    int    theCellJ = ( i / cyStride ) % cellDims[1];
    int    theCellK = ( i / czStride );
    double grdDiffs[8];
       
    numbPnts = isTwoDim ? 4 : 8;
    
    for ( j = numbPnts - 1; j >= 0; j -- )
      {
      int pntIndex = ( theCellI + shiftLUT[0][j] ) + 
                     ( theCellJ + shiftLUT[1][j] ) * pyStride +
                     ( theCellK + shiftLUT[2][j] ) * pzStride;
                 
      grdDiffs[j]  = clipAray->GetComponent( pntIndex, 0 ) - isoValue;
      caseIndx    += (  ( grdDiffs[j] >= 0.0 ) ? 1 : 0  );
      caseIndx   <<= (  1 - ( !j )  );
      }

    int             nOutputs;
    int             intrpIds[4];
    unsigned char * thisCase = NULL;
    
    if ( isTwoDim )
      {
      thisCase = &vtkTableBasedClipperClipTables::ClipShapesQua
               [  vtkTableBasedClipperClipTables::StartClipShapesQua[ caseIndx ]  ];
      nOutputs = vtkTableBasedClipperClipTables::NumClipShapesQua[ caseIndx ];
      }
    else
      {
      thisCase = &vtkTableBasedClipperClipTables::ClipShapesHex
               [  vtkTableBasedClipperClipTables::StartClipShapesHex[ caseIndx ]  ];
      nOutputs = vtkTableBasedClipperClipTables::NumClipShapesHex[ caseIndx ];
      }

    for ( j = 0; j < nOutputs; j ++ )
      {
      int      nCellPts = 0;
      int      intrpIdx = -1;
      int      theColor = -1;
      unsigned char theShape = *thisCase ++;
      
      switch ( theShape )
        {
        case ST_HEX:
          nCellPts = 8;
          theColor = *thisCase ++;
          break;
          
        case ST_WDG:
          nCellPts = 6;
          theColor = *thisCase ++;
          break;
          
        case ST_PYR:
          nCellPts = 5;
          theColor = *thisCase ++;
          break;
          
        case ST_TET:
          nCellPts = 4;
          theColor = *thisCase ++;
          break;
          
        case ST_QUA:
          nCellPts = 4;
          theColor = *thisCase ++;
          break;
          
        case ST_TRI:
          nCellPts = 3;
          theColor = *thisCase ++;
          break;
          
        case ST_LIN:
          nCellPts = 2;
          theColor = *thisCase ++;
          break;
          
        case ST_VTX:
          nCellPts = 1;
          theColor = *thisCase ++;
          break;
          
        case ST_PNT:
          intrpIdx = *thisCase ++;
          theColor = *thisCase ++;
          nCellPts = *thisCase ++;
          break;
          
        default:
          vtkErrorMacro( << "An invalid output shape was found in "
                         << "the ClipCases." << endl );
        }

      if ( (!this->InsideOut && theColor == COLOR0 ) ||
           ( this->InsideOut && theColor == COLOR1 )
         )
        {
        // We don't want this one; it's the wrong side.
        thisCase += nCellPts;
        continue;
        }

      int   shapeIds[8];
      for ( int p = 0; p < nCellPts; p ++ )
        {
        unsigned char pntIndex = *thisCase ++;
        
        if ( pntIndex <= P7 )
          {
          // We know pt P0 must be >P0 since we already
          // assume P0 == 0.  This is why we do not
          // bother subtracting P0 from pt here.
          shapeIds[p] = 
                      (   (  theCellI + shiftLUT[0][ pntIndex ]  ) +
                          (  theCellJ + shiftLUT[1][ pntIndex ]  ) * pyStride +
                          (  theCellK + shiftLUT[2][ pntIndex ]  ) * pzStride
                      );
          }
        else 
        if ( pntIndex >= EA && pntIndex <= EL )
          {
          int  pt1Index = vtkTableBasedClipperTriangulationTables::
                          HexVerticesFromEdges[ pntIndex - EA ][0];
          int  pt2Index = vtkTableBasedClipperTriangulationTables::
                          HexVerticesFromEdges[ pntIndex - EA ][1];
          
          if ( pt2Index < pt1Index )
            {
            int temp = pt2Index;
            pt2Index = pt1Index;
            pt1Index = temp;
            }
          
          double pt1ToPt2 = grdDiffs[ pt2Index] - grdDiffs[ pt1Index ];
          double pt1ToIso = 0.0 - grdDiffs[ pt1Index ];
          double p1Weight = 1.0 - pt1ToIso / pt1ToPt2;
                                  
          int    pntIndx1 = 
                 (   (  theCellI + shiftLUT[0][ pt1Index ] ) +
                     (  theCellJ + shiftLUT[1][ pt1Index ]  ) * pyStride +
                     (  theCellK + shiftLUT[2][ pt1Index ]  ) * pzStride
                 );
          int    pntIndx2 = 
                 (   (  theCellI + shiftLUT[0][ pt2Index ]  ) +
                     (  theCellJ + shiftLUT[1][ pt2Index ]  ) * pyStride +
                     (  theCellK + shiftLUT[2][ pt2Index ]  ) * pzStride
                 );
                  
          shapeIds[p] = visItVFV->AddPoint( pntIndx1, pntIndx2, p1Weight );
          }
        else 
        if ( pntIndex >= N0 && pntIndex <= N3 )
          {
          shapeIds[p] = intrpIds[ pntIndex - N0 ];
          }
        else
          {
          vtkErrorMacro( << "An invalid output point value "
                         << "was found in the ClipCases." << endl );
          }
        }

      switch ( theShape )
        {
        case ST_HEX:
          visItVFV->AddHex( i, shapeIds[0], shapeIds[1], 
                               shapeIds[2], shapeIds[3], shapeIds[4], 
                               shapeIds[5], shapeIds[6], shapeIds[7] );
          break;
          
        case ST_WDG:
          visItVFV->AddWedge( i, shapeIds[0], shapeIds[1], shapeIds[2],
                                 shapeIds[3], shapeIds[4], shapeIds[5] );
          break;
          
        case ST_PYR:
          visItVFV->AddPyramid( i, shapeIds[0], shapeIds[1],
                                   shapeIds[2], shapeIds[3], shapeIds[4] );
          break;
          
        case ST_TET:
          visItVFV->AddTet( i, shapeIds[0], shapeIds[1], 
                               shapeIds[2], shapeIds[3] );
          break;
          
        case ST_QUA:
          visItVFV->AddQuad( i, shapeIds[0], shapeIds[1], 
                                shapeIds[2], shapeIds[3] );
          break;
          
        case ST_TRI:
          visItVFV->AddTri( i, shapeIds[0], shapeIds[1], shapeIds[2] );
          break;
          
        case ST_LIN:
          visItVFV->AddLine( i, shapeIds[0], shapeIds[1] );
          break;
          
        case ST_VTX:
          visItVFV->AddVertex( i, shapeIds[0] );
          break;
          
        case ST_PNT:
          intrpIds[ intrpIdx ] = visItVFV->AddCentroidPoint
                                           ( nCellPts, shapeIds );
          break;
        }
      }
      
    thisCase = NULL;
    }
  
  int         toDelete = 0;
  double    * theCords = NULL;
  vtkPoints * inputPts = strcGrid->GetPoints();
  if ( inputPts->GetDataType() == VTK_DOUBLE )
    {
    theCords = static_cast < double * > (  inputPts->GetVoidPointer( 0 )  );
    }
  else
    {
    toDelete = 1;
    numbPnts = inputPts->GetNumberOfPoints();
    theCords = new double [ numbPnts * 3 ];
    for ( i = 0; i < numbPnts; i ++ )
      {
      inputPts->GetPoint( i, theCords + ( i << 1 ) + i );
      }
    }
  inputPts = NULL;
  
  visItVFV->ConstructDataSet( strcGrid->GetPointData(), 
                              strcGrid->GetCellData(), outputUG, theCords );
  
  
  delete visItVFV;
  if ( toDelete )
    {
    delete [] theCords;
    }
  visItVFV = NULL;
  theCords = NULL;
  strcGrid = NULL;
}

//-----------------------------------------------------------------------------
void vtkTableBasedClipDataSet::ClipUnstructuredGridData( vtkDataSet * inputGrd, 
     vtkDataArray * clipAray, double isoValue, vtkUnstructuredGrid * outputUG )
{ 
  vtkUnstructuredGrid * unstruct = vtkUnstructuredGrid::SafeDownCast( inputGrd );
  
  vtkIdType   i, j;
  vtkIdType   numbPnts = 0;
  int         numCants = 0; // number of cells not clipped by this filter
  int         numCells = unstruct->GetNumberOfCells();
  
  // volume from volume
  vtkTableBasedClipperVolumeFromVolume   * visItVFV = new
  vtkTableBasedClipperVolumeFromVolume(    unstruct->GetNumberOfPoints(), 
      int(   pow(  double( numCells ), double( 0.6667f )  )   ) * 5 + 100    );

  // the stuffs that can not be clipped by this filter
  vtkUnstructuredGrid * specials = vtkUnstructuredGrid::New();
  specials->SetPoints( unstruct->GetPoints() );
  specials->GetPointData()->ShallowCopy( unstruct->GetPointData() );
  specials->Allocate( numCells );

  for ( i = 0; i < numCells; i ++ )
    {
    int         cellType = unstruct->GetCellType( i );
    vtkIdType * pntIndxs = NULL;
    unstruct->GetCellPoints( i, numbPnts, pntIndxs );
    
    bool     bCanClip = false;
    switch ( cellType )
      {
      case VTK_TETRA:
      case VTK_PYRAMID:
      case VTK_WEDGE:
      case VTK_HEXAHEDRON:
      case VTK_VOXEL:
      case VTK_TRIANGLE:
      case VTK_QUAD:
      case VTK_PIXEL:
      case VTK_LINE:
      case VTK_VERTEX:
           bCanClip = true;
           break;
                        
      default:
           bCanClip = false;
           break;
      }
 
    if ( bCanClip )
      {
      int    caseIndx = 0;
      double grdDiffs[8];
      
      for ( j = numbPnts-1; j >= 0; j -- )
        {                 
        grdDiffs[j] = clipAray->GetComponent( pntIndxs[j], 0 ) - isoValue;
        caseIndx   += (  ( grdDiffs[j] >= 0.0 ) ? 1 : 0  );
        caseIndx  <<= (  1 - ( !j )  );
        }

      int               startIdx = 0;
      int               nOutputs = 0;
      typedef const int EDGEIDXS[2];
      EDGEIDXS        * edgeVtxs = NULL;
      unsigned char   * thisCase = NULL;

      // start index, split case, number of output, and vertices from edges
      switch ( cellType )
        {
        case VTK_TETRA:
          startIdx = vtkTableBasedClipperClipTables::StartClipShapesTet[ caseIndx ];
          thisCase =&vtkTableBasedClipperClipTables::ClipShapesTet[ startIdx ];
          nOutputs = vtkTableBasedClipperClipTables::NumClipShapesTet[ caseIndx ];
          edgeVtxs = ( EDGEIDXS * ) 
                     vtkTableBasedClipperTriangulationTables::TetVerticesFromEdges;
          break;
          
        case VTK_PYRAMID:
          startIdx = vtkTableBasedClipperClipTables::StartClipShapesPyr[ caseIndx ];
          thisCase =&vtkTableBasedClipperClipTables::ClipShapesPyr[ startIdx ];
          nOutputs = vtkTableBasedClipperClipTables::NumClipShapesPyr[ caseIndx ];
          edgeVtxs = ( EDGEIDXS * ) 
                     vtkTableBasedClipperTriangulationTables::PyramidVerticesFromEdges;
          break;
          
        case VTK_WEDGE:
          startIdx = vtkTableBasedClipperClipTables::StartClipShapesWdg[ caseIndx ];
          thisCase =&vtkTableBasedClipperClipTables::ClipShapesWdg[ startIdx ];
          nOutputs = vtkTableBasedClipperClipTables::NumClipShapesWdg[ caseIndx ];
          edgeVtxs = ( EDGEIDXS * ) 
                     vtkTableBasedClipperTriangulationTables::WedgeVerticesFromEdges;
          break;
          
        case VTK_HEXAHEDRON:
          startIdx = vtkTableBasedClipperClipTables::StartClipShapesHex[ caseIndx ];
          thisCase =&vtkTableBasedClipperClipTables::ClipShapesHex[ startIdx ];
          nOutputs = vtkTableBasedClipperClipTables::NumClipShapesHex[ caseIndx ];
          edgeVtxs = ( EDGEIDXS * ) 
                     vtkTableBasedClipperTriangulationTables::HexVerticesFromEdges;
          break;
          
        case VTK_VOXEL:
          startIdx = vtkTableBasedClipperClipTables::StartClipShapesVox[ caseIndx ];
          thisCase =&vtkTableBasedClipperClipTables::ClipShapesVox[ startIdx ];
          nOutputs = vtkTableBasedClipperClipTables::NumClipShapesVox[ caseIndx ];
          edgeVtxs = ( EDGEIDXS * ) 
                     vtkTableBasedClipperTriangulationTables::VoxVerticesFromEdges;
          break;
          
        case VTK_TRIANGLE:
          startIdx = vtkTableBasedClipperClipTables::StartClipShapesTri[ caseIndx ];
          thisCase =&vtkTableBasedClipperClipTables::ClipShapesTri[ startIdx ];
          nOutputs = vtkTableBasedClipperClipTables::NumClipShapesTri[ caseIndx ];
          edgeVtxs = ( EDGEIDXS * ) 
                     vtkTableBasedClipperTriangulationTables::TriVerticesFromEdges;
          break;
          
        case VTK_QUAD:
          startIdx = vtkTableBasedClipperClipTables::StartClipShapesQua[ caseIndx ];
          thisCase =&vtkTableBasedClipperClipTables::ClipShapesQua[ startIdx ];
          nOutputs = vtkTableBasedClipperClipTables::NumClipShapesQua[ caseIndx ];
          edgeVtxs = ( EDGEIDXS * ) 
                     vtkTableBasedClipperTriangulationTables::QuadVerticesFromEdges;
          break;
          
        case VTK_PIXEL:
          startIdx = vtkTableBasedClipperClipTables::StartClipShapesPix[ caseIndx ];
          thisCase =&vtkTableBasedClipperClipTables::ClipShapesPix[ startIdx ];
          nOutputs = vtkTableBasedClipperClipTables::NumClipShapesPix[ caseIndx ];
          edgeVtxs = ( EDGEIDXS * ) 
                     vtkTableBasedClipperTriangulationTables::PixelVerticesFromEdges;
          break;
          
        case VTK_LINE:
          startIdx = vtkTableBasedClipperClipTables::StartClipShapesLin[ caseIndx ];
          thisCase =&vtkTableBasedClipperClipTables::ClipShapesLin[ startIdx ];
          nOutputs = vtkTableBasedClipperClipTables::NumClipShapesLin[ caseIndx ];
          edgeVtxs = ( EDGEIDXS * ) 
                     vtkTableBasedClipperTriangulationTables::LineVerticesFromEdges;
          break;
          
        case VTK_VERTEX:
          startIdx = vtkTableBasedClipperClipTables::StartClipShapesVtx[ caseIndx ];
          thisCase =&vtkTableBasedClipperClipTables::ClipShapesVtx[ startIdx ];
          nOutputs = vtkTableBasedClipperClipTables::NumClipShapesVtx[ caseIndx ];
          edgeVtxs = NULL;
          break;
        }
      
      int   intrpIds[4];
      for ( j = 0; j < nOutputs; j ++ )
        {   
        int      nCellPts = 0;
        int      theColor = -1;
        int      intrpIdx = -1;
        unsigned char theShape = *thisCase ++;
        
        // number of points and color
        switch ( theShape )
          {
          case ST_HEX:
            nCellPts = 8;
            theColor = *thisCase ++;
            break;
            
          case ST_WDG:
            nCellPts = 6;
            theColor = *thisCase ++;
            break;
            
          case ST_PYR:
            nCellPts = 5;
            theColor = *thisCase ++;
            break;
            
          case ST_TET:
            nCellPts = 4;
            theColor = *thisCase ++;
            break;
            
          case ST_QUA:
            nCellPts = 4;
            theColor = *thisCase ++;
            break;
            
          case ST_TRI:
            nCellPts = 3;
            theColor = *thisCase ++;
            break;
            
          case ST_LIN:
            nCellPts = 2;
            theColor = *thisCase ++;
            break;
            
          case ST_VTX:
            nCellPts = 1;
            theColor = *thisCase ++;
            break;
            
          case ST_PNT:
            intrpIdx = *thisCase ++;
            theColor = *thisCase ++;
            nCellPts = *thisCase ++;
            break;
            
          default:
            vtkErrorMacro( << "An invalid output shape was found "
                           << "in the ClipCases." << endl );
          }
        
        if ( (!this->InsideOut && theColor == COLOR0 ) ||
             ( this->InsideOut && theColor == COLOR1 )
           )
          {
          // We don't want this one; it's the wrong side.
          thisCase += nCellPts;
          continue; 
          }
        
        int   shapeIds[8];
        for ( int p = 0; p < nCellPts; p ++ )
          {
          unsigned char pntIndex = *thisCase ++;
          
          if ( pntIndex <= P7 )
            {
            // We know pt P0 must be >P0 since we already
            // assume P0 == 0.  This is why we do not
            // bother subtracting P0 from pt here.
            shapeIds[p] = pntIndxs[ pntIndex ];
            }
          else 
          if ( pntIndex >= EA && pntIndex <= EL )
            {
            int  pt1Index = edgeVtxs[ pntIndex-EA ][0];
            int  pt2Index = edgeVtxs[ pntIndex-EA ][1];
            if ( pt2Index < pt1Index )
              {
              int temp = pt2Index;
              pt2Index = pt1Index;
              pt1Index = temp;
              }
            double pt1ToPt2 = grdDiffs[ pt2Index ] - grdDiffs[ pt1Index ];
            double pt1ToIso = 0.0 - grdDiffs[ pt1Index ];
            double p1Weight = 1.0 - pt1ToIso / pt1ToPt2;

            int    pntIndx1 = pntIndxs[ pt1Index ];
            int    pntIndx2 = pntIndxs[ pt2Index ];
            
            shapeIds[p] = visItVFV->AddPoint( pntIndx1, pntIndx2, p1Weight );
            }
          else 
          if ( pntIndex >= N0 && pntIndex <= N3 )
            {
            shapeIds[p] = intrpIds[ pntIndex - N0 ];
            }
          else
            {
            vtkErrorMacro( << "An invalid output point value was found "
                           << "in the ClipCases." << endl );
            }
          }
        
        switch ( theShape )
          {
          case ST_HEX:
            visItVFV->AddHex( i, shapeIds[0], shapeIds[1], 
                                 shapeIds[2], shapeIds[3], shapeIds[4], 
                                 shapeIds[5], shapeIds[6], shapeIds[7] );
            break;
            
          case ST_WDG:
            visItVFV->AddWedge( i, shapeIds[0], shapeIds[1], shapeIds[2],
                                   shapeIds[3], shapeIds[4], shapeIds[5] );
            break;
            
          case ST_PYR:
            visItVFV->AddPyramid( i, shapeIds[0], shapeIds[1],
                                     shapeIds[2], shapeIds[3], shapeIds[4] );
            break;
            
          case ST_TET:
            visItVFV->AddTet( i, shapeIds[0], shapeIds[1], 
                                 shapeIds[2], shapeIds[3] );
            break;
            
          case ST_QUA:
            visItVFV->AddQuad( i, shapeIds[0], shapeIds[1], 
                                  shapeIds[2], shapeIds[3] );
            break;
            
          case ST_TRI:
            visItVFV->AddTri( i, shapeIds[0], shapeIds[1], shapeIds[2] );
            break;
            
          case ST_LIN:
            visItVFV->AddLine( i, shapeIds[0], shapeIds[1] ); 
            break;
            
          case ST_VTX:
            visItVFV->AddVertex( i, shapeIds[0] );
            break;
            
          case ST_PNT:
            intrpIds[ intrpIdx ] = visItVFV->AddCentroidPoint
                                             ( nCellPts, shapeIds );
            break;
          }
        }
        
      edgeVtxs = NULL;
      thisCase = NULL;
      }
    else if (cellType == VTK_POLYHEDRON)
      {
      if ( numCants == 0 )
        {
          specials->GetCellData()
                  ->CopyAllocate( unstruct->GetCellData(), numCells );
        }
      vtkIdType nfaces, *facePtIds;
      unstruct->GetFaceStream(i, nfaces, facePtIds);
      specials->InsertNextCell(cellType, nfaces, facePtIds);
      specials->GetCellData()
              ->CopyData( unstruct->GetCellData(), i, numCants );
      numCants ++;
      }
    else
      {
      if ( numCants == 0 )
        {
          specials->GetCellData()
                  ->CopyAllocate( unstruct->GetCellData(), numCells );
        }
      specials->InsertNextCell( cellType, numbPnts, pntIndxs );
      specials->GetCellData()
              ->CopyData( unstruct->GetCellData(), i, numCants );
      numCants ++;
      }
      
    pntIndxs = NULL;
    }
  
  int         toDelete = 0;
  double    * theCords = NULL;
  vtkPoints * inputPts = unstruct->GetPoints();
  if ( inputPts->GetDataType() == VTK_DOUBLE )
    {
    theCords = static_cast < double * > (  inputPts->GetVoidPointer( 0 )  );
    }
  else
    {
    toDelete = 1;
    numbPnts = inputPts->GetNumberOfPoints();
    theCords = new double [ numbPnts * 3 ];
    for ( i = 0; i < numbPnts; i ++ )
      {
      inputPts->GetPoint( i, theCords + ( i << 1 ) + i );
      }
    }
  inputPts = NULL;
  
  
  // the stuffs that can not be clipped
  if ( numCants > 0 )
    {
    vtkUnstructuredGrid * vtkUGrid  = vtkUnstructuredGrid::New();
    this->ClipDataSet( specials, clipAray, vtkUGrid );
    
    vtkUnstructuredGrid * visItGrd = vtkUnstructuredGrid::New();
    visItVFV->ConstructDataSet( unstruct->GetPointData(), 
                                unstruct->GetCellData(), visItGrd, theCords );

    vtkAppendFilter * appender = vtkAppendFilter::New();
    appender->AddInput( vtkUGrid );
    appender->AddInput( visItGrd );
    appender->Update();

    outputUG->ShallowCopy( appender->GetOutput() );

    appender->Delete();
    visItGrd->Delete();
    vtkUGrid->Delete();
    appender = NULL;
    vtkUGrid = NULL;
    visItGrd = NULL;
    }
  else
    {
    visItVFV->ConstructDataSet( unstruct->GetPointData(), 
                                unstruct->GetCellData(), outputUG, theCords );
    }

  specials->Delete();
  delete visItVFV;
  if ( toDelete )
    {
    delete [] theCords;
    }
  specials = NULL;
  visItVFV = NULL;
  theCords = NULL;
  unstruct = NULL;
}

//-----------------------------------------------------------------------------
void vtkTableBasedClipDataSet::PrintSelf( ostream & os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );

  os << indent << "Merge Tolerance: " << this->MergeTolerance << "\n";
  if ( this->ClipFunction )
    {
    os << indent << "Clip Function: " << this->ClipFunction << "\n";
    }
  else
    {
    os << indent << "Clip Function: (none)\n";
    }
  os << indent << "InsideOut: " << (this->InsideOut ? "On\n" : "Off\n");
  os << indent << "Value: " << this->Value << "\n";
  if ( this->Locator )
    {
    os << indent << "Locator: " << this->Locator << "\n";
    }
  else
    {
    os << indent << "Locator: (none)\n";
    }

  os << indent << "Generate Clip Scalars: " 
     << (this->GenerateClipScalars ? "On\n" : "Off\n");

  os << indent << "Generate Clipped Output: " 
     << (this->GenerateClippedOutput ? "On\n" : "Off\n");

  os << indent << "UseValueAsOffset: " 
     << (this->UseValueAsOffset ? "On\n" : "Off\n");
}
