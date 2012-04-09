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

// .NAME CosmoHaloFinder - find halos within a cosmology data file
// .SECTION Description
// CosmoHaloFinder is a filter object that operates on the unstructured 
// grid created when a CosmoReader reads a .cosmo data file. 
// It operates by finding clusters of neighbors.
//
// .SECTION Note
// This halo finder implements a recursive algorithm using a k-d tree.
// Linked lists are used to connect halos found during the recursive merge. 
// Bounding boxes are calculated for each particle for pruning the merge tree.
//
// The halo finder doesn't actually build a tree that can be walked but
// rather reorganizes the particles into the k-d tree using recursion, such that
// when myFOF is walked in the same way, the data will match. This is stored
// in the seq[] array.
//
// First step is Reorder().  When it is called the first time it divides all
// the particles on the X axis such that the particle at the halfway mark in
// the array is correctly positioned, and all particles in the array below it 
// have an X value less than it and all particles in the array above it have
// an X value higher.  Reorder() calls nth_element() which is a partial sort but
// faster.  So the division does not physically divide the space in half
// but rather divides the number of particles in half on a dimension.
// 
// Next step is the first level of recursion.  Each of the halves from above
// are divided on the Y axis, again such that the number of particles is the
// same in each half although the physical space is not divided.  Partial
// ordering is done again by resequencing the seq array.  Each of these now
// four pieces is divided on the Z axis next, and this continues until there
// is one particle at the bottom of the tree.
// 
// Next step in the halo finder is to call ComputeLU() which computes a 
// lower and upper bound for each particle based on the k-d tree of the next
// axis positioning.  This is used in pruning the merge tree during myFOF().
// This means that if there is a branch of the k-d tree with some
// halos in it, but that the next jump to a particle is too far away, then
// that entire branch is ignored.
// 
// Finally myFOF() is called and its recursion mimics that done by Reorder()
// so that it is looking at the k-d tree resequence correctly.  myFOF()
// recurses down to the bottom of the tree going to the left first.  When it
// gets to the bottom it calls Merge() to see if those particles at the
// bottom are close enough to each other.  Remembering that at each stage
// of the k-d tree the two halves are divided on the next axis by count and
// not by physical space, you can see that the Merge() must be done on those
// four parts as follows.
// 
// Merge(A,C) Merge(A,D) Merge(B,C) Merge(B,D).  
//
// This is because it is unknown if A shares a boundary with C and D or 
// B shares that boundary.  As particles are found to be close to each other, 
// if they are already a part of a halo, the two halos must unite.  
// While all this is going on, we also prune which means we stop the recursion.
// As Merge() and myFOF() walk through the recursion chains of halos are
// created and joined where they have a particle withing the required distance.
// When myFOF() ends it has a chain of first particle in a halo and nextp 
// pointing on down until -1 is reached.  Also the halo tag field for each
// particle is constantly altered so that each particle knows what halo it
// is part of, and that halo tag is the id of the lowest particle in the halo.
//

#ifndef CosmoHaloFinder_h
#define CosmoHaloFinder_h

#ifdef USE_VTK_COSMO
#include "CosmoDefinition.h"
#else
#include "Definition.h"
#endif

#include <string>

#define numDataDims 3
#define dataX 0
#define dataY 1
#define dataZ 2

using namespace std;

/****************************************************************************/
typedef POSVEL_T* floatptr;

//
// Particle information for reordering the particles according to position
// Value is either the X, Y or Z position depending on the recursion
// Id in the standalone serial version is the particle tag
// Id in the parallel version is the index of that particle on a
// particular processor which is why it can be int and not ID_T
//
struct ValueIdPair {
  POSVEL_T value;
  int id;
};

class ValueIdPairLT {
public:
  bool operator() (const ValueIdPair& p, const ValueIdPair& q) const
  {
  return p.value < q.value;
  }
};

/****************************************************************************/

#ifdef USE_VTK_COSMO
class COSMO_EXPORT CosmoHaloFinder
#else
class CosmoHaloFinder
#endif
{
public:
  // create a finder
  CosmoHaloFinder();
  ~CosmoHaloFinder();

  void Finding();

  // Read alive particles
#ifndef USE_VTK_COSMO
  void Reading();
  void Writing();

  // execute the finder
  void Execute();
#endif

  void setInFile(string inFile)         { infile = inFile.c_str(); }
  void setOutFile(string outFile)       { outfile = outFile.c_str(); }

  void setParticleLocations(POSVEL_T** d) { data = d; }
  void setNumberOfParticles(int n)      { npart = n; }
  void setMyProc(int r)                 { myProc = r; }

  int* getHaloTag()                     { return ht; }

  POSVEL_T* getXLoc()                   { return xx; }
  POSVEL_T* getYLoc()                   { return yy; }
  POSVEL_T* getZLoc()                   { return zz; }
  POSVEL_T* getXVel()                   { return vx; }
  POSVEL_T* getYVel()                   { return vy; }
  POSVEL_T* getZVel()                   { return vz; }
  POSVEL_T* getMass()                   { return ms; }
  int*   getTag()                       { return pt; }
  
  // np.in
  int np;
  POSVEL_T rL;
  POSVEL_T bb;
  int pmin;
  bool periodic;
  const char *infile;
  const char *outfile;
  const char *textmode;

private:

  // input/output interface
  POSVEL_T *xx, *yy, *zz, *vx, *vy, *vz, *ms;
  int   *pt, *ht;

  // internal state
  int npart, nhalo, nhalopart;
  int myProc;

  // data[][] stores xx[], yy[], zz[].
  POSVEL_T **data;

  // scale factor
  POSVEL_T xscal, vscal;

  int *halo, *nextp, *hsize;

  // Creates a sequence array containing ids of particle rearranged into
  // a k-d tree.  Recursive method.
  ValueIdPair *v;
  int *seq;
  void Reorder
    (int first, 
     int last, 
     int flag);

  // Calculates a lower and upper bound for each particle so that the 
  // mergeing step can prune parts of the k-d tree
  POSVEL_T **lb, **ub;
  void ComputeLU(int, int);

  // Recurses through the k-d tree merging particles to create halos
  void myFOF(int, int, int);
  void Merge(int, int, int, int, int);
};

#endif
