/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeRGBAPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCompositeRGBAPass.h"
#include "vtkObjectFactory.h"
#include <assert.h>
#include "vtkRenderState.h"
#include "vtkOpenGLRenderer.h"
#include "vtkgl.h"
#include "vtkFrameBufferObject.h"
#include "vtkTextureObject.h"
#include "vtkShaderProgram2.h"
#include "vtkShader2.h"
#include "vtkShader2Collection.h"
#include "vtkUniformVariables.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkTextureUnitManager.h"

// to be able to dump intermediate result into png files for debugging.
// only for vtkCompositeRGBAPass developers.
//#define VTK_COMPOSITE_RGBAPASS_DEBUG

#include "vtkPNGWriter.h"
#include "vtkImageImport.h"
#include "vtkImageShiftScale.h"
#include "vtkPixelBufferObject.h"
#include "vtkImageExtractComponents.h"
#include "vtkMultiProcessController.h"
#include <vtksys/ios/sstream>
#include "vtkTimerLog.h"
#include "vtkStdString.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkOpenGLState.h"

#ifdef VTK_COMPOSITE_RGBAPASS_DEBUG
//#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h> // Linux specific gettid()
#endif

vtkCxxRevisionMacro(vtkCompositeRGBAPass, "1.2");
vtkStandardNewMacro(vtkCompositeRGBAPass);
vtkCxxSetObjectMacro(vtkCompositeRGBAPass,Controller,vtkMultiProcessController);

extern const char *vtkCompositeRGBAPassShader_fs;


// ----------------------------------------------------------------------------
vtkCompositeRGBAPass::vtkCompositeRGBAPass()
{
  this->Controller=0;
  this->PBO=0;
  this->RGBATexture=0;
  this->RawRGBABuffer=0;
  this->RawRGBABufferSize=0;
}

// ----------------------------------------------------------------------------
vtkCompositeRGBAPass::~vtkCompositeRGBAPass()
{
  if(this->Controller!=0)
    {
      this->Controller->Delete();
    }
  if(this->PBO!=0)
    {
    vtkErrorMacro(<<"PixelBufferObject should have been deleted in ReleaseGraphicsResources().");
    }
   if(this->RGBATexture!=0)
    {
    vtkErrorMacro(<<"RGBATexture should have been deleted in ReleaseGraphicsResources().");
    }
   if(this->Program!=0)
     {
     this->Program->Delete();
     }
   if(this->RawRGBABuffer!=0)
     {
     delete[] this->RawRGBABuffer;
     }
}

