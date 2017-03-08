/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPSurfaceLICComposite.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPSurfaceLICComposite.h"

#include "vtkObjectFactory.h"
#include "vtkPixelExtent.h"
#include "vtkPPixelTransfer.h"
#include "vtkPainterCommunicator.h"
#include "vtkPPainterCommunicator.h"
#include "vtkRenderingOpenGLConfigure.h"
#include "vtkRenderWindow.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkTextureObject.h"
#include "vtkPixelBufferObject.h"
#include "vtkRenderbuffer.h"
#include "vtkMPI.h"


#ifdef VTK_OPENGL2
# include "vtkOpenGLFramebufferObject.h"
# include "vtkOpenGLRenderUtilities.h"
# include "vtkOpenGLHelper.h"
# include "vtkOpenGLShaderCache.h"
# include "vtkShaderProgram.h"
# include "vtkTextureObjectVS.h"
# include "vtkPSurfaceLICComposite_CompFS.h"
#else
# include "vtkFrameBufferObject2.h"
# include "vtkShader2.h"
# include "vtkShaderProgram2.h"
# include "vtkUniformVariables.h"
# include "vtkShader2Collection.h"
# include "vtkOpenGLExtensionManager.h"
# include "vtkgl.h"
// compositing shader
extern const char *vtkPSurfaceLICComposite_Comp;
# ifndef GL_FRAMEBUFFER
#  define GL_FRAMEBUFFER vtkgl::FRAMEBUFFER_EXT
# endif
# ifndef GL_DRAW_FRAMEBUFFER
#  define GL_DRAW_FRAMEBUFFER vtkgl::DRAW_FRAMEBUFFER_EXT
# endif
# ifndef GL_TEXTURE0
#  define GL_TEXTURE0 vtkgl::TEXTURE0
# endif
#endif


#include <list>
#include <deque>
#include <vector>
#include <utility>
#include <algorithm>
#include <cstddef>

using std::list;
using std::deque;
using std::vector;
using std::pair;

// use parallel timer for benchmarks and scaling
// if not defined vtkTimerLOG is used.
// #define vtkSurfaceLICPainterTIME
#if defined(vtkSurfaceLICPainterTIME)
#include "vtkParallelTimer.h"
#endif

// Enable debug output.
// 1 decomp extents, 2 +intermediate compositing steps
#define vtkPSurfaceLICCompositeDEBUG 0
#if vtkPSurfaceLICCompositeDEBUG>=1
#include "vtkPixelExtentIO.h"
#endif
#if vtkPSurfaceLICCompositeDEBUG>=2
#include "vtkTextureIO.h"
#include <sstream>
using std::ostringstream;
using std::string;
//----------------------------------------------------------------------------
static
string mpifn(int rank, const char *fn)
{
  ostringstream oss;
  oss << rank << "_" << fn;
  return oss.str();
}
#endif

// use PBO's for MPI communication.

#define PBO_RECV_BUFFERS

// isolate this class's communications.
// this is a non-scalable operation so
// only use it for debugging.

// #define DUPLICATE_COMMUNICATOR

// ***************************************************************************
static
int maxNumPasses(){ return 100; }

// ***************************************************************************
static
int encodeTag(int id, int tagBase)
{
  return maxNumPasses()*(id+1)+tagBase;
}

// ***************************************************************************
static
int decodeTag(int tag, int tagBase)
{
  return (tag-tagBase)/maxNumPasses() - 1;
}

// to sort rank/extent pairs by extent size
// ***************************************************************************
static
bool operator<(
      const pair<int, vtkPixelExtent> &l,
      const pair<int, vtkPixelExtent> &r)
{
  return l.second<r.second;
}

// In Windows our callback must use the same calling convention
// as the MPI library. Currently this is only an issue with
// MS MPI which uses __stdcall/__fastcall other's use __cdecl
// which match VTK's defaults.
#ifndef MPIAPI
#define MPIAPI
#endif
// for parallel union of extents
// ***************************************************************************
static void MPIAPI
vtkPixelExtentUnion(void *in, void *out, int *len, MPI_Datatype *type)
{
  (void)type; // known to be MPI_INT
  int n = *len/4;
  for (int i=0; i<n; ++i)
  {
    int ii = 4*i;
    vtkPixelExtent lhs(((int*)in)+ii);
    vtkPixelExtent rhs(((int*)out)+ii);
    rhs |= lhs;
    rhs.GetData(((int*)out)+ii);
  }
}

// Description:
// Container for our custom MPI_Op's
class vtkPPixelExtentOps
{
public:
  vtkPPixelExtentOps() : Union(MPI_OP_NULL) {}
  ~vtkPPixelExtentOps();

  // Description:
  // Create/Delete the custom operations. If these
  // methods are used before MPI initialize or after
  // MPI finalize they have no affect.
  void CreateOps();
  void DeleteOps();

  // Description:
  // Get the operator for performing parallel
  // unions.
  MPI_Op GetUnion(){ return this->Union; }

private:
  MPI_Op Union;
};

// ---------------------------------------------------------------------------
vtkPPixelExtentOps::~vtkPPixelExtentOps()
{
  this->DeleteOps();
}

// ---------------------------------------------------------------------------
void vtkPPixelExtentOps::CreateOps()
{
  if ( (this->Union == MPI_OP_NULL)
    && vtkPPainterCommunicator::MPIInitialized() )
  {
    MPI_Op_create(vtkPixelExtentUnion, 1, &this->Union);
  }
}

// ---------------------------------------------------------------------------
void vtkPPixelExtentOps::DeleteOps()
{
  if ( (this->Union != MPI_OP_NULL)
     && vtkPPainterCommunicator::MPIInitialized()
     && !vtkPPainterCommunicator::MPIFinalized() )
  {
    MPI_Op_free(&this->Union);
  }
}

// ****************************************************************************
void MPITypeFree(deque<MPI_Datatype> &types)
{
  size_t n = types.size();
  for (size_t i=0; i<n; ++i)
  {
    MPI_Type_free(&types[i]);
  }
}

// ****************************************************************************
static
size_t Size(deque< deque<vtkPixelExtent> > exts)
{
  size_t np = 0;
  size_t nr = exts.size();
  for (size_t r=0; r<nr; ++r)
  {
    const deque<vtkPixelExtent> &rexts = exts[r];
    size_t ne = rexts.size();
    for (size_t e=0; e<ne; ++e)
    {
      np += rexts[e].Size();
    }
  }
  return np;
}

#if vtkPSurfaceLICCompositeDEBUG>=1 || defined(vtkSurfaceLICPainterTIME)
// ****************************************************************************
static
int NumberOfExtents(deque< deque<vtkPixelExtent> > exts)
{
  size_t ne = 0;
  size_t nr = exts.size();
  for (size_t r=0; r<nr; ++r)
  {
    ne += exts[r].size();
  }
  return static_cast<int>(ne);
}
#endif

#if vtkPSurfaceLICCompositeDEBUG>0
// ****************************************************************************
static
ostream &operator<<(ostream &os, const vector<float> &vf)
{
  size_t n = vf.size();
  if (n)
  {
    os << vf[0];
  }
  for (size_t i=1; i<n; ++i)
  {
    os << ", " << vf[i];
  }
  return os;
}

// ****************************************************************************
static
ostream &operator<<(ostream &os, const vector<vector<float> >  &vvf)
{
  size_t n = vvf.size();
  for (size_t i=0; i<n; ++i)
  {
    os << i << " = {" << vvf[i] << "}" << endl;
  }
  return os;
}
#endif

