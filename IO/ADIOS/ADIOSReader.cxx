/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ADIOSReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <adios_read.h>

#include "ADIOSReader.h"

namespace ADIOS
{

//----------------------------------------------------------------------------
struct Reader::InitContext
{
  static int RefCount;
  static MPI_Comm GlobalComm;
  static ADIOS_READ_METHOD Method;
  static std::string MethodArgs;

  MPI_Comm Comm;
  int Rank;
  int CommSize;

  InitContext()
  : Comm(GlobalComm)
  {
    if(this->RefCount == 0)
      {
      int init = 0;
      MPI_Initialized(&init);
      ReadError::TestEq(1, init, "InitContext: MPI is not yet initialized");

      int err = adios_read_init_method(Method, this->Comm, MethodArgs.c_str());
      ReadError::TestEq(0, err);
      }
    ++this->RefCount;

    MPI_Comm_size(this->Comm, &this->CommSize);
    MPI_Comm_rank(this->Comm, &this->Rank);
  }

  ~InitContext()
  {
    --this->RefCount;
    if(this->RefCount == 0)
      {
      // If we've gotten this far then we know that MPI has been initialized
      //already
      MPI_Barrier(this->Comm);
      adios_read_finalize_method(Method);
      }
  }
};

// Default communicator is invalid
//MPI_Comm Reader::InitContext::GlobalComm = MPI_COMM_NULL;
MPI_Comm Reader::InitContext::GlobalComm = MPI_COMM_WORLD;
ADIOS_READ_METHOD Reader::InitContext::Method = ADIOS_READ_METHOD_BP;
std::string Reader::InitContext::MethodArgs = "";
int Reader::InitContext::RefCount = 0;

//----------------------------------------------------------------------------
struct Reader::ReaderImpl
{
  ReaderImpl()
  : File(NULL), StepBegin(0), StepEnd(0)
  { }

  ~ReaderImpl()
  {
    for(std::vector<const Attribute*>::iterator a = this->Attributes.begin();
        a != this->Attributes.end();
        ++a)
      {
      delete *a;
      }
    for(std::vector<const Scalar*>::iterator s = this->Scalars.begin();
        s != this->Scalars.end();
        ++s)
      {
      delete *s;
      }
    for(std::vector<const VarInfo*>::iterator v = this->Arrays.begin();
        v != this->Arrays.end();
        ++v)
      {
      delete *v;
      }
  }

  ADIOS_FILE *File;
  size_t StepBegin;
  size_t StepEnd;
  std::vector<const Attribute*> Attributes;
  std::vector<const Scalar*> Scalars;
  std::vector<const VarInfo*> Arrays;
};

//-------------------:---------------------------------------------------------
bool Reader::SetCommunicator(MPI_Comm comm)
{
  // The communicator can only be set if ADIOS has not yet been initialized
  if(Reader::InitContext::RefCount == 0)
    {
    Reader::InitContext::GlobalComm = comm;
    return true;
    }
  return false;
}

//-------------------:---------------------------------------------------------
bool Reader::SetReadMethod(ReadMethod method, const std::string& methodArgs)
{
  // The communicator can only be set if ADIOS has not yet been initialized
  if(Reader::InitContext::RefCount == 0)
    {
    Reader::InitContext::Method = static_cast<ADIOS_READ_METHOD>(method);
    Reader::InitContext::MethodArgs = methodArgs;
    return true;
    }
  return false;
}

//----------------------------------------------------------------------------
Reader::Reader()
: Ctx(new InitContext), Impl(new ReaderImpl)
{
}

//----------------------------------------------------------------------------
Reader::~Reader()
{
  this->Close();

  delete this->Impl;
  delete this->Ctx;
}

//----------------------------------------------------------------------------
bool Reader::IsOpen() const
{
  return this->Impl->File != NULL;
}

//----------------------------------------------------------------------------
const std::vector<const Attribute*>& Reader::GetAttributes() const
{
  return this->Impl->Attributes;
}

//----------------------------------------------------------------------------
const std::vector<const Scalar*>& Reader::GetScalars() const
{
  return this->Impl->Scalars;
}

//----------------------------------------------------------------------------
const std::vector<const VarInfo*>& Reader::GetArrays() const
{
  return this->Impl->Arrays;
}

//----------------------------------------------------------------------------
void Reader::Open(const std::string &fileName)
{
  ReadError::TestEq<ADIOS_FILE*>(NULL, this->Impl->File,
    "Open: An existing file is already open");

  // Open the file
  this->Impl->File = adios_read_open_file(fileName.c_str(),
    this->Ctx->Method, this->Ctx->Comm);
  ReadError::TestNe<ADIOS_FILE*>(NULL, this->Impl->File);

  // Poplulate step information
  this->Impl->StepBegin = this->Impl->File->current_step;
  this->Impl->StepEnd = this->Impl->File->last_step;

  // Polulate attributes
  for(int i = 0; i < this->Impl->File->nattrs; ++i)
    {
    this->Impl->Attributes.push_back(new Attribute(this->Impl->File, i));
    }

  // Preload the scalar data and cache the array metadata
  for(int i = 0; i < this->Impl->File->nvars; ++i)
    {
      ADIOS_VARINFO *v = adios_inq_var_byid(this->Impl->File, i);
      ReadError::TestNe<ADIOS_VARINFO*>(NULL, v);

      if(v->ndim == 0)
        {
        this->Impl->Scalars.push_back(new Scalar(this->Impl->File, v));
        }
      else
        {
        this->Impl->Arrays.push_back(new VarInfo(this->Impl->File, v));
        }
      adios_free_varinfo(v);
    }
}

//----------------------------------------------------------------------------
void Reader::Close()
{
  if(this->Impl->File)
    {
    adios_read_close(this->Impl->File);
    this->Impl->File = NULL;
    }
}

//----------------------------------------------------------------------------
void Reader::GetStepRange(int &tStart, int &tEnd) const
{
  ReadError::TestNe<ADIOS_FILE*>(NULL, this->Impl->File,
    "GetStepRange: File not open");

  tStart = this->Impl->StepBegin;
  tEnd = this->Impl->StepEnd;
}

//----------------------------------------------------------------------------
void Reader::ScheduleReadArray(int id, void *data, int step, int block)
{
  ADIOS_SELECTION *sel = adios_selection_writeblock(block);
  ReadError::TestNe<ADIOS_SELECTION*>(NULL, sel);

  int err = adios_schedule_read_byid(this->Impl->File, sel, id,
    step, 1, data);
  ReadError::TestEq(0, err);

  adios_selection_delete(sel);
}

//----------------------------------------------------------------------------
void Reader::ReadArrays(void)
{
  int err = adios_perform_reads(this->Impl->File, 1);
  ReadError::TestEq(0, err);
}

} // End namespace ADIOS
