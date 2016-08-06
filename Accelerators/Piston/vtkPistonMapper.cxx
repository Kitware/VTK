/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPistonMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPistonMapper.h"

#include "vtkFloatArray.h"
#include "vtkgl.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkPistonDataObject.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPistonDataObject.h"
#include "vtkPistonScalarsColors.h"
#include "vtkScalarsToColors.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkOpenGLError.h"

#include <limits>

struct cudaGraphicsResource; //keeps vtkpiston namespace decl from claiming it

//----------------------------------------------------------------------------
namespace vtkpiston {
  // Forward declarations of methods defined in the cuda implementation
  void CudaGLInit();
  void CopyFromGPU(vtkPistonDataObject *id, vtkPolyData *pd);
  int QueryNumVerts(vtkPistonDataObject *id);
  int QueryVertsPer(vtkPistonDataObject *id);
  void CudaRegisterBuffer(struct cudaGraphicsResource **vboResource,
                          GLuint vboBuffer);
  void CudaUnregisterResource(struct cudaGraphicsResource* vboResource);
  void CudaTransferToGL(vtkPistonDataObject *id, vtkMTimeType dataObjectMTimeCache,
                        vtkPistonScalarsColors *psc,
                        struct cudaGraphicsResource **vboResources,
                        bool &hasNormals, bool &hasColors);
  bool AlmostEqualRelativeAndAbs(float A, float B,
                                 float maxDiff, float maxRelDiff);

  struct PistonGLRAII
    {
    PistonGLRAII(GLbitfield mask)
      {
      vtkOpenGLClearErrorMacro();
      glPushAttrib(mask);
      }

    ~PistonGLRAII()
      {
      glPopAttrib();
      vtkOpenGLStaticCheckErrorMacro("failed after ~PistonGLRAII");
      }
    };
}

//-----------------------------------------------------------------------------
class vtkPistonMapper::InternalInfo
{
public:
  InternalInfo()
    {
    this->BufferSize = 0;
    this->PistonScalarsColors = 0;

    this->DataObjectMTimeCache = 0;
    }

  int BufferSize;
  GLuint vboBuffers[3];
  struct cudaGraphicsResource* vboResources[3];

  vtkMTimeType DataObjectMTimeCache;
  vtkPistonScalarsColors *PistonScalarsColors;
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPistonMapper);

bool vtkPistonMapper::CudaGLInitted = false;

//-----------------------------------------------------------------------------
void vtkPistonMapper::InitCudaGL(vtkRenderWindow *rw)
{
  if (!vtkPistonMapper::CudaGLInitted)
    {
    vtkOpenGLExtensionManager *em = vtkOpenGLExtensionManager::New();
    em->SetRenderWindow(rw);
    if (!em->LoadSupportedExtension("GL_VERSION_1_5"))
      {
      cerr << "WARNING: Can not use direct piston rendering,"
           << "reverting to CPU rendering path." << endl;
      em->Delete();
      return;
      }
    em->Delete();

    vtkPistonMapper::CudaGLInitted = true;
    vtkpiston::CudaGLInit();
   }
}

//-----------------------------------------------------------------------------
vtkPistonMapper::vtkPistonMapper()
{
  VTK_LEGACY_BODY(vtkPistonMapper::vtkPistonMapper, "VTK 6.3");

  //cerr << "PM(" << this << ") create" << endl;
  vtkMath::UninitializeBounds(this->Bounds);
  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;
  this->Piece = 0;
  this->NumberOfPieces = 1;
  this->GhostLevel = 0;

  this->Internal = new vtkPistonMapper::InternalInfo();
  this->Internal->PistonScalarsColors = vtkPistonScalarsColors::New();
  this->Internal->PistonScalarsColors->SetLookupTable(
    this->GetLookupTable());
}

