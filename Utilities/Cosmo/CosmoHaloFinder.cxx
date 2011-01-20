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

#include "CosmoHaloFinder.h"

#include <iostream>
#include <fstream>
#include <cmath>
#include <cstdlib>
#include <algorithm>

#ifndef USE_VTK_COSMO
#ifdef DEBUG
#include <sys/time.h>
#endif
#endif

using namespace std;

/****************************************************************************/
CosmoHaloFinder::CosmoHaloFinder()
{
}

/****************************************************************************/
CosmoHaloFinder::~CosmoHaloFinder()
{
}

#ifndef USE_VTK_COSMO
/****************************************************************************/
void CosmoHaloFinder::Execute()
{
  cout << "np:       " << np << endl;
  cout << "rL:       " << rL << endl;
  cout << "bb:       " << bb << endl;
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

  // rewind file to beginning for particle reads
  FileStream->seekg(0L, ios::beg);

  // create dataspace
  data = new POSVEL_T*[numDataDims];
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
#endif  // #ifndef USE_VTK_COSMO

/****************************************************************************/
void CosmoHaloFinder::Finding()
{
  //
  // REORDER particles based on spatial locality
  //
#ifndef USE_VTK_COSMO
#ifdef DEBUG
  timeval tim;
  gettimeofday(&tim, NULL);
  double t1=tim.tv_sec+(tim.tv_usec/1000000.0);
#endif
#endif

  v = new ValueIdPair[npart];
  for (int i = 0; i < npart; i++)
    v[i].id = i;

  Reorder(0, npart, dataX);

  seq = new int[npart];
  for (int i=0; i<npart; i++)
    seq[i] = v[i].id;

  delete v;

#ifndef USE_VTK_COSMO
#ifdef DEBUG
  gettimeofday(&tim, NULL);
  double t2=tim.tv_sec+(tim.tv_usec/1000000.0);
  printf("reorder... %.2lfs\n", t2-t1);
#endif
#endif

  //
  // COMPUTE interval bounding box
  //
#ifndef USE_VTK_COSMO
#ifdef DEBUG
  gettimeofday(&tim, NULL);
  t1=tim.tv_sec+(tim.tv_usec/1000000.0);
#endif
#endif

  lb = new floatptr[numDataDims];
  for (int i=0; i<numDataDims; i++)
    lb[i] = new POSVEL_T[npart];

  ub = new floatptr[numDataDims];
  for (int i=0; i<numDataDims; i++)
    ub[i] = new POSVEL_T[npart];

  ComputeLU(0, npart);

#ifndef USE_VTK_COSMO
#ifdef DEBUG
  gettimeofday(&tim, NULL);
  t2=tim.tv_sec+(tim.tv_usec/1000000.0);
  printf("computeLU... %.2lfs\n", t2-t1);
#endif
#endif

  //
  // FIND HALOS using friends-of-friends metric
  //
#ifndef USE_VTK_COSMO
#ifdef DEBUG
  gettimeofday(&tim, NULL);
  t1=tim.tv_sec+(tim.tv_usec/1000000.0);
#endif
#endif

  // create ht[] to store halo assignment.
  ht = new int[npart];
  for (int i=0; i<npart; i++)
    ht[i] = i;

  // create workspace for halo finder.
  halo  = new int[npart];
  nextp = new int[npart];

  for (int i=0; i<npart; i++) {
    halo[i] = i;
    nextp[i] = -1;
  }

  myFOF(0, npart, dataX);

#ifndef USE_VTK_COSMO
#ifdef DEBUG
  gettimeofday(&tim, NULL);
  t2=tim.tv_sec+(tim.tv_usec/1000000.0);
  printf("myFOF... %.2lfs\n", t2-t1);
#endif
#endif

  //
  // CLEANUP
  //
  for (int i=0; i<numDataDims; i++)
    delete ub[i];

  for (int i=0; i<numDataDims; i++)
    delete lb[i];

  delete seq;

  // done!
  return;
}

/****************************************************************************/
void CosmoHaloFinder::Reorder(int first,
                              int last, 
                              int dataFlag)
{
  int len = last - first;

  // base case
  if (len == 1)
    return;

  // non-base cases
  for(int i = first; i < last; i++)
    v[i].value = data[dataFlag][v[i].id];

  // divide
  int half = len >> 1;
  nth_element(&v[first], &v[first+half], &v[last], ValueIdPairLT());

  Reorder(first, first+half, (dataFlag+1)%3);
  Reorder(first+half,  last, (dataFlag+1)%3);

  // done
  return;
}

/****************************************************************************/
void CosmoHaloFinder::ComputeLU(int first, int last)
{
  int len = last - first;
    
  int middle  = first + len/2;
  int middle1 = first + len/4;
  int middle2 = first + 3*len/4;
  
  // base cases
  if (len == 2) {
    int ii = seq[first];
    int jj = seq[first+1];

    lb[dataX][middle] = min(data[dataX][ii], data[dataX][jj]);
    lb[dataY][middle] = min(data[dataY][ii], data[dataY][jj]);
    lb[dataZ][middle] = min(data[dataZ][ii], data[dataZ][jj]);

    ub[dataX][middle] = max(data[dataX][ii], data[dataX][jj]);
    ub[dataY][middle] = max(data[dataY][ii], data[dataY][jj]);
    ub[dataZ][middle] = max(data[dataZ][ii], data[dataZ][jj]);

    return;
  }

  // this case is needed when npart is a non-power-of-two
  if (len == 3) {
    // fill lb[][middle2] and ub[][middle2]
    ComputeLU(first+1, last);

    int ii = seq[first];

    lb[dataX][middle] = min(data[dataX][ii], lb[dataX][middle2]);
    lb[dataY][middle] = min(data[dataY][ii], lb[dataY][middle2]);
    lb[dataZ][middle] = min(data[dataZ][ii], lb[dataZ][middle2]);

    ub[dataX][middle] = max(data[dataX][ii], ub[dataX][middle2]);
    ub[dataY][middle] = max(data[dataY][ii], ub[dataY][middle2]);
    ub[dataZ][middle] = max(data[dataZ][ii], ub[dataZ][middle2]);

    return;
  }

  // non-base cases

  ComputeLU(first, middle);
  ComputeLU(middle,  last);

  // compute LU at the bottom-up pass
  lb[dataX][middle] = min(lb[dataX][middle1], lb[dataX][middle2]);
  lb[dataY][middle] = min(lb[dataY][middle1], lb[dataY][middle2]);
  lb[dataZ][middle] = min(lb[dataZ][middle1], lb[dataZ][middle2]);

  ub[dataX][middle] = max(ub[dataX][middle1], ub[dataX][middle2]);
  ub[dataY][middle] = max(ub[dataY][middle1], ub[dataY][middle2]);
  ub[dataZ][middle] = max(ub[dataZ][middle1], ub[dataZ][middle2]);

  // done
  return;
}

/****************************************************************************/
void CosmoHaloFinder::myFOF(int first, int last, int dataFlag)
{
  int len = last - first;

  // base case
  if (len == 1)
    return;

  // non-base cases

  // divide
  int middle = first + len/2;

  myFOF(first, middle, (dataFlag+1)%3);
  myFOF(middle,  last, (dataFlag+1)%3);

  // recursive merge
  Merge(first, middle, middle, last, dataFlag);

  // done
  return;
}

/****************************************************************************/
void CosmoHaloFinder::Merge(int first1, int last1, int first2, int last2, int dataFlag)
{
  int len1 = last1 - first1;
  int len2 = last2 - first2;

  // base cases
  // len1 == 1 || len2 == 1
  // len1 == 1,2 && len2 == 1,2 (2 for non-power-of-two case)
  if (len1 == 1 || len2 == 1) {
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

  POSVEL_T lL = lb[dataFlag][middle1];
  POSVEL_T uL = ub[dataFlag][middle1];
  POSVEL_T lR = lb[dataFlag][middle2];
  POSVEL_T uR = ub[dataFlag][middle2];

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
  dataFlag = (dataFlag + 1) % 3;

  Merge(first1, middle1,  first2, middle2, dataFlag);
  Merge(first1, middle1, middle2,   last2, dataFlag);
  Merge(middle1,  last1,  first2, middle2, dataFlag);
  Merge(middle1,  last1, middle2,   last2, dataFlag);

  // done
  return;
}
