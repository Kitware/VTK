/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLStateCache.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGLStateCache - checks for redundancies in state-change requests 
// .SECTION Description
// This simply checks for redundancies in state-change requests and
// only calls the real OpenGL call if there has in fact been a change.
// This cannot, however, fix problems with the ordering of calls.

#ifndef VTK_IMPLEMENT_MESA_CXX
# include "vtkOpenGL.h" // Needed for GL api types.
#endif

#define vtkOpenGLCall_glEnable vtkOpenGLStateCache::CurrentGLCache->glEnable
#define vtkOpenGLCall_glDisable vtkOpenGLStateCache::CurrentGLCache->glDisable
#define vtkOpenGLCall_glAlphaFunc vtkOpenGLStateCache::CurrentGLCache->glAlphaFunc
#define vtkOpenGLCall_glBlendFunc vtkOpenGLStateCache::CurrentGLCache->glBlendFunc
#define vtkOpenGLCall_glDepthFunc vtkOpenGLStateCache::CurrentGLCache->glDepthFunc
#define vtkOpenGLCall_glTexEnvf vtkOpenGLStateCache::CurrentGLCache->glTexEnvf
#define vtkOpenGLCall_glLightModeli vtkOpenGLStateCache::CurrentGLCache->glLightModeli
#define vtkOpenGLCall_glLightModelfv vtkOpenGLStateCache::CurrentGLCache->glLightMOdelfv
#define vtkOpenGLCall_glLightfv vtkOpenGLStateCache::CurrentGLCache->glLightfv
#define vtkOpenGLCall_glLightf vtkOpenGLStateCache::CurrentGLCache->glLightf
#define vtkOpenGLCall_glLighti vtkOpenGLStateCache::CurrentGLCache->glLighti
#define vtkOpenGLCall_glMaterialfv vtkOpenGLStateCache::CurrentGLCache->glMaterialfv
#define vtkOpenGLCall_glShadeModel vtkOpenGLStateCache::CurrentGLCache->glShadeModel
#define vtkOpenGLCall_glClearColor vtkOpenGLStateCache::CurrentGLCache->glClearColor
#define vtkOpenGLCall_glClearDepth vtkOpenGLStateCache::CurrentGLCache->glClearDepth
#define vtkOpenGLCall_glDepthMask vtkOpenGLStateCache::CurrentGLCache->glDepthMask
#define vtkOpenGLCall_glCullFace vtkOpenGLStateCache::CurrentGLCache->glCullFace
#define vtkOpenGLCall_glClear vtkOpenGLStateCache::CurrentGLCache->glClear
#define vtkOpenGLCall_glDrawBuffer vtkOpenGLStateCache::CurrentGLCache->glDrawBuffer
#define vtkOpenGLCall_glMatrixMode vtkOpenGLStateCache::CurrentGLCache->glMatrixMode
#define vtkOpenGLCall_glViewport vtkOpenGLStateCache::CurrentGLCache->glViewport
#define vtkOpenGLCall_glScissor vtkOpenGLStateCache::CurrentGLCache->glScissor
#define vtkOpenGLCall_glClipPlane vtkOpenGLStateCache::CurrentGLCache->glClipPlane
#define vtkOpenGLCall_glColorMaterial vtkOpenGLStateCache::CurrentGLCache->glColorMaterial
#define vtkOpenGLCall_glPointSize vtkOpenGLStateCache::CurrentGLCache->glPointSize
#define vtkOpenGLCall_glLineWidth vtkOpenGLStateCache::CurrentGLCache->glLineWidth
#define vtkOpenGLCall_glLineStipple vtkOpenGLStateCache::CurrentGLCache->glLineStipple
#define vtkOpenGLCall_glDepthRange vtkOpenGLStateCache::CurrentGLCache->glDepthRange
#define vtkOpenGLCall_glPolygonOffset vtkOpenGLStateCache::CurrentGLCache->glPolygonOffset

