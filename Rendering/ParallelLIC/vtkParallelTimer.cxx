/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParallelTimer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkParallelTimer.h"

#if defined(_WIN32)
// The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name
#pragma warning(disable : 4996)
#endif

#include "vtkObjectFactory.h"
#include "vtkMPI.h"

using std::cerr;
using std::endl;
using std::vector;
using std::string;
using std::ostringstream;

#include <ctime>
#if !defined(_WIN32)
#include <sys/time.h>
#include <unistd.h>
#else
#include <process.h>
#include <Winsock2.h>
#include <time.h>
static
int gettimeofday(struct timeval *tv, void *)
{
  FILETIME ft;
  GetSystemTimeAsFileTime(&ft);

  __int64 tmpres = 0;
  tmpres = ft.dwHighDateTime;
  tmpres <<= 32;
  tmpres |= ft.dwLowDateTime;

  /*converting file time to unix epoch*/
  const __int64 DELTA_EPOCH_IN_MICROSECS= 11644473600000000;
  tmpres /= 10;  /*convert into microseconds*/
  tmpres -= DELTA_EPOCH_IN_MICROSECS;
  tv->tv_sec = (__int32)(tmpres*0.000001);
  tv->tv_usec = (tmpres%1000000);

  return 0;
}
#endif

#include <fstream>
using std::ofstream;
using std::ios_base;



/*
For singleton pattern
**/
vtkParallelTimer *vtkParallelTimer::GlobalInstance = 0;
vtkParallelTimer::vtkParallelTimerDestructor vtkParallelTimer::GlobalInstanceDestructor;

//-----------------------------------------------------------------------------
vtkParallelTimer::vtkParallelTimerDestructor::~vtkParallelTimerDestructor()
{
  if (this->Log)
    {
    this->Log->Delete();
    this->Log = NULL;
    }
}



// .NAME vtkParallelTimerBuffer -- A parallel buffer
//
// .SECTION Description
//  A parallel buffer for logging events and other data during an MPI
//  run. This is an implementation class you should not use it directly.
//  Use vtkParallelTimer instead.
class vtkParallelTimerBuffer
{
public:
  vtkParallelTimerBuffer();
  ~vtkParallelTimerBuffer();

  vtkParallelTimerBuffer(const vtkParallelTimerBuffer &other);
  void operator=(const vtkParallelTimerBuffer &other);

  // Description:
  // Access state and internal data.
  const char *GetData() const { return this->Data; }
  char *GetData(){ return this->Data; }
  size_t GetSize() const { return this->At; }
  size_t GetCapacity() const { return this->Size; }

  // Description:
  // Clear the buffer but don't release memory.
  void Clear(){ this->At = 0; }

  // Description:
  // Clear the buffer and release all resources.
  void ClearForReal();

  // Description:
  // Stream insertion operators for adding data to the buffer.
  vtkParallelTimerBuffer &operator<<(const int v);
  vtkParallelTimerBuffer &operator<<(const long long v);
  vtkParallelTimerBuffer &operator<<(const double v);
  vtkParallelTimerBuffer &operator<<(const char *v);
  template<size_t N> vtkParallelTimerBuffer &operator<<(const char v[N]);

  // Description:
  // Stream extraction operator for getting formatted data out.
  vtkParallelTimerBuffer &operator>>(std::ostringstream &s);

  // Description:
  // Gather buffer to a root process. This is a collective
  // operation.
  void Gather(int rootRank);

protected:
  // Description:
  // Push n bytes onto the buffer, resizing if necessary.
  void PushBack(const void *data, size_t n);

  // Description:
  // resize to at least newSize bytes.
  void Resize(size_t newSize);

private:
  size_t Size;
  size_t At;
  size_t GrowBy;
  char *Data;
};

//-----------------------------------------------------------------------------
template<size_t N>
vtkParallelTimerBuffer &vtkParallelTimerBuffer::operator<<(const char v[N])
{
  const char c = 's';
  this->PushBack(&c, 1);
  this->PushBack(&v[0], N);
  return *this;
}

//-----------------------------------------------------------------------------
vtkParallelTimerBuffer::vtkParallelTimerBuffer()
      :
    Size(0),
    At(0),
    GrowBy(4096),
    Data(0)
{}

//-----------------------------------------------------------------------------
vtkParallelTimerBuffer::~vtkParallelTimerBuffer(){ free(this->Data); }

