/*=========================================================================

  Program:   ParaView
  Module:    vtkSubGroup.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkObjectFactory.h"
#include "vtkMultiProcessController.h"
#include "vtkSubGroup.h"

#ifdef _MSC_VER
#pragma warning ( disable : 4100 )
#endif
#include <algorithm>

vtkStandardNewMacro(vtkSubGroup);

vtkSubGroup::vtkSubGroup()
{
  this->members = NULL;
  this->comm = NULL;

  this->nmembers = 0;
  this->myLocalRank = -1;
  this->tag = 0;
  this->nFrom = this->nTo = this->fanInTo = 0;
  this->nRecv = this->nSend = 0;
  this->gatherRoot = this->gatherLength = -1;
}

int vtkSubGroup::Initialize(int p0=0, int p1=0, int me=0, int itag=0, vtkCommunicator *c=NULL)
{
  int i, ii;
  this->nmembers = p1 - p0 + 1;
  this->tag = itag;
  this->comm = c;

  if (this->members)
    {
    delete [] this->members;
    }

  this->members = new int [this->nmembers];

  this->myLocalRank = -1;

  for (i=p0, ii=0; i<=p1; i++)
    {
    if (i == me)
      {
      this->myLocalRank = ii;
      }
    this->members[ii++] = i;
    }

  if (this->myLocalRank == -1)
    {
    delete [] this->members;
    this->members = NULL;
    return 1;
    }

  this->gatherRoot = this->gatherLength = -1;

  this->computeFanInTargets();

  return 0;
}

int vtkSubGroup::computeFanInTargets()
{
  int i;
  this->nTo = 0;
  this->nFrom = 0;

  for (i = 1; i < this->nmembers; i <<= 1)
    {
    int other = this->myLocalRank ^ i;

    if (other >= this->nmembers) 
      {
      continue;
      }

    if (this->myLocalRank > other)
      {
      this->fanInTo = other;

      this->nTo++;   /* one at most */

      break;
      }
    else
      {
      this->fanInFrom[this->nFrom] = other;
      this->nFrom++;
      }
    }
  return 0;
}
void vtkSubGroup::moveRoot(int root)
{
  int tmproot = this->members[root];
  this->members[root] = this->members[0];
  this->members[0] = tmproot;

  return;
}
void vtkSubGroup::restoreRoot(int root)
{
  if (root == 0) 
    {
    return;
    }

  this->moveRoot(root);

  if (this->myLocalRank == root)
    {
    this->myLocalRank = 0;
    this->computeFanInTargets();
    }
  else if (this->myLocalRank == 0)
    {
    this->myLocalRank = root;
    this->computeFanInTargets();
    }

  return;
}
void vtkSubGroup::setUpRoot(int root)
{
  if (root == 0) 
    {
    return;
    }

  this->moveRoot(root);

  if (this->myLocalRank == root)
    {
    this->myLocalRank = 0;
    this->computeFanInTargets();
    }
  else if (this->myLocalRank == 0)
    {
    this->myLocalRank = root;
    this->computeFanInTargets();
    }

  return;
}

vtkSubGroup::~vtkSubGroup()
{
  delete [] this->members;
  this->members = NULL;
}
void vtkSubGroup::setGatherPattern(int root, int length)
{
  int i;

  if ( (root == this->gatherRoot) && (length == this->gatherLength))
    {
    return;
    }

  this->gatherRoot   = root;
  this->gatherLength = length;

  int clogn; // ceiling(log2(this->nmembers))
  for (clogn=0; 1<<clogn < this->nmembers; clogn++)
    {
    }

  int left = 0;
  int right = this->nmembers - 1;
  int iroot = root;

  this->nSend = 0;
  this->nRecv = 0;

  for (i=0; i<clogn; i++)
    {
    int src, offset, len;

    int mid = (left + right) / 2;

    if (iroot <= mid) 
      {
      src = (iroot == left ? mid + 1 : right);
      } 
    else 
      {
      src = (iroot == right ? mid : left);
      }
    if (src <= mid) 
      {                   /* left ... mid */
      offset = left * length;
      len =  (mid - left + 1) * length;
      } 
    else 
      {                            /* mid+1 ... right */
      offset = (mid + 1) * length;
      len    = (right - mid) * length;
      }
    if (this->myLocalRank == iroot) 
      {
      this->recvId[this->nRecv] = this->members[src];
      this->recvOffset[this->nRecv] = offset;
      this->recvLength[this->nRecv] = len;
            
      this->nRecv++;
        
      } 
    else if (this->myLocalRank == src) 
      {
      this->sendId = this->members[iroot];
      this->sendOffset = offset;
      this->sendLength = len;
            
      this->nSend++;
      }
    if (this->myLocalRank <= mid) 
      {
      if (iroot > mid) 
        {
        iroot = src;
        }
      right = mid;
      } 
    else 
      {
      if (iroot <= mid) 
        {
        iroot = src;
        }
      left = mid + 1;
      }
    if (left == right) break;
    }
  return;
}