#define vtkOpenGLCall_glPushMatrix glPushMatrix
#define vtkOpenGLCall_glPopMatrix glPopMatrix
#define vtkOpenGLCall_glMultMatrixd glMultMatrixd
#define vtkOpenGLCall_glLoadMatrixd glLoadMatrixd
#define vtkOpenGLCall_glLoadIdentity glLoadIdentity
#define vtkOpenGLCall_glSelectBuffer glSelectBuffer
#define vtkOpenGLCall_glRenderMode glRenderMode
#define vtkOpenGLCall_glInitNames glInitNames
#define vtkOpenGLCall_glPushName glPushName
#define vtkOpenGLCall_glLoadName glLoadName
#define vtkOpenGLCall_glGetIntegerv glGetIntegerv
#define vtkOpenGLCall_glIsTexture glIsTexture
#define vtkOpenGLCall_glDeleteTextures glDeleteTexture
#define vtkOpenGLCall_glGenTextures glGenTextures
#define vtkOpenGLCall_glBindTexture glBindTexture
#define vtkOpenGLCall_glTexParameterf glTextParameterf
#define vtkOpenGLCall_glTexCoord2fv glTexCoord2fv
#define vtkOpenGLCall_glVertex3fv glVertex3fv
#define vtkOpenGLCall_glNormal3fv glNormal3fv
#define vtkOpenGLCall_glColor3f glColor3f
#define vtkOpenGLCall_glColor4ubv glColor4ubv
#define vtkOpenGLCall_glColor4fv glColor4fv
#define vtkOpenGLCall_glBegin glBegin
#define vtkOpenGLCall_glEnd glEnd
#define vtkOpenGLCall_glTexImage2D glTextImage2D
#define vtkOpenGLCall_glDeleteLists glDeleteLists
#define vtkOpenGLCall_glIsList glIsList
#define vtkOpenGLCall_glGenLists glGenLists
#define vtkOpenGLCall_glCallList glCallList
#define vtkOpenGLCall_glReadBuffer glReadBuffer
#define vtkOpenGLCall_glPixelStorei glPixelStorei
#define vtkOpenGLCall_glReadPixels glReadPixels
#define vtkOpenGLCall_glRasterPos3f glRasterPos3f
#define vtkOpenGLCall_glDrawPixels glDrawPixels
#define vtkOpenGLCall_glRasterPos2f glRasterPos2f
#define vtkOpenGLCall_glNewList glNewList
#define vtkOpenGLCall_glEndList glEndList

class vtkOpenGLStateCache  
{
public:
  static vtkOpenGLStateCache *CurrentGLCache; // recursive definition

  vtkOpenGLStateCache(); // set all members to initial values
  ~vtkOpenGLStateCache(); // delete any dynamic objects
  void Initialize();

