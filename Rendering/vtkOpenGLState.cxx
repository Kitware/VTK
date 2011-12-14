#include "vtkOpenGLState.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkIndent.h"
#include "vtkOpenGLExtensionManager.h"

const char *ValueToString(GLint value,
                          int valueTable[],
                          const char *stringTable[],
                          int tableSize);

vtkOpenGLState::vtkOpenGLState(vtkOpenGLRenderWindow *context)
{
  this->Context=context;
  vtkOpenGLExtensionManager* m=this->Context->GetExtensionManager();
  
  m->LoadExtension("GL_VERSION_1_2");
  m->LoadExtension("GL_VERSION_1_3");
  m->LoadExtension("GL_VERSION_1_4");
  m->LoadExtension("GL_VERSION_1_5");
  m->LoadExtension("GL_VERSION_2_0");
  m->LoadExtension("GL_VERSION_2_1");
  m->LoadExtension("GL_EXT_framebuffer_object");
  
  this->TCPU=0;
  this->TIU=0;
  
  this->FixedPipeline.TextureImageUnitEnabled=0;
  this->FixedPipeline.LightEnabled=0;
  
  this->ClipPlanes=0;
  this->Lights=0;
  
  this->DrawBuffers=0;
  
  this->CurrentProgram=0;
  this->CurrentProgramState=0;
}

vtkOpenGLState::~vtkOpenGLState()
{
  if(this->TCPU!=0)
    {
    delete this->TCPU;
    }
  if(this->TIU!=0)
    {
    delete this->TIU;
    }
  if(this->FixedPipeline.TextureImageUnitEnabled!=0)
    {
    delete this->FixedPipeline.TextureImageUnitEnabled;
    }
  
  if(this->ClipPlanes!=0)
    {
    delete this->ClipPlanes;
    }
  if(this->FixedPipeline.LightEnabled!=0)
    {
    delete this->FixedPipeline.LightEnabled;
    delete this->Lights;
    }
  
  if(this->DrawBuffers!=0)
    {
    delete this->DrawBuffers;
    }
  if(this->CurrentProgramState!=0)
    {
    delete this->CurrentProgramState;
    }
}