int vtkSubGroup::getLocalRank(int processId)
{
  int localRank = processId - this->members[0];

  if ( (localRank < 0) || (localRank >= this->nmembers)) return -1;
  else return localRank;
}
#define MINOP  if (tempbuf[p] < buf[p]) {buf[p] = tempbuf[p];}
#define MAXOP  if (tempbuf[p] > buf[p]) {buf[p] = tempbuf[p];}
#define SUMOP  buf[p] += tempbuf[p];

#define REDUCE(Type, name, op) \
int vtkSubGroup::Reduce##name(Type *data, Type *to, int size, int root) \
{ \
  int i, p;\
  if (this->nmembers == 1)\
    {                     \
    for (i=0; i<size; i++) to[i] = data[i];\
    return 0;\
    }\
  if ( (root < 0) || (root >= this->nmembers)) \
    { \
    return 1;\
    } \
  if (root != 0) \
    { \
    this->setUpRoot(root); \
    } \
  Type *buf, *tempbuf; \
  tempbuf = new Type [size]; \
  if (this->nTo > 0)      \
    {                     \
    buf = new Type [size];\
    }        \
  else       \
    {        \
    buf = to;\
    } \
  if (buf != data) \
    { \
    memcpy(buf, data, size * sizeof(Type));\
    } \
  for (i=0; i < this->nFrom; i++) \
   {                              \
    this->comm->Receive(tempbuf, size,\
                      this->members[this->fanInFrom[i]], this->tag);\
    for (p=0; p<size; p++){ op }\
    }\
  delete [] tempbuf;\
  if (this->nTo > 0)\
    {               \
    this->comm->Send(buf, size, this->members[this->fanInTo], this->tag);\
    delete [] buf;\
    }\
  if (root != 0) \
    { \
    this->restoreRoot(root);\
    } \
  return 0; \
}

REDUCE(int, Min, MINOP)
REDUCE(float, Min, MINOP)
REDUCE(double, Min, MINOP)
REDUCE(int, Max, MAXOP)
REDUCE(float, Max, MAXOP)
REDUCE(double, Max, MAXOP)
REDUCE(int, Sum, SUMOP)

#define BROADCAST(Type) \
int vtkSubGroup::Broadcast(Type *data, int length, int root) \
{ \
  int i;\
  if (this->nmembers == 1) \
    { \
    return 0;\
    } \
  if ( (root < 0) || (root >= this->nmembers)) \
    { \
    return 1;\
    } \
  if (root != 0) \
    { \
    this->setUpRoot(root); \
    } \
  if (this->nTo > 0) \
    {                \
    this->comm->Receive(data, length, this->members[this->fanInTo], this->tag);\
    } \
  for (i = this->nFrom-1 ; i >= 0; i--) \
    {                                   \
    this->comm->Send(data, length, this->members[this->fanInFrom[i]], this->tag); \
    } \
  if (root != 0) \
    { \
      this->restoreRoot(root); \
    } \
  return 0; \
}

BROADCAST(char)
BROADCAST(int)
BROADCAST(float)
BROADCAST(double)
#ifdef VTK_USE_64BIT_IDS
BROADCAST(vtkIdType)
#endif