//-----------------------------------------------------------------------------
vtkParallelTimerBuffer::vtkParallelTimerBuffer(const vtkParallelTimerBuffer &other)
      :
    Size(0),
    At(0),
    GrowBy(4096),
    Data(0)
{
  *this = other;
}

//-----------------------------------------------------------------------------
void vtkParallelTimerBuffer::operator=(const vtkParallelTimerBuffer &other)
{
  if (this == &other)
    {
    return;
    }
  this->Clear();
  this->Resize(other.GetSize());
  memcpy(this->Data, other.Data, other.GetSize());
}

//-----------------------------------------------------------------------------
void vtkParallelTimerBuffer::ClearForReal()
{
  this->At = 0;
  this->Size = 0;
  free(this->Data);
  this->Data = 0;
}

//-----------------------------------------------------------------------------
vtkParallelTimerBuffer &vtkParallelTimerBuffer::operator<<(const int v)
{
  const char c = 'i';
  this->PushBack(&c, 1);
  this->PushBack(&v, sizeof(int));
  return *this;
}

//-----------------------------------------------------------------------------
vtkParallelTimerBuffer &vtkParallelTimerBuffer::operator<<(const long long v)
{
  const char c = 'l';
  this->PushBack(&c, 1);
  this->PushBack(&v, sizeof(long long));
  return *this;
}

//-----------------------------------------------------------------------------
vtkParallelTimerBuffer &vtkParallelTimerBuffer::operator<<(const double v)
{
  const char c = 'd';
  this->PushBack(&c, 1);
  this->PushBack(&v, sizeof(double));
  return *this;
}

//-----------------------------------------------------------------------------
vtkParallelTimerBuffer &vtkParallelTimerBuffer::operator<<(const char *v)
{
  const char c='s';
  this->PushBack(&c, 1);
  size_t n = strlen(v) + 1;
  this->PushBack(v, n);
  return *this;
}

//-----------------------------------------------------------------------------
vtkParallelTimerBuffer &vtkParallelTimerBuffer::operator>>(ostringstream &s)
{
  size_t i = 0;
  while (i < this->At)
    {
    char c = this->Data[i];
    ++i;
    switch (c)
      {
      case 'i':
        s << *(reinterpret_cast<int*>(this->Data+i));
        i += sizeof(int);
        break;

      case 'l':
        s << *(reinterpret_cast<long long*>(this->Data+i));
        i += sizeof(long long);
        break;

      case 'd':
        s << *(reinterpret_cast<double*>(this->Data+i));
        i += sizeof(double);
        break;

      case 's':
        {
        s << this->Data+i;
        size_t n = strlen(this->Data+i)+1;
        i += n;
        }
        break;

      default:
        cerr <<
          "Bad case at " << i-1 << " " << c << ", " << (int)c;
        return *this;
      }
    }
  return *this;
}

//-----------------------------------------------------------------------------
void vtkParallelTimerBuffer::Gather(int rootRank)
{
  int mpiOk;
  MPI_Initialized(&mpiOk);
  if (!mpiOk)
    {
    return;
    }
  int worldRank;
  MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);
  int worldSize;
  MPI_Comm_size(MPI_COMM_WORLD, &worldSize);

  // in serial this is a no-op
  if (worldSize > 1)
    {
    int *bufferSizes = 0;
    int *disp = 0;
    if (worldRank == rootRank)
      {
      bufferSizes = static_cast<int*>(malloc(worldSize*sizeof(int)));
      disp = static_cast<int*>(malloc(worldSize*sizeof(int)));
      }
    int bufferSize = static_cast<int>(this->GetSize());
    MPI_Gather(
        &bufferSize,
        1,
        MPI_INT,
        bufferSizes,
        1,
        MPI_INT,
        rootRank,
        MPI_COMM_WORLD);
    char *log = 0;
    int cumSize = 0;
    if (worldRank == rootRank)
      {
      for (int i=0; i<worldSize; ++i)
        {
        disp[i] = cumSize;
        cumSize += bufferSizes[i];
        }
      log = static_cast<char*>(malloc(cumSize));
      }
    MPI_Gatherv(
      this->Data,
      bufferSize,
      MPI_CHAR,
      log,
      bufferSizes,
      disp,
      MPI_CHAR,
      rootRank,
      MPI_COMM_WORLD);
    if (worldRank == rootRank)
      {
      this->Clear();
      this->PushBack(log,cumSize);
      free(bufferSizes);
      free(disp);
      free(log);
      }
    else
      {
      this->Clear();
      }
    }
}