  // GL_BLEND         = 0x0BE2
  // GL_POINT_SMOOTH  = 0x0B10
  // GL_LINE_SMOOTH   = 0x0B20
  // GL_POLYGON_SMOOTH= 0x0B41
  // GL_DEPTH_TEST    = 0x0B71
  // GL_ALPHA_TEST    = 0x0BC0
  // GL_TEXTURE_2D    = 0x0DE1
  // GL_CLIP_PLANE0+i = 0x3000
  // GL_LIGHTING      = 0x0B50
  // GL_COLOR_MATERIAL= 0x0B57
  // GL_NORMALIZE     = 0x0BA1
  // GL_CULL_FACE     = 0x0B44
  // GL_SCISSOR_TEST  = 0x0C11
  // GL_POLYGON_OFFSET_FILL = 0x8037
  // GL_LINE_STIPPLE  = 0x0B24
  // GL_LIGHT+i       = 0x4000
  char Enable_buckets[0xDE1-0xB10+1]; // 0xB10-0xDE1
  char Enable_GL_LIGHT_buckets[8]; // 0x4000 + i (0<i<8)
  char Enable_GL_CLIP_PLANE_buckets[8]; // 0x8000 + i (0<i<8)
  /* Need to have special handling for disabling and enabling the 
     GL_LIGHT's because they are disabling too many lights!
     need to propagate in how many lights are actually *on*
     and only apply the op to them.
   */
  inline void glEnable(GLenum e) 
    {
      register int ex;
      register char *val=0;
      if(e&0x4000)
        {
        ex=e-0x4000;
        if(ex<8) {val=Enable_GL_LIGHT_buckets+ex; }
        }    
      else 
        {
        if(e&0x8000)
          {
          ex=e-0x8000;
          if(ex<8) { val=Enable_GL_CLIP_PLANE_buckets+ex; }
          }
        else 
          {
          if(e>=0xB10 && e<=0xDE1)
            {
            ex=e-0xB10;
            val=Enable_buckets+ex;
            }
          else
            {
            printf("Error: glEnable of 0x%X failed\n",e);
            }
          }
        }
      if(val && *val!=1)
        {
        *val=1;
        ::glEnable(e);
        }
    }
  inline void glDisable(GLenum e) 
    {
      register int ex;
      register char *val=0;
      if(e&0x4000)
        {
        ex=e-0x4000;
        if(ex<8) { val=Enable_GL_LIGHT_buckets+ex; }
        }    
      else
        {
        if(e&0x8000)
          {
          ex=e-0x8000;
          if(ex<8) { val=Enable_GL_CLIP_PLANE_buckets+ex; }
          }
        else 
          {
          if(e>=0xB10 && e<=0xDE1)
            {
            ex=e-0xB10;
            val=Enable_buckets+ex;
            }
          else
            {
            printf("Error: glEnable of 0x%X failed\n",e);
            }
          }
        }
      if(val && *val!=0)
        {
        *val=0;
        ::glDisable(e);
        }
    }
  
  // GL_GREATER = 0x0204, (GLclampf) 0
  GLclampf AlphaFunc_bucket;
  inline void glAlphaFunc(GLenum e,GLclampf cf) 
    {
      if(e==GL_GREATER && cf!=AlphaFunc_bucket)
        {
        AlphaFunc_bucket=cf;
        ::glAlphaFunc(e,cf);
        }
    }
  
  // GL_SRC_ALPHA = 0x0302, GL_ONE_MINUS_SRC_ALPHA = 0x0303
  GLenum BlendFunc_bucket; // multibucket if any other blendfunc is used
  inline void glBlendFunc(GLenum e,GLenum e1) 
    {
      if(e==GL_SRC_ALPHA && e1!=BlendFunc_bucket)
        {
        BlendFunc_bucket=e1;
        ::glBlendFunc(e,e1);
        }
    }
  
  // GL_GREATER = 0x0204
  // GL_LESS    = 0x0201
  // GL_LEQUAL  = 0x0203
  GLenum DepthFunc_bucket;
  inline void glDepthFunc(GLenum e) 
    {
      if(e!=DepthFunc_bucket)
        {
        DepthFunc_bucket=e;
        ::glDepthFunc(e);
        }
    }
  
  // GL_TEXTURE_ENV = 0x2300, GL_TEXTURE_ENV_MODE = 0x2200, GL_MODULATE = 0x2100
  GLfloat TexEnvf_MODE_bucket; // total kludge right now
  inline void glTexEnvf(GLenum e,GLenum e1,GLfloat f) 
    {
      if(e==GL_TEXTURE_ENV && e1==GL_TEXTURE_ENV_MODE)
        {
        if(f!=TexEnvf_MODE_bucket)
          {
          TexEnvf_MODE_bucket=f;
          ::glTexEnvf(e,e1,f);
          }
        }
    }
  
  // GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE/FALSE
  // GL_LIGHT_MODEL_TWO_SIDE, 0
  GLint LightModeli_LIGHT_MODEL_TWO_SIDE_bucket; // shoudld check other modes
  inline void glLightModeli(GLenum e,GLint i) 
    {
      if(e==GL_LIGHT_MODEL_TWO_SIDE && i!=LightModeli_LIGHT_MODEL_TWO_SIDE_bucket){
      LightModeli_LIGHT_MODEL_TWO_SIDE_bucket=i;
      ::glLightModeli(e,i);
      }
    }
  