#if vtkPSurfaceLICCompositeDEBUG>=2
// ****************************************************************************
static
int ScanMPIStatusForError(vector<MPI_Status> &stat)
{
  int nStats = stat.size();
  for (int q=0; q<nStats; ++q)
  {
    int ierr = stat[q].MPI_ERROR;
    if ((ierr != MPI_SUCCESS) && (ierr != MPI_ERR_PENDING))
    {
      char eStr[MPI_MAX_ERROR_STRING] = {'\0'};
      int eStrLen = 0;
      MPI_Error_string(ierr, eStr, &eStrLen);
      cerr
        << "transaction for request " << q << " failed." << endl
        << eStr << endl
        << endl;
      return -1;
    }
  }
  return 0;
}
#endif


//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPSurfaceLICComposite);

// ----------------------------------------------------------------------------
vtkPSurfaceLICComposite::vtkPSurfaceLICComposite()
        :
     vtkSurfaceLICComposite(),
     PainterComm(NULL),
     PixelOps(NULL),
     CommRank(0),
     CommSize(1),
     Context(NULL),
     FBO(NULL),
     CompositeShader(NULL)
{
  this->PainterComm = new vtkPPainterCommunicator;
  this->PixelOps = new vtkPPixelExtentOps;
}

// ----------------------------------------------------------------------------
vtkPSurfaceLICComposite::~vtkPSurfaceLICComposite()
{
  delete this->PainterComm;
  delete this->PixelOps;
  if (this->CompositeShader)
  {
#ifdef VTK_OPENGL2
    delete this->CompositeShader;
#else
    this->CompositeShader->Delete();
#endif
    this->CompositeShader = 0;
  }
  if (this->FBO)
  {
    this->FBO->Delete();
  }
}

// ----------------------------------------------------------------------------
void vtkPSurfaceLICComposite::SetCommunicator(vtkPainterCommunicator *comm)
{
  #if DUPLICATE_COMMUNICATOR
  this->PainterComm->Duplicate(comm);
  #else
  this->PainterComm->Copy(comm, false);
  #endif
  this->CommRank = this->PainterComm->GetRank();
  this->CommSize = this->PainterComm->GetSize();
  // do this here since we know that
  // mpi is initialized by now.
  this->PixelOps->CreateOps();
}

// ----------------------------------------------------------------------------
void vtkPSurfaceLICComposite::SetContext(vtkOpenGLRenderWindow *rwin)
{
  if (this->Context == rwin)
  {
    return;
  }
  this->Context = rwin;

  // free the existing shader and fbo
  if ( this->CompositeShader )
  {
#ifdef VTK_OPENGL2
    this->CompositeShader->ReleaseGraphicsResources(rwin);
    delete this->CompositeShader;
#else
    this->CompositeShader->Delete();
#endif
    this->CompositeShader = NULL;
  }

  if ( this->FBO )
  {
    this->FBO->Delete();
    this->FBO = NULL;
  }

  if ( this->Context )
  {
    // load, compile, and link the shader
#ifdef VTK_OPENGL2
    this->CompositeShader = new vtkOpenGLHelper;
    std::string GSSource;
    this->CompositeShader->Program =
        rwin->GetShaderCache()->ReadyShaderProgram(vtkTextureObjectVS,
                                          vtkPSurfaceLICComposite_CompFS,
                                            GSSource.c_str());

    // setup a FBO for rendering
    this->FBO = vtkOpenGLFramebufferObject::New();
#else
    vtkShader2 *compositeShaderSrc = vtkShader2::New();
    compositeShaderSrc->SetContext(this->Context);
    compositeShaderSrc->SetType(VTK_SHADER_TYPE_FRAGMENT);
    compositeShaderSrc->SetSourceCode(vtkPSurfaceLICComposite_Comp);

    this->CompositeShader = vtkShaderProgram2::New();
    this->CompositeShader->SetContext(this->Context);
    this->CompositeShader->GetShaders()->AddItem(compositeShaderSrc);
    this->CompositeShader->Build();

    compositeShaderSrc->Delete();

    // setup a FBO for rendering
    this->FBO = vtkFrameBufferObject2::New();
#endif

    this->FBO->SetContext(this->Context);
  }
}

// ----------------------------------------------------------------------------
int vtkPSurfaceLICComposite::AllGatherExtents(
        const deque<vtkPixelExtent> &localExts,
        deque<deque<vtkPixelExtent> >&remoteExts,
        vtkPixelExtent &dataSetExt)
{
  #if vtkPSurfaceLICCompositeDEBUG>=2
  cerr << "=====vtkPSurfaceLICComposite::AllGatherExtents" << endl;
  #endif

  // serialize the local extents
  int nLocal = static_cast<int>(localExts.size());
  int localSize = 4*nLocal;
  int *sendBuf = static_cast<int*>(malloc(localSize*sizeof(int)));
  for (int i=0; i<nLocal; ++i)
  {
    localExts[i].GetData(sendBuf+4*i);
  }

  // share local extent counts
  MPI_Comm comm = *(static_cast<MPI_Comm*>(this->PainterComm->GetCommunicator()));
  int *nRemote = static_cast<int*>(malloc(this->CommSize*sizeof(int)));

  MPI_Allgather(
        &nLocal,
        1,
        MPI_INT,
        nRemote,
        1,
        MPI_INT,
        comm);

  // allocate a bufffer to receive the remote exts
  int *recvCounts = static_cast<int*>(malloc(this->CommSize*sizeof(int)));
  int *recvDispls = static_cast<int*>(malloc(this->CommSize*sizeof(int)));
  int bufSize = 0;
  for (int i=0; i<this->CommSize; ++i)
  {
    int n = 4*nRemote[i];
    recvCounts[i] = n;
    recvDispls[i] = bufSize;
    bufSize += n;
  }
  int *recvBuf = static_cast<int*>(malloc(bufSize*sizeof(int)));

  // collect remote extents
  MPI_Allgatherv(
        sendBuf,
        localSize,
        MPI_INT,
        recvBuf,
        recvCounts,
        recvDispls,
        MPI_INT,
        comm);

  // de-serialize the set of extents
  dataSetExt.Clear();
  remoteExts.resize(this->CommSize);
  for (int i=0; i<this->CommSize; ++i)
  {
    int nRemt = recvCounts[i]/4;
    remoteExts[i].resize(nRemt);

    int *pBuf = recvBuf+recvDispls[i];

    for (int j=0; j<nRemt; ++j)
    {
      vtkPixelExtent &remoteExt = remoteExts[i][j];
      remoteExt.SetData(pBuf+4*j);
      dataSetExt |= remoteExt;
    }
  }

  free(sendBuf);
  free(nRemote);
  free(recvCounts);
  free(recvDispls);
  free(recvBuf);

  return 0;
}

