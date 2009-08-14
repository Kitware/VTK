/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestIncrementalOctreePointLocator.cxx
  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/


#include "vtkMath.h"
#include "vtkPoints.h"
#include "vtkIdList.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridReader.h"
#include "vtkIncrementalOctreePointLocator.h"


#define  _BRUTE_FORCE_VERIFICATION_


// The following epsilon value is needed to address numerical inaccuracy /
// precision issues on some platforms: Fast-Release-g++, Win32-cygwin344,
// Win32-minGW, Win32-FreeVC++, and Win32-VS71. In particular, two of the
// references of this value below, as marked, handle Win32-cygwin344 and
// Win32-minGW. The numerical inaccuracy problem has nothing to do with 
// the incremental octree point locator or the associated incremental octree 
// node itself. Instead it is just due to the multiple sub-tests themselves
// (combined in this single file) in which the brute-force mode employs many
// if-statements / comparisons that involve double values.
//
// For example, vtkMath::Distance2BetweenPoints( point1, point2 ) may not be 
// directly used in if-statements (even for some platforms other than the above
// mentioned 5), even though the incremental octree point locator always uses
// double variables for computation and returning values. Another example is 
// that the min SQUARED distance D between point A (a given point) and point B
// (the closest point to A) may not be directly used to test the number of points
// within the SQUARED radius D relative to point A herein, though it is supposed
// to be ok (and the number is expected to be 1). The fact is that an epsilon 
// needs to be added to D for such a test. Otherwise the numerical inaccuracy 
// issue would just cause 0 to be returned --- no any point exists within the 
// SQUARED radius D relative to A. Please note that this problem is not caused by
// sqrt() at all because the incremental octree point locator offers an accurate
// function variant FindPointsWithinSquaredRadius() to avoid the obvious numerical
// inaccuracy related to sqrt().
// 
// Given the numerical inaccuracy issues on some platforms, the rapid verification
// mode might not be used. Fortunately, this test is fast enough.
#define  INC_OCT_PNT_LOC_TESTS_ZERO  0.00000000000001


// NOTE: ALL THE FOLLOWING FLAGS SHOULD BE OFF


//#ifdef   _BRUTE_FORCE_VERIFICATION_
//#define  _BRUTE_FORCE_VERIFICATION_WRITE_RESULT_
//#define  _BRUTE_FORCE_VERIFICATION_WRITE_DISTANCE2_ // do NOT turn this ON
//#endif


// ---------------------------------------------------------------------------
// Meta information of the test data
//
// number of grid points           = 2288
// number of unique points         = 2200 (in terms of zero tolerance)
// number of duplicate occurrences = 88
// bounding box: [ -2.839926, 2.862497 ]
//               [ -2.856848, 2.856848 ]
//               [  0.000000, 1.125546 ]
//
// min squared distance = 1.036624e-005 (for zero-tolerance unique points)  
// max squared distance = 3.391319e+001


//----------------------------------------------------------------------------
// Swap an array of (or a single) int, double, or vtkIdType values when
// loading little-endian / Win-based disk files on big-endian platforms.
// Note the two data files for this test are in little-endian binary format.
void SwapForBigEndian( unsigned char * theArray, int tupleSiz, int numTuple )
{
  int      i, j;
  unsigned char * tmpChar0 = theArray;
  unsigned char * tmpChar1 = ( unsigned char * ) malloc( tupleSiz );
  
  for ( j = 0; j < numTuple; j ++, tmpChar0 += tupleSiz )
    {
    for ( i = 0; i < tupleSiz; i ++ )
      {
      tmpChar1[i] = tmpChar0[ tupleSiz - 1 - i ];
      }
    
    for ( i = 0; i < tupleSiz; i ++ )
      {
      tmpChar0[i] = tmpChar1[i];
      }
    }
    
  tmpChar0 = NULL;
  free( tmpChar1 );  tmpChar1 = NULL;
}