  // GL_LIGHT_MODEL_AMBIENT, fvect(amb color), A=1.0
  // GL_LIGHT_MODEL_AMBIENT = 0x0B53
  GLfloat LightModelfv_LIGHT_MODEL_AMBIENT_bucket[3];
  inline void glLightModelfv(GLenum e,GLfloat *fv) 
    {
      if(e==GL_LIGHT_MODEL_AMBIENT && 
         (fv[0]!=LightModelfv_LIGHT_MODEL_AMBIENT_bucket[0] ||
          fv[1]!=LightModelfv_LIGHT_MODEL_AMBIENT_bucket[1] ||
          fv[2]!=LightModelfv_LIGHT_MODEL_AMBIENT_bucket[2])){
      fv[0]=LightModelfv_LIGHT_MODEL_AMBIENT_bucket[0];
      fv[1]=LightModelfv_LIGHT_MODEL_AMBIENT_bucket[1];
      fv[2]=LightModelfv_LIGHT_MODEL_AMBIENT_bucket[2]; 
      ::glLightModelfv(e,fv);
      }
    }
  
  // light=GL_LIGHT index
  // pname= lighting type
  //   GL_DIFFUSE        = 0x1201
  //   GL_SPECULAR       = 0x1202
  //   GL_POSITION       = 0x1203
  //   GL_SPOT_DIRECTION = 0x1204
  GLfloat Lightfv_buckets[8*4*8];
  inline void glLightfv( GLenum light, GLenum pname, const GLfloat *params) 
    {
      register GLfloat *val = Lightfv_buckets + ((((int)(pname-0x1201))|((int)(light-GL_LIGHT0)<<3))<<2);
      if(params[0]!=val[0] ||
         params[1]!=val[1] ||
         params[2]!=val[2] ||
         params[3]!=val[3])
        {
        val[0]=params[0];
        val[1]=params[1];
        val[2]=params[2];
        val[3]=params[3];
        ::glLightfv(light,pname,params);
        }
    } 
  
  // light=GL_LIGHT index
  // pname= lighting parameter
  //   GL_SPOT_EXPONENT        = 0x1205
  //   GL_SPOT_CUTOFF          = 0x1206
  //   GL_CONSTANT_ATTENUATION = 0x1207
  //   GL_LINEAR_ATTENUATION   = 0x1208
  //   GL_QUADRATIC_ATTENUATION= 0x1209
  GLfloat Lightf_buckets[8*8];
  GLint Lighti_SPOT_CUTOFF_buckets[8];
  inline void glLightf( GLenum light, GLenum pname, GLfloat f){
    register GLfloat *val=Lightf_buckets+(((int)(light-GL_LIGHT0)<<3)|((int)(pname-0x1205)));
    if(val[0]!=f)
      {
      val[0]=f;
      ::glLightf(light,pname,f);
      if(pname==GL_SPOT_CUTOFF) // invalidate integer spot cutoff
        Lighti_SPOT_CUTOFF_buckets[light-GL_LIGHT0]=-1;
      }
  }
  
  // light=GL_LIGHT index
  // pname=lighting parameter
  //   GL_SPOT_CUTOFF = 0x1206
  // needs to invalidate the float light cutoff
  inline void glLighti( GLenum light, GLenum pname, GLint f) 
    {
      if(pname==GL_SPOT_CUTOFF && f!=Lighti_SPOT_CUTOFF_buckets[light-GL_LIGHT0]){
      Lighti_SPOT_CUTOFF_buckets[light-GL_LIGHT0]=f;
      ::glLighti(light,pname,f);
      // need to invalidate the float cutoff
      Lightf_buckets[((int)(light-GL_LIGHT0)<<3)|0x02] = -1.0f;
      }
    }  
  