void vtkOpenGLState::Update()
{
  // Unfortunately the binding points in OpenGL are used both for use and editing
  // For example, if the active texture unit is 4 and you want to query the state for
  // texture unit 3, you first have to switch the active texture unit to be 3. In other
  // words, you have to change the some part of the state of OpenGL to query some other
  // part of the state! So you have to make sure you restore the original state after
  // the query!!!
  
  GLint ivalues[4];
  
  this->ErrorCode=glGetError(); // this change the state..
  // Texture environment
  // Has to be restored to this value.
  glGetIntegerv(vtkgl::ACTIVE_TEXTURE,ivalues);
  this->ActiveTexture=static_cast<GLenum>(ivalues[0]);
  
  glGetIntegerv(vtkgl::FRAMEBUFFER_BINDING_EXT,&this->FrameBufferBinding);
  
  this->UpdateCurrentProgram();
  
  // max number of TCPU
  glGetIntegerv(vtkgl::MAX_TEXTURE_COORDS,&this->MaxTextureCoords); // 8
  
  // Max number of TIU available to the fixed-pipeline (enable/disable state)
  glGetIntegerv(vtkgl::MAX_TEXTURE_UNITS,&this->MaxTextureUnits); // 4
  
  // Max number of TIU.
  glGetIntegerv(vtkgl::MAX_COMBINED_TEXTURE_IMAGE_UNITS,
                &this->MaxCombinedTextureImageUnits); // 16
  
  // max number of TIU available from a vertex shader.
  glGetIntegerv(vtkgl::MAX_VERTEX_TEXTURE_IMAGE_UNITS,
                &this->MaxVertexTextureImageUnits);
  
  // max number of TIU available from a fragment shader.
  glGetIntegerv(vtkgl::MAX_TEXTURE_IMAGE_UNITS,&this->MaxTextureImageUnits);
  
  
  if(this->TCPU!=0 &&
     this->TCPU->size()!=static_cast<size_t>(this->MaxTextureCoords))
    {
    delete this->TCPU;
    this->TIU=0;
    }
  if(this->TCPU==0)
    {
    this->TCPU=new std::vector<vtkOpenGLTextureCoordinateProcessingUnit>(
      static_cast<size_t>(this->MaxTextureCoords));
    }
  
  unsigned int i=0;
  while(i<static_cast<size_t>(this->MaxTextureCoords))
    {
    vtkgl::ActiveTexture(vtkgl::TEXTURE0+i);
    glGetFloatv(GL_TEXTURE_MATRIX,
                (*this->TCPU)[i].CurrentMatrix);
    glGetIntegerv(GL_TEXTURE_STACK_DEPTH,
                  &(*this->TCPU)[i].MatrixStackDepth);
    ++i;
    }
  
  if(this->TIU!=0 && this->TIU->size()!=static_cast<size_t>(
       this->MaxCombinedTextureImageUnits))
    {
    delete this->TIU;
    this->TIU=0;
    }
  if(this->TIU==0)
    {
    this->TIU=new std::vector<vtkOpenGLTextureImageUnit>(
      static_cast<size_t>(this->MaxCombinedTextureImageUnits));
    }
  
  i=0;
  while(i<static_cast<size_t>(this->MaxCombinedTextureImageUnits))
    {
    vtkgl::ActiveTexture(vtkgl::TEXTURE0+i);
    glGetIntegerv(GL_TEXTURE_BINDING_1D,&(*this->TIU)[i].TextureBinding1D);
    glGetIntegerv(GL_TEXTURE_BINDING_2D,&(*this->TIU)[i].TextureBinding2D);
    glGetIntegerv(vtkgl::TEXTURE_BINDING_3D,&(*this->TIU)[i].TextureBinding3D);
    glGetIntegerv(vtkgl::TEXTURE_BINDING_CUBE_MAP,&(*this->TIU)[i].TextureBindingCubeMap);  
    ++i;
    }
  
  glGetIntegerv(GL_MAX_CLIP_PLANES,&this->MaxClipPlanes);
  if(this->ClipPlanes!=0 &&
     this->ClipPlanes->size()!=static_cast<size_t>(this->MaxClipPlanes))
    {
    delete this->ClipPlanes;
    this->ClipPlanes=0;
    }
  if(this->ClipPlanes==0)
    {
    this->ClipPlanes=new std::vector<vtkOpenGLClipPlaneState>(
      static_cast<size_t>(this->MaxClipPlanes));
    }
  
  
  glGetIntegerv(GL_MAX_LIGHTS,&this->MaxLights);
  if(this->FixedPipeline.LightEnabled!=0 && this->FixedPipeline.LightEnabled->size()!=static_cast<size_t>(this->MaxLights))
    {
    delete this->FixedPipeline.LightEnabled;
    this->FixedPipeline.LightEnabled=0;
    
    delete Lights;
    this->Lights=0;   
    }
  if(this->FixedPipeline.LightEnabled==0)
    {
    this->FixedPipeline.LightEnabled=
      new std::vector<GLboolean>(static_cast<size_t>(this->MaxLights));
    this->Lights=
      new std::vector<vtkOpenGLLightState>(
        static_cast<size_t>(this->MaxLights));
    }
  
  if(this->FixedPipeline.TextureImageUnitEnabled!=0 &&
     this->FixedPipeline.TextureImageUnitEnabled->size()!=
     static_cast<size_t>(this->MaxTextureUnits))
    {
    delete this->FixedPipeline.TextureImageUnitEnabled;
    this->FixedPipeline.TextureImageUnitEnabled=0;
    }
  if(this->FixedPipeline.TextureImageUnitEnabled==0)
    {
    this->FixedPipeline.TextureImageUnitEnabled=
      new std::vector<vtkOpenGLTextureImageUnitFixedPipelineState>(
        static_cast<size_t>(this->MaxTextureUnits));
    }
  
  i=0;
  while(i<static_cast<size_t>(this->MaxTextureUnits))
    {
    vtkgl::ActiveTexture(vtkgl::TEXTURE0+i);
    (*this->FixedPipeline.TextureImageUnitEnabled)[i].Texture1DEnabled=
      glIsEnabled(GL_TEXTURE_1D);
    (*this->FixedPipeline.TextureImageUnitEnabled)[i].Texture2DEnabled=
      glIsEnabled(GL_TEXTURE_2D);
    (*this->FixedPipeline.TextureImageUnitEnabled)[i].Texture3DEnabled=
      glIsEnabled(vtkgl::TEXTURE_3D);
    (*this->FixedPipeline.TextureImageUnitEnabled)[i].TextureCubeMapEnabled=
      glIsEnabled(vtkgl::TEXTURE_CUBE_MAP);
    ++i;
    }
  
  // Restore real active texture
  vtkgl::ActiveTexture(this->ActiveTexture);
  
  
  glGetFloatv(GL_MODELVIEW_MATRIX,this->ModelViewMatrix);
  glGetIntegerv(GL_MODELVIEW_STACK_DEPTH,&this->ModelViewStackDepth);
  
  glGetFloatv(GL_PROJECTION_MATRIX,this->ProjectionMatrix);
  glGetIntegerv(GL_PROJECTION_STACK_DEPTH,&this->ProjectionStackDepth);
  
  glGetIntegerv(GL_VIEWPORT,this->Viewport);
  glGetFloatv(GL_DEPTH_RANGE,this->DepthRange);
  
  glGetIntegerv(GL_MATRIX_MODE,&this->MatrixMode);
  
  // fragment fixed-pipeline
  this->FixedPipeline.ColorSumEnabled=glIsEnabled(vtkgl::COLOR_SUM);
  
  glGetIntegerv(GL_SHADE_MODEL,&this->ShadeModel);
  
  // vertex fixed-pipeline
  this->FixedPipeline.LightingEnabled=glIsEnabled(GL_LIGHTING);
  
  // rasterization
  this->CullFaceEnabled=glIsEnabled(GL_CULL_FACE);
  glGetIntegerv(GL_CULL_FACE_MODE,&this->CullFaceMode);
  glGetIntegerv(GL_FRONT_FACE,&this->FrontFace);
  this->PolygonSmoothEnabled=glIsEnabled(GL_POLYGON_SMOOTH);
  glGetIntegerv(GL_POLYGON_MODE,this->PolygonMode);
  glGetFloatv(GL_POLYGON_OFFSET_FACTOR,&this->PolygonOffsetFactor);
  glGetFloatv(GL_POLYGON_OFFSET_UNITS,&this->PolygonOffsetUnits);
  this->PolygonOffsetPointEnabled=glIsEnabled(GL_POLYGON_OFFSET_POINT);
  this->PolygonOffsetLineEnabled=glIsEnabled(GL_POLYGON_OFFSET_LINE);
  this->PolygonOffsetFillEnabled=glIsEnabled(GL_POLYGON_OFFSET_FILL);
  this->PolygonStippleEnabled=glIsEnabled(GL_POLYGON_STIPPLE);
  
  // multisampling
  this->MultiSampleEnabled=glIsEnabled(vtkgl::MULTISAMPLE);
  this->SampleAlphaToCoverageEnabled=glIsEnabled(vtkgl::SAMPLE_ALPHA_TO_COVERAGE);
  this->SampleAlphaToOneEnabled=glIsEnabled(vtkgl::SAMPLE_ALPHA_TO_ONE);
  this->SampleCoverageEnabled=glIsEnabled(vtkgl::SAMPLE_COVERAGE);
  glGetFloatv(vtkgl::SAMPLE_COVERAGE_VALUE,&this->SampleCoverageValue);
  glGetBooleanv(vtkgl::SAMPLE_COVERAGE_INVERT,&this->SampleCoverageInvert);
  
  // pixel operations
  this->ScissorTestEnabled=glIsEnabled(GL_SCISSOR_TEST);
  glGetIntegerv(GL_SCISSOR_BOX,this->ScissorBox);
  
  this->AlphaTestEnabled=glIsEnabled(GL_ALPHA_TEST);
  glGetIntegerv(GL_ALPHA_TEST_FUNC,&this->AlphaTestFunc);
  glGetFloatv(GL_ALPHA_TEST_REF,&this->AlphaTestRef);
  
  this->StencilTestEnabled=glIsEnabled(GL_STENCIL_TEST);
  this->DepthTestEnabled=glIsEnabled(GL_DEPTH_TEST);
  glGetIntegerv(GL_DEPTH_FUNC,&this->DepthFunc);
  
  this->BlendEnabled=glIsEnabled(GL_BLEND);
  glGetIntegerv(vtkgl::BLEND_SRC_RGB,&this->BlendSrcRGB);
  glGetIntegerv(vtkgl::BLEND_SRC_ALPHA,&this->BlendSrcAlpha);
  glGetIntegerv(vtkgl::BLEND_DST_RGB,&this->BlendDstRGB);
  glGetIntegerv(vtkgl::BLEND_DST_ALPHA,&this->BlendDstAlpha);
  glGetIntegerv(vtkgl::BLEND_EQUATION_RGB,&this->BlendEquationRGB);
  glGetIntegerv(vtkgl::BLEND_EQUATION_ALPHA,&this->BlendEquationAlpha);
  glGetFloatv(vtkgl::BLEND_COLOR,this->BlendColor);
  
  
  this->DitherEnabled=glIsEnabled(GL_DITHER);
  
  this->IndexLogicOpEnabled=glIsEnabled(GL_INDEX_LOGIC_OP);
  this->ColorLogicOpEnabled=glIsEnabled(GL_COLOR_LOGIC_OP);
  glGetIntegerv(GL_LOGIC_OP_MODE,&this->LogicOpMode);
  
  // framebuffer control
  glGetIntegerv(vtkgl::MAX_DRAW_BUFFERS,&this->MaxDrawBuffers);
  
  if(this->DrawBuffers!=0)
    {
    delete this->DrawBuffers;
    this->DrawBuffers=0;
    }
  if(this->DrawBuffers==0)
    {
    this->DrawBuffers=new std::vector<GLint>(
      static_cast<size_t>(this->MaxDrawBuffers));
    }
  i=0;
  while(i<static_cast<size_t>(this->MaxDrawBuffers))
    {
    glGetIntegerv(vtkgl::DRAW_BUFFER0+i,&(*this->DrawBuffers)[i]);
    ++i;
    }
  
  glGetIntegerv(GL_INDEX_WRITEMASK,&this->IndexWriteMask);
  glGetBooleanv(GL_COLOR_WRITEMASK,this->ColorWriteMask);
  glGetBooleanv(GL_DEPTH_WRITEMASK,&this->DepthWriteMask);
  glGetIntegerv(GL_STENCIL_WRITEMASK,ivalues);
  this->StencilWriteMask=static_cast<GLuint>(ivalues[0]);
  glGetIntegerv(vtkgl::STENCIL_BACK_WRITEMASK,ivalues);
  this->StencilBackWriteMask=static_cast<GLuint>(ivalues[0]);
  glGetFloatv(GL_COLOR_CLEAR_VALUE,this->ColorClearValue);
  glGetFloatv(GL_INDEX_CLEAR_VALUE,&this->IndexClearValue);
  glGetFloatv(GL_DEPTH_CLEAR_VALUE,&this->DepthClearValue);
  glGetIntegerv(GL_STENCIL_CLEAR_VALUE,&this->StencilClearValue);
  glGetFloatv(GL_ACCUM_CLEAR_VALUE,this->AccumClearValue);
  
  // pixels
  
  glGetBooleanv(GL_UNPACK_SWAP_BYTES,&this->Unpack.SwapBytes);
  glGetBooleanv(GL_UNPACK_LSB_FIRST,&this->Unpack.LsbFirst);
  glGetIntegerv(vtkgl::UNPACK_IMAGE_HEIGHT,&this->Unpack.ImageHeight);
  glGetIntegerv(vtkgl::UNPACK_SKIP_IMAGES,&this->Unpack.SkipImages);
  glGetIntegerv(GL_UNPACK_ROW_LENGTH,&this->Unpack.RowLength);
  glGetIntegerv(GL_UNPACK_SKIP_ROWS,&this->Unpack.SkipRows);
  glGetIntegerv(GL_UNPACK_SKIP_PIXELS,&this->Unpack.SkipPixels);
  glGetIntegerv(GL_UNPACK_ALIGNMENT,&this->Unpack.Alignment);
  
  glGetBooleanv(GL_PACK_SWAP_BYTES,&this->Pack.SwapBytes);
  glGetBooleanv(GL_PACK_LSB_FIRST,&this->Pack.LsbFirst);
  glGetIntegerv(vtkgl::PACK_IMAGE_HEIGHT,&this->Pack.ImageHeight);
  glGetIntegerv(vtkgl::PACK_SKIP_IMAGES,&this->Pack.SkipImages);
  glGetIntegerv(GL_PACK_ROW_LENGTH,&this->Pack.RowLength);
  glGetIntegerv(GL_PACK_SKIP_ROWS,&this->Pack.SkipRows);
  glGetIntegerv(GL_PACK_SKIP_PIXELS,&this->Pack.SkipPixels);
  glGetIntegerv(GL_PACK_ALIGNMENT,&this->Pack.Alignment);
  
  glGetIntegerv(vtkgl::PIXEL_PACK_BUFFER_BINDING,ivalues);
  this->PixelPackBufferBinding=static_cast<GLenum>(ivalues[0]);
  
  glGetIntegerv(vtkgl::PIXEL_UNPACK_BUFFER_BINDING,ivalues);
  this->PixelUnpackBufferBinding=static_cast<GLenum>(ivalues[0]);
  
  if(this->PixelPackBufferBinding>0)
    {
    this->PixelPackBufferObject.Id=this->PixelPackBufferBinding;
    vtkgl::GetBufferParameteriv(vtkgl::PIXEL_PACK_BUFFER,vtkgl::BUFFER_SIZE,
                                &this->PixelPackBufferObject.Size);
    vtkgl::GetBufferParameteriv(vtkgl::PIXEL_PACK_BUFFER,vtkgl::BUFFER_USAGE,
                                ivalues);
    this->PixelPackBufferObject.Usage=static_cast<GLenum>(ivalues[0]);
    vtkgl::GetBufferParameteriv(vtkgl::PIXEL_PACK_BUFFER,vtkgl::BUFFER_ACCESS,
                                ivalues);
    this->PixelPackBufferObject.Access=static_cast<GLenum>(ivalues[0]);
    
#if 0 // buggy header files (2009/05/05)
    vtkgl::GetBufferParameteriv(vtkgl::PIXEL_PACK_BUFFER,
                                vtkgl::BUFFER_ACCESS_FLAGS,
                                &this->PixelPackBufferObject.AccessFlags);
#endif
    
    vtkgl::GetBufferParameteriv(vtkgl::PIXEL_PACK_BUFFER,
                                vtkgl::BUFFER_MAPPED,
                                ivalues);
    this->PixelPackBufferObject.Mapped=static_cast<GLboolean>(ivalues[0]);
    vtkgl::GetBufferPointerv(vtkgl::PIXEL_PACK_BUFFER,
                             vtkgl::BUFFER_MAP_POINTER,
                             &(this->PixelPackBufferObject.MapPointer));

#if 0 // buggy header files (2009/05/05)
    vtkgl::GetBufferParameteriv(vtkgl::PIXEL_PACK_BUFFER,
                                vtkgl::BUFFER_MAP_OFFSET,
                                &this->PixelPackBufferObject.MapOffset);
    vtkgl::GetBufferParameteriv(vtkgl::PIXEL_PACK_BUFFER,
                                vtkgl::BUFFER_MAP_LENGTH,
                                &this->PixelPackBufferObject.MapLength);
#endif    
    }
  if(this->PixelUnpackBufferBinding>0)
    {
    this->PixelUnpackBufferObject.Id=PixelUnpackBufferBinding;
    this->PixelUnpackBufferObject.Id=this->PixelUnpackBufferBinding;
    vtkgl::GetBufferParameteriv(vtkgl::PIXEL_UNPACK_BUFFER,vtkgl::BUFFER_SIZE,
                                &this->PixelUnpackBufferObject.Size);
    vtkgl::GetBufferParameteriv(vtkgl::PIXEL_UNPACK_BUFFER,vtkgl::BUFFER_USAGE,
                                ivalues);
    this->PixelUnpackBufferObject.Usage=static_cast<GLenum>(ivalues[0]);
    vtkgl::GetBufferParameteriv(vtkgl::PIXEL_UNPACK_BUFFER,
                                vtkgl::BUFFER_ACCESS,
                                ivalues);
    this->PixelUnpackBufferObject.Access=static_cast<GLenum>(ivalues[0]);
#if 0 // buggy header files (2009/05/05)
    vtkgl::GetBufferParameteriv(vtkgl::PIXEL_UNPACK_BUFFER,
                                vtkgl::BUFFER_ACCESS_FLAGS,
                                &this->PixelUnpackBufferObject.AccessFlags);
#endif
    vtkgl::GetBufferParameteriv(vtkgl::PIXEL_UNPACK_BUFFER,
                                vtkgl::BUFFER_MAPPED,
                                ivalues);
    this->PixelUnpackBufferObject.Mapped=static_cast<GLboolean>(ivalues[0]);
    
    vtkgl::GetBufferPointerv(vtkgl::PIXEL_UNPACK_BUFFER,
                             vtkgl::BUFFER_MAP_POINTER,
                             &(this->PixelPackBufferObject.MapPointer));
    
#if 0 // buggy header files (2009/05/05)
    vtkgl::GetBufferParameteriv(vtkgl::PIXEL_UNPACK_BUFFER,
                                vtkgl::BUFFER_MAP_OFFSET,
                                &this->PixelUnpackBufferObject.MapOffset);
    vtkgl::GetBufferParameteriv(vtkgl::PIXEL_UNPACK_BUFFER,
                                vtkgl::BUFFER_MAP_LENGTH,
                                &this->PixelUnpackBufferObject.MapLength);
#endif
    }
  
  glGetFloatv(GL_RED_SCALE,&this->RedTransform.Scale);
  glGetFloatv(GL_RED_BIAS,&this->RedTransform.Bias);
  glGetFloatv(GL_GREEN_SCALE,&this->GreenTransform.Scale);
  glGetFloatv(GL_GREEN_BIAS,&this->GreenTransform.Bias);
  glGetFloatv(GL_BLUE_SCALE,&this->BlueTransform.Scale);
  glGetFloatv(GL_BLUE_BIAS,&this->BlueTransform.Bias);
  glGetFloatv(GL_ALPHA_SCALE,&this->AlphaTransform.Scale);
  glGetFloatv(GL_ALPHA_BIAS,&this->AlphaTransform.Bias);
  glGetFloatv(GL_DEPTH_SCALE,&this->DepthTransform.Scale);
  glGetFloatv(GL_DEPTH_BIAS,&this->DepthTransform.Bias);
  
  glGetFloatv(GL_ZOOM_X,&this->ZoomX);
  glGetFloatv(GL_ZOOM_Y,&this->ZoomY);
  
  glGetIntegerv(GL_READ_BUFFER,&this->ReadBuffer);
  
  glGetIntegerv(GL_AUX_BUFFERS,&this->AuxBuffers);
  glGetBooleanv(GL_RGBA_MODE,&this->RGBAMode);
  glGetBooleanv(GL_INDEX_MODE,&this->IndexMode);
  glGetBooleanv(GL_DOUBLEBUFFER,&this->DoubleBuffer);
  glGetBooleanv(GL_STEREO,&this->Stereo);
  
  glGetIntegerv(vtkgl::MAX_COLOR_ATTACHMENTS,&this->MaxColorAttachments);
  
  glGetIntegerv(GL_LIST_BASE,&this->ListBase);
  glGetIntegerv(GL_LIST_INDEX,&this->ListIndex);
  if(this->ListIndex!=0)
    {
    glGetIntegerv(GL_LIST_MODE,&this->ListMode);
    }
  else
    {
    this->ListMode=0; // not relevant
    }
  
  glGetIntegerv(GL_RENDER_MODE,&this->RenderMode);
}

