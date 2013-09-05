/*=========================================================================
                                                                                
Copyright (c) 2007, Los Alamos National Security, LLC

All rights reserved.

Copyright 2007. Los Alamos National Security, LLC. 
This software was produced under U.S. Government contract DE-AC52-06NA25396 
for Los Alamos National Laboratory (LANL), which is operated by 
Los Alamos National Security, LLC for the U.S. Department of Energy. 
The U.S. Government has rights to use, reproduce, and distribute this software. 
NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.  
If software is modified to produce derivative works, such modified software 
should be clearly marked, so as not to confuse it with the version available 
from LANL.
 
Additionally, redistribution and use in source and binary forms, with or 
without modification, are permitted provided that the following conditions 
are met:
-   Redistributions of source code must retain the above copyright notice, 
    this list of conditions and the following disclaimer. 
-   Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution. 
-   Neither the name of Los Alamos National Security, LLC, Los Alamos National
    Laboratory, LANL, the U.S. Government, nor the names of its contributors
    may be used to endorse or promote products derived from this software 
    without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
                                                                                
=========================================================================*/

#include <iostream>
#include <fstream>
#include <cmath>
#include <cstdlib>
#include <algorithm>

#include "CosmoHaloFinder.h"


#ifdef DEBUG
#include <sys/time.h>
#endif

using namespace std;