// ----------------------------------------------------------------------------
int vtkPSurfaceLICComposite::AllReduceVectorMax(
    const deque<vtkPixelExtent> &originalExts, // local data
    const deque<deque<vtkPixelExtent> > &newExts, // all composited regions
    float *vectors,
    vector<vector<float> > &vectorMax)
{
  #if vtkPSurfaceLICCompositeDEBUG>=2
  cerr << "=====vtkPSurfaceLICComposite::AllReduceVectorMax" << endl;
  #endif

  // vector data is currently on the original decomp (m blocks for n ranks)
  // the new decomp (p blocks for n ranks), for each of the p new blocks
  // each rank computes the max on this region, a reduction is made to get
  // the true value.
  size_t nOriginal = originalExts.size();
  MPI_Comm comm = *(static_cast<MPI_Comm*>(this->PainterComm->GetCommunicator()));
  vector<vector<float> > tmpMax(this->CommSize);
  for (int r=0; r<this->CommSize; ++r)
  {
    // check the intersection of each new extent with that of each
    // original extent. data for origial extent is local.
    size_t nNew = newExts[r].size();
    tmpMax[r].resize(nNew, -VTK_FLOAT_MAX);
    for (size_t n=0; n<nNew; ++n)
    {
      const vtkPixelExtent &newExt = newExts[r][n];
      float eMax = -VTK_FLOAT_MAX;
      for (size_t o=0; o<nOriginal; ++o)
      {
        vtkPixelExtent intExt(originalExts[o]);
        intExt &= newExt;
        if (!intExt.Empty())
        {
          float oMax = this->VectorMax(intExt, vectors);
          eMax = eMax<oMax ? oMax : eMax;
        }
      }

      MPI_Allreduce(
            MPI_IN_PLACE,
            &eMax,
            1,
            MPI_FLOAT,
            MPI_MAX,
            comm);

      tmpMax[r][n] = eMax;
    }
  }

  // since integration run's into other blocks data use the max of the
  // block and it's neighbors for guard cell size computation
  vectorMax.resize(this->CommSize);
  for (int r=0; r<this->CommSize; ++r)
  {
    size_t nNew = newExts[r].size();
    vectorMax[r].resize(nNew);
    for (size_t n=0; n<nNew; ++n)
    {
      vtkPixelExtent newExt = newExts[r][n];
      newExt.Grow(1);

      float eMax = tmpMax[r][n];

      // find neighbors
      for (int R=0; R<this->CommSize; ++R)
      {
        size_t NNew = newExts[R].size();
        for (size_t N=0; N<NNew; ++N)
        {
          vtkPixelExtent intExt(newExts[R][N]);
          intExt &= newExt;

          if (!intExt.Empty())
          {
            // this is a neighbor(or self), take the larger of ours
            // and theirs
            float nMax = tmpMax[R][N];
            eMax = eMax<nMax ? nMax : eMax;
          }
        }
      }

      vectorMax[r][n] = eMax;
    }
  }

  return 0;
}

// ----------------------------------------------------------------------------
int vtkPSurfaceLICComposite::DecomposeExtent(
      vtkPixelExtent &in,
      int nPieces,
      list<vtkPixelExtent> &out)
{
  #if vtkPSurfaceLICCompositeDEBUG>=2
  cerr << "=====vtkPSurfaceLICComposite::DecomposeWindowExtent" << endl;
  #endif

  int res[3];
  in.Size(res);

  int nPasses[2] = {0,0};
  int maxPasses[2] = {res[0]/2, res[1]/2};

  out.push_back(in);

  list<vtkPixelExtent> splitExts;

  int dir=0;
  while(1)
  {
    // stop when we have enough out or all out have unit size
    int nExts = static_cast<int>(out.size());
    if ( (nExts >= nPieces)
     || ((nPasses[0] > maxPasses[0]) && (nPasses[1] > maxPasses[1])) )
    {
      break;
    }

    for (int i=0; i<nExts; ++i)
    {
      int nExtsTotal = static_cast<int>(out.size() + splitExts.size());
      if (nExtsTotal >= nPieces)
      {
        break;
      }

      // split this ext into two
      vtkPixelExtent ext = out.back();
      out.pop_back();

      vtkPixelExtent newExt = ext.Split(dir);

      splitExts.push_back(ext);

      if (!newExt.Empty())
      {
        splitExts.push_back(newExt);
      }
    }

    // transfer the split out to the head so that
    // they are split again only after others.
    out.insert(out.begin(), splitExts.begin(), splitExts.end());
    splitExts.clear();

    nPasses[dir] += 1;

    // alternate splitting direction
    dir = (dir + 1) % 2;
    if (nPasses[dir] > maxPasses[dir])
    {
      dir = (dir + 1) % 2;
    }
  }

  return 0;
}

// ----------------------------------------------------------------------------
int vtkPSurfaceLICComposite::DecomposeScreenExtent(
      deque< deque<vtkPixelExtent> >&newExts,
      float *vectors)
{
  #if vtkPSurfaceLICCompositeDEBUG>=2
  cerr << "=====vtkPSurfaceLICComposite::DecomposeWindowExtent" << endl;
  #endif

  // TODO -- the balanced compositor is not finished. details
  // below.
  (void)vectors;

  // use 128x128 extents
  int dataSetSize[2];
  this->DataSetExt.Size(dataSetSize);

  int ni = dataSetSize[0]/128;
  ni = ni<1 ? 1 : ni;

  int nj = dataSetSize[1]/128;
  nj = nj<1 ? 1 : nj;

  int nPieces = ni*nj;
  nPieces = nPieces<this->CommSize ? this->CommSize : nPieces;

  // decompose
  list<vtkPixelExtent> tmpOut0;
  this->DecomposeExtent(this->DataSetExt, nPieces, tmpOut0);

  // make the assignment to ranks
  int nPer = nPieces/this->CommSize;
  int nLarge = nPieces%this->CommSize;

  deque<deque<vtkPixelExtent> > tmpOut1;
  tmpOut1.resize(this->CommSize);

  int N = static_cast<int>(tmpOut0.size());
  list<vtkPixelExtent>::iterator it = tmpOut0.begin();

  for (int r=0; r<this->CommSize; ++r)
  {
    int n = nPer;
    if (r < nLarge)
    {
      ++n;
    }
    for (int i=0; (i<n) && (N>0); ++i,--N,++it)
    {
      tmpOut1[r].push_back(*it);
    }
  }

  // TODO -- we need to implement some sore of load
  // balancing here.
  // compute tight extents and assign to ranks based on weight
  // and location
  newExts = tmpOut1;

  return 0;
}

// ----------------------------------------------------------------------------
int vtkPSurfaceLICComposite::MakeDecompLocallyDisjoint(
     const deque< deque< vtkPixelExtent> > &in,
     deque< deque< vtkPixelExtent> > &out)
{
  size_t nr = in.size();
  out.clear();
  out.resize(nr);
  for (size_t r=0; r<nr; ++r)
  {
    deque<vtkPixelExtent> tmp(in[r]);
    this->MakeDecompDisjoint(tmp, out[r]);
  }
  return 0;
}