void vtkOpenGLState::UpdateCurrentProgram()
{
  GLint ivalues[4];
  
  if(this->CurrentProgramState!=0)
    {
    delete this->CurrentProgramState;
    this->CurrentProgramState=0;
    }
  
  glGetIntegerv(vtkgl::CURRENT_PROGRAM,ivalues);
  this->CurrentProgram=static_cast<GLuint>(ivalues[0]);
  if(this->CurrentProgram!=0)
    {
    this->CurrentProgramState=new vtkOpenGLProgramState;
    this->CurrentProgramState->Id=this->CurrentProgram;
    GLuint progId=this->CurrentProgramState->Id;
    
    vtkgl::GetProgramiv(progId,vtkgl::DELETE_STATUS,ivalues);
    this->CurrentProgramState->DeleteStatus=static_cast<GLboolean>(ivalues[0]);
    
    vtkgl::GetProgramiv(progId,vtkgl::LINK_STATUS,ivalues);
    this->CurrentProgramState->LinkStatus=static_cast<GLboolean>(ivalues[0]);
    
    vtkgl::GetProgramiv(progId,vtkgl::VALIDATE_STATUS,ivalues);
    this->CurrentProgramState->ValidateStatus=static_cast<GLboolean>(ivalues[0]);
    
    vtkgl::GetProgramiv(progId,vtkgl::INFO_LOG_LENGTH,
                        &this->CurrentProgramState->InfoLogLength);
    vtkgl::GetProgramiv(progId,vtkgl::ATTACHED_SHADERS,
                        &this->CurrentProgramState->NumberOfAttachedShaders);
    vtkgl::GetProgramiv(progId,vtkgl::ACTIVE_ATTRIBUTES,
                        &this->CurrentProgramState->ActiveAttributes);
    vtkgl::GetProgramiv(progId,
                        vtkgl::ACTIVE_ATTRIBUTE_MAX_LENGTH,
                        &this->CurrentProgramState->ActiveAttributeMaxLength);
    vtkgl::GetProgramiv(progId,vtkgl::ACTIVE_UNIFORMS,
                        &this->CurrentProgramState->ActiveUniforms);
    vtkgl::GetProgramiv(progId,
                        vtkgl::ACTIVE_UNIFORM_MAX_LENGTH,
                        &this->CurrentProgramState->ActiveUniformMaxLength);
    
    GLsizei numberOfShaders=this->CurrentProgramState->NumberOfAttachedShaders;
    this->CurrentProgramState->AttachedShaders=
      new std::vector<vtkOpenGLShaderState>(
        static_cast<size_t>(numberOfShaders));
    
    GLuint *shaders=new GLuint[numberOfShaders];
    
    vtkgl::GetAttachedShaders(progId,numberOfShaders,0,shaders);
    
    size_t i=0;
    this->CurrentProgramState->HasVertexShader=false;
    this->CurrentProgramState->HasFragmentShader=false;
    while(i<static_cast<size_t>(numberOfShaders))
      {
      (*this->CurrentProgramState->AttachedShaders)[i].Id=shaders[i];
      this->UpdateShader(i);
      this->CurrentProgramState->HasVertexShader=
        this->CurrentProgramState->HasVertexShader||
        (*this->CurrentProgramState->AttachedShaders)[i].Type
        ==vtkgl::VERTEX_SHADER;
      this->CurrentProgramState->HasFragmentShader=
         this->CurrentProgramState->HasFragmentShader||
        (*this->CurrentProgramState->AttachedShaders)[i].Type
        ==vtkgl::FRAGMENT_SHADER;
      ++i;
      }
    
    delete[] shaders;
    
    this->CurrentProgramState->InfoLog=
      new vtkgl::GLchar[this->CurrentProgramState->InfoLogLength];
    
    vtkgl::GetProgramInfoLog(progId,this->CurrentProgramState->InfoLogLength,
                             0,this->CurrentProgramState->InfoLog);
    
    // Active vertex attributes
    // TODO
    
    
    // Active uniforms
    }
}