//-----------------------------------------------------------------------------
vtkPistonMapper::~vtkPistonMapper()
{
  //cerr << "PM(" << this << ") destroy" << endl;
  this->PrepareDirectRenderBuffers(0);
  this->Internal->PistonScalarsColors->Delete();
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void vtkPistonMapper::PrepareDirectRenderBuffers(int nPoints)
{
  vtkOpenGLClearErrorMacro();

  if (nPoints==this->Internal->BufferSize)
    {
    return;
    }
  if (this->Internal->BufferSize != 0)
    {
    // Release old buffer
    vtkpiston::CudaUnregisterResource(this->Internal->vboResources[0]);
    vtkpiston::CudaUnregisterResource(this->Internal->vboResources[1]);
    vtkpiston::CudaUnregisterResource(this->Internal->vboResources[2]);
    vtkgl::DeleteBuffers(3, this->Internal->vboBuffers);
    vtkOpenGLCheckErrorMacro("failed at glDeleteBuffers");
    }

  this->Internal->BufferSize = nPoints;
  if (this->Internal->BufferSize == 0)
    {
    return;
    }

  // Prep shared mem buffer between gl and cuda
  vtkgl::GenBuffers(3, this->Internal->vboBuffers);
  vtkgl::BindBuffer(vtkgl::ARRAY_BUFFER,
                    this->Internal->vboBuffers[0]);
  vtkgl::BufferData(vtkgl::ARRAY_BUFFER,
                    this->Internal->BufferSize*3*sizeof(float), 0,
                    vtkgl::DYNAMIC_DRAW);
  vtkgl::BindBuffer(vtkgl::ARRAY_BUFFER,
                    this->Internal->vboBuffers[1]);
  vtkgl::BufferData(vtkgl::ARRAY_BUFFER,
                    this->Internal->BufferSize*3*3*sizeof(float), 0,
                    vtkgl::DYNAMIC_DRAW);
  vtkgl::BindBuffer(vtkgl::ARRAY_BUFFER,
                    this->Internal->vboBuffers[2]);
  vtkgl::BufferData(vtkgl::ARRAY_BUFFER,
                    this->Internal->BufferSize*3*sizeof(float), 0,
                    vtkgl::DYNAMIC_DRAW);

  vtkOpenGLCheckErrorMacro("failed after allocate shared memory");

  vtkpiston::CudaRegisterBuffer(&this->Internal->vboResources[0],
                               this->Internal->vboBuffers[0]);
  vtkpiston::CudaRegisterBuffer(&this->Internal->vboResources[1],
                               this->Internal->vboBuffers[1]);
  vtkpiston::CudaRegisterBuffer(&this->Internal->vboResources[2],
                               this->Internal->vboBuffers[2]);
}

//-----------------------------------------------------------------------------
void vtkPistonMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Piece : " << this->Piece << endl;
  os << indent << "NumberOfPieces : " << this->NumberOfPieces << endl;
  os << indent << "GhostLevel: " << this->GhostLevel << endl;
}

//-----------------------------------------------------------------------------
void vtkPistonMapper::ShallowCopy(vtkAbstractMapper *mapper)
{
  //cerr << "PM(" << this << ") shallow copy" << mapper << endl;
  vtkPistonMapper *m = vtkPistonMapper::SafeDownCast(mapper);
  if ( m != NULL )
    {
      this->SetPiece(m->GetPiece());
      this->SetNumberOfPieces(m->GetNumberOfPieces());
      this->SetGhostLevel(m->GetGhostLevel());
    }

  // Now do superclass
  this->vtkAbstractMapper3D::ShallowCopy(mapper);
}

//----------------------------------------------------------------------------
int vtkPistonMapper::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPistonDataObject");
  return 1;
}