// ----------------------------------------------------------------------------
int vtkPSurfaceLICComposite::MakeDecompDisjoint(
     const deque< deque< vtkPixelExtent> > &in,
     deque< deque< vtkPixelExtent> > &out,
     float *vectors)
{
  // flatten
  deque<pair<int, vtkPixelExtent> > tmpIn;
  for (int r=0; r<this->CommSize; ++r)
  {
    const deque<vtkPixelExtent> &blocks = in[r];
    size_t nBlocks = blocks.size();
    for (size_t b=0; b<nBlocks; ++b)
    {
      pair<int, vtkPixelExtent> elem(r, blocks[b]);
      tmpIn.push_back(elem);
    }
  }
  // sort by size
  sort(tmpIn.begin(), tmpIn.end());

  // from largest to smallest, make it disjoint
  // to others
  deque<pair<int, vtkPixelExtent> > tmpOut0;

  while ( !tmpIn.empty() )
  {
    // largest element
    int rank = tmpIn.back().first;
    deque<vtkPixelExtent> tmpOut1(1,tmpIn.back().second);

    tmpIn.pop_back();

    // subtract smaller elements
    size_t ns = tmpIn.size();
    for (size_t se=0; se<ns; ++se)
    {
      vtkPixelExtent &selem = tmpIn[se].second;
      deque<vtkPixelExtent> tmpOut2;
      size_t nl = tmpOut1.size();
      for (size_t le=0; le<nl; ++le)
      {
        vtkPixelExtent &lelem = tmpOut1[le];
        vtkPixelExtent::Subtract(lelem, selem, tmpOut2);
      }
      tmpOut1 = tmpOut2;
    }

    // move to output
    size_t nn = tmpOut1.size();
    for (size_t ne=0; ne<nn; ++ne)
    {
      pair<int, vtkPixelExtent> elem(rank, tmpOut1[ne]);
      tmpOut0.push_back(elem);
    }
  }

  // reduce communication and compositing overhead by
  // shrinking the new set of extents to tightly bound the
  // data on it's new/future layout.
  int nx[2];
  this->WindowExt.Size(nx);

  const deque<vtkPixelExtent> &inR = in[this->CommRank];
  size_t ni = inR.size();

  deque<pair<int, vtkPixelExtent> > tmpOut1(tmpOut0);
  size_t ne = tmpOut1.size();
  for (size_t e=0; e<ne; ++e)
  {
    vtkPixelExtent &newExt = tmpOut1[e].second;
    vtkPixelExtent tightExt;
    for (size_t i=0; i<ni; ++i)
    {
      vtkPixelExtent inExt(inR[i]);
      inExt &= newExt;
      if (!inExt.Empty())
      {
        GetPixelBounds(vectors, nx[0], inExt);
        tightExt |= inExt; // accumulate the contrib from local data
      }
    }
    newExt = tightExt;
  }

  // accumulate contrib from remote data
  size_t remSize = 4*ne;
  vector<int> rem(remSize);
  int *pRem = ne ? &rem[0] : NULL;
  for (size_t e=0; e<ne; ++e, pRem+=4)
  {
    tmpOut1[e].second.GetData(pRem);
  }
  MPI_Comm comm = *(static_cast<MPI_Comm*>(this->PainterComm->GetCommunicator()));
  MPI_Op parUnion = this->PixelOps->GetUnion();
  MPI_Allreduce(
        MPI_IN_PLACE,
        ne ? &rem[0] : NULL,
        (int)remSize,
        MPI_INT,
        parUnion,
        comm);

  // move from flat order back to rank indexed order and remove
  // empty extents
  pRem = ne ? &rem[0] : NULL;
  out.resize(this->CommSize);
  for (size_t e=0; e<ne; ++e, pRem+=4)
  {
    int r = tmpOut1[e].first;
    vtkPixelExtent ext(pRem);
    if (!ext.Empty())
    {
      out[r].push_back(ext);
    }
  }

  // merge compatible extents
  for (int r=0; r<this->CommSize; ++r)
  {
    vtkPixelExtent::Merge(out[r]);
  }

  return 0;
}

// ----------------------------------------------------------------------------
int vtkPSurfaceLICComposite::AddGuardPixels(
      const deque<deque<vtkPixelExtent> > &exts,
      deque<deque<vtkPixelExtent> > &guardExts,
      deque<deque<vtkPixelExtent> > &disjointGuardExts,
      float *vectors)
{
  #if vtkPSurfaceLICCompositeDEBUG>=2
  cerr << "=====vtkPSurfaceLICComposite::AddGuardPixels" << endl;
  #endif
  #ifdef vtkSurfaceLICPainterTIME
  vtkParallelTimer *log = vtkParallelTimer::GetGlobalInstance();
  #endif

  guardExts.resize(this->CommSize);
  disjointGuardExts.resize(this->CommSize);

  int nx[2];
  this->WindowExt.Size(nx);
  float fudge = this->GetFudgeFactor(nx);
  #if vtkPSurfaceLICCompositeDEBUG>=2
  cerr << " fudge=" << fudge << endl;
  #endif

  float arc
    = this->StepSize*this->NumberOfSteps*this->NumberOfGuardLevels*fudge;

  if (this->NormalizeVectors)
  {
    // when normalizing velocity is always 1, all extents have the
    // same number of guard cells.
    int ng
      = static_cast<int>(arc)
      + this->NumberOfEEGuardPixels
      + this->NumberOfAAGuardPixels;
    ng = ng<2 ? 2 : ng;
    #ifdef vtkSurfaceLICPainterTIME
    log->GetHeader() << "ng=" << ng << "\n";
    #endif
    #if vtkPSurfaceLICCompositeDEBUG>=2
    cerr << "ng=" << ng << endl;
    #endif
    for (int r=0; r<this->CommSize; ++r)
    {
      deque<vtkPixelExtent> tmpExts(exts[r]);
      int nExts = static_cast<int>(tmpExts.size());
      // add guard pixles
      for (int b=0; b<nExts; ++b)
      {
        tmpExts[b].Grow(ng);
        tmpExts[b] &= this->DataSetExt;
      }
      guardExts[r] = tmpExts;
      // make sure it's disjoint
      disjointGuardExts[r].clear();
      this->MakeDecompDisjoint(tmpExts, disjointGuardExts[r]);
    }
  }
  else
  {
    // when not normailzing during integration we need max(V) on the LIC
    // decomp. Each domain has the potential to require a unique number
    // of guard cells.
    vector<vector<float> > vectorMax;
    this->AllReduceVectorMax(
            this->BlockExts,
            exts,
            vectors,
            vectorMax);

    #ifdef vtkSurfaceLICPainterTIME
    log->GetHeader() << "ng=";
    #endif
    #if vtkPSurfaceLICCompositeDEBUG>=2
    cerr << "ng=";
    #endif
    for (int r=0; r<this->CommSize; ++r)
    {
      deque<vtkPixelExtent> tmpExts(exts[r]);
      size_t nExts = tmpExts.size();
      for (size_t b=0; b<nExts; ++b)
      {
        int ng
          = static_cast<int>(vectorMax[r][b]*arc)
          + this->NumberOfEEGuardPixels
          + this->NumberOfAAGuardPixels;;
        ng = ng<2 ? 2 : ng;
        #ifdef vtkSurfaceLICPainterTIME
        log->GetHeader() << " " << ng;
        #endif
        #if vtkPSurfaceLICCompositeDEBUG>=2
        cerr << "  " << ng;
        #endif
        tmpExts[b].Grow(ng);
        tmpExts[b] &= this->DataSetExt;
      }
      guardExts[r] = tmpExts;
      // make sure it's disjoint
      disjointGuardExts[r].clear();
      this->MakeDecompDisjoint(tmpExts, disjointGuardExts[r]);
    }
    #ifdef vtkSurfaceLICPainterTIME
    log->GetHeader() << "\n";
    #endif
    #if vtkPSurfaceLICCompositeDEBUG>=2
    cerr << endl;
    #endif
  }

  return 0;
}

// ----------------------------------------------------------------------------
double vtkPSurfaceLICComposite::EstimateCommunicationCost(
      const deque<deque<vtkPixelExtent> > &srcExts,
      const deque<deque<vtkPixelExtent> > &destExts)
{
  // compute the number off rank overlaping pixels, this is the
  // the number of pixels that need to be communicated. This is
  // not the number of pixels to be composited since some of those
  // may be on-rank.

  size_t total = 0;
  size_t overlap = 0;

  for (int sr=0; sr<this->CommSize; ++sr)
  {
    size_t nse = srcExts[sr].size();
    for (size_t se=0; se<nse; ++se)
    {
      const vtkPixelExtent &srcExt = srcExts[sr][se];
      total += srcExt.Size(); // count all pixels in the total

      for (int dr=0; dr<this->CommSize; ++dr)
      {
        // only off rank overlap incurrs comm cost
        if (sr == dr)
        {
          continue;
        }

        size_t nde = destExts[dr].size();
        for (size_t de=0; de<nde; ++de)
        {
          vtkPixelExtent destExt = destExts[dr][de];
          destExt &= srcExt;
          if (!destExt.Empty())
          {
            overlap += destExt.Size(); // cost is number of overlap pixels
          }
        }
      }
    }
  }

  return (static_cast<double>(overlap))/(static_cast<double>(total));
}