void vtkOpenGLState::UpdateShader(size_t i)
{
  // Id is already initialized by UpdateCurrentProgram().
  
  vtkOpenGLShaderState *s=&((*this->CurrentProgramState->AttachedShaders)[i]);
  
  GLint ivalues[4];
  
  vtkgl::GetShaderiv(s->Id,vtkgl::SHADER_TYPE,ivalues);
  s->Type=static_cast<GLenum>(ivalues[0]);
  
  vtkgl::GetShaderiv(s->Id,vtkgl::DELETE_STATUS,ivalues);
  s->DeleteStatus=static_cast<GLboolean>(ivalues[0]);
  
  vtkgl::GetShaderiv(s->Id,vtkgl::COMPILE_STATUS,ivalues);
  s->CompileStatus=static_cast<GLboolean>(ivalues[0]);
  vtkgl::GetShaderiv(s->Id,vtkgl::INFO_LOG_LENGTH,&(s->InfoLogLength));
  vtkgl::GetShaderiv(s->Id,vtkgl::SHADER_SOURCE_LENGTH,&(s->SourceLength));
  
  s->InfoLog=new vtkgl::GLchar[s->InfoLogLength];
  vtkgl::GetShaderInfoLog(s->Id,s->InfoLogLength,0,s->InfoLog);
  
  s->Source=new vtkgl::GLchar[s->SourceLength];
  vtkgl::GetShaderSource(s->Id,s->SourceLength,0,s->Source);
}