//----------------------------------------------------------------------------
int TestIncrementalOctreePointLocator( int argc, char * argv[] )
{
  // specifications
  int         nClzNpts    = 4;   
  int         octreRes[3] = { 64, 128, 256 };
  double      tolerans[2] = { 0.0, 0.01 };
  
  
  // variables
  int         r, t, m, i, j, k;
  int         retValue = 0;
  int         numbPnts = 0;
  int         inserted = 0;
  int         numInsrt = 0;
  int         nLocPnts = 0;
  int         nUniques = 0;
  int         nDuplics = 0;
  int         arrayIdx = 0;
  int         numExPts = 0;
  char *      fileName = NULL;
  FILE *      diskFile = NULL;
  FILE *      pntsFile = NULL;
  double      pntCoord[3] = { 0.0, 0.0, 0.0 };
  double      tmpDist2 = 0.0;
  double      fTempRad = 0.0;
  double *    pDataPts = NULL;
  double *    xtentPts = NULL;
  double *    pLocPnts = NULL;
  double *    minDist2 = NULL;
  double *    maxDist2 = NULL;
  double *    clzNdst2 = NULL;
  vtkPoints * dataPnts = NULL;
  vtkPoints * insrtPts = NULL;
  vtkIdType   pointIdx;
  vtkIdType * truthIds = NULL;
  vtkIdType * resltIds = NULL;
  vtkIdList * ptIdList = NULL;
  vtkIdList * idxLists[3] = { NULL, NULL, NULL };
  vtkUnstructuredGrid *              unstruct = NULL;
  vtkUnstructuredGridReader *        ugReader = NULL;
  vtkIncrementalOctreePointLocator * octLocat = NULL;
  
  
  // load an unstructured grid dataset
  fileName = vtkTestUtilities::ExpandDataFileName
                               ( argc, argv, "Data/post.vtk" );
  ugReader = vtkUnstructuredGridReader::New();
  ugReader->SetFileName( fileName );
  delete []  fileName;  fileName = NULL;
  ugReader->Update();
  unstruct = ugReader->GetOutput();
  dataPnts = unstruct->GetPoints();
  numbPnts = dataPnts->GetNumberOfPoints();
  
  
  // open a file for reading or writing the ground truth data
  fileName = vtkTestUtilities::ExpandDataFileName
             ( argc, argv, "Data/IncOctPntLocResult.dat" );
  #ifdef  _BRUTE_FORCE_VERIFICATION_WRITE_RESULT_
  diskFile = fopen( fileName, "wb" );
  #else
  #ifndef _BRUTE_FORCE_VERIFICATION_
  diskFile = fopen( fileName, "rb" );
  truthIds = ( vtkIdType * ) 
             realloc(  truthIds,  sizeof( vtkIdType ) * numbPnts  );
  #endif
  #endif
  delete []  fileName;  fileName = NULL;
  
  
  // obtain the 3D point coordinates
  pDataPts = ( double * ) 
             realloc(  pDataPts,  sizeof( double ) * 3 * numbPnts  );
  for ( i = 0; i < numbPnts; i ++ )
    {
    dataPnts->GetPoint(  i,  pDataPts + ( i << 1 ) + i  );
    }
  
    
  // allocate memory for exactly duplicate points and inherit some points
  nUniques = 4;
  nDuplics = 300;
  arrayIdx = numbPnts * 3;
  numExPts = numbPnts + nUniques * nDuplics;
  xtentPts = ( double * )
             realloc(  xtentPts, sizeof( double ) * 3 * numExPts  );                                 
  memcpy(  xtentPts,  pDataPts,  sizeof( double ) * 3 * numbPnts  );
  
  // add an additional number of exactly duplicate points
  for ( j = 0; j < nUniques; j ++ )
    {
    i = (  numbPnts / ( nUniques + 2 )  )  *  ( j + 1 );
    i = ( i << 1 ) + i;
    pntCoord[0] = pDataPts[ i     ];
    pntCoord[1] = pDataPts[ i + 1 ];
    pntCoord[2] = pDataPts[ i + 2 ];
    for ( i = 0; i < nDuplics; i ++, arrayIdx += 3 )
      {
      xtentPts[ arrayIdx     ] = pntCoord[0];
      xtentPts[ arrayIdx + 1 ] = pntCoord[1];
      xtentPts[ arrayIdx + 2 ] = pntCoord[2];
      }
    }
    
    
  // memory allocation
  ptIdList = vtkIdList::New();
  ptIdList->Allocate( numbPnts, numbPnts >> 1 );
  insrtPts = vtkPoints::New();
  insrtPts->Allocate( numbPnts, numbPnts >> 1 );
  octLocat = vtkIncrementalOctreePointLocator::New();
  
  
  // =========================================================================
  // ============================ Point Insertion ============================
  // =========================================================================
  for (  r = 0;  ( r < 3 ) && ( retValue == 0 );  r ++  ) // three resolutions
    {
    
    // -----------------------------------------------------------------------
    // --------------------- check-based point insertion ---------------------
    // -----------------------------------------------------------------------
    for (  t = 0;  ( t < 2 ) && ( retValue == 0 );  t ++  ) // two  tolerances
    for (  m = 0;  ( m < 3 ) && ( retValue == 0 );  m ++  ) // three functions
      {
      
      ptIdList->Reset();  // indices of the inserted points: based on dataPnts
      insrtPts->Reset();
      octLocat->FreeSearchStructure();
      octLocat->SetMaxPointsPerLeaf( octreRes[r] );
      octLocat->SetTolerance( tolerans[t] );
      octLocat->InitPointInsertion
                ( insrtPts, dataPnts->GetBounds(), numbPnts );
      
      // ---------------------------------------------------------------------  
      if ( m == 0 )   // vtkIncrementalOctreePointLocator::InsertUniquePoint()
        {
        for ( i = 0; i < numbPnts; i ++ )
          {
          inserted = octLocat->InsertUniquePoint
                               (  pDataPts + ( i << 1 ) + i,  pointIdx  );
          if ( inserted ) ptIdList->InsertNextId( i );
          } 
        }
      else
      if ( m == 1 )   // vtkIncrementalOctreePointLocator::InsertNextPoint()
        {
        for ( i = 0; i < numbPnts; i ++ )
          {
          inserted = octLocat->IsInsertedPoint(  pDataPts + ( i << 1 ) + i  );
          if ( inserted == -1 )
            {
            octLocat->InsertNextPoint(  pDataPts + ( i << 1 ) + i  );
            ptIdList->InsertNextId( i );
            }
          }
        }
      else            // vtkIncrementalOctreePointLocator::InsertPoint()
        {
        numInsrt = 0;
        for ( i = 0; i < numbPnts; i ++ )
          {
          inserted = octLocat->IsInsertedPoint(  pDataPts + ( i << 1 ) + i  );
          if ( inserted == -1 )
            {
            octLocat->InsertPoint(  numInsrt ++,  pDataPts + ( i << 1 ) + i  );
            ptIdList->InsertNextId( i );
            }
          }
        }
      // -------------------------------------------------------------------//
        
      #ifdef _BRUTE_FORCE_VERIFICATION_
      // ---------------------------------------------------------------------
      // verify the point insertion result in brute force mode
      
      double     tempPnt0[3];
      double     tempPnt1[3];
      double     tolerns2[2] = { tolerans[0] * tolerans[0],
                                 tolerans[1] * tolerans[1] 
                               };
      numInsrt = ptIdList->GetNumberOfIds();
      
      // check if the squared distance between any two inserted points is 
      // less than the threshold
      for (  j = 0;  ( j < numInsrt - 1 ) && ( retValue == 0 );  j ++  )
        {
        insrtPts->GetPoint( j, tempPnt0 );
        for (  i = j + 1;  ( i < numInsrt ) && ( retValue == 0 );  i ++  )
          {
          insrtPts->GetPoint( i, tempPnt1 );
          tmpDist2 = vtkMath::Distance2BetweenPoints( tempPnt0, tempPnt1 );
          if ( tmpDist2 <= tolerns2[t]) retValue = 1;
          }
        }
      
      // check if there is any point whose distance to ALL inserted points
      // is greater than the threshold
      for (  j = 0;  ( j < numbPnts ) && ( retValue == 0 );  j ++  )
        {
        if (  ptIdList->IsId( j )  !=  -1  )  continue;    // already inserted
        int   bGreater = 1;
        for ( i = 0; i < numInsrt; i ++ )
          {
          insrtPts->GetPoint( i, tempPnt1 );
          tmpDist2 = vtkMath::Distance2BetweenPoints
                              (  pDataPts + ( j << 1 ) + j,  tempPnt1  );
          if ( tmpDist2 <= tolerns2[t] )  bGreater = 0; // No 'break' here !!!
          }
        retValue = bGreater;
        }
        
      // -------------------------------------------------------------------//
      #else  
      // ---------------------------------------------------------------------
      // rapid point index-based verification
      fread(  &numInsrt,  sizeof( int ),  1,  diskFile  );
      #ifdef VTK_WORDS_BIGENDIAN
      SwapForBigEndian
        (  ( unsigned char * ) ( &numInsrt ),  sizeof( int ),  1  );
      #endif
       
      if (  numInsrt  ==  ptIdList->GetNumberOfIds()  )
        {
        int  samePtId = 1;
        fread(  truthIds,  sizeof( vtkIdType ),  numInsrt,  diskFile  );
        #ifdef VTK_WORDS_BIGENDIAN
        SwapForBigEndian
          (  ( unsigned char * ) truthIds,  sizeof( vtkIdType ),  numInsrt  );
        #endif
                           
        for (  i = 0;  ( i < numInsrt ) && ( samePtId == 1 );  i ++  )
          {
          samePtId = (  truthIds[i] == ptIdList->GetId( i )  )  ?  1  :  0;
          }
        retValue = 1 - samePtId;
        }
      else  retValue = 1;
      // -------------------------------------------------------------------//
      #endif
      
      
      // ---------------------------------------------------------------------
      // write the point indices as the ground truth
      #ifdef  _BRUTE_FORCE_VERIFICATION_WRITE_RESULT_
      if ( retValue == 0 )
        {
        numInsrt = ptIdList->GetNumberOfIds();
        fwrite( &numInsrt,  sizeof( int ),  1,  diskFile  );
        fwrite(  ptIdList->GetPointer( 0 ), 
                 sizeof( vtkIdType ), numInsrt, diskFile  );
        }
      #endif
      // -------------------------------------------------------------------//
      
      } // end of two tolerances and three functions
      
     
    // -----------------------------------------------------------------------
    // ------------------ direct check-free point insertion ------------------
    // -----------------------------------------------------------------------
    insrtPts->Reset();
    octLocat->FreeSearchStructure();
    octLocat->SetMaxPointsPerLeaf( octreRes[r] );
    octLocat->InitPointInsertion
              ( insrtPts, dataPnts->GetBounds(), numbPnts );
    for ( i = 0; i < numbPnts; i ++ )
      {
      octLocat->InsertPointWithoutChecking
                ( pDataPts + ( i << 1 ) + i, pointIdx, 1 );
      }
    retValue = ( insrtPts->GetNumberOfPoints() == numbPnts ) ? 0 : 1;
    
    } // end of three resolutions
  
  
  // =========================================================================
  // direct check-free insertion of  a huge number of EXACTLY DUPLICATE points
  //           (number > the maximum number of points per leaf node)
  // =========================================================================
  if ( retValue == 0 )
    {
    // perform direct / check-free point insertion          
    for (  r = 0;  ( r < 3 ) && ( retValue == 0 );  r ++ ) // three resolutions
      {
      insrtPts->Reset();
      octLocat->FreeSearchStructure();
      octLocat->SetMaxPointsPerLeaf( octreRes[r] );
      octLocat->InitPointInsertion
                ( insrtPts, dataPnts->GetBounds(), numExPts );
      for ( i = 0; i < numExPts; i ++ )
        {
        octLocat->InsertPointWithoutChecking
                  (  xtentPts + ( i << 1 ) + i,  pointIdx,  1  );
        }
    
      retValue = ( insrtPts->GetNumberOfPoints() == numExPts ) ? 0 : 1;
      }
    }
  if ( xtentPts ) free( xtentPts );  xtentPts = NULL;
  // =======================================================================//
  // =======================================================================// 
  
  
  // reclaim this vtkPoints as it will be never used again  
  if ( insrtPts ) insrtPts->Delete();  insrtPts = NULL;
   
  
  // =========================================================================
  // ============================ Point  Location ============================
  // =========================================================================
  
  
  // load points and radius data from a disk file for point location tasks
  fileName = vtkTestUtilities::ExpandDataFileName
                               ( argc, argv, "Data/IncOctPntLocData.dat" );
  pntsFile = fopen( fileName, "rb" );
  delete []  fileName;  fileName = NULL;
  fread(  &nLocPnts,  sizeof( int ),  1,  pntsFile  );
  #ifdef VTK_WORDS_BIGENDIAN
  SwapForBigEndian
    (  ( unsigned char * ) ( &nLocPnts ),  sizeof( int ),  1  );
  #endif
  pLocPnts = ( double * ) realloc( pLocPnts, sizeof( double ) * nLocPnts * 3 );
  minDist2 = ( double * ) realloc( minDist2, sizeof( double ) * nLocPnts     );
  maxDist2 = ( double * ) realloc( maxDist2, sizeof( double ) * nLocPnts     );
  fread( pLocPnts, sizeof( double ), nLocPnts * 3, pntsFile );
  //fread( minDist2, sizeof( double ), nLocPnts,     pntsFile );
  //fread( maxDist2, sizeof( double ), nLocPnts,     pntsFile );
  #ifdef VTK_WORDS_BIGENDIAN
  SwapForBigEndian
    (  ( unsigned char * ) pLocPnts,  sizeof( double ),  nLocPnts * 3  );
  //SwapForBigEndian
  //  (  ( unsigned char * ) minDist2,  sizeof( double ),  nLocPnts      );
  //SwapForBigEndian
  //  (  ( unsigned char * ) maxDist2,  sizeof( double ),  nLocPnts      );
  #endif
  fclose( pntsFile );                pntsFile = NULL;
  
  
  // destroy the context of point insertion while attaching the dataset
  octLocat->FreeSearchStructure();   
  octLocat->SetDataSet( unstruct );
  
  
  // memory allocation
  clzNdst2 = ( double * ) realloc(  clzNdst2,  sizeof( double ) * nClzNpts  );
  for ( i = 0; i < 3; i ++ )
    {
    idxLists[i] = vtkIdList::New();
    idxLists[i]->Allocate( nClzNpts, nClzNpts );
    }
  
  
  // the main component
  for (  r = 0;  ( r < 3 ) && ( retValue == 0 );  r ++  ) // three resolutions
    {
    
    // establish a new octree with the specified resolution
    octLocat->Modified();
    octLocat->SetMaxPointsPerLeaf( octreRes[r] );
    octLocat->BuildLocator();
    
    // memory allocation
    resltIds = ( vtkIdType * )
               realloc( resltIds, sizeof( vtkIdType ) * nLocPnts );
    #ifndef _BRUTE_FORCE_VERIFICATION_
    truthIds = ( vtkIdType * ) 
               realloc( truthIds, sizeof( vtkIdType ) * nLocPnts );
    #endif
    
    
    // -----------------------------------------------------------------------
    // -------------------- location of the closest point --------------------
    // -----------------------------------------------------------------------
    
    // find the closest point
    for ( i = 0; i < nLocPnts; i ++ )
      {
      resltIds[i] = octLocat->FindClosestPoint(  pLocPnts + ( i << 1 ) + i,
                                                 minDist2 + i  );
      }
      
    #ifdef _BRUTE_FORCE_VERIFICATION_
    // -----------------------------------------------------------------------
    // verify the result in brute force mode
    for (  j = 0;  ( j < nLocPnts ) && ( retValue == 0 );  j ++  )
    for (  i = 0;  ( i < numbPnts ) && ( retValue == 0 );  i ++  )
      {
      if ( i == resltIds[j] ) continue;     // just the selected closest point
      tmpDist2 = vtkMath::Distance2BetweenPoints
                 (  pLocPnts + ( j << 1 ) + j,  pDataPts + ( i << 1 ) + i  );
      if ( tmpDist2 + INC_OCT_PNT_LOC_TESTS_ZERO < minDist2[j] ) retValue = 1;
      }
    // ---------------------------------------------------------------------//
    #else
    // -----------------------------------------------------------------------
    // rapid point index-based verification
    fread( &nLocPnts,  sizeof( int       ),  1,         diskFile  );
    #ifdef VTK_WORDS_BIGENDIAN
    SwapForBigEndian
      (  ( unsigned char * ) ( &nLocPnts ),  sizeof( int ),  1  );
    #endif
      
    fread(  truthIds,  sizeof( vtkIdType ),  nLocPnts,  diskFile  );
    #ifdef VTK_WORDS_BIGENDIAN
    SwapForBigEndian
      (  ( unsigned char * ) truthIds,  sizeof( vtkIdType ),  nLocPnts  );
    #endif
    
    for (  i = 0;  ( i < nLocPnts ) && ( retValue == 0 );  i ++  )
      {
      retValue = ( resltIds[i] == truthIds[i] ) ? 0 : 1;
      }
    // ---------------------------------------------------------------------//
    #endif 
    
    // -----------------------------------------------------------------------
    // write the point indices as the ground truth
    #ifdef  _BRUTE_FORCE_VERIFICATION_WRITE_RESULT_
    if ( retValue == 0 )
      {
      fwrite( &nLocPnts,  sizeof( int       ),  1,         diskFile  );
      fwrite(  resltIds,  sizeof( vtkIdType ),  nLocPnts,  diskFile  );
      }
    #endif
    // ---------------------------------------------------------------------//
    
    
    if ( retValue == 1 ) continue;
    
    
    // -----------------------------------------------------------------------
    // ------------------ location of the closest N points -------------------
    // -----------------------------------------------------------------------
    
    // memory allocation
    ptIdList->SetNumberOfIds( nClzNpts * 10 );  // to claim part of the memory
    resltIds = ( vtkIdType * ) 
               realloc( resltIds, sizeof( vtkIdType ) * nClzNpts * nLocPnts );
    #ifndef _BRUTE_FORCE_VERIFICATION_
    truthIds = ( vtkIdType * ) 
               realloc( truthIds, sizeof( vtkIdType ) * nClzNpts * nLocPnts );
    fread( &numInsrt,  sizeof( int       ),  1,         diskFile  );
    #ifdef VTK_WORDS_BIGENDIAN
    SwapForBigEndian
      (  ( unsigned char * ) ( &numInsrt ),  sizeof( int ),  1  );
    #endif
      
    fread(  truthIds,  sizeof( vtkIdType ),  numInsrt,  diskFile  );
    #ifdef VTK_WORDS_BIGENDIAN
    SwapForBigEndian
      (  ( unsigned char * ) truthIds,  sizeof( vtkIdType ),  numInsrt  );
    #endif
    #endif
    
    // -----------------------------------------------------------------------
    // -----------------------------------------------------------------------
    // find the closest N points (with embedded brute-force verification)
    for ( i = 0; i < nLocPnts; i ++ )
      {
      ptIdList->Reset();
      octLocat->FindClosestNPoints
                (  nClzNpts,  pLocPnts + ( i << 1 ) + i,  ptIdList  );
      
      // ---------------------------------------------------------------------
      // verify the result in brute force mode
      #ifdef _BRUTE_FORCE_VERIFICATION_
      
      // check the order of the closest points
      for ( j = 0; j < nClzNpts; j ++ )
        {
        pointIdx    = ptIdList->GetId( j );
        clzNdst2[j] = vtkMath::Distance2BetweenPoints
                               (  pLocPnts + ( i        << 1 ) + i, 
                                  pDataPts + ( pointIdx << 1 ) + pointIdx  );
        }
      for (  j = 0;  ( j < nClzNpts - 1 ) && ( retValue == 0 );  j ++  )
        {
        retValue = ( clzNdst2[j + 1] >= clzNdst2[j] ) ? 0 : 1;
        }
      
      // write some data for locating the closest point
      // within a radius and the points within a radius 
      //#ifdef _BRUTE_FORCE_VERIFICATION_WRITE_DISTANCE2_
      minDist2[i] = clzNdst2[0];
      maxDist2[i] = clzNdst2[ nClzNpts - 1 ];
      //#endif
        
      // check if there are any ignored but closer points
      for (  j = 0;  ( j < numbPnts ) && ( retValue == 0 );  j ++  )
        {
        tmpDist2 = vtkMath::Distance2BetweenPoints
                   (  pLocPnts + ( i << 1 ) + i,  pDataPts + ( j << 1 ) + j  ); 
         if (    tmpDist2 + INC_OCT_PNT_LOC_TESTS_ZERO // for cygwin and minGW
                 < clzNdst2[ nClzNpts - 1 ]   // Not "<=" here as there
              && ptIdList->IsId( j ) == -1    // may be other points that were
            )    retValue = 1;        // rejected simply due to the limit of N
        }
        
      if ( retValue == 1 ) break;
   
      #endif
      // -------------------------------------------------------------------//
      
      // transfer the point indices for subsequent file writing purposes
      memcpy(  resltIds + i * nClzNpts,  ptIdList->GetPointer( 0 ),
               sizeof( vtkIdType ) * nClzNpts  );
      }
    // ---------------------------------------------------------------------//
    // ---------------------------------------------------------------------//
      
    // -----------------------------------------------------------------------
    // write the point indices as the ground truth
    #ifdef  _BRUTE_FORCE_VERIFICATION_WRITE_RESULT_
    if ( retValue == 0 )
      {
      numInsrt = nClzNpts * nLocPnts;
      fwrite( &numInsrt,  sizeof( int       ),  1,         diskFile  );
      fwrite(  resltIds,  sizeof( vtkIdType ),  numInsrt,  diskFile  );
      }
    #endif
    // ---------------------------------------------------------------------//
    
    // -----------------------------------------------------------------------
    // rapid point index-based verification
    #ifndef _BRUTE_FORCE_VERIFICATION_
    numInsrt = nClzNpts * nLocPnts;     // data has been read at the beginning
    for (  i = 0;  ( i < numInsrt ) && ( retValue == 0 );  i ++  )
      {
      retValue = ( resltIds[i] == truthIds[i] ) ? 0 : 1;
      }
    #endif
    // ---------------------------------------------------------------------//
    
    
    if ( retValue == 1 ) continue;
    
    
    // -----------------------------------------------------------------------
    // ------------ location of the closest point within a radius ------------
    // -----------------------------------------------------------------------
    
    // memory allocation
    resltIds = ( vtkIdType * )
               realloc(  resltIds,  sizeof( vtkIdType ) * nLocPnts * 3  );
               
    // find the closest point within three radii
    for ( i = 0; i < nLocPnts; i ++ )
      {
      j = ( i << 1 ) + i;
      pointIdx = octLocat->FindClosestPoint( pLocPnts + j, minDist2 + i );
            
      // there are some points falling on the in-octree points
      fTempRad = ( minDist2[i] <= INC_OCT_PNT_LOC_TESTS_ZERO )
                 ? 0.000001 : minDist2[i];
      fTempRad = sqrt( fTempRad ); // causes inaccuracy if minDist2 is nonzero
      
      resltIds[ j     ] = octLocat->FindClosestPointWithinRadius
                                    ( fTempRad * 0.5, pLocPnts + j, tmpDist2 );
      if ( minDist2[i] <= INC_OCT_PNT_LOC_TESTS_ZERO )    
      resltIds[ j + 1 ] = octLocat->FindClosestPointWithinRadius
                                    ( fTempRad * 1.0, pLocPnts + j, tmpDist2 );
      else  // for non-zero cases, use the original squared radius for accuracy
      resltIds[ j + 1 ] = octLocat->FindClosestPointWithinSquaredRadius
                                    ( minDist2[i],    pLocPnts + j, tmpDist2 );
      resltIds[ j + 2 ] = octLocat->FindClosestPointWithinRadius
                                    ( fTempRad * 1.5, pLocPnts + j, tmpDist2 );
      
      // -----------------------------------------------------------------------
      // verify the result in brute force mode  
      #ifdef _BRUTE_FORCE_VERIFICATION_
      if (    (    ( minDist2[i] >  INC_OCT_PNT_LOC_TESTS_ZERO )
                && ( resltIds[j] != -1 )
              )
           || (    ( minDist2[i] <= INC_OCT_PNT_LOC_TESTS_ZERO )
                && ( resltIds[j] != pointIdx )
              )
           || ( resltIds[ j + 1 ] != pointIdx )
           || ( resltIds[ j + 2 ] != pointIdx )
         ) {  retValue = 1;  break;  }
      // ---------------------------------------------------------------------//
      #endif
      }
      
    // -----------------------------------------------------------------------
    // write the point indices as the ground truth
    #ifdef  _BRUTE_FORCE_VERIFICATION_WRITE_RESULT_
    if ( retValue == 0 )
      {
      numInsrt = nLocPnts * 3;                       // as we test three radii
      fwrite( &numInsrt,  sizeof( int       ),  1,         diskFile  );
      fwrite(  resltIds,  sizeof( vtkIdType ),  numInsrt,  diskFile  );
      }
    #endif
    // ---------------------------------------------------------------------//
    
    // -----------------------------------------------------------------------
    // rapid point index-based verification 
    #ifndef _BRUTE_FORCE_VERIFICATION_
    fread( &numInsrt,  sizeof( int ),  1,  diskFile  );
    #ifdef VTK_WORDS_BIGENDIAN
    SwapForBigEndian
      (  ( unsigned char * ) ( &numInsrt ),  sizeof( int ),  1  );
    #endif 
   
    truthIds = ( vtkIdType * )
               realloc(  truthIds,  sizeof( vtkIdType ) * numInsrt  );
    fread(  truthIds,  sizeof( vtkIdType ),  numInsrt,  diskFile  );  
    #ifdef VTK_WORDS_BIGENDIAN
    SwapForBigEndian
      (  ( unsigned char * ) truthIds,  sizeof( vtkIdType ),  numInsrt  );
    #endif
    
    for (  i = 0;  ( i < numInsrt ) && ( retValue == 0 );  i ++  )
      {
      retValue = ( resltIds[i] == truthIds[i] ) ? 0 : 1;
      } 
    #endif
    // ---------------------------------------------------------------------//
    
    
    if ( retValue == 1 ) continue;
    
    
    // -----------------------------------------------------------------------
    // --------------- location of the points within a radius ----------------
    // ----------------------------------------------------------------------- 
    
    ptIdList->Reset();
    for ( i = 0; i < nLocPnts; i ++ )
      {
      // set the coordinate index of the point under check
      j = ( i << 1 ) + i;
      minDist2[i] += INC_OCT_PNT_LOC_TESTS_ZERO; // for cygwin and minGW
      
      // obtain the points within three radii (use squared radii for test
      // as sqrt can incur inaccuracy that complicates the test)
      idxLists[0]->Reset();
      idxLists[1]->Reset();
      idxLists[2]->Reset();
      if ( minDist2[i] <= INC_OCT_PNT_LOC_TESTS_ZERO )
        {
        // each ( maxDist2[i] * 0.3 ) has been guranteed to be > 
        // INC_OCT_PNT_LOC_TESTS_ZERO
        octLocat->FindPointsWithinSquaredRadius
                  ( maxDist2[i] * 0.3, pLocPnts + j, idxLists[0] );
        octLocat->FindPointsWithinSquaredRadius
                  ( maxDist2[i] * 0.6, pLocPnts + j, idxLists[1] );
        octLocat->FindPointsWithinSquaredRadius
                  ( maxDist2[i],       pLocPnts + j, idxLists[2] );
        }
      else
        {
        octLocat->FindPointsWithinSquaredRadius
                  ( minDist2[i] * 0.5, pLocPnts + j, idxLists[0] );
        octLocat->FindPointsWithinSquaredRadius
                  ( minDist2[i],       pLocPnts + j, idxLists[1] );
        octLocat->FindPointsWithinSquaredRadius
                  ( maxDist2[i],       pLocPnts + j, idxLists[2] );
        if ( idxLists[0]->GetNumberOfIds() == 0 )
             idxLists[0]->InsertNextId( -1 );    // to handle an empty id list
        }
      
      // copy the point indices to a vtkIdList for comparison and file writing
      for ( m = 0; m < 3; m ++ )
        {
        numInsrt = idxLists[m]->GetNumberOfIds();
        for ( k = 0; k < numInsrt; k ++ )
          {
          ptIdList->InsertNextId(  idxLists[m]->GetId( k )  );
          }
        }
        
      // ---------------------------------------------------------------------
      // verify the result in brute force mode  
      #ifdef _BRUTE_FORCE_VERIFICATION_
      
      // check if the monotonical property holds among the 3 point-index lists
      if ( minDist2[i] <= INC_OCT_PNT_LOC_TESTS_ZERO )
        {
        pointIdx = octLocat->FindClosestPoint( pLocPnts + j );
        if ( idxLists[0]->IsId( pointIdx ) == -1 ||
             idxLists[1]->IsId( pointIdx ) == -1 ||
             idxLists[2]->IsId( pointIdx ) == -1 || 
             idxLists[1]->GetNumberOfIds() < idxLists[0]->GetNumberOfIds() ||
             idxLists[2]->GetNumberOfIds() < idxLists[0]->GetNumberOfIds() ||
             idxLists[2]->GetNumberOfIds() < idxLists[1]->GetNumberOfIds()
           ) retValue = 1;
        }
      else
        {
        if ( idxLists[0]->GetNumberOfIds() !=  1 ||
             idxLists[0]->GetId( 0 )       != -1 ||
             idxLists[1]->GetNumberOfIds() <   1 ||
             idxLists[2]->GetNumberOfIds() <  idxLists[1]->GetNumberOfIds()
           ) retValue = 1;
        }
      
      // check the points within each of the three radii
      for (  m = 0;  ( m < 3 ) && ( retValue == 0 );  m ++  )
        {
        // get the squared radius actually used       
        if ( minDist2[i] <= INC_OCT_PNT_LOC_TESTS_ZERO )
          {
          if ( m == 0 ) fTempRad = maxDist2[i] * 0.3;
          else
          if ( m == 1 ) fTempRad = maxDist2[i] * 0.6;
          else          fTempRad = maxDist2[i];
          }
        else
          {
          if ( m == 0 ) fTempRad = minDist2[i] * 0.5;
          else
          if ( m == 1 ) fTempRad = minDist2[i];
          else          fTempRad = maxDist2[i];
          }
        
        // check if there is any false insertion
        numInsrt = idxLists[m]->GetNumberOfIds();
        for (  k = 0;  ( k < numInsrt ) && ( retValue == 0 );  k ++  )
          {
          if (  m == 0 && idxLists[0]->GetId( 0 )  ==  -1  ) break;
          
          pointIdx = idxLists[m]->GetId( k );
          pointIdx = ( pointIdx << 1 ) + pointIdx;
          tmpDist2 = vtkMath::Distance2BetweenPoints
                              ( pLocPnts + j, pDataPts + pointIdx );
                              
          if ( tmpDist2 > fTempRad + INC_OCT_PNT_LOC_TESTS_ZERO ) retValue = 1;
          }
          
        // check if there is any missed insertion
        numInsrt = 0;
        for ( k = 0; k < numbPnts; k ++ )
          {      
          tmpDist2 = vtkMath::Distance2BetweenPoints
                              (  pLocPnts + j,  pDataPts + ( k << 1 ) + k  );
          if ( tmpDist2 + INC_OCT_PNT_LOC_TESTS_ZERO <= fTempRad ) numInsrt ++;
          }
        
        // get the actual size of the vtkIdList for comparison
        int  listSize = ( m == 0 && idxLists[0]->GetId( 0 ) == -1 )
                        ? 0       // for an actually NULL vtkIdList
                        : idxLists[m]->GetNumberOfIds();
        if ( numInsrt > listSize ) retValue = 1;
        }
      #endif
      // -------------------------------------------------------------------//
      }
      
    // -----------------------------------------------------------------------
    // write the point indices as the ground truth
    #ifdef  _BRUTE_FORCE_VERIFICATION_WRITE_RESULT_
    if ( retValue == 0 )
      {
      numInsrt = ptIdList->GetNumberOfIds();
      fwrite( &numInsrt, sizeof( int ),  1,  diskFile  );
      fwrite(  ptIdList->GetPointer( 0 ), 
               sizeof( vtkIdType ),  numInsrt,  diskFile  );
      }
    #endif
    // ---------------------------------------------------------------------//
    
    // -----------------------------------------------------------------------
    // rapid point index-based verification 
    #ifndef _BRUTE_FORCE_VERIFICATION_
    fread( &numInsrt,  sizeof( int ),  1,  diskFile  );
    #ifdef VTK_WORDS_BIGENDIAN
    SwapForBigEndian
      (  ( unsigned char * ) ( &numInsrt ),  sizeof( int ),  1  );
    #endif
    
    truthIds = ( vtkIdType * )
               realloc(  truthIds,  sizeof( vtkIdType ) * numInsrt  );
    fread(  truthIds,  sizeof( vtkIdType ),  numInsrt,  diskFile  );  
    #ifdef VTK_WORDS_BIGENDIAN
    SwapForBigEndian
      (  ( unsigned char * ) truthIds,  sizeof( vtkIdType ),  numInsrt  );
    #endif
    
    vtkIdType * tmpPtIds = ptIdList->GetPointer( 0 );
    for (  i = 0;  ( i < numInsrt ) && ( retValue == 0 );  i ++  )
      {
      retValue = ( tmpPtIds[i] == truthIds[i] ) ? 0 : 1;
      } 
    tmpPtIds = NULL;
    #endif
    // ---------------------------------------------------------------------//
    
    }
    
    
  // =========================================================================  
  // ================== DO NOT TURN ON THIS SEGMENT OF CODE ==================
  // ========================================================================= 
  #ifdef _BRUTE_FORCE_VERIFICATION_WRITE_DISTANCE2_
  fileName = vtkTestUtilities::ExpandDataFileName
                               ( argc, argv, "Data/IncOctPntLocData.dat" );
  pntsFile = fopen( fileName, "wb" );
  delete []  fileName;  fileName = NULL;
  fwrite(&nLocPnts, sizeof( int    ), 1,            pntsFile );
  fwrite( pLocPnts, sizeof( double ), nLocPnts * 3, pntsFile );
  fwrite( minDist2, sizeof( double ), nLocPnts,     pntsFile );
  fwrite( maxDist2, sizeof( double ), nLocPnts,     pntsFile );
  fclose( pntsFile );                 pntsFile = NULL;
  #endif
  // =======================================================================//
  // =======================================================================//
  
  
  // =========================================================================  
  // ================== DO NOT TURN ON THIS SEGMENT OF CODE ==================
  // =========================================================================  
  // NOTE: do *NOT* turn on this segment of code as the points resulting from
  //       the following random generator would change the points disk file!!!
  //       The points generated below are used to challege point location.
  //       In case this segment is executed, it needs to be turned off and
  //       re-run the test with _BRUTE_FORCE_VERIFICATION_ on to create new
  //       disk files.
  #if 0
  int      duplPnts = 200;
  int      inerPnts = 500;
  int      outrPnts = 300;
  int      totalPts = duplPnts + inerPnts + outrPnts;
  int      coordIdx = 0;
  int      gridIndx = 0;
  double * ptCoords = ( double * ) malloc( sizeof( double ) * 3 * totalPts );
  
  // duplicating grid points
  for ( i = 0; i < duplPnts; i ++, coordIdx ++ )
    {
    gridIndx = rand() % numbPnts;
    ptCoords[ ( coordIdx << 1 ) + coordIdx + 0 ] = 
    pDataPts[ ( gridIndx << 1 ) + gridIndx + 0 ];
    ptCoords[ ( coordIdx << 1 ) + coordIdx + 1 ] = 
    pDataPts[ ( gridIndx << 1 ) + gridIndx + 1 ];
    ptCoords[ ( coordIdx << 1 ) + coordIdx + 2 ] = 
    pDataPts[ ( gridIndx << 1 ) + gridIndx + 2 ];
    }
    
  // non-duplicate within-data points
  // some randomly selected grid points are jittered, in each axis, 
  // between [ -0.1, 0.1 ] with resolution 0.001
  for ( i = 0; i < inerPnts; i ++, coordIdx ++ )
    {
    gridIndx = ( rand() % numbPnts ) + ( numbPnts >> 1 );
    gridIndx = gridIndx % numbPnts;
    
    ptCoords[ ( coordIdx << 1 ) + coordIdx + 0 ] = 
    pDataPts[ ( gridIndx << 1 ) + gridIndx + 0 ] +
    0.001 * (   (  ( rand() % 2 ) << 1  )  -  1   ) * ( rand() % 101 );
    
    ptCoords[ ( coordIdx << 1 ) + coordIdx + 1 ] = 
    pDataPts[ ( gridIndx << 1 ) + gridIndx + 1 ] +
    0.001 * (   (  ( rand() % 2 ) << 1  )  -  1   ) * ( rand() % 101 );
    
    ptCoords[ ( coordIdx << 1 ) + coordIdx + 2 ] = 
    pDataPts[ ( gridIndx << 1 ) + gridIndx + 2 ] +
    0.001 * (   (  ( rand() % 2 ) << 1  )  -  1   ) * ( rand() % 101 );
    }
    
  // outer points: x --- [ -5.0, -3.0 ] or [ 3.0, 5.0 ]
  //                 AND
  //               y --- [ -5.0, -3.0 ] or [ 3.0, 5.0 ]
  //                 AND
  //               z --- [ -5.0, -3.0 ] or [ 3.0, 5.0 ]
  for ( i = 0; i < outrPnts; i ++, coordIdx ++ )
    {
    ptCoords[ ( coordIdx << 1 ) + coordIdx + 0 ]  = 
      3.0 + 0.01 * ( rand() % 201 );
    ptCoords[ ( coordIdx << 1 ) + coordIdx + 0 ] *= 
      (  ( rand() % 2 ) << 1  )  -  1;
      
    ptCoords[ ( coordIdx << 1 ) + coordIdx + 1 ]  = 
      3.0 + 0.01 * ( rand() % 201 );
    ptCoords[ ( coordIdx << 1 ) + coordIdx + 1 ] *= 
      (  ( rand() % 2 ) << 1  )  -  1;
      
    ptCoords[ ( coordIdx << 1 ) + coordIdx + 2 ]  = 
      3.0 + 0.01 * ( rand() % 201 );
    ptCoords[ ( coordIdx << 1 ) + coordIdx + 2 ] *= 
      (  ( rand() % 2 ) << 1  )  -  1;
    }
    
  // write the points to a disk file
  fileName = vtkTestUtilities::ExpandDataFileName
                               ( argc, argv, "Data/IncOctPntLocData.dat" );
  pntsFile = fopen( fileName, "wb" );
  delete []  fileName;  fileName = NULL;
  fwrite( &totalPts, sizeof( int    ), 1,            pntsFile );
  fwrite(  ptCoords, sizeof( double ), totalPts * 3, pntsFile );
  fclose(  pntsFile  );  pntsFile = NULL;
  free( ptCoords );      ptCoords = NULL;
  #endif
  // =======================================================================//
  // =======================================================================//
 
 
  // memory clearance
  dataPnts = NULL;                     unstruct = NULL;
  if ( ptIdList ) ptIdList->Delete();  ptIdList = NULL; 
  if ( octLocat ) octLocat->Delete();  octLocat = NULL;
  if ( ugReader ) ugReader->Delete();  ugReader = NULL;
  
  if ( truthIds ) free( truthIds );    truthIds = NULL;
  if ( resltIds ) free( resltIds );    resltIds = NULL;
  if ( pDataPts ) free( pDataPts );    pDataPts = NULL;
  if ( pLocPnts ) free( pLocPnts );    pLocPnts = NULL;
  if ( minDist2 ) free( minDist2 );    minDist2 = NULL;
  if ( maxDist2 ) free( maxDist2 );    maxDist2 = NULL;
  if ( clzNdst2 ) free( clzNdst2 );    clzNdst2 = NULL;
  
  if ( diskFile ) fclose( diskFile );  diskFile = NULL;
  
  for ( i = 0; i < 3; i ++ )
    {
    if ( idxLists[i] ) idxLists[i]->Delete();  idxLists[i] = NULL;
    }
  
  
  return retValue;
}
