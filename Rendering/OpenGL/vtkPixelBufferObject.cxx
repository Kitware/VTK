/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPixelBufferObject.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPixelBufferObject.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkRenderWindow.h"

#include "vtkgl.h"
#include "vtkOpenGL.h"
#include "vtkOpenGLError.h"

//#define VTK_PBO_DEBUG
//#define VTK_PBO_TIMING

#ifdef VTK_PBO_TIMING
#include "vtkTimerLog.h"
#endif

#include <cassert>

// Mapping from Usage values to OpenGL values.

static const GLenum OpenGLBufferObjectUsage[9]=
{
  vtkgl::STREAM_DRAW,
  vtkgl::STREAM_READ,
  vtkgl::STREAM_COPY,
  vtkgl::STATIC_DRAW,
  vtkgl::STATIC_READ,
  vtkgl::STATIC_COPY,
  vtkgl::DYNAMIC_DRAW,
  vtkgl::DYNAMIC_READ,
  vtkgl::DYNAMIC_COPY
};

static const char *BufferObjectUsageAsString[9]=
{
  "StreamDraw",
  "StreamRead",
  "StreamCopy",
  "StaticDraw",
  "StaticRead",
  "StaticCopy",
  "DynamicDraw",
  "DynamicRead",
  "DynamicCopy"
};

// access modes
const GLenum OpenGLBufferObjectAccess[2]=
{
  vtkgl::WRITE_ONLY,
  vtkgl::READ_ONLY
};

// targets
const GLenum OpenGLBufferObjectTarget[2]=
{
  vtkgl::PIXEL_UNPACK_BUFFER_ARB,
  vtkgl::PIXEL_PACK_BUFFER_ARB
};


#ifdef  VTK_PBO_DEBUG
#include <pthread.h> // for debugging with MPI, pthread_self()
#endif

// converting double to float behind the
// scene so we need sizeof(double)==4
template< class T >
class vtksizeof
{
public:
  static int GetSize() { return sizeof(T); }
};

template<>
class vtksizeof< double >
{
public:
  static int GetSize() { return sizeof(float); }
};

static int vtkGetSize(int type)
{
  switch (type)
  {
    vtkTemplateMacro(
      return ::vtksizeof<VTK_TT>::GetSize();
      );
  }
  return 0;
}

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPixelBufferObject);

//----------------------------------------------------------------------------
vtkPixelBufferObject::vtkPixelBufferObject()
{
  this->Handle = 0;
  this->Context = NULL;
  this->BufferTarget = 0;
  this->Components = 0;
  this->Size = 0;
  this->Type = VTK_UNSIGNED_CHAR;
  this->Usage = StaticDraw;
}

//----------------------------------------------------------------------------
vtkPixelBufferObject::~vtkPixelBufferObject()
{
  this->DestroyBuffer();
}