void vtkOpenGLState::PrintSelf(ostream &os,
                               vtkIndent indent)
{
  os << indent << "**** OpenGLState ****"<< endl;
  
  os << indent << "ErrorCode: " << this->ErrorCodeToString() << endl;
  
  os << indent << "FrameBufferBinding (drawFB and readFB)=";
  if(this->FrameBufferBinding==0)
    {
    os << "0 (default framebuffer)" << endl;
    }
  else
    {
    os << this->FrameBufferBinding << endl;
    }
  
  os << indent << "CurrentProgram=";
  if(this->CurrentProgram==0)
    {
    os << "0 (fixed-pipeline)" << endl;
    }
  else
    {
    os << this->CurrentProgram << endl;
    this->CurrentProgramState->PrintSelf(os,indent);
    }
  
  os << indent << "ModelViewMatrix=" << endl;
  this->PrintMatrix(os,indent,this->ModelViewMatrix);
  os << indent << "ModelViewStackDepth="<< this->ModelViewStackDepth << endl;
  os << indent << "ProjectionMatrix=" << endl;
  this->PrintMatrix(os,indent,this->ProjectionMatrix);
  os << indent << "ProjectionStackDepth=" << this->ProjectionStackDepth 
     << endl;
  
  size_t i;
  
  if(this->CurrentProgram==0 || !this->CurrentProgramState->HasVertexShader)
    {
    i=0;
    while(i<static_cast<size_t>(this->MaxTextureCoords))
      {
      os << indent << "TextureCoordinateProcessingUnit " << i << ":" <<endl;
      this->PrintMatrix(os,indent,(*this->TCPU)[i].CurrentMatrix);
      os << indent << "TextureStackDepth=" << (*this->TCPU)[i].MatrixStackDepth 
         << endl;
      ++i;
      }
    }
  
  i=0;
  while(i<static_cast<size_t>(this->MaxCombinedTextureImageUnits))
    {
    // only display texture unit with at least one binding, otherwise it is
    // too verbose.
    
    if((*this->TIU)[i].TextureBinding1D!=0 ||
       (*this->TIU)[i].TextureBinding2D!=0 ||
       (*this->TIU)[i].TextureBinding3D!=0 ||
       (*this->TIU)[i].TextureBindingCubeMap!=0)
      {
      os << indent << "TextureImageUnit " << i << ":" <<endl;
      os << indent << " Binding1D=" << (*this->TIU)[i].TextureBinding1D << endl;
      os << indent << " Binding2D=" << (*this->TIU)[i].TextureBinding2D << endl;
      os << indent << " Binding3D=" << (*this->TIU)[i].TextureBinding3D << endl;
      os << indent << " BindingCubeMap=" << (*this->TIU)[i].TextureBindingCubeMap << endl;
      }
      ++i;
    }
  
  if(this->CurrentProgram==0 || !this->CurrentProgramState->HasVertexShader)
    {
    os << indent << "fixed-pipeline vertex shader flags:" << endl;
    os << indent << " LightingEnabled=" << static_cast<bool>(this->FixedPipeline.LightingEnabled==GL_TRUE) << endl;
    os << indent << " ColorSumEnabled=" << static_cast<bool>(this->FixedPipeline.ColorSumEnabled==GL_TRUE) << endl;
    }
  
  if(this->CurrentProgram==0 || !this->CurrentProgramState->HasFragmentShader)
    {
    os << indent << " fixed-pipeline texture flags:" <<endl;
    i=0;
    while(i<static_cast<size_t>(this->MaxTextureUnits))
      {
      // only display texture unit with at least one enabled flag,
      // otherwise it is too verbose.
      if((*this->FixedPipeline.TextureImageUnitEnabled)[i].Texture1DEnabled==GL_TRUE||
         (*this->FixedPipeline.TextureImageUnitEnabled)[i].Texture2DEnabled==GL_TRUE||
         (*this->FixedPipeline.TextureImageUnitEnabled)[i].Texture3DEnabled==GL_TRUE||
         (*this->FixedPipeline.TextureImageUnitEnabled)[i].TextureCubeMapEnabled==GL_TRUE)
        {
        os << indent << " TextureImageUnitFixedFlag" << i<< ":" << endl;
        os << indent << "  Texture1DEnabled=" << 
          static_cast<bool>((*this->FixedPipeline.TextureImageUnitEnabled)[i].Texture1DEnabled==GL_TRUE)
           << endl;
        os << indent << "  Texture2DEnabled=" << 
          static_cast<bool>((*this->FixedPipeline.TextureImageUnitEnabled)[i].Texture2DEnabled==GL_TRUE)
           << endl;
        os << indent << "  Texture3DEnabled=" << 
          static_cast<bool>((*this->FixedPipeline.TextureImageUnitEnabled)[i].Texture3DEnabled==GL_TRUE)
           << endl;
        os << indent << "  TextureCubeMapEnabled=" << 
          static_cast<bool>((*this->FixedPipeline.TextureImageUnitEnabled)[i].TextureCubeMapEnabled==GL_TRUE)
           << endl;
        }
      ++i;
      }
    }
  
  os << indent << "Viewport=" << this->Viewport[0] << ", "
     << this->Viewport[1] << ", " << this->Viewport[2] << ", "
     << this->Viewport[3] << endl;
  
  os << indent << "DepthRange=" << this->DepthRange[0] << ", "
     << this->DepthRange[1] << endl;
  
  os << indent << "MatrixMode=" << this->MatrixModeToString()
     << endl;
  
  os << indent << "ShadeModel=" << this->ShadeModelToString() << endl;
  
  os << indent << "CullFaceEnabled=" << static_cast<bool>(this->CullFaceEnabled==GL_TRUE) << endl;
  
  os << indent << "CullFaceMode=" << this->CullFaceModeToString() << endl;
  
  os << indent << "FrontFace=" << this->FrontFaceToString() << endl;
  
  os << indent << "PolygonSmoothEnabled=" << static_cast<bool>(this->PolygonSmoothEnabled==GL_TRUE) << endl;
  os << indent << "PolygonMode Front=" << this->PolygonModeToString(this->PolygonMode[0]);
  os  << " Back=" << this->PolygonModeToString(this->PolygonMode[1]) << endl;
  
  os << indent << "PolygonOffsetFactor=" << this->PolygonOffsetFactor << endl;
  os << indent << "PolygonOffsetUnits=" << this->PolygonOffsetUnits << endl;
  
  os << indent << "PolygonOffsetPointEnabled=" << static_cast<bool>(this->PolygonOffsetPointEnabled==GL_TRUE) << endl;
  os << indent << "PolygonOffsetLineEnabled=" << static_cast<bool>(this->PolygonOffsetLineEnabled==GL_TRUE) << endl;
  os << indent << "PolygonOffsetFillEnabled=" << static_cast<bool>(this->PolygonOffsetFillEnabled==GL_TRUE) << endl;
  os << indent << "PolygonStippleEnabled=" << static_cast<bool>(this->PolygonStippleEnabled==GL_TRUE) << endl;
  
  // multisampling
  os << indent << "-- Multisampling" << endl;
  os << indent << "MultiSampleEnabled=" << static_cast<bool>(this->MultiSampleEnabled==GL_TRUE) << endl;
  os << indent << "SampleAlphaToCoverageEnabled=" << static_cast<bool>(this->SampleAlphaToCoverageEnabled==GL_TRUE) << endl;
  os << indent << "SampleAlphaToOneEnabled=" << static_cast<bool>(this->SampleAlphaToOneEnabled==GL_TRUE) << endl;
  os << indent << "SampleCoverageEnabled=" << static_cast<bool>(this->SampleCoverageEnabled==GL_TRUE) << endl;
  os << indent << "SampleCoverageValue=" << this->SampleCoverageValue << endl;
  os << indent << "SampleCoverageInvert=" << this->BooleanToString(this->SampleCoverageInvert) << endl;
  
  // texture env
  os << indent << "-- Texture environment" << endl;
  os << indent << "ActiveTexture=GL_TEXTURE" << (this->ActiveTexture-vtkgl::TEXTURE0) << endl;
  
  // pixel operations
  os << indent << "-- Pixel operations" << endl;
  os << endl;
  os << indent << "ScissorTestEnabled=" << static_cast<bool>(this->ScissorTestEnabled==GL_TRUE) << endl;
  os << indent << "ScissorBox=" << this->ScissorBox[0] << ", " << this->ScissorBox[1] << ", " << this->ScissorBox[2] << ", " << this->ScissorBox[3] << endl;
  os << endl;
  os << indent << "AlphaTestEnabled=" << static_cast<bool>(this->AlphaTestEnabled==GL_TRUE) << endl;
  os << indent << "AlphaTestFunc=" << this->AlphaTestFuncToString() << endl;
  os << indent << "AlphaTestRef=" << this->AlphaTestRef << endl;
  os << endl;
  os << indent << "StencilTestEnabled=" << static_cast<bool>(this->StencilTestEnabled==GL_TRUE) << endl;
  os << endl;
  os << indent << "DepthTestEnabled=" << static_cast<bool>(this->DepthTestEnabled==GL_TRUE) << endl;
  os << indent << "DepthFunc=" << this->DepthFuncToString() << endl;
  os << endl;
  os << indent << "BlendEnabled=" << static_cast<bool>(this->BlendEnabled==GL_TRUE) << endl;
  os << indent << "BlendSrcRGB=" << this->BlendFuncToString(this->BlendSrcRGB) << endl;
  os << indent << "BlendSrcAlpha=" << this->BlendFuncToString(this->BlendSrcAlpha) << endl;
  os << indent << "BlendDstRGB=" << this->BlendFuncToString(this->BlendDstRGB) << endl;
  os << indent << "BlendDstAlpha=" << this->BlendFuncToString(this->BlendDstAlpha) << endl;
  os << indent << "BlendEquationRGB=" << this->BlendEquationToString(this->BlendEquationRGB) << endl;
  os << indent << "BlendEquationAlpha=" << this->BlendEquationToString(this->BlendEquationAlpha) << endl;
  os << endl;
  os << indent << "DitherEnabled=" << static_cast<bool>(this->DitherEnabled==GL_TRUE) << endl;
  os << endl;
  os << indent << "IndexLogicOpEnabled=" << static_cast<bool>(this->IndexLogicOpEnabled==GL_TRUE) << endl;
  os << indent << "ColorLogicOpEnabled=" << static_cast<bool>(this->ColorLogicOpEnabled==GL_TRUE) << endl;
  
  os << indent << "LogicOpMode=" << this->LogicOpModeToString() << endl;
  os << endl;
  
  os << indent << "-- Framebuffer control" << endl;

  os << indent << "MaxDrawBuffers=" << this->MaxDrawBuffers << endl;
  i=0;
  while(i<static_cast<size_t>(this->MaxDrawBuffers))
    {
    os << indent << "DrawBuffer[" << i <<"]=";
    this->ColorBufferToStream(os,(*this->DrawBuffers)[static_cast<size_t>(i)]);
    os << endl;
    ++i;
    }

  os << indent << "IndexWriteMask=" << this->IndexWriteMask << endl;
  os << indent << "ColorWriteMask=" << static_cast<bool>(this->ColorWriteMask[0]==GL_TRUE) << ", " << static_cast<bool>(this->ColorWriteMask[1]==GL_TRUE)<< ", " << static_cast<bool>(this->ColorWriteMask[2]==GL_TRUE) << ", " << static_cast<bool>(this->ColorWriteMask[3]==GL_TRUE) << endl;
  os << indent << "DepthWriteMask=" << static_cast<bool>(this->DepthWriteMask==GL_TRUE) << endl;
  os << indent << "StencilWriteMask=0x" << hex << this->StencilWriteMask << dec << endl;
  os << indent << "StencilBackWriteMask=0x" << hex << this->StencilBackWriteMask << dec << endl;
  os << indent << "this->ColorClearValue=" <<  this->ColorClearValue[0] << ", " << this->ColorClearValue[1] << ", " << this->ColorClearValue[2] << ", " << this->ColorClearValue[3] << endl;
  os << indent << "this->IndexClearValue=" <<  this->IndexClearValue << endl;
  os << indent << "this->DepthClearValue=" <<  this->DepthClearValue << endl;
  os << indent << "this->StencilClearValue=0x" << hex <<  this->StencilClearValue << dec << endl;
  os << indent << "this->AccumClearValue=" <<  this->AccumClearValue[0] << ", " << this->AccumClearValue[1] << ", " << this->AccumClearValue[2] << ", " << this->AccumClearValue[3] << endl;
  
  os << indent << "-- Pixels" << endl;
  
  os << indent << "Unpack:"<<endl;
  this->Unpack.PrintSelf(os,indent.GetNextIndent());
  os << indent << "Pack:"<<endl;
  this->Pack.PrintSelf(os,indent.GetNextIndent());
  
  os << indent << "PixelPackBufferBinding=" << this->PixelPackBufferBinding << endl;

  if(this->PixelPackBufferBinding>0)
    {
    this->PixelPackBufferObject.PrintSelf(os,indent);
    }

  os << indent << "PixelUnpackBufferBinding=" << this->PixelUnpackBufferBinding << endl;
 if(this->PixelUnpackBufferBinding>0)
    {
    this->PixelUnpackBufferObject.PrintSelf(os,indent);
    }


  os << indent << "RedTransform:";
  this->RedTransform.PrintSelf(os,indent.GetNextIndent());
  os << indent << "GreenTransform:";
  this->GreenTransform.PrintSelf(os,indent.GetNextIndent());
  os << indent << "BlueTransform:";
  this->BlueTransform.PrintSelf(os,indent.GetNextIndent());
  os << indent << "AlphaTransform:";
  this->AlphaTransform.PrintSelf(os,indent.GetNextIndent());
  os << indent << "DepthTransform:";
  this->DepthTransform.PrintSelf(os,indent.GetNextIndent());
  
  os << indent << "ZoomX=" << this->ZoomX << endl;
  os << indent << "ZoomY=" << this->ZoomY << endl;
  
os << indent << "ReadBuffer=";
this->ColorBufferToStream(os,this->ReadBuffer);
os << endl;

os << indent << "AuxBuffers=" << this->AuxBuffers << endl;
os << indent << "RGBAMode="<< static_cast<bool>(this->RGBAMode==GL_TRUE)<< endl;
os << indent << "IndexMode="<< static_cast<bool>(this->IndexMode==GL_TRUE)<< endl;
os << indent << "DoubleBuffer="<< static_cast<bool>(this->DoubleBuffer==GL_TRUE)<< endl;
os << indent << "Stereo="<< static_cast<bool>(this->Stereo==GL_TRUE)<< endl;

os << indent << "MaxColorAttachments=" << this->MaxColorAttachments << endl;
os << indent << "MaxDrawBuffers=" <<  this->MaxDrawBuffers << endl;

  os << indent << "ListBase=" << this->ListBase << endl;
  os << indent << "ListIndex=" << this->ListIndex << endl;
  os << indent << "ListMode=" << this->ListModeToString() << endl;
  
  os << indent << "RenderMode=" << this->RenderModeToString() << endl;
}