// ----------------------------------------------------------------------------
double vtkPSurfaceLICComposite::EstimateDecompEfficiency(
      const deque< deque<vtkPixelExtent> > &exts,
      const deque< deque<vtkPixelExtent> > &guardExts)
{
  // number of pixels in the domain decomp
  double ne = static_cast<double>(Size(exts));
  double nge = static_cast<double>(Size(guardExts));

  // efficiency is the ratio of valid pixels
  // to guard pixels
  return ne/fabs(ne - nge);
}

// ----------------------------------------------------------------------------
int vtkPSurfaceLICComposite::BuildProgram(float *vectors)
{
  #if vtkPSurfaceLICCompositeDEBUG>=2
  cerr << "=====vtkPSurfaceLICComposite::BuildProgram" << endl;
  #endif

  #ifdef vtkSurfaceLICPainterTIME
  vtkParallelTimer *log = vtkParallelTimer::GetGlobalInstance();
  #endif

  // gather current geometry extents, compute the whole extent
  deque<deque<vtkPixelExtent> >allBlockExts;
  this->AllGatherExtents(
        this->BlockExts,
        allBlockExts,
        this->DataSetExt);

  if (this->Strategy == COMPOSITE_AUTO)
  {
    double commCost = this->EstimateCommunicationCost(allBlockExts, allBlockExts);
    #ifdef vtkSurfaceLICPainterTIME
    log->GetHeader() << "in-place comm cost=" << commCost << "\n";
    #endif
    #if vtkPSurfaceLICCompositeDEBUG>=2
    cerr << "in-place comm cost=" << commCost << endl;
    #endif
    if (commCost <= 0.3)
    {
      this->Strategy = COMPOSITE_INPLACE;
      #ifdef vtkSurfaceLICPainterTIME
      log->GetHeader() << "using in-place composite\n";
      #endif
      #if vtkPSurfaceLICCompositeDEBUG>=2
      cerr << "using in-place composite" << endl;
      #endif
    }
    else
    {
      this->Strategy = COMPOSITE_INPLACE_DISJOINT;
      #ifdef vtkSurfaceLICPainterTIME
      log->GetHeader() << "using disjoint composite\n";
      #endif
      #if vtkPSurfaceLICCompositeDEBUG>=2
      cerr << "using disjoint composite" << endl;
      #endif
    }
  }

  // decompose the screen
  deque< deque<vtkPixelExtent> > newExts;
  switch (this->Strategy)
  {
    case COMPOSITE_INPLACE:
      // make it locally disjoint to avoid redundant computation
      this->MakeDecompLocallyDisjoint(allBlockExts, newExts);
      break;

    case COMPOSITE_INPLACE_DISJOINT:
      this->MakeDecompDisjoint(allBlockExts, newExts, vectors);
      break;

    case COMPOSITE_BALANCED:
      this->DecomposeScreenExtent(newExts, vectors);
      break;

    default:
      return -1;
  }

  #if defined(vtkSurfaceLICPainterTIME) || vtkPSurfaceLICCompositeDEBUG>=2
  double commCost = this->EstimateCommunicationCost(allBlockExts, newExts);
  #endif
  #ifdef vtkSurfaceLICPainterTIME
  log->GetHeader() << "actual comm cost=" << commCost << "\n";
  #endif
  #if vtkPSurfaceLICCompositeDEBUG>=2
  cerr << "actual comm cost=" << commCost << endl;
  #endif

  // save the local decomp
  // it's the valid region as no guard pixels were added
  this->CompositeExt = newExts[this->CommRank];

  int id=0;
  this->ScatterProgram.clear();
  if (this->Strategy != COMPOSITE_INPLACE)
  {
    // construct program describing communication patterns that are
    // required to move data to geometry decomp from the new lic
    // decomp after LIC
    for (int srcRank=0; srcRank<this->CommSize; ++srcRank)
    {
      deque<vtkPixelExtent> &srcBlocks = newExts[srcRank];
      int nSrcBlocks = static_cast<int>(srcBlocks.size());

      for (int sb=0; sb<nSrcBlocks; ++sb)
      {
        const vtkPixelExtent &srcExt = srcBlocks[sb];

        for (int destRank=0; destRank<this->CommSize; ++destRank)
        {
          int nBlocks = static_cast<int>(allBlockExts[destRank].size());
          for (int b=0; b<nBlocks; ++b)
          {
            const vtkPixelExtent &destExt = allBlockExts[destRank][b];

            vtkPixelExtent sharedExt(destExt);
            sharedExt &= srcExt;

            if (!sharedExt.Empty())
            {
              this->ScatterProgram.push_back(
                    vtkPPixelTransfer(
                          srcRank,
                          this->WindowExt,
                          sharedExt,
                          destRank,
                          this->WindowExt,
                          sharedExt,
                          id));
            }
            id += 1;
          }
        }
      }
    }
  }

  #if vtkPSurfaceLICCompositeDEBUG>=1
  vtkPixelExtentIO::Write(this->CommRank, "ViewExtent.vtk", this->WindowExt);
  vtkPixelExtentIO::Write(this->CommRank, "GeometryDecomp.vtk", allBlockExts);
  vtkPixelExtentIO::Write(this->CommRank, "LICDecomp.vtk", newExts);
  #endif

  // add guard cells to the new decomp that prevent artifacts
  deque<deque<vtkPixelExtent> > guardExts;
  deque<deque<vtkPixelExtent> > disjointGuardExts;
  this->AddGuardPixels(newExts, guardExts, disjointGuardExts, vectors);

  #if vtkPSurfaceLICCompositeDEBUG>=1
  vtkPixelExtentIO::Write(this->CommRank, "LICDecompGuard.vtk", guardExts);
  vtkPixelExtentIO::Write(this->CommRank, "LICDisjointDecompGuard.vtk", disjointGuardExts);
  #endif

  #if defined(vtkSurfaceLICPainterTIME) || vtkPSurfaceLICCompositeDEBUG>=2
  double efficiency = this->EstimateDecompEfficiency(newExts, disjointGuardExts);
  size_t nNewExts = NumberOfExtents(newExts);
  #endif
  #if defined(vtkSurfaceLICPainterTIME)
  log->GetHeader()
    << "decompEfficiency=" << efficiency << "\n"
    << "numberOfExtents=" << nNewExts << "\n";
  #endif
  #if vtkPSurfaceLICCompositeDEBUG>=2
  cerr
    << "decompEfficiency=" << efficiency << endl
    << "numberOfExtents=" << nNewExts << endl;
  #endif

  // save the local decomp with guard cells
  this->GuardExt = guardExts[this->CommRank];
  this->DisjointGuardExt = disjointGuardExts[this->CommRank];

  // construct program describing communication patterns that are
  // required to move data from the geometry decomp to the new
  // disjoint decomp containing guard pixels
  this->GatherProgram.clear();
  id=0;
  for (int destRank=0; destRank<this->CommSize; ++destRank)
  {
    deque<vtkPixelExtent> &destBlocks = disjointGuardExts[destRank];
    int nDestBlocks = static_cast<int>(destBlocks.size());

    for (int db=0; db<nDestBlocks; ++db)
    {
      const vtkPixelExtent &destExt = destBlocks[db];

      for (int srcRank=0; srcRank<this->CommSize; ++srcRank)
      {
        int nBlocks = static_cast<int>(allBlockExts[srcRank].size());
        for (int b=0; b<nBlocks; ++b)
        {
          const vtkPixelExtent &srcExt = allBlockExts[srcRank][b];

          vtkPixelExtent sharedExt(destExt);
          sharedExt &= srcExt;

          if (!sharedExt.Empty())
          {
            // to move vectors for the LIC decomp
            // into a contiguous recv buffer
            this->GatherProgram.push_back(
                  vtkPPixelTransfer(
                        srcRank,
                        this->WindowExt,
                        sharedExt,
                        destRank,
                        sharedExt, // dest ext
                        sharedExt,
                        id));
          }

          id += 1;
        }
      }
    }
  }

  #if vtkSurfaceLICCompoisteDEBUG>=2
  cerr << *this << endl;
  #endif

  return 0;
}

