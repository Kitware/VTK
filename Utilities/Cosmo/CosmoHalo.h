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

#ifndef CosmoHalo_h
#define CosmoHalo_h

#ifdef USE_VTK_COSMO
#include "CosmoDefinition.h"
#else
#include "Definition.h"
#endif

#include <string>
#include <vector>
#include <set>
#include <algorithm>

using namespace std;

////////////////////////////////////////////////////////////////////////////
//
// CosmoHalo functions as a container for mixed halos received from the serial
// halo finder where the particle vector contains the index of the particle
// on a particular processor and the tag of that particle for the problem.
//
// It also functions as a merge container for MASTER processor where it
// contains the mixed halos crossing more than one boundary.
//
////////////////////////////////////////////////////////////////////////////

#ifdef USE_VTK_COSMO
class COSMO_EXPORT CosmoHalo {
#else
class CosmoHalo {
#endif
public:
  CosmoHalo(ID_T id, int alive, int dead)
                {
                  this->numberOfAlive = alive;
                  this->numberOfDead = dead;
                  this->haloID = id;
                  this->valid = VALID;
                  this->particles = new vector<ID_T>;
                  this->tags = new vector<ID_T>;
                  this->neighbors = new set<int>;
                  this->partners = new set<int>;
                }
  ~CosmoHalo()
                {
                  delete this->particles;
                  delete this->tags;
                  delete this->neighbors;
                  delete this->partners;
                }

  // Add a particle index for this halo on this processor
  // Add to the neighbor zones to know how many processors share this halo
  void addParticle(ID_T indx, ID_T tag, int neighbor)
                {
                  this->particles->push_back(indx);
                  this->tags->push_back(tag);
                  if (neighbor != ALIVE)
                    this->neighbors->insert(neighbor);
                }

  // Add a mixed particle
  void addParticle(ID_T tag)
                {
                  this->tags->push_back(tag);
                }

  // Add a matching mixed halo index indicating same halo
  void addPartner(int index)
                {
                  this->partners->insert(index);
                }

  // Sort the members to help identify the same halo on multiple processors
  void sortParticleTags()
                {
                  sort(this->tags->begin(), this->tags->end());
                }

  void         setAliveCount(int c)     { this->numberOfAlive = c; }
  void         setDeadCount(int c)      { this->numberOfDead = c; }
  void         setRankID(int rank)      { this->rankID = rank; }
  void         setValid(int v)          { this->valid = v; }

  ID_T         getHaloID()              { return this->haloID; }
  int          getRankID()              { return this->rankID; }
  int          getAliveCount()          { return this->numberOfAlive; }
  int          getDeadCount()           { return this->numberOfDead; }
  int          getValid()               { return this->valid; }

  vector<ID_T>* getParticles()          { return this->particles; }
  vector<ID_T>* getTags()               { return this->tags; }
  set<int>*    getNeighbors()           { return this->neighbors; }
  set<int>*    getPartners()            { return this->partners; }


private:
  ID_T haloID;                  // Halo id is smallest particle index/tag
  int rankID;                   // Processor which owns this halo

  vector<ID_T>* particles;      // Index of halo particle on this processor
  vector<ID_T>* tags;           // Tag of halo particle
  set<int>* neighbors;          // Zones with dead particles from this halo
  set<int>* partners;           // Index of matching mixed halo

  int numberOfAlive;            // Number of alive particles in halo
  int numberOfDead;             // Number of dead particles in halo

  int valid;                    // Mixed halo to be used or not
};

#endif