void vtkOpenGLProgramState::PrintSelf(ostream &os,
                                      vtkIndent indent)
{
  if(this->HasVertexShader)
    {
    os << indent << "customized verter shader" << endl;
    }
  else
    {
    os << indent << "fixed-pipeline verter shader" << endl;
    }
  if(this->HasFragmentShader)
    {
    os << indent << "customized fragment shader" << endl;
    }
  else
    {
    os << indent << "fixed-pipeline fragment shader" << endl;
    }
  
  os << indent << "DeleteStatus=" << static_cast<bool>(this->DeleteStatus==GL_TRUE)
     << endl;
  os << indent << "LinkStatus=" << static_cast<bool>(this->LinkStatus==GL_TRUE)
     << endl;
  os << indent << "ValidateStatus=" << static_cast<bool>(this->ValidateStatus==GL_TRUE)
     << endl;
  os << indent << "NumberOfAttachedShaders=" << this->NumberOfAttachedShaders
     << endl;
  
  os << indent << "InfoLogLength=" << this->InfoLogLength << endl;
  os << indent << "InfoLog=|" << endl << this->InfoLog << "|" << endl;
  os << indent << "ActiveUniforms=" << this->ActiveUniforms << endl;
  os << indent << "ActiveUniformMaxLength=" << this->ActiveUniformMaxLength
     << endl;
  os << indent << "ActiveAttributes=" << this->ActiveAttributes << endl;
  os << indent << "ActiveAttributeMaxLength=" << this->ActiveAttributeMaxLength
     << endl;
  os << indent << "Shaders: " << endl;
  
  size_t i=0;
  while(i<static_cast<size_t>(this->NumberOfAttachedShaders))
    {
    (*this->AttachedShaders)[i].PrintSelf(os,indent.GetNextIndent());
    ++i;
    }
}

void vtkOpenGLShaderState::PrintSelf(ostream &os,
                                     vtkIndent indent)
{
  os << indent << "Id=" << this->Id << endl;
  os << indent << "Type=" << this->ShaderTypeToString() << endl;
  os << indent << "DeleteStatus=" << static_cast<bool>(this->DeleteStatus==GL_TRUE)
     << endl;
  os << indent << "CompileStatus=" << static_cast<bool>(this->CompileStatus==GL_TRUE)
     << endl;
  os << indent << "InfoLogLength=" << this->InfoLogLength << endl;
  os << indent << "InfoLog=|" << endl << this->InfoLog << "|" << endl;
  os << indent << "SourceLength=" << this->SourceLength << endl;
  os << indent << "Source=|" << endl << this->Source << "|" << endl;
  
}

int ShaderTypeValueTable[2]={
  vtkgl::VERTEX_SHADER,
  vtkgl::FRAGMENT_SHADER
};

const char *ShaderTypeStringTable[2]={
  "GL_VERTEX_SHADER",
  "GL_FRAGMENT_SHADER"
};

const char *vtkOpenGLShaderState::ShaderTypeToString()
{
  return ::ValueToString(static_cast<GLint>(this->Type),ShaderTypeValueTable,
                         ShaderTypeStringTable,2);
}


void vtkOpenGLPixelControl::PrintSelf(ostream &os,
                                      vtkIndent indent)
{
  os << indent << "SwapBytes=" << static_cast<bool>(this->SwapBytes==GL_TRUE) << endl;
  os << indent << "LSBFirst=" << static_cast<bool>(this->LsbFirst==GL_TRUE) << endl;
  os << indent << "ImageHeight=" << this->ImageHeight << endl;
  os << indent << "SkipImages=" << this->SkipImages << endl;
  os << indent << "RowLength=" << this->RowLength << endl;
  os << indent << "SkipRows=" << this->SkipRows << endl;
  os << indent << "SkipPixels=" << this->SkipPixels << endl;
  os << indent << "Alignment=" << this->Alignment << endl;
}