  // Face, GL_AMBIENT, float Info[4] 
  //   GL_FRONT          = 0x0404  
  //   GL_BACK           = 0x0405
  //   GL_FRONT_AND_BACK = 0x0408
  // GL_AMBIENT   = 0x1200
  // GL_DIFFUSE   = 0x1201
  // GL_SPECULAR  = 0x1202
  // GL_EMISSION  = 0x1600
  // GL_SHININESS = 0x1601
  // GL_AMBIENT_AND_DIFFUSE = 0x1602
  // GL_COLOR_INDEXES       = 0x1603
  GLfloat Materialfv_buckets[8*8*4]; 
  inline void glMaterialfv(GLenum face, GLenum pname, const GLfloat *params ) 
    {
      register int idx;
      register GLfloat *val;
      if(pname>=0x1600) 
        {
        idx=pname-0x1600 + 4; // put it just past the 120x buckets
        }
      else 
        {
        idx=pname-0x1200;
        }
      // FRONT/BACK and FRONT_AND_BACK should do both.
      // or perhaps should be a separate state key?
      // For now, we will treat FRONT_AND_BACK independently
      // because from a practical standpoint, that's how 
      // it tends to get used.
      val = Materialfv_buckets + ((((face-0x0404)<<3)|idx)<<2);
      if(val[0]!=params[0] ||
         val[1]!=params[1] || 
         val[2]!=params[2] ||
         val[3]!=params[3])
        {
        val[0]=params[0];
        val[1]=params[1];
        val[2]=params[2];
        val[3]=params[3];
        ::glMaterialfv(face,pname,params);
        }
    }

  /*
    a=0;
    a|=(val[0]^params[0])
    a|=(val[1]^params[1])
    a|=(val[2]^params[2])
    a|=(val[3]^params[3])
   */
  // GL_FLAT   = 0x1D00
  // GL_SMOOTH = 0x1D01
  GLenum ShadeModel_bucket; 
  inline void glShadeModel(GLenum e)
    {
      if(ShadeModel_bucket!=e)
        {
        ShadeModel_bucket=e;
        ::glShadeModel(e);
        }
    }
  
  GLclampf ClearColor_buckets[4];
  inline void glClearColor(GLclampf r,GLclampf g,GLclampf b,GLclampf a)
    {
      register GLclampf *c=ClearColor_buckets;
      if(c[0]!=r ||
         c[1]!=g ||
         c[2]!=b ||
         c[3]!=a)
        {
        c[0]=r;
        c[1]=g;
        c[2]=b;
        c[3]=a;
        ::glClearColor(r,g,b,a);
        }
    }
  
  GLclampd ClearDepth_bucket;
  inline void glClearDepth(GLclampd d) 
    { 
      if(d!=ClearDepth_bucket)
        {
        ClearDepth_bucket=d;
        ::glClearDepth(d);
        }
    }
  
  GLclampf DepthMask_bucket;
  inline void glDepthMask(GLenum e)
    {
      if(DepthMask_bucket!=e)
        {
        DepthMask_bucket=e;
        ::glDepthMask(e);
        }
    }
  
  // GL_FRONT = 0x0404
  // GL_BACK  = 0x0405
  GLenum CullFace_bucket;
  inline void glCullFace(GLenum e)
    {
      if(CullFace_bucket!=e)
        {
        CullFace_bucket=e;
        ::glCullFace(e);
        }
    }
  
  // well, lets go ahead and let it clear when it wants to
  inline void glClear(GLbitfield b) { ::glClear(b);}
  // GL_BACK_LEFT  = 0x0402
  // GL_BACK_RIGHT = 0x0403
  // GL_FRONT      = 0x0404
  // GL_BACK       = 0x0405
  GLenum DrawBuffer_bucket;
  inline void glDrawBuffer(GLenum e) {
    if(e!=DrawBuffer_bucket){
      DrawBuffer_bucket=e;
      ::glDrawBuffer(e);
    }
  }
  //============Matrix Ops (behave different for deferred ops)===
  // GL_MODELVIEW=0x1700
  // GL_PROJECTION=0x1701
  GLenum  MatrixMode_bucket;
  inline void glMatrixMode(GLenum e) {
    if(e!=MatrixMode_bucket){
      MatrixMode_bucket=e;
      ::glMatrixMode(e);
    }
  }