// ----------------------------------------------------------------------------
int vtkPSurfaceLICComposite::Gather(
        void *pSendPBO,
        int dataType,
        int nComps,
        vtkTextureObject *&newImage)
{
  #if vtkPSurfaceLICCompositeDEBUG>=2
  cerr << "=====vtkPSurfaceLICComposite::Composite" << endl;
  #endif

  // two pipleines depending on if this process recv's or send's
  //
  // send:
  // tex -> pbo -> mpi_send
  //
  // recv:
  // mpi_recv -> pbo -> tex -> composite shader -> fbo

  // pass id is decoded into mpi tag form non-blocking comm
  this->Pass += 1;

  // validate inputs
  if (this->Pass >= maxNumPasses())
  {
    return -1;
  }
  if (pSendPBO == NULL)
  {
    return -2;
  }
  if (this->Context == NULL)
  {
    return -3;
  }
  if (this->CompositeShader == NULL)
  {
    return -4;
  }

  // get the size of the array datatype
  int dataTypeSize = 0;
  switch (dataType)
  {
    vtkTemplateMacro(dataTypeSize = sizeof(VTK_TT););
    default:
      return -5;
  }

  // initiate non-blocking comm
  MPI_Comm comm = *(static_cast<MPI_Comm*>(this->PainterComm->GetCommunicator()));
  int nTransactions = static_cast<int>(this->GatherProgram.size());
  vector<MPI_Request> mpiRecvReqs;
  vector<MPI_Request> mpiSendReqs;
  deque<MPI_Datatype> mpiTypes;
  #ifdef PBO_RECV_BUFFERS
  deque<vtkPixelBufferObject*> recvPBOs(nTransactions, static_cast<vtkPixelBufferObject*>(NULL));
  #else
  deque<void*> recvBufs(nTransactions, static_cast<void*>(NULL));
  #endif
  for (int j=0; j<nTransactions; ++j)
  {
    vtkPPixelTransfer &transaction = this->GatherProgram[j];

    // postpone local transactions, they will be overlapped
    // with transactions requiring communication
    if (transaction.Local(this->CommRank))
    {
      continue;
    }

    #ifdef PBO_RECV_BUFFERS
    void *pRecvPBO = NULL;
    #endif

    // encode transaction.
    int tag = encodeTag(j, this->Pass);

    if ( transaction.Receiver(this->CommRank) )
    {
      // allocate receive buffers
      const vtkPixelExtent &destExt = transaction.GetDestinationExtent();

      unsigned int pboSize = static_cast<unsigned int>(destExt.Size()*nComps);
      unsigned int bufSize = pboSize*dataTypeSize;

      #ifdef PBO_RECV_BUFFERS
      vtkPixelBufferObject *pbo;
      pbo = vtkPixelBufferObject::New();
      pbo->SetContext(this->Context);
      pbo->SetType(dataType);
      pbo->SetComponents(nComps);
      pbo->SetSize(pboSize);
      recvPBOs[j] = pbo;

      pRecvPBO = pbo->MapUnpackedBuffer(bufSize);
      #else
      recvBufs[j] = malloc(bufSize);
      #endif
    }

    vector<MPI_Request> &mpiReqs
      = transaction.Receiver(this->CommRank) ? mpiRecvReqs : mpiSendReqs;

    // start send/recv data
    int iErr = 0;
    iErr = transaction.Execute(
        comm,
        this->CommRank,
        nComps,
        dataType,
        pSendPBO,
        dataType,
        #ifdef PBO_RECV_BUFFERS
        pRecvPBO,
        #else
        recvBufs[j],
        #endif
        mpiReqs,
        mpiTypes,
        tag);
    if (iErr)
    {
      cerr
        << this->CommRank
        << " transaction " << j << ":" << tag
        << " failed " << iErr << endl
        << transaction << endl;
    }
  }

  // overlap framebuffer and shader config with communication
  unsigned int winExtSize[2];
  this->WindowExt.Size(winExtSize);

  if (newImage == NULL)
  {
    newImage = vtkTextureObject::New();
    newImage->SetContext(this->Context);
    newImage->Create2D(
          winExtSize[0],
          winExtSize[1],
          nComps,
          dataType,
          false);
  }

  this->FBO->SaveCurrentBindings();
  this->FBO->Bind(GL_FRAMEBUFFER);
  this->FBO->AddColorAttachment(GL_DRAW_FRAMEBUFFER, 0U, newImage);
  this->FBO->ActivateDrawBuffer(0U);

  vtkRenderbuffer *depthBuf = vtkRenderbuffer::New();
  depthBuf->SetContext(this->Context);
  depthBuf->CreateDepthAttachment(winExtSize[0], winExtSize[1]);
  this->FBO->AddDepthAttachment(GL_DRAW_FRAMEBUFFER, depthBuf);

  vtkCheckFrameBufferStatusMacro(GL_FRAMEBUFFER);

  // the LIC'er requires all fragments in the vector
  // texture to be initialized to 0
  this->FBO->InitializeViewport(winExtSize[0], winExtSize[1]);
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_SCISSOR_TEST);
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

#ifdef VTK_OPENGL2
  this->Context->GetShaderCache()->ReadyShaderProgram(
    this->CompositeShader->Program);
#else
  vtkUniformVariables *uniforms = this->CompositeShader->GetUniformVariables();
  uniforms->SetUniformit("texData", 0);
  this->CompositeShader->Use();