//-----------------------------------------------------------------------------
void vtkPistonMapper::RenderOnCPU()
{
  vtkOpenGLClearErrorMacro();

  vtkpiston::PistonGLRAII(GL_LIGHTING_BIT);

  vtkScalarsToColors *lut = this->GetLookupTable();

  vtkPistonDataObject *id = this->GetPistonDataObjectInput(0);

  vtkPolyData *od = vtkPolyData::New();
  vtkpiston::CopyFromGPU(id, od);

  int vertsPer = vtkpiston::QueryVertsPer(id);

  vtkFloatArray *normals = vtkArrayDownCast<vtkFloatArray>(
      od->GetPointData()->GetNormals());
  if (!normals)
    {
     normals = vtkArrayDownCast<vtkFloatArray>(
       od->GetPointData()->GetArray("Normals"));
    }

  vtkFloatArray *scalars = vtkArrayDownCast<vtkFloatArray>(
      od->GetPointData()->GetScalars());
  if (scalars)
    {
      //TODO: Remove this when piston gives us exactly same values for isocontour.
      float *scalarRange = scalars->GetValueRange();
      float tempRange[2] =
        {
          static_cast<float>(scalarRange[0]),
          static_cast<float>(scalarRange[1])
        };
      if (vtkpiston::AlmostEqualRelativeAndAbs(scalarRange[0], scalarRange[1],
                                               std::numeric_limits<float>::epsilon(),
                                               std::numeric_limits<float>::epsilon() * 10))
        {
        tempRange[1] = tempRange[0]+1.0;
        }

      lut->SetRange(tempRange[0], tempRange[1]);
      lut->Build();

      glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
      glEnable(GL_COLOR_MATERIAL);
    }

  double nextpt0[3];
  double nextpt1[3];
  double nextpt2[3];
  double nextpt3[3];

  double nextnorm0[3];
  double nextnorm1[3];
  double nextnorm2[3];
  double nextnorm3[3];

  unsigned char nextcolor0[3];
  unsigned char nextcolor1[3];
  unsigned char nextcolor2[3];
  unsigned char nextcolor3[3];

  switch (vertsPer) {
    case 4:
      glBegin(GL_QUADS);
      for (int i = 0; i < od->GetNumberOfPoints(); i+=4)
        {
          od->GetPoint(i+0, nextpt0);
          od->GetPoint(i+1, nextpt1);
          od->GetPoint(i+2, nextpt2);
          od->GetPoint(i+3, nextpt3);
          if (normals)
            {
            normals->GetTuple(i+0, nextnorm0);
            normals->GetTuple(i+1, nextnorm1);
            normals->GetTuple(i+2, nextnorm2);
            normals->GetTuple(i+3, nextnorm3);
            }
          if (scalars)
            {
              unsigned char *ptr;
              ptr = lut->MapValue(scalars->GetValue(i+0));
              nextcolor0[0] = ptr[0];
              nextcolor0[1] = ptr[1];
              nextcolor0[2] = ptr[2];
              ptr = lut->MapValue(scalars->GetValue(i+1));
              nextcolor1[0] = ptr[0];
              nextcolor1[1] = ptr[1];
              nextcolor1[2] = ptr[2];
              ptr = lut->MapValue(scalars->GetValue(i+2));
              nextcolor2[0] = ptr[0];
              nextcolor2[1] = ptr[1];
              nextcolor2[2] = ptr[2];
              ptr = lut->MapValue(scalars->GetValue(i+3));
              nextcolor3[0] = ptr[0];
              nextcolor3[1] = ptr[1];
              nextcolor3[2] = ptr[2];
            }
          if (normals) glNormal3dv(nextnorm0);
          if (scalars) glColor3ubv(nextcolor0);
          glVertex3dv(nextpt0);
          if (normals) glNormal3dv(nextnorm1);
          if (scalars) glColor3ubv(nextcolor1);
          glVertex3dv(nextpt1);
          if (normals) glNormal3dv(nextnorm2);
          if (scalars) glColor3ubv(nextcolor2);
          glVertex3dv(nextpt2);
          if (normals) glNormal3dv(nextnorm3);
          if (scalars) glColor3ubv(nextcolor3);
          glVertex3dv(nextpt3);
        }
      glEnd();
      break;
    case 3:
    {
      glBegin(GL_TRIANGLES);
      for (int i = 0; i < od->GetNumberOfPoints(); i+=3)
        {
        od->GetPoint(i+0, nextpt0);
        od->GetPoint(i+1, nextpt1);
        od->GetPoint(i+2, nextpt2);
        if (normals)
          {
          normals->GetTuple(i+0, nextnorm0);
          normals->GetTuple(i+1, nextnorm1);
          normals->GetTuple(i+2, nextnorm2);
          }
        if (scalars)
          {
            unsigned char *ptr;
            ptr = lut->MapValue(scalars->GetValue(i+0));
            nextcolor0[0] = ptr[0];
            nextcolor0[1] = ptr[1];
            nextcolor0[2] = ptr[2];
            ptr = lut->MapValue(scalars->GetValue(i+1));
            nextcolor1[0] = ptr[0];
            nextcolor1[1] = ptr[1];
            nextcolor1[2] = ptr[2];
            ptr = lut->MapValue(scalars->GetValue(i+2));
            nextcolor2[0] = ptr[0];
            nextcolor2[1] = ptr[1];
            nextcolor2[2] = ptr[2];
          }

        if (normals) glNormal3dv(nextnorm0);
        if (scalars) glColor3ubv(nextcolor0);
        glVertex3dv(nextpt0);
        if (normals) glNormal3dv(nextnorm1);
        if (scalars) glColor3ubv(nextcolor1);
        glVertex3dv(nextpt1);
        if (normals) glNormal3dv(nextnorm2);
        if (scalars) glColor3ubv(nextcolor2);
        glVertex3dv(nextpt2);
        }
      glEnd();
    }
    break;
    default:
      glBegin(GL_POINTS);
      for (int i = 0; i < od->GetNumberOfPoints(); i++)
        {
        od->GetPoint(i+0, nextpt0);
        if (normals)
          {
          normals->GetTuple(i+0, nextnorm0);
          glNormal3dv(nextnorm0);
          }
        if (scalars)
          {
            unsigned char *ptr;
            ptr = lut->MapValue(scalars->GetValue(i+0));
            glColor3ubv(ptr);
           }
        glVertex3dv(nextpt0);
        }
      glEnd();
  }
  od->Delete();

  vtkOpenGLCheckErrorMacro("failed after RenderOnCPU");
}