void vtkOpenGLBufferObjectState::BufferAccessFlagsToStream(ostream &os)
{
  bool firstFlag=true;
  if((this->AccessFlags&vtkgl::MAP_READ_BIT)!=0)
    {
    os << "GL_MAP_READ_BIT";
    firstFlag=false;
    }
  if((this->AccessFlags&vtkgl::MAP_WRITE_BIT)!=0)
    {
    if(!firstFlag)
      {
      os << "|";
      }
    os << "GL_MAP_WRITE_BIT";
    firstFlag=false;
    }
  if((this->AccessFlags&vtkgl::MAP_INVALIDATE_RANGE_BIT)!=0)
    {
    if(!firstFlag)
      {
      os << "|";
      }
    os << "GL_MAP_INVALIDATE_RANGE_BIT";
    firstFlag=false;
    }
  if((this->AccessFlags&vtkgl::MAP_INVALIDATE_BUFFER_BIT)!=0)
    {
    if(!firstFlag)
      {
      os << "|";
      }
    os << "GL_MAP_INVALIDATE_BUFFER_BIT";
    firstFlag=false;
    }
   if((this->AccessFlags&vtkgl::MAP_FLUSH_EXPLICIT_BIT)!=0)
    {
    if(!firstFlag)
      {
      os << "|";
      }
    os << "GL_MAP_FLUSH_EXPLICIT_BIT";
    firstFlag=false;
    }
   
   if((this->AccessFlags&vtkgl::MAP_UNSYNCHRONIZED_BIT)!=0)
    {
    if(!firstFlag)
      {
      os << "|";
      }
    os << "GL_MAP_UNSYNCHRONIZED_BIT";
    firstFlag=false;
    }
}