// ----------------------------------------------------------------------------
void vtkCompositeRGBAPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Controller:";
  if(this->Controller!=0)
    {
    this->Controller->PrintSelf(os,indent);
    }
  else
    {
    os << "(none)" <<endl;
    }
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void vtkCompositeRGBAPass::Render(const vtkRenderState *s)
{
  assert("pre: s_exists" && s!=0);
  
  if(this->Controller==0)
    {
    vtkErrorMacro(<<" no controller.");
    return;
    }
  
  int numProcs=this->Controller->GetNumberOfProcesses();
  
  if(numProcs==1)
    {
    return; // nothing to do.
    }
  
  int me=this->Controller->GetLocalProcessId();
  
  const int VTK_COMPOSITE_RGBA_PASS_MESSAGE_GATHER=201;
  
  vtkOpenGLRenderer *r=static_cast<vtkOpenGLRenderer *>(s->GetRenderer());
  vtkOpenGLRenderWindow *context=static_cast<vtkOpenGLRenderWindow *>(
    r->GetRenderWindow());
  
#ifdef VTK_COMPOSITE_RGBAPASS_DEBUG
  vtkOpenGLState *state=new vtkOpenGLState(context);
#endif
  
  int w=0;
  int h=0;
  
  vtkFrameBufferObject *fbo=s->GetFrameBuffer();
  if(fbo==0)
    {
    r->GetTiledSize(&w,&h);
    }
  else
    {
    int size[2];
    fbo->GetLastSize(size);
    w=size[0];
    h=size[1];
    }
  
  unsigned int byteSize=static_cast<unsigned int>(w*h*4)
    *static_cast<unsigned int>(sizeof(float));
  
  // pbo arguments.
  unsigned int dims[2];
  vtkIdType continuousInc[3];
  
  dims[0]=static_cast<unsigned int>(w);
  dims[1]=static_cast<unsigned int>(h);
  continuousInc[0]=0;
  continuousInc[1]=0;
  continuousInc[2]=0;
  
  
  if(this->RawRGBABuffer!=0 &&
     this->RawRGBABufferSize<static_cast<size_t>(w*h*4))
    {
    delete[] this->RawRGBABuffer;
    }
  if(this->RawRGBABuffer==0)
    {
    this->RawRGBABufferSize=static_cast<size_t>(w*h*4);
    this->RawRGBABuffer=new float[this->RawRGBABufferSize];
    }
  
  if(this->PBO==0)
    {
    this->PBO=vtkPixelBufferObject::New();
    this->PBO->SetContext(context);
    }
  if(this->RGBATexture==0)
    {
    this->RGBATexture=vtkTextureObject::New();
    this->RGBATexture->SetContext(context);
    }
  
  // TO: texture object
  // PBO: pixel buffer object
  // FB: framebuffer
  
#ifdef VTK_COMPOSITE_RGBAPASS_DEBUG
  vtkTimerLog *timer=vtkTimerLog::New();
  
  vtkImageImport *importer;
  vtkImageShiftScale *converter;
  vtkPNGWriter *writer;
  
  cout << "me=" << me<< " TID="<< syscall(SYS_gettid) << " thread=" << pthread_self() << endl;
  timer->StartTimer();
#endif
  
  
  if(me==0)
    {
    // root
    // 1. for each satellite
    // 1.a   receive zbuffer
    // 1.b   composite z against zbuffer in framebuffer
    // 2. send final zbuffer of the framebuffer to all satellites
    
#ifdef VTK_COMPOSITE_RGBAPASS_DEBUG
    
    // get z-buffer of root before any blending with satellite
    // for debugging only.
    
    // Framebuffer to PBO
    this->PBO->Allocate(byteSize);
    this->PBO->Bind(vtkPixelBufferObject::PACKED_BUFFER);
    glReadPixels(0,0,w,h,GL_DEPTH_COMPONENT,GL_FLOAT,
                 static_cast<GLfloat *>(NULL));
    
    state->Update();
    vtkIndent indent;
    vtksys_ios::ostringstream ost00;
    ost00.setf(ios::fixed,ios::floatfield);
    ost00.precision(5);
    ost00 << "OpenGLState_" << pthread_self() << "_" << vtkTimerLog::GetUniversalTime() << "_root00.txt";
    ofstream outfile00(ost00.str().c_str());
    state->PrintSelf(outfile00,indent);
    outfile00.close();
    
    this->PBO->Download2D(VTK_FLOAT,this->RawRGBABuffer,dims,1,continuousInc);
    
    state->Update();
    vtksys_ios::ostringstream ost01;
    ost01.setf(ios::fixed,ios::floatfield);
    ost01.precision(5);
    ost01 << "OpenGLState_" << pthread_self() << "_" << vtkTimerLog::GetUniversalTime() << "_root01.txt";
    ofstream outfile01(ost01.str().c_str());
    state->PrintSelf(outfile01,indent);
    outfile01.close();
    
    importer=vtkImageImport::New();
    importer->CopyImportVoidPointer(this->RawZBuffer,
                                    static_cast<int>(byteSize));
    importer->SetDataScalarTypeToFloat();
    importer->SetNumberOfScalarComponents(1);
    importer->SetWholeExtent(0,w-1,0,h-1,0,0);
    importer->SetDataExtentToWholeExtent();
    
    importer->Update();
    double range[2];
    importer->GetOutput()->GetPointData()->GetScalars()->GetRange(range);
    
    cout << "root0 scalar range=" << range[0] << "," << range[1] << endl;
    
    converter=vtkImageShiftScale::New();
    converter->SetInputConnection(importer->GetOutputPort());
    converter->SetOutputScalarTypeToUnsignedChar();
    converter->SetShift(0.0);
    converter->SetScale(255.0);
    
//      vtkImageExtractComponents *rgbaToRgb=vtkImageExtractComponents::New();
//    rgbaToRgb->SetInputConnection(importer->GetOutputPort());
//    rgbaToRgb->SetComponents(0,1,2);
    
    vtksys_ios::ostringstream ostxx;
    ostxx.setf(ios::fixed,ios::floatfield);
    ostxx.precision(5);
    timer->StopTimer();
    ostxx << "root0_" << vtkTimerLog::GetUniversalTime() << "_.png";
    
    vtkStdString *sssxx=new vtkStdString;
    (*sssxx)=ostxx.str();
    
    writer=vtkPNGWriter::New();
    writer->SetFileName(*sssxx);
    delete sssxx;
    writer->SetInputConnection(converter->GetOutputPort());
    importer->Delete();
//    rgbaToRgb->Delete();
    cout << "Writing " << writer->GetFileName() << endl;
    writer->Write();
    cout << "Wrote " << writer->GetFileName() << endl;
//    sleep(30);
    writer->Delete();
    
#endif // #ifdef VTK_COMPOSITE_RGBAPASS_DEBUG
    
    
    
    
    
    
    int proc=1;
    while(proc<numProcs)
      {
      // receive the zbuffer from satellite process.
      this->Controller->Receive(this->RawRGBABuffer,
                                static_cast<vtkIdType>(this->RawRGBABufferSize),
                                proc,VTK_COMPOSITE_RGBA_PASS_MESSAGE_GATHER);
      
      
#ifdef VTK_COMPOSITE_RGBAPASS_DEBUG
      importer=vtkImageImport::New();
      importer->CopyImportVoidPointer(this->RawZBuffer,
                                      static_cast<int>(byteSize));
      importer->SetDataScalarTypeToFloat();
      importer->SetNumberOfScalarComponents(1);
      importer->SetWholeExtent(0,w-1,0,h-1,0,0);
      importer->SetDataExtentToWholeExtent();
      
      converter=vtkImageShiftScale::New();
      converter->SetInputConnection(importer->GetOutputPort());
      converter->SetOutputScalarTypeToUnsignedChar();
      converter->SetShift(0.0);
      converter->SetScale(255.0);
      
//      vtkImageExtractComponents *rgbaToRgb=vtkImageExtractComponents::New();
//    rgbaToRgb->SetInputConnection(importer->GetOutputPort());
//    rgbaToRgb->SetComponents(0,1,2);
      
      writer=vtkPNGWriter::New();
      vtksys_ios::ostringstream ost;
      timer->StopTimer();
      ost.setf(ios::fixed,ios::floatfield);
      ost.precision(5);    
      ost << "root1_proc_" << proc << "_"<< vtkTimerLog::GetUniversalTime() << "_.png";
      
      vtkStdString *sss=new vtkStdString;
      (*sss)=ost.str();
      
      writer->SetFileName(*sss);
      delete sss;
      writer->SetInputConnection(converter->GetOutputPort());
      importer->Delete();
//    rgbaToRgb->Delete();
      cout << "Writing " << writer->GetFileName() << endl;
      writer->Write();
      cout << "Wrote " << writer->GetFileName() << endl;
//    sleep(30);
      writer->Delete();
#endif
      
      // send it to a PBO
      glPixelStorei(GL_UNPACK_ALIGNMENT,1); // client to server
      
#ifdef VTK_COMPOSITE_RGBAPASS_DEBUG
      state->Update();
      vtksys_ios::ostringstream ost02;
      ost02.setf(ios::fixed,ios::floatfield);
      ost02.precision(5);
      ost02 << "OpenGLState_" << pthread_self() << "_" << vtkTimerLog::GetUniversalTime() << "_root02_proc_" << proc <<"_"<<".txt";
      ofstream outfile02(ost02.str().c_str());
      state->PrintSelf(outfile02,indent);
      outfile02.close();
#endif
      
      this->PBO->Upload2D(VTK_FLOAT,this->RawRGBABuffer,dims,4,continuousInc);
      
#ifdef VTK_COMPOSITE_RGBAPASS_DEBUG
      state->Update();
      vtksys_ios::ostringstream ost03;
      ost03.setf(ios::fixed,ios::floatfield);
      ost03.precision(5);
      ost03 << "OpenGLState_" << pthread_self() << "_" << vtkTimerLog::GetUniversalTime() << "_root03_proc_" << proc << "_"<<".txt";
      ofstream outfile03(ost03.str().c_str());
      state->PrintSelf(outfile03,indent);
      outfile03.close();
      
      
      GLint value;
      glGetIntegerv(vtkgl::PIXEL_UNPACK_BUFFER_BINDING,&value);
      cout << pthread_self() << "compz pixel unpack buffer=" << value << endl;
      glGetIntegerv(vtkgl::PIXEL_PACK_BUFFER_BINDING,&value);
      cout << pthread_self() << "compz pixel unpack buffer=" << value << endl;
#endif
      
      // Send PBO to TO
      this->RGBATexture->Create2D(dims[0],dims[1],4,this->PBO,false);
      
      // Apply TO on quad with special rgbacomposite fragment shader.
      glPushAttrib(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
      glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
      glEnable(GL_BLEND);
      glDisable(GL_DEPTH_TEST);
      
      if(this->Program==0)
        {
        this->CreateProgram(context);
        }
      
      vtkTextureUnitManager *tu=context->GetTextureUnitManager();
      int sourceId=tu->Allocate();
      
#ifdef VTK_COMPOSITE_RGBAPASS_DEBUG
      cout << "sourceId=" << sourceId << endl;
#endif
      
      this->Program->GetUniformVariables()->SetUniformi("rgba",1,&sourceId);
      vtkgl::ActiveTexture(vtkgl::TEXTURE0+static_cast<GLenum>(sourceId));
      this->Program->Use();
      if(!this->Program->IsValid())
        {
        vtkErrorMacro("prog not valid in current OpenGL state");
        }
      
#ifdef VTK_COMPOSITE_RGBAPASS_DEBUG
      state->Update();
      vtksys_ios::ostringstream ost04;
      ost04.setf(ios::fixed,ios::floatfield);
      ost04.precision(5);
      ost04 << "OpenGLState_" << pthread_self() << "_" << vtkTimerLog::GetUniversalTime() << "_root_proc_" << proc << "_before_copyframe.txt";
      ofstream outfile04(ost04.str().c_str());
      state->PrintSelf(outfile04,indent);
      outfile04.close();
#endif
      
      this->RGBATexture->Bind();
      this->RGBATexture->CopyToFrameBuffer(0,0,
                                           w-1,h-1,
                                           0,0,w,h);
      
#ifdef VTK_COMPOSITE_RGBAPASS_DEBUG
      state->Update();
      vtksys_ios::ostringstream ost05;
      ost05.setf(ios::fixed,ios::floatfield);
      ost05.precision(5);
      ost05 << "OpenGLState_" << pthread_self() << "_" << vtkTimerLog::GetUniversalTime() << "_root_proc_" << proc << "_after_copyframe.txt";
      ofstream outfile05(ost05.str().c_str());
      state->PrintSelf(outfile05,indent);
      outfile05.close();
#endif
      
      this->RGBATexture->UnBind();
      this->Program->Restore();
       
      tu->Free(sourceId);
      vtkgl::ActiveTexture(vtkgl::TEXTURE0);
      
#ifdef VTK_COMPOSITE_RGBAPASS_DEBUG
      state->Update();
      vtksys_ios::ostringstream ost06;
      ost06.setf(ios::fixed,ios::floatfield);
      ost06.precision(5);
      ost06 << "OpenGLState_" << pthread_self() << "_" << vtkTimerLog::GetUniversalTime() << "_root_proc_" << proc << "_before_popattrib.txt";
      ofstream outfile06(ost06.str().c_str());
      state->PrintSelf(outfile06,indent);
      outfile06.close();
#endif
      
      glPopAttrib();
      
#ifdef VTK_COMPOSITE_RGBAPASS_DEBUG
      state->Update();
      vtksys_ios::ostringstream ost07;
      ost07.setf(ios::fixed,ios::floatfield);
      ost07.precision(5);
      ost07 << "OpenGLState_" << pthread_self() << "_" << vtkTimerLog::GetUniversalTime() << "_root_proc_" << proc << "_after_popattrib.txt";
      ofstream outfile07(ost07.str().c_str());
      state->PrintSelf(outfile07,indent);
      outfile07.close();
#endif
      
      ++proc;
      }
    
    // root node Done.
    }
  else
    {
    // satellite
    // send rgba-buffer
    
    // framebuffer to PBO.
    this->PBO->Allocate(byteSize);
    this->PBO->Bind(vtkPixelBufferObject::PACKED_BUFFER);
    glReadPixels(0,0,w,h,GL_RGBA,GL_FLOAT,
                 static_cast<GLfloat *>(NULL));
    
    // PBO to client
    this->PBO->Download2D(VTK_FLOAT,this->RawRGBABuffer,dims,4,continuousInc);
    
#ifdef VTK_COMPOSITE_RGBAPASS_DEBUG
    importer=vtkImageImport::New();
    importer->CopyImportVoidPointer(this->RawZBuffer,
                                    static_cast<int>(byteSize));
    importer->SetDataScalarTypeToFloat();
    importer->SetNumberOfScalarComponents(1);
    importer->SetWholeExtent(0,w-1,0,h-1,0,0);
    importer->SetDataExtentToWholeExtent();
    
    importer->Update();
    double range[2];
    importer->GetOutput()->GetPointData()->GetScalars()->GetRange(range);
    
    cout << " scalar range=" << range[0] << "," << range[1] << endl;
    
    converter=vtkImageShiftScale::New();
    converter->SetInputConnection(importer->GetOutputPort());
    converter->SetOutputScalarTypeToUnsignedChar();
    converter->SetShift(0.0);
    converter->SetScale(255.0);
    
//      vtkImageExtractComponents *rgbaToRgb=vtkImageExtractComponents::New();
//    rgbaToRgb->SetInputConnection(importer->GetOutputPort());
//    rgbaToRgb->SetComponents(0,1,2);
    
    vtksys_ios::ostringstream ost;
    ost.setf(ios::fixed,ios::floatfield);
    ost.precision(5);
    timer->StopTimer();
    ost << "satellite1_"<< vtkTimerLog::GetUniversalTime() << "_.png";
    
    vtkStdString *sss=new vtkStdString;
    (*sss)=ost.str();
    
    writer=vtkPNGWriter::New();
    writer->SetFileName(*sss);
    delete sss;
    writer->SetInputConnection(converter->GetOutputPort());
    importer->Delete();
//    rgbaToRgb->Delete();
    cout << "Writing " << writer->GetFileName() << endl;
    writer->Write();
    cout << "Wrote " << writer->GetFileName() << endl;
//    sleep(30);
    writer->Delete();
#endif
    
    
    // client to root process
    this->Controller->Send(this->RawRGBABuffer,
                           static_cast<vtkIdType>(this->RawRGBABufferSize),0,
                           VTK_COMPOSITE_RGBA_PASS_MESSAGE_GATHER);
    }
#ifdef VTK_COMPOSITE_RGBAPASS_DEBUG
  delete state;
  timer->Delete();
#endif
}

// ----------------------------------------------------------------------------
void vtkCompositeRGBAPass::CreateProgram(vtkOpenGLRenderWindow *context)
{
  assert("pre: context_exists" && context!=0);
  assert("pre: Program_void" && this->Program==0);
  
  this->Program=vtkShaderProgram2::New();
  this->Program->SetContext(context);
  
  vtkShader2 *shader=vtkShader2::New();
  shader->SetContext(context);
  
  this->Program->GetShaders()->AddItem(shader);
  shader->Delete();
  shader->SetType(VTK_SHADER_TYPE_FRAGMENT);
  shader->SetSourceCode(vtkCompositeRGBAPassShader_fs);
  this->Program->Build();
  if(this->Program->GetLastBuildStatus()!=VTK_SHADER_PROGRAM2_LINK_SUCCEEDED)
    {
    vtkErrorMacro("prog build failed");
    }
  
  assert("post: Program_exists" && this->Program!=0);
}

// ----------------------------------------------------------------------------
// Description:
// Release graphics resources and ask components to release their own
// resources.
// \pre w_exists: w!=0
void vtkCompositeRGBAPass::ReleaseGraphicsResources(vtkWindow *w)
{
  assert("pre: w_exists" && w!=0);

  (void)w;
  
  if(this->PBO!=0)
    {
    this->PBO->Delete();
    this->PBO=0;
    }
  if(this->RGBATexture!=0)
    {
    this->RGBATexture->Delete();
    this->RGBATexture=0;
    }
  if(this->Program!=0)
    {
    this->Program->ReleaseGraphicsResources();
    }
}