#endif

  // overlap compositing of local data with communication
  for (int j=0; j<nTransactions; ++j)
  {
    vtkPPixelTransfer &transaction = this->GatherProgram[j];

    if (!transaction.Local(this->CommRank))
    {
      continue;
    }

    #if vtkPSurfaceLICCompositeDEBUG>=2
    cerr
      << this->CommRank << ":" << j << ":"
      << encodeTag(j, this->Pass) << " Local " << transaction
      << endl;
    #endif

    const vtkPixelExtent &destExt = transaction.GetDestinationExtent();
    unsigned int pboSize = static_cast<unsigned int>(destExt.Size()*nComps);
    unsigned int bufSize = pboSize*dataTypeSize;

    vtkPixelBufferObject *pbo = vtkPixelBufferObject::New();
    pbo->SetContext(this->Context);
    pbo->SetType(dataType);
    pbo->SetComponents(nComps);
    pbo->SetSize(pboSize);

    void *pRecvPBO = pbo->MapUnpackedBuffer(bufSize);

    int iErr = transaction.Blit(
          nComps,
          dataType,
          pSendPBO,
          dataType,
          pRecvPBO);

    if (iErr)
    {
      cerr
        << this->CommRank
        << " local transaction " << j << ":" << this->Pass
        << " failed " << iErr << endl
        << transaction << endl;
    }

    pbo->UnmapUnpackedBuffer();

    unsigned int destDims[2];
    destExt.Size(destDims);

    vtkTextureObject *tex = vtkTextureObject::New();
    tex->SetContext(this->Context);
    tex->Create2D(destDims[0], destDims[1], nComps, pbo, false);

    pbo->Delete();

    #if vtkPSurfaceLICCompositeDEBUG>=2
    ostringstream oss;
    oss << j << ":" << this->Pass << "_localRecvdData.vtk";
    vtkTextureIO::Write(mpifn(this->CommRank, oss.str().c_str()), tex);
    #endif

    // Compositing because of overlap in guard pixels
    this->ExecuteShader(destExt, tex);

    tex->Delete();
  }

  // composite inflight data as it arrives.
  int nRecvReqs = static_cast<int>(mpiRecvReqs.size());
  for (int i=0; i<nRecvReqs; ++i)
  {
    // wait for the completion of one of the recvs
    MPI_Status stat;
    int reqId;
    int iErr = MPI_Waitany(nRecvReqs, &mpiRecvReqs[0], &reqId, &stat);
    if (iErr)
    {
      vtkErrorMacro("comm error in recv");
    }

    // decode transaction id
    int j = decodeTag(stat.MPI_TAG, this->Pass);
    vtkPPixelTransfer &transaction = this->GatherProgram[j];

    #if vtkPSurfaceLICCompositeDEBUG>=2
    cerr
      << this->CommRank << ":" << j << ":"
      << stat.MPI_TAG << " Recv " << transaction
      << endl;
    #endif

    // move recv'd data from pbo to texture
    const vtkPixelExtent &destExt = transaction.GetDestinationExtent();

    unsigned int destDims[2];
    destExt.Size(destDims);

    #ifdef PBO_RECV_BUFFERS
    vtkPixelBufferObject *&pbo = recvPBOs[j];
    pbo->UnmapUnpackedBuffer();
    #else
    unsigned int pboSize = nComps*destExt.Size();
    unsigned int bufSize = pboSize*dataTypeSize;

    vtkPixelBufferObject *pbo = vtkPixelBufferObject::New();
    pbo->SetContext(this->Context);
    pbo->SetType(dataType);
    pbo->SetComponents(nComps);
    pbo->SetSize(pboSize);

    void *pbuf = pbo->MapUnpackedBuffer(bufSize);

    void *&rbuf = recvBufs[j];

    memcpy(pbuf, rbuf, bufSize);

    pbo->UnmapUnpackedBuffer();

    free(rbuf);
    rbuf = NULL;
    #endif

    vtkTextureObject *tex = vtkTextureObject::New();
    tex->SetContext(this->Context);
    tex->Create2D(destDims[0], destDims[1], nComps, pbo, false);

    pbo->Delete();
    pbo = NULL;

    #if vtkPSurfaceLICCompositeDEBUG>=2
    ostringstream oss;
    oss << j << ":" << this->Pass << "_recvdData.vtk";
    vtkTextureIO::Write(mpifn(this->CommRank, oss.str().c_str()), tex);
    #endif

    this->ExecuteShader(destExt, tex);

    tex->Delete();
  }
#ifndef VTK_OPENGL2
  this->CompositeShader->Restore();
#endif

  this->FBO->DeactivateDrawBuffers();
  this->FBO->RemoveTexColorAttachment(GL_DRAW_FRAMEBUFFER, 0U);
  this->FBO->RemoveRenDepthAttachment(GL_DRAW_FRAMEBUFFER);
  this->FBO->UnBind(GL_FRAMEBUFFER);
  depthBuf->Delete();

  // wait for sends to complete
  int nSendReqs = static_cast<int>(mpiSendReqs.size());
  if (nSendReqs)
  {
    int iErr = MPI_Waitall(nSendReqs, &mpiSendReqs[0], MPI_STATUSES_IGNORE);
    if (iErr)
    {
      vtkErrorMacro("comm error in send");
    }
  }

  MPITypeFree(mpiTypes);

  return 0;
}

// ----------------------------------------------------------------------------
int vtkPSurfaceLICComposite::ExecuteShader(
      const vtkPixelExtent &ext,
      vtkTextureObject *tex)
{
  // cell to node
  vtkPixelExtent next(ext);
  next.CellToNode();

  float fext[4];
  next.GetData(fext);

#ifdef VTK_OPENGL2
  float tcoords[8] =
    {0.0f, 0.0f,
     1.0f, 0.0f,
     1.0f, 1.0f,
     0.0f, 1.0f};

  tex->Activate();
  this->CompositeShader->Program->SetUniformi("texData",
    tex->GetTextureUnit());

  unsigned int winExtSize[2];
  this->WindowExt.Size(winExtSize);

  float verts[] = {
    2.0f*fext[0]/winExtSize[0]-1.0f, 2.0f*fext[2]/winExtSize[1]-1.0f, 0.0f,
    2.0f*(fext[1]+1.0f)/winExtSize[0]-1.0f, 2.0f*fext[2]/winExtSize[1]-1.0f, 0.0f,
    2.0f*(fext[1]+1.0f)/winExtSize[0]-1.0f, 2.0f*(fext[3]+1.0f)/winExtSize[1]-1.0f, 0.0f,
    2.0f*fext[0]/winExtSize[0]-1.0f, 2.0f*(fext[3]+1.0f)/winExtSize[1]-1.0f, 0.0f};

  vtkOpenGLRenderUtilities::RenderQuad(verts, tcoords,
    this->CompositeShader->Program, this->CompositeShader->VAO);
  tex->Deactivate();
#else
  float tcoords[4] = {0.0f,1.0f, 0.0f,1.0f};

  tex->Activate(GL_TEXTURE0);

  int ids[8] = {0,2, 1,2, 1,3, 0,3};

  glBegin(GL_QUADS);
  for (int q=0; q<4; ++q)
  {
    int qq = 2*q;
    glTexCoord2f(tcoords[ids[qq]], tcoords[ids[qq+1]]);
    glVertex2f(fext[ids[qq]], fext[ids[qq+1]]);
  }
  glEnd();

  //tex->Deactivate(GL_TEXTURE0);
#endif

  return 0;
}