//-----------------------------------------------------------------------------
void vtkPistonMapper::RenderOnGPU()
{
  vtkOpenGLClearErrorMacro();

  vtkPistonDataObject *id = this->GetPistonDataObjectInput(0);

  int nPoints = vtkpiston::QueryNumVerts(id);
  this->PrepareDirectRenderBuffers(nPoints);

  // Transfer what is in tdo to buffer and render it directly on card
  bool hasNormals = false;
  bool hasColors = false;

  vtkpiston::CudaTransferToGL(id, this->Internal->DataObjectMTimeCache,
    this->Internal->PistonScalarsColors,
    this->Internal->vboResources, hasNormals, hasColors);

  // Draw the result
  glEnableClientState(GL_VERTEX_ARRAY);
  vtkgl::BindBuffer(vtkgl::ARRAY_BUFFER, this->Internal->vboBuffers[0]);
  glVertexPointer(3, GL_FLOAT, 0, 0);

  if (hasNormals)
    {
    glEnableClientState(GL_NORMAL_ARRAY);
    vtkgl::BindBuffer(vtkgl::ARRAY_BUFFER, this->Internal->vboBuffers[1]);
    glNormalPointer(GL_FLOAT, 0, 0);
    }

  if (hasColors)
    {
    glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
    glEnableClientState(GL_COLOR_ARRAY);
    vtkgl::BindBuffer(vtkgl::ARRAY_BUFFER, this->Internal->vboBuffers[2]);
    glColorPointer(3, GL_FLOAT, 0, 0);
    }

  int vertsPer = vtkpiston::QueryVertsPer(id);
  switch (vertsPer) {
    case 4:
      glDrawArrays(GL_QUADS, 0, nPoints);
      break;
    case 3:
      glDrawArrays(GL_TRIANGLES, 0, nPoints);
      break;
    default:
      glDrawArrays(GL_POINTS, 0, nPoints);
  }

  glDisableClientState(GL_VERTEX_ARRAY);
  if (hasNormals) glDisableClientState(GL_NORMAL_ARRAY);
  if (hasColors) glDisableClientState(GL_COLOR_ARRAY);

  vtkOpenGLCheckErrorMacro("failed after RenderOnGPU");
}