//----------------------------------------------------------------------------
bool vtkPixelBufferObject::IsSupported(vtkRenderWindow* win)
{
  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(win);
  if (renWin)
  {
    vtkOpenGLExtensionManager* mgr = renWin->GetExtensionManager();

    bool vbo=mgr->ExtensionSupported("GL_VERSION_1_5") ||
      mgr->ExtensionSupported("GL_ARB_vertex_buffer_object");

    // pbo extension does not define new functions but uses functions defined
    // by vbo extension.
    bool pbo=mgr->ExtensionSupported("GL_VERSION_2_1") ||
      mgr->ExtensionSupported("GL_ARB_pixel_buffer_object");

    return vbo && pbo;
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkPixelBufferObject::LoadRequiredExtensions(vtkRenderWindow *renWin)
{
  vtkOpenGLRenderWindow* context =
    vtkOpenGLRenderWindow::SafeDownCast(renWin);

  if ( !context ) return false;

  vtkOpenGLExtensionManager* mgr = context->GetExtensionManager();

  bool gl15=mgr->ExtensionSupported("GL_VERSION_1_5")==1;
  bool gl21=mgr->ExtensionSupported("GL_VERSION_2_1")==1;

  bool vbo=gl15 || mgr->ExtensionSupported("GL_ARB_vertex_buffer_object");

  // pbo extension does not define new functions but uses functions defined
  // by vbo extension.
  bool pbo=gl21 || mgr->ExtensionSupported("GL_ARB_pixel_buffer_object");

  bool result=vbo && pbo;

  if(result)
  {
    if(gl15)
    {
      mgr->LoadExtension("GL_VERSION_1_5");
    }
    else
    {
      mgr->LoadCorePromotedExtension("GL_ARB_vertex_buffer_object");
    }
    // pbo does not define new functions, nothing to do here.
  }
  return result;
}

//----------------------------------------------------------------------------
void vtkPixelBufferObject::SetContext(vtkRenderWindow* renWin)
{
  // avoid pointless re-assignment
  if (this->Context==renWin)
  {
    return;
  }
  // free resource allocations
  this->DestroyBuffer();
  this->Context = NULL;
  this->Modified();
  // all done if assigned null
  if (!renWin)
  {
    return;
  }
  // check for support
  vtkOpenGLRenderWindow* context =
    vtkOpenGLRenderWindow::SafeDownCast(renWin);
  if ( !context
    || !this->LoadRequiredExtensions(renWin) )
  {
    vtkErrorMacro("Required OpenGL extensions not supported by the context.");
    return;
  }
  // update context
  this->Context = renWin;
  this->Context->MakeCurrent();
}

//----------------------------------------------------------------------------
vtkRenderWindow* vtkPixelBufferObject::GetContext()
{
  return this->Context;
}

//----------------------------------------------------------------------------
void vtkPixelBufferObject::SetSize(unsigned int nTups, int nComps)
{
  this->Size = nTups*nComps;
}

//----------------------------------------------------------------------------
void vtkPixelBufferObject::Bind(BufferType type)
{
  assert(this->Context);

  this->CreateBuffer();

  GLenum target;
  switch (type)
  {
    case vtkPixelBufferObject::PACKED_BUFFER:
      target = vtkgl::PIXEL_PACK_BUFFER_ARB;
      break;

    case vtkPixelBufferObject::UNPACKED_BUFFER:
      target = vtkgl::PIXEL_UNPACK_BUFFER_ARB;
      break;

    default:
      vtkErrorMacro("Impossible BufferType.");
      target = static_cast<GLenum>(this->BufferTarget);
      break;
  }

  if (this->BufferTarget && this->BufferTarget != target)
  {
    this->UnBind();
  }
  this->BufferTarget = target;
  vtkgl::BindBuffer(static_cast<GLenum>(this->BufferTarget), this->Handle);
  vtkOpenGLCheckErrorMacro("failed at glBindBuffer");
}

//----------------------------------------------------------------------------
void vtkPixelBufferObject::UnBind()
{
  assert(this->Context);
  if (this->Handle && this->BufferTarget)
  {
    vtkgl::BindBuffer(this->BufferTarget, 0);
    vtkOpenGLCheckErrorMacro("failed at glBindBuffer(0)");
    this->BufferTarget = 0;
  }
}

//----------------------------------------------------------------------------
void vtkPixelBufferObject::CreateBuffer()
{
  if (!this->Handle)
  {
    GLuint ioBuf;
    vtkgl::GenBuffers(1, &ioBuf);
    vtkOpenGLCheckErrorMacro("failed at glGenBuffers");
    this->Handle = ioBuf;
  }
}

//----------------------------------------------------------------------------
void vtkPixelBufferObject::DestroyBuffer()
{
  // because we don't hold a reference to the render
  // context we don't have any control on when it is
  // destroyed. In fact it may be destroyed before
  // we are(eg smart pointers), in which case we should
  // do nothing.
  if (this->Context && this->Handle)
  {
    GLuint ioBuf = static_cast<GLuint>(this->Handle);
    vtkgl::DeleteBuffers(1, &ioBuf);
    vtkOpenGLCheckErrorMacro("failed at glDeleteBuffers");
  }
  this->Handle = 0;
}

//----------------------------------------------------------------------------
template <class T>
class vtkUpload3D
{
public:
  static void Upload(void *pboPtr,
                     T *inData,
                     unsigned int dims[3],
                     int numComponents,
                     vtkIdType continuousIncrements[3],
                     int components,
                     int *componentList)
  {
      //  cout<<"incs[3]="<<continuousIncrements[0]<<" "<<continuousIncrements[1]
      //      <<" "<<continuousIncrements[2]<<endl;

      T* fIoMem = static_cast<T*>(pboPtr);

      int numComp;
      int *permutation=0;
      if(components==0)
      {
        numComp=numComponents;
        permutation=new int[numComponents];
        int i=0;
        while(i<numComp)
        {
          permutation[i]=i;
          ++i;
        }
      }
      else
      {
        numComp=components;
        permutation=componentList;
      }

      vtkIdType tupleSize =
        static_cast<vtkIdType>(numComponents + continuousIncrements[0]);
      for (unsigned int zz=0; zz < dims[2]; zz++)
      {
        for (unsigned int yy = 0; yy < dims[1]; yy++)
        {
          for (unsigned int xx=0; xx < dims[0]; xx++)
          {
            for (int compNo=0; compNo < numComp; compNo++)
            {
              *fIoMem = inData[permutation[compNo]];
//              cout<<"upload[zz="<<zz<<"][yy="<<yy<<"][xx="<<xx<<"][compNo="<<
//              compNo<<"] from inData to pbo="<<(double)(*fIoMem)<<endl;

              fIoMem++;
            }
            inData += tupleSize+continuousIncrements[0];
          }
          // Reached end of row, go to start of next row.
          inData += continuousIncrements[1] * tupleSize;
        }
        // Reached end of 2D plane.
        inData += continuousIncrements[2] * tupleSize;
      }

      if(components==0)
      {
        delete[] permutation;
      }
  }
};

template<>
class vtkUpload3D< double >
{
public:
  static void Upload(void *pboPtr,
                     double *inData,
                     unsigned int dims[3],
                     int numComponents,
                     vtkIdType continuousIncrements[3],
                     int components,
                     int *componentList)
  {
      float* fIoMem = static_cast<float*>(pboPtr);

      int *permutation=0;
      if(components==0)
      {
        permutation=new int[numComponents];
        int i=0;
        while(i<numComponents)
        {
          permutation[i]=i;
          ++i;
        }
      }
      else
      {
        permutation=componentList;
      }

      vtkIdType tupleSize =
        static_cast<vtkIdType>(numComponents + continuousIncrements[0]);
      for (unsigned int zz=0; zz < dims[2]; zz++)
      {
        for (unsigned int yy = 0; yy < dims[1]; yy++)
        {
          for (unsigned int xx=0; xx < dims[0]; xx++)
          {
            for (int compNo=0; compNo < numComponents; compNo++)
            {
              *fIoMem = static_cast<float>(inData[permutation[compNo]]);

              //        cout<<"upload specialized double[zz="<<zz<<"][yy="<<yy<<"][xx="<<xx<<"][compNo="<<compNo<<"] from inData="<<(*inData)<<" to pbo="<<(*fIoMem)<<endl;

              fIoMem++;
            }

            inData += tupleSize+continuousIncrements[0];
          }
          // Reached end of row, go to start of next row.
          inData += continuousIncrements[1] * tupleSize;
        }
        // Reached end of 2D plane.
        inData += continuousIncrements[2] * tupleSize;
      }
      if(components==0)
      {
        delete[] permutation;
      }
  }
};

//----------------------------------------------------------------------------
void *vtkPixelBufferObject::MapBuffer(
        unsigned int nbytes,
        BufferType mode)
{
  // from vtk to opengl enums
  GLenum target = OpenGLBufferObjectTarget[mode];
  GLenum access = OpenGLBufferObjectAccess[mode];
  GLenum usage = OpenGLBufferObjectUsage[mode];
  GLuint size = static_cast<GLuint>(nbytes);
  GLuint ioBuf = static_cast<GLuint>(this->Handle);

  if (!ioBuf)
  {
    vtkgl::GenBuffers(1, &ioBuf);
    vtkOpenGLCheckErrorMacro("failed at glGenBuffers");
    this->Handle = static_cast<unsigned int>(ioBuf);
  }
  this->BufferTarget = 0;

  // pointer to the mapped memory
  vtkgl::BindBuffer(target, ioBuf);
  vtkOpenGLCheckErrorMacro("failed at glBindBuffer");

  vtkgl::BufferData(target, size, NULL, usage);
  vtkOpenGLCheckErrorMacro("failed at glBufferData");

  void *pPBO = vtkgl::MapBuffer(target, access);
  vtkOpenGLCheckErrorMacro("failed at glMapBuffer");

  vtkgl::BindBuffer(target, 0);

  return pPBO;
}

//----------------------------------------------------------------------------
void *vtkPixelBufferObject::MapBuffer(
        int type,
        unsigned int numtuples,
        int comps,
        BufferType mode)
{
  // from vtk to opengl enums
  this->Size = numtuples*comps;
  this->Type = type;
  this->Components = comps;
  unsigned int size = ::vtkGetSize(type)*this->Size;

  return this->MapBuffer(size, mode);
}

//----------------------------------------------------------------------------
void *vtkPixelBufferObject::MapBuffer(BufferType mode)
{
  // from vtk to opengl enum
  GLuint ioBuf = static_cast<GLuint>(this->Handle);
  if (!ioBuf)
  {
    vtkErrorMacro("Uninitialized object");
    return NULL;
  }
  GLenum target = OpenGLBufferObjectTarget[mode];
  GLenum access = OpenGLBufferObjectAccess[mode];

  // pointer to the mnapped memory
  vtkgl::BindBuffer(target, ioBuf);
  vtkOpenGLCheckErrorMacro("failed at glBindBuffer");

  void *pPBO = vtkgl::MapBuffer(target, access);
  vtkOpenGLCheckErrorMacro("failed at glMapBuffer");

  vtkgl::BindBuffer(target, 0);
  vtkOpenGLCheckErrorMacro("failed at glBindBuffer(0)");

  this->BufferTarget = 0;

  return pPBO;
}

//----------------------------------------------------------------------------
void vtkPixelBufferObject::UnmapBuffer(BufferType mode)
{
  GLuint ioBuf = static_cast<GLuint>(this->Handle);
  if (!ioBuf)
  {
    vtkErrorMacro("Uninitialized object");
    return;
  }
  GLenum target = OpenGLBufferObjectTarget[mode];

  vtkgl::BindBuffer(target, ioBuf);
  vtkOpenGLCheckErrorMacro("failed at glBindBuffer");

  vtkgl::UnmapBuffer(target);
  vtkOpenGLCheckErrorMacro("failed at glUnmapBuffer");

  vtkgl::BindBuffer(target, 0);
  vtkOpenGLCheckErrorMacro("failed at glBindBuffer(0)");
}

//----------------------------------------------------------------------------
bool vtkPixelBufferObject::Upload3D(
  int type, void* data,
  unsigned int dims[3],
  int numComponents,
  vtkIdType continuousIncrements[3],
  int components,
  int *componentList)
{
#ifdef VTK_PBO_TIMING
  vtkTimerLog *timer=vtkTimerLog::New();
  timer->StartTimer();
#endif
  assert(this->Context);

  this->CreateBuffer();
  this->Bind(vtkPixelBufferObject::UNPACKED_BUFFER);

  unsigned int size;

  if(components==0)
  {
    size = dims[0]*dims[1]*dims[2]*static_cast<unsigned int>(numComponents);
  }
  else
  {
    size = dims[0]*dims[1]*dims[2]*static_cast<unsigned int>(components);
  }

  this->Components = numComponents;

  if(data!=0)
  {
    this->Usage=StreamDraw;
  }
  else
  {
    this->Usage=StreamRead;
  }

  vtkgl::BufferData(this->BufferTarget,
                    size*static_cast<unsigned int>(::vtkGetSize(type)),
                    NULL,OpenGLBufferObjectUsage[this->Usage]);
  vtkOpenGLCheckErrorMacro("failed at glBufferData");
  this->Type = type;
  if (this->Type == VTK_DOUBLE)
  {
    this->Type = VTK_FLOAT;
  }
  this->Size = size;

  if (data)
  {
    void* ioMem = vtkgl::MapBuffer(this->BufferTarget, vtkgl::WRITE_ONLY);
    vtkOpenGLCheckErrorMacro("");
    switch (type)
    {
      vtkTemplateMacro(
        ::vtkUpload3D< VTK_TT >::Upload(ioMem, static_cast<VTK_TT*>(data),
                                        dims, numComponents,
                                        continuousIncrements,
                                        components,componentList);
        );
      default:
        vtkErrorMacro("unsupported vtk type");
        return false;
    }
    vtkgl::UnmapBuffer(this->BufferTarget);
    vtkOpenGLCheckErrorMacro("failed at glUnmapBuffer");
  }

  this->UnBind();
#ifdef VTK_PBO_TIMING
  timer->StopTimer();
  double time=timer->GetElapsedTime();
  timer->Delete();
  cout<<"Upload data to PBO"<<time<<" seconds."<<endl;
#endif
  return true;
}

//----------------------------------------------------------------------------
void vtkPixelBufferObject::Allocate(
        int type,
        unsigned int numtuples,
        int comps,
        BufferType mode)
{
  assert(this->Context);

  // from vtk to opengl enums
  this->Size = numtuples*comps;
  this->Type = type;
  this->Components = comps;
  unsigned int size = ::vtkGetSize(type)*this->Size;

  this->Allocate(size, mode);
}

//----------------------------------------------------------------------------
void vtkPixelBufferObject::Allocate(
        unsigned int nbytes,
        BufferType mode)
{
  assert(this->Context);

  // from vtk to opengl enums
  GLenum target = OpenGLBufferObjectTarget[mode];
  GLenum usage = OpenGLBufferObjectUsage[mode];
  GLuint size = static_cast<GLuint>(nbytes);
  GLuint ioBuf = static_cast<GLuint>(this->Handle);

  if (!ioBuf)
  {
    vtkgl::GenBuffers(1, &ioBuf);
    vtkOpenGLCheckErrorMacro("failed at glGenBuffers");
    this->Handle = static_cast<unsigned int>(ioBuf);
  }
  this->BufferTarget = 0;

  vtkgl::BindBuffer(target, ioBuf);
  vtkOpenGLCheckErrorMacro("failed at glBindBuffer");

  vtkgl::BufferData(target, size, NULL, usage);
  vtkOpenGLCheckErrorMacro("failed at glBufferData");

  vtkgl::BindBuffer(target, 0);
}


//----------------------------------------------------------------------------
void vtkPixelBufferObject::ReleaseMemory()
{
  assert(this->Context);
  assert(this->Handle);

  this->Bind(vtkPixelBufferObject::PACKED_BUFFER);
  vtkgl::BufferData(this->BufferTarget, 0, NULL, vtkgl::STREAM_DRAW);
  vtkOpenGLCheckErrorMacro("failed at glBufferData");
  this->Size = 0;
}

// ----------------------------------------------------------------------------
template <class TPBO, class TCPU>
void vtkDownload3D(TPBO *pboPtr,
                   TCPU *cpuPtr,
                   unsigned int dims[3],
                   int numcomps,
                   vtkIdType increments[3])
{
#ifdef  VTK_PBO_DEBUG
  cout << "template vtkDownload3D" << endl;
#endif
  vtkIdType tupleSize = static_cast<vtkIdType>(numcomps + increments[0]);
  for (unsigned int zz=0; zz < dims[2]; zz++)
  {
    for (unsigned int yy = 0; yy < dims[1]; yy++)
    {
      for (unsigned int xx=0; xx < dims[0]; xx++)
      {
        for (int comp=0; comp < numcomps; comp++)
        {
          *cpuPtr = static_cast<TCPU>(*pboPtr);
//          cout<<"download[zz="<<zz<<"][yy="<<yy<<"][xx="<<xx<<"][comp="<<comp<<"] from pbo="<<(*pboPtr)<<" to cpu="<<(*cpuPtr)<<endl;
          pboPtr++;
          cpuPtr++;
        }
        cpuPtr += increments[0];
      }
      // Reached end of row, go to start of next row.
      cpuPtr += increments[1]*tupleSize;
    }
    cpuPtr += increments[2]*tupleSize;
  }
}

// ----------------------------------------------------------------------------
template <class OType>
void vtkDownload3DSpe(int iType,
                      void *iData,
                      OType odata,
                      unsigned int dims[3],
                      int numcomps,
                      vtkIdType increments[3])
{
#ifdef  VTK_PBO_DEBUG
  cout << "vtkDownload3DSpe" << endl;
#endif
  switch(iType)
  {
    vtkTemplateMacro(
      ::vtkDownload3D(static_cast<VTK_TT*>(iData), odata,
                      dims, numcomps, increments);
      );
    default:
#ifdef  VTK_PBO_DEBUG
      cout << "d nested default." << endl;
#endif
      break;
  }
}

//----------------------------------------------------------------------------
bool vtkPixelBufferObject::Download3D(
  int type, void* data,
  unsigned int dims[3],
  int numcomps,
  vtkIdType increments[3])
{
#ifdef VTK_PBO_TIMING
  vtkTimerLog *timer=vtkTimerLog::New();
  timer->StartTimer();
#endif
  assert(this->Context);

  if (!this->Handle)
  {
    vtkErrorMacro("No GPU data available.");
    return false;
  }

  if (this->Size < dims[0]*dims[1]*dims[2]*static_cast<unsigned int>(numcomps))
  {
    vtkErrorMacro("Size too small.");
    return false;
  }

  this->Bind(vtkPixelBufferObject::PACKED_BUFFER);


  void* ioMem = vtkgl::MapBuffer(this->BufferTarget, vtkgl::READ_ONLY);
  vtkOpenGLCheckErrorMacro("failed at glMapBuffer");

  switch (type)
  {
    vtkTemplateMacro(
      VTK_TT* odata = static_cast<VTK_TT*>(data);
      ::vtkDownload3DSpe(this->Type,ioMem,odata,dims,numcomps,increments);
      );
    default:
      vtkErrorMacro("unsupported vtk type");
      return false;
  }
  vtkgl::UnmapBuffer(this->BufferTarget);
  vtkOpenGLCheckErrorMacro("failed at glUnmapBuffer");
  this->UnBind();

#ifdef VTK_PBO_TIMING
  timer->StopTimer();
  double time=timer->GetElapsedTime();
  timer->Delete();
  cout<<"dowmload data from PBO"<<time<<" seconds."<<endl;
#endif

  return true;
}

//----------------------------------------------------------------------------
void vtkPixelBufferObject::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Context: " << this->Context << endl;
  os << indent << "Handle: " << this->Handle << endl;
  os << indent << "Size: " << this->Size << endl;
  os << indent << "VTK Type: " << vtkImageScalarTypeNameMacro(this->Type)
     << endl;
  os << indent << "Usage:" << BufferObjectUsageAsString[this->Usage] << endl;
}