#define GATHER(Type)\
int vtkSubGroup::Gather(Type *data, Type *to, int length, int root)\
{ \
  int i;\
  Type *recvBuf;\
  if (this->nmembers == 1)\
    {                     \
    for (i=0; i<length; i++) to[i] = data[i];\
    return 0;\
    }\
  if ( (root < 0) || (root >= this->nmembers)) \
    { \
    return 1;\
    } \
  this->setGatherPattern(root, length);\
  if (this->nSend > 0)\
    {                 \
    recvBuf = new Type [length * this->nmembers];\
    }\
  else   \
    {    \
    recvBuf = to;\
    }\
  for (i=0; i<this->nRecv; i++) \
    {                           \
    this->comm->Receive(recvBuf + this->recvOffset[i], \
              this->recvLength[i], this->recvId[i], this->tag);\
    }\
  memcpy(recvBuf + (length * this->myLocalRank), data,\
         length * sizeof(Type));\
  if (this->nSend > 0) \
    {                  \
    this->comm->Send(recvBuf + this->sendOffset,\
                     this->sendLength, this->sendId, this->tag);\
    delete [] recvBuf;\
    }\
  return 0; \
}

GATHER(int)
GATHER(char)
GATHER(float)
#ifdef VTK_USE_64BIT_IDS
GATHER(vtkIdType)
#endif

int vtkSubGroup::AllReduceUniqueList(int *list, int len, int **newList)
{
  int transferLen, myListLen, lastListLen, nextListLen;
  int *myList, *lastList, *nextList;

  myListLen = vtkSubGroup::MakeSortedUnique(list, len, &myList);

  if (this->nmembers == 1)
    { 
    *newList = myList;
    return myListLen;
    }

  lastList = myList;
  lastListLen = myListLen;

  for (int i=0; i < this->nFrom; i++)
    { 
    this->comm->Receive(&transferLen, 1,
                      this->members[this->fanInFrom[i]], this->tag); 

    int *buf = new int [transferLen];

    this->comm->Receive(buf, transferLen,
                      this->members[this->fanInFrom[i]], this->tag+1); 

    nextListLen = vtkSubGroup::MergeSortedUnique(lastList, lastListLen, 
                                           buf, transferLen, &nextList);

    delete [] buf;
    delete [] lastList;

    lastList = nextList;
    lastListLen = nextListLen;
    }                                                     

  if (this->nTo > 0)
    { 
    this->comm->Send(&lastListLen, 1, this->members[this->fanInTo], this->tag);

    this->comm->Send(lastList, lastListLen, 
                     this->members[this->fanInTo], this->tag+1); 
    }                


  this->Broadcast(&lastListLen, 1, 0);

  if (this->myLocalRank > 0)
    {
    delete [] lastList;
    lastList = new int [lastListLen];
    }

  this->Broadcast(lastList, lastListLen, 0);

  *newList = lastList;

  return lastListLen; 
}
int vtkSubGroup::MergeSortedUnique(int *list1, int len1, int *list2, int len2,
                                   int **newList)
{
  int newLen = 0;
  int i1=0;
  int i2=0;

  int *newl = new int [len1 + len2];

  if (newl == NULL)
    {
    return 0;
    }

  while ((i1 < len1) || (i2 < len2))
    {
    if (i2 == len2)
      {
      newl[newLen++] = list1[i1++];
      }
    else if (i1 == len1)
      {
      newl[newLen++] = list2[i2++];
      }
    else if (list1[i1] < list2[i2])
      {
      newl[newLen++] = list1[i1++];
      }
    else if (list1[i1] > list2[i2])
      {
      newl[newLen++] = list2[i2++];
      }
    else
      {
      newl[newLen++] = list1[i1++];
      i2++;
      }
    }

  *newList = newl;

  return newLen;
}
int vtkSubGroup::MakeSortedUnique(int *list, int len, int **newList)
{
  int i, newlen;
  int *newl;

  newl = new int [len];
  if (newl == NULL)
    {
    return 0;
    }

  memcpy(newl, list, len * sizeof(int));
  std::sort(newl, newl + len);

  for (i=1, newlen=1; i<len; i++)
    {
    if (newl[i] == newl[newlen-1]) 
      {
      continue;
      }

    newl[newlen++] = newl[i];
    }
  
  *newList = newl;

  return newlen;
}