// ----------------------------------------------------------------------------
int vtkPSurfaceLICComposite::Scatter(
        void *pSendPBO,
        int dataType,
        int nComps,
        vtkTextureObject *&newImage)
{
  #if vtkPSurfaceLICCompositeDEBUG>=2
  cerr << "=====vtkPSurfaceLICComposite::Scatter" << endl;
  #endif

  int iErr = 0;
  // two pipleines depending on if this process recv's or send's
  //
  // send:
  // tex -> pbo -> mpi_send
  //
  // recv:
  // mpi_recv -> pbo -> tex -> composite shader -> fbo

  // pass id is decoded into mpi tag form non-blocking comm
  this->Pass += 1;

  // validate inputs
  if (this->Pass >= maxNumPasses())
  {
    return -1;
  }
  if (pSendPBO == NULL)
  {
    return -2;
  }
  if (this->Context == NULL)
  {
    return -3;
  }

  // get the size of the array datatype
  int dataTypeSize = 0;
  switch (dataType)
  {
    vtkTemplateMacro(dataTypeSize = sizeof(VTK_TT););
    default:
      return -4;
  }
  unsigned int pboSize = (unsigned int)this->WindowExt.Size()*nComps;
  unsigned int bufSize = pboSize*dataTypeSize;

  #ifdef PBO_RECV_BUFFERS
  vtkPixelBufferObject *recvPBO;
  recvPBO = vtkPixelBufferObject::New();
  recvPBO->SetContext(this->Context);
  recvPBO->SetType(dataType);
  recvPBO->SetComponents(nComps);
  recvPBO->SetSize(pboSize);

  void *pRecvPBO = recvPBO->MapUnpackedBuffer(bufSize);
  memset(pRecvPBO, 0, bufSize);
  #else
  void *pRecvBuf = malloc(bufSize);
  memset(pRecvBuf, 0, bufSize);
  #endif

  // initiate non-blocking comm
  MPI_Comm comm = *(static_cast<MPI_Comm*>(this->PainterComm->GetCommunicator()));
  int nTransactions = static_cast<int>(this->ScatterProgram.size());
  vector<MPI_Request> mpiRecvReqs;
  vector<MPI_Request> mpiSendReqs;
  deque<MPI_Datatype> mpiTypes;
  for (int j=0; j<nTransactions; ++j)
  {
    vtkPPixelTransfer &transaction = this->ScatterProgram[j];

    // postpone local transactions, they will be overlapped
    // with transactions requiring communication
    if (transaction.Local(this->CommRank))
    {
      continue;
    }

    // encode transaction.
    int tag = encodeTag(j, this->Pass);

    vector<MPI_Request> &mpiReqs
      = transaction.Receiver(this->CommRank) ? mpiRecvReqs : mpiSendReqs;

    // start send/recv data
    iErr = transaction.Execute(
        comm,
        this->CommRank,
        nComps,
        dataType,
        pSendPBO,
        dataType,
        #ifdef PBO_RECV_BUFFERS
        pRecvPBO,
        #else
        pRecvBuf,
        #endif
        mpiReqs,
        mpiTypes,
        tag);
    if (iErr)
    {
      vtkErrorMacro(
        << this->CommRank
        << " transaction " << j << ":" << tag
        << " failed " << iErr << endl
        << transaction);
    }
  }

  // overlap transfer of local data with communication. compositing is not
  // needed since source blocks are disjoint.
  for (int j=0; j<nTransactions; ++j)
  {
    vtkPPixelTransfer &transaction = this->ScatterProgram[j];

    if (!transaction.Local(this->CommRank))
    {
      continue;
    }

    #if vtkPSurfaceLICCompositeDEBUG>=2
    cerr
      << this->CommRank << ":" << j << ":"
      << encodeTag(j, this->Pass) << " Local " << transaction
      << endl;
    #endif

    iErr = transaction.Blit(
        nComps,
        dataType,
        pSendPBO,
        dataType,
        #ifdef PBO_RECV_BUFFERS
        pRecvPBO
        #else
        pRecvBuf
        #endif
        );
    if (iErr)
    {
      vtkErrorMacro(
        << this->CommRank
        << " local transaction " << j << ":" << this->Pass
        << " failed " << iErr << endl
        << transaction);
    }
  }

  // recv remote data. compsiting is not needed since source blocks are
  // disjoint.
  int nRecvReqs = static_cast<int>(mpiRecvReqs.size());
  if (nRecvReqs)
  {
    iErr = MPI_Waitall(nRecvReqs, &mpiRecvReqs[0], MPI_STATUSES_IGNORE);
    if (iErr)
    {
      vtkErrorMacro("comm error in recv");
    }
  }

  unsigned int winExtSize[2];
  this->WindowExt.Size(winExtSize);

  if (newImage == NULL)
  {
    newImage = vtkTextureObject::New();
    newImage->SetContext(this->Context);
    newImage->Create2D(
          winExtSize[0],
          winExtSize[1],
          nComps,
          dataType,
          false);
  }

  // transfer received data to the icet/decomp.
  #ifdef PBO_RECV_BUFFERS
  recvPBO->UnmapUnpackedBuffer();
  newImage->Create2D(winExtSize[0], winExtSize[1], nComps, recvPBO, false);
  recvPBO->Delete();
  #else
  vtkPixelBufferObject *recvPBO;
  recvPBO = vtkPixelBufferObject::New();
  recvPBO->SetContext(this->Context);
  recvPBO->SetType(dataType);
  recvPBO->SetComponents(nComps);
  recvPBO->SetSize(pboSize);
  void *pRecvPBO = recvPBO->MapUnpackedBuffer(bufSize);
  memcpy(pRecvPBO, pRecvBuf, bufSize);
  recvPBO->UnmapUnpackedBuffer();
  newImage->Create2D(winExtSize[0], winExtSize[1], nComps, recvPBO, false);
  recvPBO->Delete();
  #endif

  // wait for sends to complete
  int nSendReqs = static_cast<int>(mpiSendReqs.size());
  if (nSendReqs)
  {
    iErr = MPI_Waitall(nSendReqs, &mpiSendReqs[0], MPI_STATUSES_IGNORE);
    if (iErr)
    {
      vtkErrorMacro("comm error in send");
    }
  }

  MPITypeFree(mpiTypes);

  return 0;
}

// ----------------------------------------------------------------------------
void vtkPSurfaceLICComposite::PrintSelf(ostream &os, vtkIndent indent)
{
  vtkObject::PrintSelf(os, indent);
  os << *this << endl;
}

// ****************************************************************************
ostream &operator<<(ostream &os, vtkPSurfaceLICComposite &ss)
{
  // this puts output in rank order
  MPI_Comm comm = *(static_cast<MPI_Comm*>(ss.PainterComm->GetCommunicator()));
  int rankBelow = ss.CommRank-1;
  if (rankBelow >= 0)
  {
    MPI_Recv(NULL, 0, MPI_BYTE, rankBelow, 13579, comm, MPI_STATUS_IGNORE);
  }
  os << "winExt=" << ss.WindowExt << endl;
  os << "blockExts=" << endl;
  size_t nExts = ss.BlockExts.size();
  for (size_t i=0; i<nExts; ++i)
  {
    os << "  " << ss.BlockExts[i] << endl;
  }
  os << "compositeExts=" << endl;
  nExts = ss.CompositeExt.size();
  for (size_t i=0; i<nExts; ++i)
  {
    os << ss.CompositeExt[i] << endl;
  }
  os << "guardExts=" << endl;
  for (size_t i=0; i<nExts; ++i)
  {
    os << ss.GuardExt[i] << endl;
  }
  os << "disjointGuardExts=" << endl;
  for (size_t i=0; i<nExts; ++i)
  {
    os << ss.DisjointGuardExt[i] << endl;
  }
  os << "SuffleProgram:" << endl;
  size_t nTransactions = ss.GatherProgram.size();
  for (size_t j=0; j<nTransactions; ++j)
  {
    os << "  " << ss.GatherProgram[j] << endl;
  }
  os << "UnSuffleProgram:" << endl;
  nTransactions = ss.ScatterProgram.size();
  for (size_t j=0; j<nTransactions; ++j)
  {
    os << "  " << ss.ScatterProgram[j] << endl;
  }
  int rankAbove = ss.CommRank+1;
  if (rankAbove < ss.CommSize)
  {
    MPI_Send(NULL, 0, MPI_BYTE, rankAbove, 13579, comm);
  }
  return os;
}