//-----------------------------------------------------------------------------
void vtkParallelTimerBuffer::PushBack(const void *data, size_t n)
{
  size_t nextAt = this->At+n;
  this->Resize(nextAt);
  memcpy(this->Data+this->At, data, n);
  this->At = nextAt;
}

//-----------------------------------------------------------------------------
void vtkParallelTimerBuffer::Resize(size_t newSize)
{
  #if defined(vtkParallelTimerBufferDEBUG)
  size_t oldSize = this->Size;
  #endif
  if (newSize <= this->Size)
    {
    return;
    }
  while(this->Size < newSize)
    {
    this->Size += this->GrowBy;
    }
  this->Data = static_cast<char*>(realloc(this->Data,this->Size));
  #if defined(vtkParallelTimerBufferDEBUG)
  memset(this->Data+oldSize, -1, this->Size-oldSize);
  #endif
}




//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkParallelTimer);

//-----------------------------------------------------------------------------
vtkParallelTimer::vtkParallelTimer()
        :
    GlobalLevel(0),
    WorldRank(0),
    WriterRank(0),
    FileName(0),
    WriteOnClose(0),
    Log(0)
{
  #if vtkParallelTimerDEBUG > 1
  cerr << "=====vtkParallelTimer::vtkParallelTimer" << endl;
  #endif

  MPI_Initialized(&this->Initialized);
  if (this->Initialized)
    {
    MPI_Comm_rank(MPI_COMM_WORLD,&this->WorldRank);
    }
  this->StartTime.reserve(256);
  this->Log=new vtkParallelTimerBuffer;
}

//-----------------------------------------------------------------------------
vtkParallelTimer::~vtkParallelTimer()
{
  #if vtkParallelTimerDEBUG > 1
  cerr << "=====vtkParallelTimer::~vtkParallelTimer" << endl;
  #endif

  // Alert the user that he left events on the stack,
  // this is usually a sign of trouble.
  if (this->StartTime.size()>0)
    {
    vtkErrorMacro(
      << "Start time stack has "
      << this->StartTime.size()
      << " remaining.");
    }

  #if vtkParallelTimerDEBUG < 0
  if (this->EventId.size()>0)
    {
    size_t nIds=this->EventId.size();
    vtkErrorMacro(
      << "Event id stack has "
      << nIds << " remaining.");
    for (size_t i=0; i<nIds; ++i)
      {
      cerr << "EventId[" << i << "]=" << this->EventId[i] << endl;
      }
    }
  #endif

  this->SetFileName(0);

  delete this->Log;
}

//-----------------------------------------------------------------------------
vtkParallelTimer *vtkParallelTimer::GetGlobalInstance()
{
  #if vtkParallelTimerDEBUG > 1
  cerr << "=====vtkParallelTimer::GetGlobalInstance" << endl;
  #endif

  if (vtkParallelTimer::GlobalInstance==0)
    {
    vtkParallelTimer *log=vtkParallelTimer::New();
    ostringstream oss;
    oss << getpid() << ".log";
    log->SetFileName(oss.str().c_str());

    vtkParallelTimer::GlobalInstance=log;
    vtkParallelTimer::GlobalInstanceDestructor.SetLog(log);
    }
  return vtkParallelTimer::GlobalInstance;
}

//-----------------------------------------------------------------------------
void vtkParallelTimer::DeleteGlobalInstance()
{
  #if vtkParallelTimerDEBUG > 1
  cerr << "=====vtkParallelTimer::GetGlobalInstance" << endl;
  #endif

  if (vtkParallelTimer::GlobalInstance)
    {
    vtkParallelTimer::GlobalInstance->Delete();
    vtkParallelTimer::GlobalInstance = NULL;

    vtkParallelTimer::GlobalInstanceDestructor.SetLog(0);
    }
}

//-----------------------------------------------------------------------------
void vtkParallelTimer::Clear()
{
  #if vtkParallelTimerDEBUG > 1
  cerr << "=====vtkParallelTimer::Clear" << endl;
  #endif

  this->Log->Clear();
  this->HeaderBuffer.str("");
}

//-----------------------------------------------------------------------------
void vtkParallelTimer::StartEvent(int rank, const char *event)
{
  #if vtkParallelTimerDEBUG > 2
  cerr << "=====vtkParallelTimer::StartEvent" << endl;
  #endif

  if (this->WorldRank != rank)
    {
    return;
    }
  this->StartEvent(event);
}