namespace cosmologytools {


/****************************************************************************/
CosmoHaloFinder::CosmoHaloFinder()
{

  nmin = 1;
}

/****************************************************************************/
CosmoHaloFinder::~CosmoHaloFinder()
{
}


/****************************************************************************/
void CosmoHaloFinder::Execute()
{
  cout << "np:       " << np << endl;
  cout << "rL:       " << rL << endl;
  cout << "bb:       " << bb << endl;
  cout << "nmin:     " << nmin << endl;
  cout << "pmin:     " << pmin << endl;
  cout << "periodic: " << (periodic ? "true" : "false") << endl;

#ifdef DEBUG
  timeval tim;
  gettimeofday(&tim, NULL);
  double t1=tim.tv_sec+(tim.tv_usec/1000000.0);
  Reading();
  gettimeofday(&tim, NULL);
  double t2=tim.tv_sec+(tim.tv_usec/1000000.0);
  printf("reading... %.2lfs\n", t2-t1);

  gettimeofday(&tim, NULL);
  t1=tim.tv_sec+(tim.tv_usec/1000000.0);
  Finding();
  gettimeofday(&tim, NULL);
  t2=tim.tv_sec+(tim.tv_usec/1000000.0);
  printf("finding... %.2lfs\n", t2-t1);

  gettimeofday(&tim, NULL);
  t1=tim.tv_sec+(tim.tv_usec/1000000.0);
  Writing();
  gettimeofday(&tim, NULL);
  t2=tim.tv_sec+(tim.tv_usec/1000000.0);
  printf("writing... %.2lfs\n", t2-t1);
#else
  Reading();
  Finding();
  Writing();
#endif

  // Memory for the standalone halo finder is allocated in Reading()
  // once the number of particles is known and it must be deleted here and
  // not in the destructor because when the serial is called from the
  // parallel, the halo structure is allocated there

  if (xx != 0) delete [] xx;
  if (yy != 0) delete [] yy;
  if (zz != 0) delete [] zz;
  if (vx != 0) delete [] vx;
  if (vy != 0) delete [] vy;
  if (vz != 0) delete [] vz;
  if (ms != 0) delete [] ms;
  if (pt != 0) delete [] pt;

  if (ht != 0) delete [] ht;
  if (halo != 0) delete [] halo;
  if (nextp != 0) delete [] nextp;
}
 
/****************************************************************************/
void CosmoHaloFinder::Reading()
{
  // Verify that file exists and is readable
  if ( !infile ) {
    cout << "No input file specified" << endl;
    exit (-1);
  }

  // Open the file and make sure everything is ok.
  ifstream *FileStream = new ifstream(infile, ios::in);
  if (FileStream->fail()) {
    delete FileStream;
    cout << "File: " << infile << " cannot be opened" << endl;
    exit (-1);
  }

  // compute the number of particles
  FileStream->seekg(0L, ios::end);
  npart = FileStream->tellg() / 32;

  cout << "npart:    " << npart << endl;

  // these arrays are only used in the writing phase
  xx = new POSVEL_T[npart];
  yy = new POSVEL_T[npart];
  zz = new POSVEL_T[npart];
  vx = new POSVEL_T[npart];
  vy = new POSVEL_T[npart];
  vz = new POSVEL_T[npart];
  ms = new POSVEL_T[npart];
  pt = new int[npart];

  // arrays used in finding halos
  ht = new int[npart];
  halo = new int[npart];
  nextp = new int[npart];

  // rewind file to beginning for particle reads
  FileStream->seekg(0L, ios::beg);

  // create dataspace
  for (int i=0; i<numDataDims; i++)
    data[i] = new POSVEL_T[npart];

  // declare temporary read buffers
  int nfloat = 7, nint = 1;
  POSVEL_T fBlock[nfloat];
  ID_T iBlock[nint];

  // Loop to read and scale all particles
  xscal = rL / (1.0*np);

  for (int i=0; i<npart; i++)
  {
    // Set file pointer to the requested particle
    FileStream->read((char *)fBlock, nfloat * sizeof(float));

    if (FileStream->gcount() != (int)(nfloat * sizeof(float))) {
      cout << "Premature end-of-file" << endl;
      exit (-1);
    }

    FileStream->read((char *)iBlock, nint * sizeof(int));
    if (FileStream->gcount() != (int)(nint * sizeof(int))) {
      cout << "Premature end-of-file" << endl;
      exit (-1);
    }

    // These files are always little-endian
    //vtkByteSwap::Swap4LERange(fBlock, nfloat);
    //vtkByteSwap::Swap4LERange(iBlock, nint);

    // sanity check
    if (fBlock[0] > rL || fBlock[2] > rL || fBlock[4] > rL) {
      cout << "rL is too small" << endl; 
      exit (-1);
    }

    data[dataX][i] = fBlock[0] / xscal;
    data[dataY][i] = fBlock[2] / xscal;
    data[dataZ][i] = fBlock[4] / xscal;

    // these assignment are only used in the writing phase.
    xx[i] = fBlock[0];
    vx[i] = fBlock[1];
    yy[i] = fBlock[2];
    vy[i] = fBlock[3];
    zz[i] = fBlock[4];
    vz[i] = fBlock[5];
    ms[i] = fBlock[6];
    pt[i] = iBlock[0]; 
        
  } // i-loop
  
  delete FileStream;

  return;
}

/****************************************************************************/
void CosmoHaloFinder::Writing()
{
  // compute halos statistics
  hsize = new int[npart];
  for (int h=0; h<npart; h++)
    hsize[h] = 0;

  for (int i=0; i<npart; i++)
    hsize[ht[i]] += 1;

  nhalo = 0;
  for (int h=0; h<npart; h++) {
    if (hsize[h] >= pmin)
      nhalo++;
  }

  cout << "nhalo:    " << nhalo << endl;

  nhalopart = 0;
  for (int i=0; i<npart; i++)
    if (hsize[ht[i]] >= pmin)
      nhalopart++;

  cout << "nhalopart:" << nhalopart << endl;

  // Verify that file exists and is writable
  if ( !outfile ) {
    cout << "No output file specified" << endl;
    exit (-1);
  }

  // Open the file and make sure everything is ok.
  ofstream *FileStream = new ofstream(outfile, ios::out);
  if (FileStream->fail()) {
    delete FileStream;
    cout << "File: " << outfile << " cannot be opened" << endl;
    exit (-1);
  }

  for (int i=0; i<npart; i++)
  {
    // output in ASCII form
    char str[1024];
    sprintf(str, "%12.4E %12.4E", xx[i], vx[i]); *FileStream << str;
    sprintf(str, " %12.4E %12.4E", yy[i], vy[i]); *FileStream << str;
    sprintf(str, " %12.4E %12.4E", zz[i], vz[i]); *FileStream << str;
    sprintf(str, " %12d", (hsize[ht[i]] < pmin) ? -1: pt[ht[i]]); *FileStream << str;
    sprintf(str, " %12d", pt[i]); *FileStream << str;
    *FileStream << "\n";
  } // i-loop

  delete FileStream;
  delete hsize;

  // done
  return;
}

/****************************************************************************/
void CosmoHaloFinder::Finding()
{
  //
  // REORDER particles based on spatial locality
  //

#ifdef DEBUG
  timeval tim;
  gettimeofday(&tim, NULL);
  double t1=tim.tv_sec+(tim.tv_usec/1000000.0);
#endif

  seq.resize(npart);
  for (int i = 0; i < npart; i++)
    seq[i] = i;

  Reorder(seq.begin(), seq.end(), dataX);


#ifdef DEBUG
  gettimeofday(&tim, NULL);
  double t2=tim.tv_sec+(tim.tv_usec/1000000.0);
  printf("reorder... %.2lfs\n", t2-t1);
#endif

  //
  // COMPUTE interval bounding box
  //
#ifdef DEBUG
  gettimeofday(&tim, NULL);
  t1=tim.tv_sec+(tim.tv_usec/1000000.0);
#endif

  lbound = new POSVEL_T[npart];
  ubound = new POSVEL_T[npart];
  POSVEL_T lb1[numDataDims], ub1[numDataDims];
  ComputeLU(0, npart, dataX, lb1, ub1);

#ifdef DEBUG
  gettimeofday(&tim, NULL);
  t2=tim.tv_sec+(tim.tv_usec/1000000.0);
  printf("computeLU... %.2lfs\n", t2-t1);
#endif

  //
  // FIND HALOS using friends-of-friends metric
  //
#ifdef DEBUG
  gettimeofday(&tim, NULL);
  t1=tim.tv_sec+(tim.tv_usec/1000000.0);
#endif

  for (int i=0; i<npart; i++) {
    ht[i] = i;
    halo[i] = i;
    nextp[i] = -1;
  }

  myFOF(0, npart, dataX);

#ifdef DEBUG
  gettimeofday(&tim, NULL);
  t2=tim.tv_sec+(tim.tv_usec/1000000.0);
  printf("myFOF... %.2lfs\n", t2-t1);
#endif

  //
  // CLEANUP
  //
  delete [] lbound;
  delete [] ubound;
  seq.clear();

  // done!
  return;
}

/****************************************************************************/
void CosmoHaloFinder::Reorder(
                        vector<int>::iterator first,
                        vector<int>::iterator last,
                        int axis)
{
    int length = std::distance(first, last);
    vector<int>::iterator middle = first + length/2;

    if (length == 1)
    return;

    nth_element(first, middle, last, kdCompare(data[axis]));

    Reorder(first, middle, (axis+1) % numDataDims);
    Reorder(middle, last, (axis+1) % numDataDims);
}

/****************************************************************************/
void CosmoHaloFinder::ComputeLU(
                        int first,
                        int last,
                        int axis,
                        POSVEL_T* ret_lb,
                        POSVEL_T* ret_ub)
{
  int len = last - first;
    
  int middle  = first + len/2;

  int useDim = (axis + 2) % numDataDims;
  POSVEL_T lb1[numDataDims], ub1[numDataDims];
  POSVEL_T lb2[numDataDims], ub2[numDataDims];
  
  // base cases
  if (len == 2) {
    int ii = seq[first];
    int jj = seq[first+1];

    lbound[middle] = min(data[useDim][ii], data[useDim][jj]);
    ubound[middle] = max(data[useDim][ii], data[useDim][jj]);

    ret_lb[dataX] = min(data[dataX][ii], data[dataX][jj]);
    ret_lb[dataY] = min(data[dataY][ii], data[dataY][jj]);
    ret_lb[dataZ] = min(data[dataZ][ii], data[dataZ][jj]);

    ret_ub[dataX] = max(data[dataX][ii], data[dataX][jj]);
    ret_ub[dataY] = max(data[dataY][ii], data[dataY][jj]);
    ret_ub[dataZ] = max(data[dataZ][ii], data[dataZ][jj]);

    return;
  }

  // this case is needed when npart is a non-power-of-two
  if (len == 3) {
    ComputeLU(first+1, last, (axis + 1) %3, lb2, ub2);

    int ii = seq[first];

    lbound[middle] = min(data[useDim][ii], lb2[useDim]);
    ubound[middle] = max(data[useDim][ii], ub2[useDim]);

    ret_lb[dataX] = min(data[dataX][ii], lb2[dataX]);
    ret_lb[dataY] = min(data[dataY][ii], lb2[dataY]);
    ret_lb[dataZ] = min(data[dataZ][ii], lb2[dataZ]);

    ret_ub[dataX] = max(data[dataX][ii], ub2[dataX]);
    ret_ub[dataY] = max(data[dataY][ii], ub2[dataY]);
    ret_ub[dataZ] = max(data[dataZ][ii], ub2[dataZ]);

    return;
  }

  // non-base cases

  ComputeLU(first, middle, (axis + 1) % numDataDims, lb1, ub1);
  ComputeLU(middle,  last, (axis + 1) % numDataDims, lb2, ub2);

  // compute LU at the bottom-up pass
  lbound[middle] = min(lb1[useDim], lb2[useDim]);
  ubound[middle] = max(ub1[useDim], ub2[useDim]);

  ret_lb[dataX] = min(lb1[dataX], lb2[dataX]);
  ret_lb[dataY] = min(lb1[dataY], lb2[dataY]);
  ret_lb[dataZ] = min(lb1[dataZ], lb2[dataZ]);

  ret_ub[dataX] = max(ub1[dataX], ub2[dataX]);
  ret_ub[dataY] = max(ub1[dataY], ub2[dataY]);
  ret_ub[dataZ] = max(ub1[dataZ], ub2[dataZ]);

  // done
  return;
}

/****************************************************************************/
void CosmoHaloFinder::myFOF(
                        int first,
                        int last,
                        int dataFlag)
{
  int len = last - first;

  // base case
  if (len == 1)
    return;

  // non-base cases

  // divide
  int middle = first + len/2;

  myFOF(first, middle, (dataFlag+1) % numDataDims);
  myFOF(middle,  last, (dataFlag+1) % numDataDims);

  // recursive merge
  Merge(first, middle, middle, last, dataFlag);

  // done
  return;
}

/****************************************************************************/
void CosmoHaloFinder::Merge(
                        int first1, int last1, 
                        int first2, int last2, 
                        int dataFlag)
{
  int len1 = last1 - first1;
  int len2 = last2 - first2;

  // base cases
  // len1 == 1 || len2 == 1
  // len1 == 1,2 && len2 == 1,2 (2 for non-power-of-two case)
  if (len1 == 1 || len2 == 1) {
    // If the minimum number of neighbors is at least two, then we need to check
    // before actually doing the neighbor merge.
    bool hasNMin = nmin < 2 ? true : false;
    int nCnt = 0;

    for (int i=0; i<len1 && !hasNMin; i++)
    for (int j=0; j<len2 && !hasNMin; j++) {
      int ii = seq[first1+i];
      int jj = seq[first2+j];
  
      // fast exit
      if (ht[ii] == ht[jj])
        continue;
  
      // ht[ii] != ht[jj]
      POSVEL_T xdist = fabs(data[dataX][jj] - data[dataX][ii]);
      POSVEL_T ydist = fabs(data[dataY][jj] - data[dataY][ii]);
      POSVEL_T zdist = fabs(data[dataZ][jj] - data[dataZ][ii]);
  
      if (periodic) {
        xdist = min(xdist, np-xdist);
        ydist = min(ydist, np-ydist);
        zdist = min(zdist, np-zdist);
      }
  
      if ((xdist<bb) && (ydist<bb) && (zdist<bb)) {
 
        POSVEL_T dist = xdist*xdist + ydist*ydist + zdist*zdist;
        if (dist < bb*bb) {
          ++nCnt;
          if (nCnt >= nmin)
            hasNMin = true;
        }
      }
    } // (i,j)-loop

    // If we don't have the required number of neighbors, then we're done.
    if (!hasNMin)
      return;

    for (int i=0; i<len1; i++)
    for (int j=0; j<len2; j++) {
      int ii = seq[first1+i];
      int jj = seq[first2+j];
  
      // fast exit
      if (ht[ii] == ht[jj])
        continue;
  
      // ht[ii] != ht[jj]
      POSVEL_T xdist = fabs(data[dataX][jj] - data[dataX][ii]);
      POSVEL_T ydist = fabs(data[dataY][jj] - data[dataY][ii]);
      POSVEL_T zdist = fabs(data[dataZ][jj] - data[dataZ][ii]);
  
      if (periodic) {
        xdist = min(xdist, np-xdist);
        ydist = min(ydist, np-ydist);
        zdist = min(zdist, np-zdist);
      }
  
      if ((xdist<bb) && (ydist<bb) && (zdist<bb)) {
  
        POSVEL_T dist = xdist*xdist + ydist*ydist + zdist*zdist;
        if (dist < bb*bb) {
  
          // union two halos to one
          int newHaloId = min(ht[ii], ht[jj]);
          int oldHaloId = max(ht[ii], ht[jj]);
  
          // update particles with oldHaloId
          int last = -1;
          int ith = halo[oldHaloId];
          while (ith != -1) {
            ht[ith] = newHaloId;
            last = ith;
            ith = nextp[ith];
          }
  
          // update halo's linked list
          nextp[last] = halo[newHaloId];
          halo[newHaloId] = halo[oldHaloId];
          halo[oldHaloId] = -1;
        }
      }
    } // (i,j)-loop

    return;
  }

  // non-base case

  // pruning?
  int middle1 = first1 + len1/2;
  int middle2 = first2 + len2/2;

  POSVEL_T lL = lbound[middle1];
  POSVEL_T uL = ubound[middle1];
  POSVEL_T lR = lbound[middle2];
  POSVEL_T uR = ubound[middle2];

  POSVEL_T dL = uL - lL;
  POSVEL_T dR = uR - lR;
  POSVEL_T dc = max(uL,uR) - min(lL,lR);

  POSVEL_T dist = dc - dL - dR;
  if (periodic)
    dist = min(dist, np-dc);

  if (dist >= bb)
    return;

  // continue merging

  // move to the next axis
  dataFlag = (dataFlag + 1) % numDataDims;

  Merge(first1, middle1,  first2, middle2, dataFlag);
  Merge(first1, middle1, middle2,   last2, dataFlag);
  Merge(middle1,  last1,  first2, middle2, dataFlag);
  Merge(middle1,  last1, middle2,   last2, dataFlag);

  // done
  return;
}

}