int vtkSubGroup::Barrier()
{
  float token = 0;
  float result;

  this->ReduceMin(&token, &result, 1, 0);
  this->Broadcast(&token, 1, 0);
  return 0;
}

void vtkSubGroup::PrintSubGroup() const
{
  int i;
  cout << "(Fan In setup ) nFrom: " << this->nFrom << ", nTo: " << this->nTo << endl;
  if (this->nFrom > 0)
    {
    for (i=0; i<nFrom; i++)
      {
      cout << "fanInFrom[" << i << "] = " << this->fanInFrom[i] << endl;
      }
    }
  if (this->nTo > 0) 
    {
    cout << "fanInTo = " << this->fanInTo << endl;
    }

  cout << "(Gather setup ) nRecv: " << this->nRecv << ", nSend: " << this->nSend << endl;
  if (this->nRecv > 0)
    {
    for (i=0; i<nRecv; i++)
      {
      cout << "recvId[" << i << "] = " << this->recvId[i];
      cout << ", recvOffset[" << i << "] = " << this->recvOffset[i]; 
      cout << ", recvLength[" << i << "] = " << this->recvLength[i] << endl;
      }
    }
  if (nSend > 0)
    {
    cout << "sendId = " << this->sendId;
    cout << ", sendOffset = " << this->sendOffset;
    cout << ", sendLength = " << this->sendLength << endl;
    }
  cout << "gatherRoot " << this->gatherRoot ;
  cout << ", gatherLength " << this->gatherLength << endl;

  cout << "nmembers: " << this->nmembers << endl;
  cout << "myLocalRank: " << this->myLocalRank << endl;
  for (i=0; i<this->nmembers; i++)
    {
    cout << "  " << this->members[i];
    if (i && (i%20 == 0))
      {
      cout << endl;
      }
    }
  cout << endl;
  cout << "comm: " << this->comm;
  cout << endl;
}

void vtkSubGroup::PrintSelf(ostream &os, vtkIndent indent)
{
  int i;
  os << indent << "(Fan In setup ) nFrom: " << this->nFrom << ", nTo: " << this->nTo << endl;
  if (this->nFrom > 0)
    {
    for (i=0; i<nFrom; i++)
      {
      os << indent << "fanInFrom[" << i << "] = " << this->fanInFrom[i] << endl;
      }
    }
  if (this->nTo > 0) 
    { 
    os << indent << "fanInTo = " << this->fanInTo << endl;
    }

  os << indent << "(Gather setup ) nRecv: " << this->nRecv << ", nSend: " << this->nSend << endl;
  if (this->nRecv > 0)
    {
    for (i=0; i<nRecv; i++)
      {
      os << indent << "recvId[" << i << "] = " << this->recvId[i];
      os << indent << ", recvOffset[" << i << "] = " << this->recvOffset[i]; 
      os << indent << ", recvLength[" << i << "] = " << this->recvLength[i] << endl;
      }
    }
  if (this->nSend > 0)
    {
    os << indent << "sendId = " << this->sendId;
    os << indent << ", sendOffset = " << this->sendOffset;
    os << indent << ", sendLength = " << this->sendLength << endl;
    }
  os << indent << "gatherRoot " << this->gatherRoot ;
  os << indent << ", gatherLength " << this->gatherLength << endl;

  os << indent << "nmembers: " << this->nmembers << endl;
  os << indent << "myLocalRank: " << this->myLocalRank << endl;
  for (i=0; i<this->nmembers; i++)
    {
    os << indent << "  " << this->members[i];
    if (i && (i%20 == 0))
      {
      os << indent << endl;
      }
    }
  os << indent << endl;
  os << indent << "comm: " << this->comm;
  os << indent << endl;
}