//-----------------------------------------------------------------------------
void vtkParallelTimer::StartEvent(const char *event)
{
  #if vtkParallelTimerDEBUG > 1
  cerr << "=====vtkParallelTimer::StartEvent" << endl;
  #endif

  double walls = 0.0;
  timeval wallt;
  gettimeofday(&wallt, 0x0);
  walls = static_cast<double>(wallt.tv_sec)
    + static_cast<double>(wallt.tv_usec)/1.0E6;

  #if vtkParallelTimerDEBUG < 0
  this->EventId.push_back(event);
  #endif

  this->StartTime.push_back(walls);
}

//-----------------------------------------------------------------------------
void vtkParallelTimer::EndEvent(int rank, const char *event)
{
  #if vtkParallelTimerDEBUG > 2
  cerr << "=====vtkParallelTimer::EndEvent" << endl;
  #endif

  if (this->WorldRank != rank)
    {
    return;
    }
  this->EndEvent(event);
}

//-----------------------------------------------------------------------------
void vtkParallelTimer::EndEvent(const char *event)
{
  #if vtkParallelTimerDEBUG > 1
  cerr << "=====vtkParallelTimer::EndEvent" << endl;
  #endif

  double walle = 0.0;
  timeval wallt;
  gettimeofday(&wallt, 0x0);
  walle = static_cast<double>(wallt.tv_sec)
    + static_cast<double>(wallt.tv_usec)/1.0E6;

  #if vtkParallelTimerDEBUG > 0
  if (this->StartTime.size() == 0)
    {
    vtkErrorMacro("No event to end! " << event);
    return;
    }
  #endif

  double walls = this->StartTime.back();
  this->StartTime.pop_back();

  *this->Log
    << this->WorldRank << " "
    << event << " "
    << walls << " "
    << walle << " "
    << walle-walls
    << "\n";

  #if vtkParallelTimerDEBUG < 0
  const string &sEventId = this->EventId.back();
  const string eEventId = event;
  if (sEventId != eEventId)
    {
    vtkErrorMacro(
      << "Event mismatch " << sEventId.c_str() << " != " << eEventId.c_str());
    }
  this->EventId.pop_back();
  #endif

}

//-----------------------------------------------------------------------------
void vtkParallelTimer::EndEventSynch(int rank, const char *event)
{
  #if vtkParallelTimerDEBUG > 1
  cerr << "=====vtkParallelTimer::EndEventSynch" << endl;
  #endif

  if (this->Initialized)
    {
    MPI_Barrier(MPI_COMM_WORLD);
    }
  if (this->WorldRank != rank)
    {
    return;
    }
  this->EndEvent(event);
}

//-----------------------------------------------------------------------------
void vtkParallelTimer::EndEventSynch(const char *event)
{
  #if vtkParallelTimerDEBUG > 1
  cerr << "=====vtkParallelTimer::EndEventSynch" << endl;
  #endif

  if (this->Initialized)
    {
    MPI_Barrier(MPI_COMM_WORLD);
    }
  this->EndEvent(event);
}

//-----------------------------------------------------------------------------
void vtkParallelTimer::Update()
{
  #if vtkParallelTimerDEBUG > 1
  cerr << "=====vtkParallelTimer::Update" << endl;
  #endif

  if (this->Initialized)
    {
    this->Log->Gather(this->WriterRank);
    }
}

//-----------------------------------------------------------------------------
int vtkParallelTimer::Write()
{
  #if vtkParallelTimerDEBUG > 1
  cerr << "=====vtkParallelTimer::Write" << endl;
  #endif

  if ((this->WorldRank == this->WriterRank) && this->Log->GetSize())
    {
    cerr << "Wrote " << this->FileName << endl;

    ostringstream oss;
    *this->Log >> oss;
    ofstream f(this->FileName, ios_base::out|ios_base::app);
    if (!f.good())
      {
      vtkErrorMacro(
        << "Failed to open "
        << this->FileName
        << " for  writing.");
      return -1;
      }
    time_t t;
    time(&t);
    f << "# " << ctime(&t) << this->HeaderBuffer.str() << oss.str();
    f.close();
    }
  return 0;
}

//-----------------------------------------------------------------------------
void vtkParallelTimer::PrintSelf(ostream& os, vtkIndent)
{
  time_t t;
  time(&t);
  os << "# " << ctime(&t);
  if (this->WorldRank == this->WriterRank)
    {
    os << this->HeaderBuffer.str();
    }
  ostringstream oss;
  *this->Log >> oss;
  os << oss.str();
}