//-----------------------------------------------------------------------------
void vtkPistonMapper::RenderImageDataOutline()
{
  vtkOpenGLClearErrorMacro();

  vtkpiston::PistonGLRAII(GL_LIGHTING_BIT);
  glDisable(GL_LIGHTING);

  glBegin(GL_LINES);

  double nextpt[8][3];
  nextpt[0][0] = this->Bounds[0];
  nextpt[0][1] = this->Bounds[2];
  nextpt[0][2] = this->Bounds[4];
  nextpt[1][0] = this->Bounds[1];
  nextpt[1][1] = this->Bounds[2];
  nextpt[1][2] = this->Bounds[4];
  nextpt[2][0] = this->Bounds[1];
  nextpt[2][1] = this->Bounds[3];
  nextpt[2][2] = this->Bounds[4];
  nextpt[3][0] = this->Bounds[0];
  nextpt[3][1] = this->Bounds[3];
  nextpt[3][2] = this->Bounds[4];
  nextpt[4][0] = this->Bounds[0];
  nextpt[4][1] = this->Bounds[2];
  nextpt[4][2] = this->Bounds[5];
  nextpt[5][0] = this->Bounds[1];
  nextpt[5][1] = this->Bounds[2];
  nextpt[5][2] = this->Bounds[5];
  nextpt[6][0] = this->Bounds[1];
  nextpt[6][1] = this->Bounds[3];
  nextpt[6][2] = this->Bounds[5];
  nextpt[7][0] = this->Bounds[0];
  nextpt[7][1] = this->Bounds[3];
  nextpt[7][2] = this->Bounds[5];

  glVertex3dv(nextpt[0]);
  glVertex3dv(nextpt[1]);
  glVertex3dv(nextpt[1]);
  glVertex3dv(nextpt[2]);
  glVertex3dv(nextpt[2]);
  glVertex3dv(nextpt[3]);
  glVertex3dv(nextpt[3]);
  glVertex3dv(nextpt[0]);

  glVertex3dv(nextpt[4]);
  glVertex3dv(nextpt[5]);
  glVertex3dv(nextpt[5]);
  glVertex3dv(nextpt[6]);
  glVertex3dv(nextpt[6]);
  glVertex3dv(nextpt[7]);
  glVertex3dv(nextpt[7]);
  glVertex3dv(nextpt[4]);

  glVertex3dv(nextpt[0]);
  glVertex3dv(nextpt[4]);

  glVertex3dv(nextpt[1]);
  glVertex3dv(nextpt[5]);

  glVertex3dv(nextpt[2]);
  glVertex3dv(nextpt[6]);
  glVertex3dv(nextpt[3]);
  glVertex3dv(nextpt[7]);
  glEnd();

  vtkOpenGLCheckErrorMacro("failed after RenderImageDataOutline");
}

//----------------------------------------------------------------------------
vtkPistonDataObject* vtkPistonMapper::GetPistonDataObjectInput(int port)
{
  return vtkPistonDataObject::SafeDownCast(this->GetInputDataObject(port, 0));
}

//-----------------------------------------------------------------------------
// Get the bounds for the input of this mapper as
// (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
double *vtkPistonMapper::GetBounds()
{
  vtkPistonDataObject *id = this->GetPistonDataObjectInput(0);
  if (id)
    {
    id->GetBounds(this->Bounds);
    //cerr << "FOUND BOUNDS " << this->Bounds[0] << "," << this->Bounds[1] << endl;
    }
  else
    {
    vtkMath::UninitializeBounds(this->Bounds);
    }
  return this->Bounds;
}

//-----------------------------------------------------------------------------
void vtkPistonMapper::Render(vtkRenderer *vtkNotUsed(ren), vtkActor *vtkNotUsed(a))
{
  vtkPistonDataObject *id = this->GetPistonDataObjectInput(0);
  if (id->GetReferredType() == VTK_IMAGE_DATA)
  {
    this->RenderImageDataOutline();
    return;
  }

  if (!vtkPistonMapper::CudaGLInitted)
    {
    this->RenderOnCPU();
    }
  else
    {
    this->RenderOnGPU();
    }

  // Update object modified time
  this->Internal->DataObjectMTimeCache = id->GetMTime();
}

//-----------------------------------------------------------------------------
void vtkPistonMapper::Update()
{
  this->UpdateInformation();

  // vtkInformation* inInfo = this->GetInputInformation();

  // If the estimated pipeline memory usage is larger than
  // the memory limit, break the current piece into sub-pieces.
  // if (inInfo)
  //   {
  //   vtkStreamingDemandDrivenPipeline::SetUpdateExtent(
  //     inInfo,
  //     this->Piece,
  //     this->NumberOfPieces,
  //     this->GhostLevel);
  //   }

  this->vtkMapper::Update();
}