  GLint Viewport_bucket[4];
  inline void glViewport(GLint llx,GLint lly,GLint u,GLint v){
    register GLint *val=Viewport_bucket;
    if(val[0]!=llx ||
       val[1]!=lly ||
       val[2]!=u ||
       val[3]!=v){
      val[0]=llx;
      val[1]=lly;
      val[2]=u;
      val[3]=v;
      ::glViewport(llx,lly,u,v);
    }
  }
  // only needs to be called if scissor changes (and it usually won't)
  GLint Scissor_bucket[4];
  inline void glScissor(GLint llx,GLint lly,GLint u,GLint v){
    register GLint *val=Scissor_bucket;
    if(val[0]!=llx ||
       val[1]!=lly ||
       val[2]!=u ||
       val[3]!=v){
      val[0]=llx;
      val[1]=lly;
      val[2]=u;
      val[3]=v;
      ::glScissor(llx,lly,u,v);
    }
  }
  
  // what is the order of the clip plane eqn???
  // GL_CLIP_PLANE0 = 0x3000
  GLdouble ClipPlane_bucket[4*GL_MAX_CLIP_PLANES];
  inline void glClipPlane(GLenum e,const GLdouble *eqn){
    register GLdouble *val=ClipPlane_bucket + ((e-0x3000)<<2);
    if(val[0]!=eqn[0] ||
       val[1]!=eqn[1] ||
       val[2]!=eqn[2] ||
       val[3]!=eqn[3]){
      val[0]=eqn[0];
      val[1]=eqn[1];
      val[2]=eqn[2];
      val[3]=eqn[3];
      ::glClipPlane(e,eqn);
    }
  }

  // face= 
  //   GL_FRONT          = 0x0404  
  //   GL_BACK           = 0x0405
  //   GL_FRONT_AND_BACK = 0x0408
  GLenum ColorMaterial_bucket[8];
  inline void glColorMaterial(GLenum face,GLenum mode ){
    register GLenum *val= ColorMaterial_bucket + (face-0x0404);
    if(*val!=mode){
      *val=mode;
      ::glColorMaterial(face,mode);
    }
  }
  GLfloat PointSize_bucket;
  inline void glPointSize(GLfloat f) {
    if(f!=PointSize_bucket){
      PointSize_bucket=f;
      ::glPointSize(f);
    }
  }
  GLfloat LineWidth_bucket;
  inline void glLineWidth(GLfloat f){
    if(f!=LineWidth_bucket){
      LineWidth_bucket=f;
      ::glPointSize(f);
    }
  }
  GLint LineStipple_FACTOR_bucket;
  GLushort LineStipple_PATTERN_bucket;
  inline void glLineStipple(GLint factor, GLushort pattern )
    {
      if(factor!=LineStipple_FACTOR_bucket ||
         pattern!=LineStipple_PATTERN_bucket)
        {
        LineStipple_FACTOR_bucket=factor;
        LineStipple_PATTERN_bucket=pattern;
        ::glLineStipple(factor,pattern);
        }
    }

  GLclampd DepthRange_NEAR_bucket;
  GLclampd DepthRange_FAR_bucket;
  inline void glDepthRange(GLclampd nearval,GLclampd farval )
    {
      if(DepthRange_NEAR_bucket!=nearval ||
         DepthRange_FAR_bucket!=farval)
        {
        DepthRange_NEAR_bucket=nearval;
        DepthRange_FAR_bucket=farval;
        ::glDepthRange(nearval,farval);
        }
    }
  
#ifdef GL_VERSION_1_1
  // enable GL_POLYGON_OFFSET_FILL = 0x8037
  GLfloat PolygonOffset_bucket[2];
  inline void glPolygonOffset( GLfloat f,GLfloat u) {
    if(PolygonOffset_bucket[0]!=f ||
       PolygonOffset_bucket[1]!=u){
      PolygonOffset_bucket[0]=f;
      PolygonOffset_bucket[1]=u;
      ::glPolygonOffset(f,u);
    }
  }
#endif
};


//#ifdef vtkOpenGLStateCache_Cache
//#undef vtkOpenGLStateCache_Cache
//#endif