int BufferUsageValueTable[9]={
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

const char *BufferUsageStringTable[9]={
  "GL_STREAM_DRAW",
  "GL_STREAM_READ",
  "GL_STREAM_COPY",
  "GL_STATIC_DRAW",
  "GL_STATIC_READ",
  "GL_STATIC_COPY",
  "GL_DYNAMIC_DRAW",
  "GL_DYNAMIC_READ",
  "GL_DYNAMIC_COPY"
};

const char *vtkOpenGLBufferObjectState::BufferUsageToString()
{
  return ::ValueToString(static_cast<GLint>(this->Usage),BufferUsageValueTable,
                         BufferUsageStringTable,9);
}

int BufferAccessValueTable[3]={
  vtkgl::READ_ONLY,
  vtkgl::WRITE_ONLY,
  vtkgl::READ_WRITE
};

const char *BufferAccessStringTable[3]={
  "GL_READ_ONLY",
  "GL_WRITE_ONLY",
  "GL_READ_WRITE"
};

const char *vtkOpenGLBufferObjectState::BufferAccessToString()
{
  return ::ValueToString(static_cast<GLint>(this->Access),
                         BufferAccessValueTable,
                         BufferAccessStringTable,3);
}


void vtkOpenGLBufferObjectState::PrintSelf(ostream &os,
                                           vtkIndent indent)
{
  os << indent << " Size=" << this->Size << endl;
  os << indent << " Usage=" << this->BufferUsageToString() << endl;
  os << indent << " Access=" << this->BufferAccessToString() << endl;
//  os << indent << " AccessFlags=";
//   this->BufferAccessFlagsToStream(os);
  os << endl;
  os << indent << " Mapped=" << static_cast<bool>(this->Mapped==GL_TRUE) << endl;
  os << indent << " MapPointer=" << this->MapPointer << endl;
//  os << indent << " MapOffset=" << this->MapOffset << endl;
//  os << indent << " MapLength=" << this->MapLength << endl;
}
void vtkOpenGLComponentTransform::PrintSelf(ostream &os,
                                             vtkIndent indent)
{
  os << indent << "Scale=" << this->Scale << endl;
  os << indent << "Bias=" << this->Bias << endl;
}
  
// Unknown is a reserved value on BCC:
// Bcc55\include\winioctl.h 682:
const char *UnknownValue="Unknown value";
const char *NA="N/A";

int BooleanValueTable[2]=
{
  GL_FALSE,
  GL_TRUE
};

const char *BooleanStringTable[2]=
{
  "GL_FALSE",
  "GL_TRUE"
};

int MatrixModeValueTable[4]=
{
  GL_MODELVIEW,
  GL_PROJECTION,
  GL_TEXTURE,
  GL_COLOR
};
  
const char *MatrixModeStringTable[4]=
{
  "GL_MODELVIEW",
  "GL_PROJECTION",
  "GL_TEXTURE",
  "GL_COLOR"
};
 
int ShadeModelValueTable[2]=
{
  GL_SMOOTH,
  GL_FLAT
};

const char *ShadeModelStringTable[2]=
{
  "GL_SMOOTH",
  "GL_FLAT"
};
 
int CullFaceModeValueTable[3]=
{
  GL_FRONT,
  GL_BACK,
  GL_FRONT_AND_BACK
};

const char *CullFaceModeStringTable[3]=
{
  "GL_FRONT",
  "GL_BACK",
  "GL_FRONT_AND_BACK"
};

int FrontFaceValueTable[2]=
{
  GL_CW,
  GL_CCW
};

const char *FrontFaceStringTable[2]=
{
  "GL_CW",
  "GL_CCW",
};

int PolygonModeValueTable[3]=
{
  GL_POINT,
  GL_LINE,
  GL_FILL
};

const char *PolygonModeStringTable[3]=
{
  "GL_POINT",
  "GL_LINE",
  "GL_FILL"
};

int AlphaTestFuncValueTable[8]=
{
  GL_NEVER,
  GL_ALWAYS,
  GL_LESS,
  GL_LEQUAL,
  GL_EQUAL,
  GL_GEQUAL,
  GL_GREATER,
  GL_NOTEQUAL
};

const char *AlphaTestFuncStringTable[8]=
{
  "GL_NEVER",
  "GL_ALWAYS",
  "GL_LESS",
  "GL_LEQUAL",
  "GL_EQUAL",
  "GL_GEQUAL",
  "GL_GREATER",
  "GL_NOTEQUAL"
};

int RenderModeValueTable[3]=
{
  GL_RENDER,
  GL_SELECT,
  GL_FEEDBACK
};

const char *RenderModeStringTable[3]=
{
  "GL_RENDER",
  "GL_SELECT",
  "GL_FEEDBACK"
};

int ListModeValueTable[2]=
{
  GL_COMPILE,
  GL_COMPILE_AND_EXECUTE
};

const char *ListModeStringTable[2]=
{
  "GL_COMPILE",
  "GL_COMPILE_AND_EXECUTE"
};

int BlendFuncValueTable[15]=
{
  GL_ZERO,
  GL_ONE,
  GL_SRC_COLOR,
  GL_ONE_MINUS_SRC_COLOR,
  GL_DST_COLOR,
  GL_ONE_MINUS_DST_COLOR,
  GL_SRC_ALPHA,
  GL_ONE_MINUS_SRC_ALPHA,
  GL_DST_ALPHA,
  GL_ONE_MINUS_DST_ALPHA,
  vtkgl::CONSTANT_COLOR,
  vtkgl::ONE_MINUS_CONSTANT_COLOR,
  vtkgl::CONSTANT_ALPHA,
  vtkgl::ONE_MINUS_CONSTANT_ALPHA,
  GL_SRC_ALPHA_SATURATE
};

const char *BlendFuncStringTable[15]=
{
  "GL_ZERO",
  "GL_ONE",
  "GL_SRC_COLOR",
  "GL_ONE_MINUS_SRC_COLOR",
  "GL_DST_COLOR",
  "GL_ONE_MINUS_DST_COLOR",
  "GL_SRC_ALPHA",
  "GL_ONE_MINUS_SRC_ALPHA",
  "GL_DST_ALPHA",
  "GL_ONE_MINUS_DST_ALPHA",
  "GL_CONSTANT_COLOR",
  "GL_ONE_MINUS_CONSTANT_COLOR",
  "GL_CONSTANT_ALPHA",
  "GL_ONE_MINUS_CONSTANT_ALPHA",
  "GL_SRC_ALPHA_SATURATE"
};

int BlendEquationValueTable[5]=
{
  vtkgl::FUNC_ADD,
  vtkgl::FUNC_SUBTRACT,
  vtkgl::FUNC_REVERSE_SUBTRACT,
  vtkgl::MIN,
  vtkgl::MAX
};

const char *BlendEquationStringTable[5]=
{
  "GL_FUNC_ADD",
  "GL_FUNC_SUBTRACT",
  "GL_FUNC_REVERSE_SUBTRACT",
  "GL_MIN",
  "GL_MAX"
};

int LogicOpModeValueTable[16]={
  GL_CLEAR,
  GL_AND,
  GL_AND_REVERSE,
  GL_COPY,
  GL_AND_INVERTED,
  GL_NOOP,
  GL_XOR,
  GL_OR,
  GL_NOR,
  GL_EQUIV,
  GL_INVERT,
  GL_OR_REVERSE,
  GL_COPY_INVERTED,
  GL_OR_INVERTED,
  GL_NAND,
  GL_SET
};

const char *LogicOpModeStringTable[16]={
  "GL_CLEAR",
  "GL_AND",
  "GL_AND_REVERSE",
  "GL_COPY",
  "GL_AND_INVERTED",
  "GL_NOOP",
  "GL_XOR",
  "GL_OR",
  "GL_NOR",
  "GL_EQUIV",
  "GL_INVERT",
  "GL_OR_REVERSE",
  "GL_COPY_INVERTED",
  "GL_OR_INTERTED",
  "GL_NAND",
  "GL_SET"
};

int ColorBufferValueTable[10]={
  GL_NONE,
  GL_FRONT_LEFT,
  GL_FRONT_RIGHT,
  GL_BACK_LEFT,
  GL_BACK_RIGHT,
  GL_FRONT,
  GL_BACK,
  GL_LEFT,
  GL_RIGHT,
  GL_FRONT_AND_BACK
};

const char *ColorBufferStringTable[10]={
  "GL_NONE",
  "GL_FRONT_LEFT",
  "GL_FRONT_RIGHT",
  "GL_BACK_LEFT",
  "GL_BACK_RIGHT",
  "GL_FRONT",
  "GL_BACK",
  "GL_LEFT",
  "GL_RIGHT",
  "GL_FRONT_AND_BACK"
};

int ErrorCodeValueTable[9]={
  GL_NO_ERROR,
  GL_INVALID_ENUM,
  GL_INVALID_VALUE,
  GL_INVALID_OPERATION,
  GL_STACK_OVERFLOW,
  GL_STACK_UNDERFLOW,
  GL_OUT_OF_MEMORY,
  vtkgl::TABLE_TOO_LARGE,
  vtkgl::INVALID_FRAMEBUFFER_OPERATION_EXT
};

const char *ErrorCodeStringTable[9]={
  "GL_NO_ERROR",
  "GL_INVALID_ENUM",
  "GL_INVALID_VALUE",
  "GL_INVALID_OPERATION",
  "GL_STACK_OVERFLOW",
  "GL_STACK_UNDERFLOW",
  "GL_OUT_OF_MEMORY",
  "GL_TABLE_TOO_LARGE",
  "GL_INVALID_FRAMEBUFFER_OPERATION_EXT"
};

void vtkOpenGLState::ColorBufferToStream(ostream &os,
                                         GLint colorBuffer)
{
  GLint auxBuffer=colorBuffer-GL_AUX0;
  if(auxBuffer>=0 && auxBuffer<this->AuxBuffers)
    {
    os << "GL_AUX" << auxBuffer;
    }
  else
    {
    GLint colorAttachment=static_cast<int>(static_cast<unsigned int>(colorBuffer)
                                           -vtkgl::COLOR_ATTACHMENT0);
    if(colorAttachment>=0 && colorAttachment<this->MaxColorAttachments)
      {
      os << "GL_COLOR_ATTACHMENT" << colorAttachment;
      }
    else
      {
      os << this->ValueToString(colorBuffer,ColorBufferValueTable,
                                ColorBufferStringTable,10);
      }
    }
}

const char *vtkOpenGLState::ErrorCodeToString()
{
  return this->ValueToString(static_cast<GLint>(this->ErrorCode),
                             ErrorCodeValueTable,ErrorCodeStringTable,9);
}

const char *vtkOpenGLState::BlendFuncToString(GLint blendFunc)
{
  return this->ValueToString(blendFunc,BlendFuncValueTable,
                             BlendFuncStringTable,15);
}

const char *vtkOpenGLState::BlendEquationToString(GLint blendEquation)
{
  return this->ValueToString(blendEquation,BlendEquationValueTable,
                             BlendEquationStringTable,5);
}

const char *vtkOpenGLState::LogicOpModeToString()
{
  return this->ValueToString(this->LogicOpMode,LogicOpModeValueTable,
                             LogicOpModeStringTable,16);
}
  
const char *vtkOpenGLState::ListModeToString()
{
  const char*result;
  if(this->ListMode==0)
    {
    result=NA;
    }
  else
    {
    result=this->ValueToString(this->ListMode,ListModeValueTable,
                               ListModeStringTable,2);
    }
  return result;
}

const char *vtkOpenGLState::BooleanToString(GLint booleanValue)
{
  return this->ValueToString(booleanValue,BooleanValueTable,
                             BooleanStringTable,2);
}
const char *vtkOpenGLState::ShadeModelToString()
{
  return this->ValueToString(this->ShadeModel,ShadeModelValueTable,
                             ShadeModelStringTable,2);
}

const char *vtkOpenGLState::CullFaceModeToString()
{
  return this->ValueToString(this->CullFaceMode,CullFaceModeValueTable,
                             CullFaceModeStringTable,3);
}

const char *vtkOpenGLState::FrontFaceToString()
{
  return this->ValueToString(this->FrontFace,FrontFaceValueTable,
                             FrontFaceStringTable,2);
}

const char *vtkOpenGLState::PolygonModeToString(GLint polygonMode)
{
  return this->ValueToString(polygonMode,PolygonModeValueTable,
                             PolygonModeStringTable,3);
}

const char *vtkOpenGLState::AlphaTestFuncToString()
{
  return this->ValueToString(this->AlphaTestFunc,AlphaTestFuncValueTable,
                             AlphaTestFuncStringTable,8);
}

const char *vtkOpenGLState::DepthFuncToString()
{
  // yes, same functions values for alpha test and depth test.
  return this->ValueToString(this->DepthFunc,AlphaTestFuncValueTable,
                             AlphaTestFuncStringTable,8);
}
 
const char *vtkOpenGLState::RenderModeToString()
{
  return this->ValueToString(this->RenderMode,RenderModeValueTable,
                             RenderModeStringTable,3);
}

const char *vtkOpenGLState::MatrixModeToString()
{
  return this->ValueToString(this->MatrixMode,MatrixModeValueTable,
                             MatrixModeStringTable,4);
}

const char *ValueToString(GLint value,
                          int valueTable[],
                          const char *stringTable[],
                          int tableSize)
{
  const char *result;
  bool found=false;
  int i=0;
  while(!found && i<tableSize)
    {
    found=valueTable[i]==value;
    ++i;
    }
  if(!found)
    {
    result=UnknownValue;
    }
  else
    {
    result=stringTable[i-1];
    }
  return result;
}

const char *vtkOpenGLState::ValueToString(GLint value,
                                          int valueTable[],
                                          const char *stringTable[],
                                          int tableSize)
{
  return ::ValueToString(value,valueTable,stringTable,tableSize);
}

void vtkOpenGLState::PrintMatrix(ostream &os,
                                 vtkIndent indent,
                                 GLfloat matrix[16])
{
  // Spec 2.1, page 43: row-major:
  // a1 a5 a9 a13
  // a2 ...
  // a3 ..
  // a4 ...
  
  // starting at 0, not 1:
  // a0 a4 a8 a12
  // a1 ...
  // a2 ..
  // a3 ...
  int row=0;
  while(row<4)
    {
    os << indent;
    int column=0;
    while(column<4)
      {
      os << matrix[column*4+row] << ", ";
      ++column;
      }
    os << endl;
    ++row;
    }
}
