/* Id */
/*
 * GL2PS, an OpenGL to PostScript Printing Library
 * Copyright (C) 1999-2003 Christophe Geuzaine <geuz@geuz.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of either:
 *
 * a) the GNU Library General Public License as published by the Free
 * Software Foundation, either version 2 of the License, or (at your
 * option) any later version; or
 *
 * b) the GL2PS License as published by Christophe Geuzaine, either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See either
 * the GNU Library General Public License or the GL2PS License for
 * more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library in the file named "COPYING.LGPL";
 * if not, write to the Free Software Foundation, Inc., 675 Mass Ave,
 * Cambridge, MA 02139, USA.
 *
 * You should have received a copy of the GL2PS License with this
 * library in the file named "COPYING.GL2PS"; if not, I will be glad
 * to provide one.
 *
 * Contributors:
 *   Michael Sweet <mike@easysw.com>
 *   Marc Ume <marc.ume@digitalgraphics.be>
 *   Jean-Francois Remacle <remacle@scorec.rpi.edu>
 *   Bart Kaptein <B.L.Kaptein@lumc.nl>
 *   Quy Nguyen-Dai<quy@vnilux.com>
 *   Sam Buss <sbuss@ucsd.edu>
 *   Shane Hill <Shane.Hill@dsto.defence.gov.au>
 *   Romain Boman <r_boman@yahoo.fr>
 *   Rouben Rostamian <rostamian@umbc.edu>
 *   Diego Santa Cruz <Diego.SantaCruz@epfl.ch>
 *   Shahzad Muzaffar <Shahzad.Muzaffar@cern.ch>
 *   Lassi Tuura <lassi.tuura@cern.ch>
 *   Guy Barrand <barrand@lal.in2p3.fr>
 *   Micha Bieber <bieber@traits.de>
 *
 * For the latest info about gl2ps, see http://www.geuz.org/gl2ps/
 */

#include <string.h>
#include <sys/types.h>
#include <stdarg.h>
#include <time.h>
#include <float.h>
#include "gl2ps.h"

/* The gl2ps context. gl2ps is not thread safe (we should create a
   local GL2PScontext during gl2psBeginPage) */

GL2PScontext *gl2ps = NULL;

/********************************************************************* 
 *
 * Utility routines
 *
 *********************************************************************/

void gl2psMsg(GLint level, char *fmt, ...){
  va_list args;

  if(!(gl2ps->options & GL2PS_SILENT)){
    switch(level){
    case GL2PS_INFO : fprintf(stderr, "GL2PS info: "); break;
    case GL2PS_WARNING : fprintf(stderr, "GL2PS warning: "); break;
    case GL2PS_ERROR : fprintf(stderr, "GL2PS error: "); break;
    }
    va_start(args, fmt);
    vfprintf(stderr, fmt, args); 
    va_end(args);
    fprintf(stderr, "\n");
  }
  /* if(level == GL2PS_ERROR) exit(1); */
}

void *gl2psMalloc(size_t size){
  void *ptr;

  if(!size) return(NULL);
  ptr = malloc(size);
  if(!ptr){
    gl2psMsg(GL2PS_ERROR, "Couldn't allocate requested memory");
    exit(1);
  }
  return(ptr);
}

void *gl2psRealloc(void *ptr, size_t size){
  if(!size) return(NULL);
  ptr = realloc(ptr, size);
  if(!ptr){
    gl2psMsg(GL2PS_ERROR, "Couldn't reallocate requested memory");
    exit(1);
  }
  return(ptr);
}

void gl2psFree(void *ptr){
  if(!ptr) return;
  free(ptr);
}

/* zlib compression helper routines */

#ifdef GL2PS_HAVE_ZLIB

void gl2psSetupCompress(){
  gl2ps->compress = (GL2PScompress*)gl2psMalloc(sizeof(GL2PScompress));
  gl2ps->compress->src = NULL;
  gl2ps->compress->start = NULL;
  gl2ps->compress->dest = NULL;
  gl2ps->compress->srcLen = 0;
  gl2ps->compress->destLen = 0;
}

void gl2psFreeCompress(){
  if(!gl2ps->compress)
    return;
  gl2psFree(gl2ps->compress->start);
  gl2psFree(gl2ps->compress->dest);
  gl2ps->compress->src = NULL;
  gl2ps->compress->start = NULL;
  gl2ps->compress->dest = NULL;
  gl2ps->compress->srcLen = 0;
  gl2ps->compress->destLen = 0;
}

int gl2psAllocCompress(unsigned int srcsize){
  gl2psFreeCompress();
  
  if(!gl2ps->compress || !srcsize)
    return GL2PS_ERROR;
  
  gl2ps->compress->srcLen = srcsize;
  gl2ps->compress->destLen = (int)ceil(1.001 * gl2ps->compress->srcLen + 12);
  gl2ps->compress->src = (Bytef*)gl2psMalloc(gl2ps->compress->srcLen);
  gl2ps->compress->start = gl2ps->compress->src;
  gl2ps->compress->dest = (Bytef*)gl2psMalloc(gl2ps->compress->destLen);
  
  return GL2PS_SUCCESS;
}

void* gl2psReallocCompress(unsigned int srcsize){
  if(!gl2ps->compress || !srcsize)
    return NULL;
  
  if(srcsize < gl2ps->compress->srcLen)
    return gl2ps->compress->start;
  
  gl2ps->compress->srcLen = srcsize;
  gl2ps->compress->destLen = (int)ceil(1.001 * gl2ps->compress->srcLen + 12);
  gl2ps->compress->src = (Bytef*)gl2psRealloc(gl2ps->compress->src, gl2ps->compress->srcLen);
  gl2ps->compress->start = gl2ps->compress->src;
  gl2ps->compress->dest = (Bytef*)gl2psRealloc(gl2ps->compress->dest, gl2ps->compress->destLen);
  
  return gl2ps->compress->start;
}

size_t gl2psWriteBigEndianCompress(unsigned long data, size_t bytes){
  size_t i;
  size_t size = sizeof(unsigned long);
  for(i = 1; i <= bytes; ++i){
    *gl2ps->compress->src = (Bytef)(0xff & (data >> (size-i) * 8));
    ++gl2ps->compress->src;
  }
  return bytes;
}

int gl2psDeflate(){
  /* For compatibility with older zlib versions, we use compress(...)
     instead of compress2(..., Z_BEST_COMPRESSION) */
  return compress(gl2ps->compress->dest, &gl2ps->compress->destLen, 
                  gl2ps->compress->start, gl2ps->compress->srcLen);  
}

#endif

int gl2psPrintf(const char* fmt, ...){
  int ret = 0;
  va_list args;

#ifdef GL2PS_HAVE_ZLIB
  unsigned int bufsize = 0;
  unsigned int oldsize = 0;
  static char buf[1000];
  if(gl2ps->options & GL2PS_COMPRESS){
    va_start(args, fmt);
    bufsize = vsprintf(buf, fmt, args);
    va_end(args);
    oldsize = gl2ps->compress->srcLen;
    gl2ps->compress->start = (Bytef*)gl2psReallocCompress(oldsize + bufsize);
    memcpy(gl2ps->compress->start+oldsize, buf, bufsize);
  }
  else{
#endif
    va_start(args, fmt);
    ret = vfprintf(gl2ps->stream, fmt, args);
    va_end(args);
#ifdef GL2PS_HAVE_ZLIB
  }
#endif
  return ret;
}

size_t gl2psWriteBigEndian(unsigned long data, size_t bytes){
  size_t i;
  size_t size = sizeof(unsigned long);
  for(i = 1; i <= bytes; ++i){
    fputc(0xff & (data >> (size-i) * 8), gl2ps->stream);
  }
  return bytes;
}

/* The list handling routines */

void gl2psListRealloc(GL2PSlist *list, GLint n){
  if(n <= 0) return;
  if(!list->array){
    list->nmax = ((n - 1) / list->incr + 1) * list->incr;
    list->array = (char *)gl2psMalloc(list->nmax * list->size);
  }
  else{
    if(n > list->nmax){
      list->nmax = ((n - 1) / list->incr + 1) * list->incr;
      list->array = (char *)gl2psRealloc(list->array,
                                         list->nmax * list->size);
    }
  }
}

GL2PSlist *gl2psListCreate(GLint n, GLint incr, GLint size){
  GL2PSlist *list;

  if(n < 0) n = 0;
  if(incr <= 0) incr = 1;
  list = (GL2PSlist *)gl2psMalloc(sizeof(GL2PSlist));
  list->nmax = 0;
  list->incr = incr;
  list->size = size;
  list->n = 0;
  list->array = NULL;
  gl2psListRealloc(list, n);
  return(list);
}

void gl2psListReset(GL2PSlist *list){
  list->n = 0;
}

void gl2psListDelete(GL2PSlist *list){
  gl2psFree(list->array);
  gl2psFree(list);
}

void gl2psListAdd(GL2PSlist *list, void *data){
  list->n++;
  gl2psListRealloc(list, list->n);
  memcpy(&list->array[(list->n - 1) * list->size], data, list->size);
}

int gl2psListNbr(GL2PSlist *list){
  return(list->n);
}

void *gl2psListPointer(GL2PSlist *list, GLint index){
  if((index < 0) || (index >= list->n)){
    gl2psMsg(GL2PS_ERROR, "Wrong list index in gl2psListPointer");
    return(&list->array[0]);
  }
  return(&list->array[index * list->size]);
}

void gl2psListRead(GL2PSlist *list, GLint index, void *data){
  if((index < 0) || (index >= list->n)){
    gl2psMsg(GL2PS_ERROR, "Wrong list index in gl2psListRead");
    data = 0;
  }
  else{
    memcpy(data, &list->array[index * list->size], list->size);
  }
}

void gl2psListSort(GL2PSlist *list,
                   int (*fcmp)(const void *a, const void *b)){
  qsort(list->array, list->n, list->size, fcmp);
}

void gl2psListAction(GL2PSlist *list, 
                     void (*action)(void *data, void *dummy)){
  GLint i, dummy;

  for(i = 0; i < gl2psListNbr(list); i++){
    (*action)(gl2psListPointer(list, i), &dummy);
  }
}

void gl2psListActionInverse(GL2PSlist *list, 
                            void (*action)(void *data, void *dummy)){
  GLint i, dummy;

  for(i = gl2psListNbr(list); i > 0; i--){
    (*action)(gl2psListPointer(list, i-1), &dummy);
  }
}

/* Helper for pixmaps and strings */

GL2PSimage* gl2psCopyPixmap(GL2PSimage* im){
  int size;
  GL2PSimage* image = (GL2PSimage*)gl2psMalloc(sizeof(GL2PSimage));
  
  image->width = im->width;
  image->height = im->height;
  image->format = im->format;
  image->type = im->type;

  /* FIXME: handle other types/formats */
  size = image->height*image->width*3*sizeof(GLfloat);

  image->pixels = (GLfloat*)gl2psMalloc(size);
  memcpy(image->pixels, im->pixels, size);
  
  return image;
}

void gl2psFreePixmap(GL2PSimage* im){
  if(!im)
    return;
  if(im->pixels)
    gl2psFree(im->pixels);
  gl2psFree(im);
}

GL2PSstring* gl2psCopyText(GL2PSstring* t){
  GL2PSstring* text = (GL2PSstring*)gl2psMalloc(sizeof(GL2PSstring));
  text->str = (char*)gl2psMalloc((strlen(t->str)+1)*sizeof(char));
  strcpy(text->str, t->str); 
  text->fontname = (char*)gl2psMalloc((strlen(t->fontname)+1)*sizeof(char));
  strcpy(text->fontname, t->fontname);
  text->fontsize = t->fontsize;
  text->alignment = t->alignment;
  
  return text;
}

void gl2psFreeText(GL2PSstring* text){
  if(!text)
    return;
  if(text->str)
    gl2psFree(text->str);
  if(text->fontname)
    gl2psFree(text->fontname);
  gl2psFree(text);
}

/* Helpers for rgba colors */

GLfloat gl2psColorDiff(GL2PSrgba rgba1, GL2PSrgba rgba2){
  int i;        
  GLfloat res = 0;
  for(i = 0; i < 3; ++i){
    res += (rgba1[i] - rgba2[i]) * (rgba1[i] - rgba2[i]);
  }
  return res;
}

GLboolean gl2psSameColor(GL2PSrgba rgba1, GL2PSrgba rgba2){
  return !(rgba1[0] != rgba2[0] || 
           rgba1[1] != rgba2[1] ||
           rgba1[2] != rgba2[2]);
}
  
GLboolean gl2psVertsSameColor(const GL2PSprimitive *prim){
  int i;

  for(i = 1; i < prim->numverts; i++){
    if(!gl2psSameColor(prim->verts[0].rgba, prim->verts[i].rgba)){
      return 0;
    }
  }
  return 1;
}

void gl2psSetLastColor(GL2PSrgba rgba){
  int i;        
  for(i = 0; i < 3; ++i){
    gl2ps->lastrgba[i] = rgba[i];
  }
}

/********************************************************************* 
 *
 * 3D sorting routines 
 *
 *********************************************************************/

GLfloat gl2psComparePointPlane(GL2PSxyz point, GL2PSplane plane){
  return(plane[0] * point[0] + 
         plane[1] * point[1] + 
         plane[2] * point[2] + 
         plane[3]);
}

GLfloat gl2psPsca(GLfloat *a, GLfloat *b){
  return(a[0]*b[0] + a[1]*b[1] + a[2]*b[2]);
}

void gl2psPvec(GLfloat *a, GLfloat *b, GLfloat *c){
  c[0] = a[1]*b[2] - a[2]*b[1];
  c[1] = a[2]*b[0] - a[0]*b[2];
  c[2] = a[0]*b[1] - a[1]*b[0];
}

GLfloat gl2psNorm(GLfloat *a){
  return (GLfloat)sqrt(a[0]*a[0] + a[1]*a[1] + a[2]*a[2]);
}

void gl2psGetNormal(GLfloat *a, GLfloat *b, GLfloat *c){
  GLfloat norm;

  gl2psPvec(a, b, c);
  if(!GL2PS_ZERO(norm = gl2psNorm(c))){
    c[0] = c[0] / norm;
    c[1] = c[1] / norm;
    c[2] = c[2] / norm;
  }
  else{
    /* FIXME: the plane is still wrong, despite our tests in
       gl2psGetPlane... Let's return a dummy value for now (this is a
       hack: we should do more tests in GetPlane) */
    c[0] = c[1] = 0.0F;
    c[2] = 1.0F;
  }
}

void gl2psGetPlane(GL2PSprimitive *prim, GL2PSplane plane){
  GL2PSxyz v = {0.0F, 0.0F, 0.0F}, w = {0.0F, 0.0F, 0.0F};

  switch(prim->type){
  case GL2PS_TRIANGLE :
  case GL2PS_QUADRANGLE :
    v[0] = prim->verts[1].xyz[0] - prim->verts[0].xyz[0]; 
    v[1] = prim->verts[1].xyz[1] - prim->verts[0].xyz[1]; 
    v[2] = prim->verts[1].xyz[2] - prim->verts[0].xyz[2]; 
    w[0] = prim->verts[2].xyz[0] - prim->verts[0].xyz[0]; 
    w[1] = prim->verts[2].xyz[1] - prim->verts[0].xyz[1]; 
    w[2] = prim->verts[2].xyz[2] - prim->verts[0].xyz[2]; 
    if((GL2PS_ZERO(v[0]) && GL2PS_ZERO(v[1]) && GL2PS_ZERO(v[2])) || 
       (GL2PS_ZERO(w[0]) && GL2PS_ZERO(w[1]) && GL2PS_ZERO(w[2]))){
      plane[0] = plane[1] = 0.0F;
      plane[2] = 1.0F;
      plane[3] = -prim->verts[0].xyz[2];
    }
    else{
      gl2psGetNormal(v, w, plane);
      plane[3] = 
        - plane[0] * prim->verts[0].xyz[0] 
        - plane[1] * prim->verts[0].xyz[1] 
        - plane[2] * prim->verts[0].xyz[2];
    }
    break;
  case GL2PS_LINE :
    v[0] = prim->verts[1].xyz[0] - prim->verts[0].xyz[0]; 
    v[1] = prim->verts[1].xyz[1] - prim->verts[0].xyz[1]; 
    v[2] = prim->verts[1].xyz[2] - prim->verts[0].xyz[2]; 
    if(GL2PS_ZERO(v[0]) && GL2PS_ZERO(v[1]) && GL2PS_ZERO(v[2])){
      plane[0] = plane[1] = 0.0F;
      plane[2] = 1.0F;
      plane[3] = -prim->verts[0].xyz[2];
    }
    else{
      if(GL2PS_ZERO(v[0]))      w[0] = 1.0F;
      else if(GL2PS_ZERO(v[1])) w[1] = 1.0F;
      else                      w[2] = 1.0F;
      gl2psGetNormal(v, w, plane);
      plane[3] = 
        - plane[0] * prim->verts[0].xyz[0] 
        - plane[1] * prim->verts[0].xyz[1] 
        - plane[2] * prim->verts[0].xyz[2];
    }
    break;
  case GL2PS_POINT :
  case GL2PS_PIXMAP :
  case GL2PS_TEXT :
    plane[0] = plane[1] = 0.0F;
    plane[2] = 1.0F;
    plane[3] = -prim->verts[0].xyz[2];
    break;
  default :
    gl2psMsg(GL2PS_ERROR, "Unknown primitive type in BSP tree");
    plane[0] = plane[1] = plane[3] = 0.0F;
    plane[2] = 1.0F;
    break;
  }
}

void gl2psCutEdge(GL2PSvertex *a, GL2PSvertex *b, GL2PSplane plane, 
                  GL2PSvertex *c){
  GL2PSxyz v;
  GLfloat sect;

  v[0] = b->xyz[0] - a->xyz[0];
  v[1] = b->xyz[1] - a->xyz[1];
  v[2] = b->xyz[2] - a->xyz[2];
  sect = - gl2psComparePointPlane(a->xyz, plane) / gl2psPsca(plane, v);

  c->xyz[0] = a->xyz[0] + v[0] * sect;
  c->xyz[1] = a->xyz[1] + v[1] * sect;
  c->xyz[2] = a->xyz[2] + v[2] * sect;
  
  c->rgba[0] = (1 - sect) * a->rgba[0] + sect * b->rgba[0];
  c->rgba[1] = (1 - sect) * a->rgba[1] + sect * b->rgba[1];
  c->rgba[2] = (1 - sect) * a->rgba[2] + sect * b->rgba[2];
  c->rgba[3] = (1 - sect) * a->rgba[3] + sect * b->rgba[3];
}

void gl2psCreateSplitPrimitive(GL2PSprimitive *parent, GL2PSplane plane,
                               GL2PSprimitive *child, GLshort numverts,
                               GLshort *index0, GLshort *index1){
  GLshort i;

  if(numverts > 4){
    gl2psMsg(GL2PS_WARNING, "%d vertices in polygon", numverts);
    numverts = 4;
  }

  switch(numverts){
  case 1 : child->type = GL2PS_POINT; break; 
  case 2 : child->type = GL2PS_LINE; break; 
  case 3 : child->type = GL2PS_TRIANGLE; break; 
  case 4 : child->type = GL2PS_QUADRANGLE; break;    
  }
  child->boundary = 0; /* not done! */
  child->depth = parent->depth; /* should not be used in this case */
  child->culled = parent->culled;
  child->dash = parent->dash;
  child->width = parent->width;
  child->numverts = numverts;
  child->verts = (GL2PSvertex *)gl2psMalloc(numverts * sizeof(GL2PSvertex));

  for(i = 0; i < numverts; i++){
    if(index1[i] < 0){
      child->verts[i] = parent->verts[index0[i]];
    }
    else{
      gl2psCutEdge(&parent->verts[index0[i]], &parent->verts[index1[i]], 
                   plane, &child->verts[i]);
    }
  }
}

void gl2psAddIndex(GLshort *index0, GLshort *index1, GLshort *nb, 
                   GLshort i, GLshort j){
  GLint k;

  for(k = 0; k < *nb; k++){
    if((index0[k] == i && index1[k] == j) ||
       (index1[k] == i && index0[k] == j)) return;
  }
  index0[*nb] = i;
  index1[*nb] = j;
  (*nb)++;
}

GLshort gl2psGetIndex(GLshort i, GLshort num){
  return(i < num-1) ? i+1 : 0;
}

GLint gl2psTestSplitPrimitive(GL2PSprimitive *prim, GL2PSplane plane){
  GLint type = GL2PS_COINCIDENT;
  GLshort i, j;
  GLfloat d[5]; 

  for(i = 0; i < prim->numverts; i++){  
    d[i] = gl2psComparePointPlane(prim->verts[i].xyz, plane);
  }

  if(prim->numverts < 2){
    return 0;
  }
  else{
    for(i = 0; i < prim->numverts; i++){
      j = gl2psGetIndex(i, prim->numverts);
      if(d[j] > GL2PS_EPSILON){
        if(type == GL2PS_COINCIDENT)      type = GL2PS_IN_BACK_OF;
        else if(type != GL2PS_IN_BACK_OF) return 1; 
        if(d[i] < -GL2PS_EPSILON)         return 1;
      }
      else if(d[j] < -GL2PS_EPSILON){
        if(type == GL2PS_COINCIDENT)       type = GL2PS_IN_FRONT_OF;   
        else if(type != GL2PS_IN_FRONT_OF) return 1;
        if(d[i] > GL2PS_EPSILON)           return 1;
      }
    }
  }
  return 0;
}

GLint gl2psSplitPrimitive(GL2PSprimitive *prim, GL2PSplane plane, 
                          GL2PSprimitive **front, GL2PSprimitive **back){
  GLshort i, j, in=0, out=0, in0[5], in1[5], out0[5], out1[5];
  GLint type;
  GLfloat d[5]; 

  type = GL2PS_COINCIDENT;

  for(i = 0; i < prim->numverts; i++){  
    d[i] = gl2psComparePointPlane(prim->verts[i].xyz, plane);
  }

  switch(prim->type){
  case GL2PS_POINT :
    if(d[0] > GL2PS_EPSILON)       type = GL2PS_IN_BACK_OF;
    else if(d[0] < -GL2PS_EPSILON) type = GL2PS_IN_FRONT_OF;
    else                           type = GL2PS_COINCIDENT;
    break;
  default :
    for(i = 0; i < prim->numverts; i++){
      j = gl2psGetIndex(i, prim->numverts);
      if(d[j] > GL2PS_EPSILON){
        if(type == GL2PS_COINCIDENT)      type = GL2PS_IN_BACK_OF;
        else if(type != GL2PS_IN_BACK_OF) type = GL2PS_SPANNING; 
        if(d[i] < -GL2PS_EPSILON){
          gl2psAddIndex(in0, in1, &in, i, j);
          gl2psAddIndex(out0, out1, &out, i, j);
          type = GL2PS_SPANNING;
        }
        gl2psAddIndex(out0, out1, &out, j, -1);
      }
      else if(d[j] < -GL2PS_EPSILON){
        if(type == GL2PS_COINCIDENT)       type = GL2PS_IN_FRONT_OF;   
        else if(type != GL2PS_IN_FRONT_OF) type = GL2PS_SPANNING;
        if(d[i] > GL2PS_EPSILON){
          gl2psAddIndex(in0, in1, &in, i, j);
          gl2psAddIndex(out0, out1, &out, i, j);
          type = GL2PS_SPANNING;
        }
        gl2psAddIndex(in0, in1, &in, j, -1);
      }
      else{
        gl2psAddIndex(in0, in1, &in, j, -1);
        gl2psAddIndex(out0, out1, &out, j, -1);
      }
    }
    break;
  }

  if(type == GL2PS_SPANNING){
    *back = (GL2PSprimitive*)gl2psMalloc(sizeof(GL2PSprimitive));
    *front = (GL2PSprimitive*)gl2psMalloc(sizeof(GL2PSprimitive));
    gl2psCreateSplitPrimitive(prim, plane, *back, out, out0, out1);
    gl2psCreateSplitPrimitive(prim, plane, *front, in, in0, in1);
  }

  return type;
}

void gl2psDivideQuad(GL2PSprimitive *quad, 
                     GL2PSprimitive **t1, GL2PSprimitive **t2){
  *t1 = (GL2PSprimitive*)gl2psMalloc(sizeof(GL2PSprimitive));
  *t2 = (GL2PSprimitive*)gl2psMalloc(sizeof(GL2PSprimitive));
  (*t1)->type = (*t2)->type = GL2PS_TRIANGLE;
  (*t1)->numverts = (*t2)->numverts = 3;
  (*t1)->depth = (*t2)->depth = quad->depth;
  (*t1)->culled = (*t2)->culled = quad->culled;
  (*t1)->dash = (*t2)->dash = quad->dash;
  (*t1)->width = (*t2)->width = quad->width;
  (*t1)->verts = (GL2PSvertex *)gl2psMalloc(3 * sizeof(GL2PSvertex));
  (*t2)->verts = (GL2PSvertex *)gl2psMalloc(3 * sizeof(GL2PSvertex));
  (*t1)->verts[0] = quad->verts[0];
  (*t1)->verts[1] = quad->verts[1];
  (*t1)->verts[2] = quad->verts[2];
  (*t1)->boundary = ((quad->boundary & 1) ? 1 : 0) | ((quad->boundary & 2) ? 2 : 0);
  (*t2)->verts[0] = quad->verts[0];
  (*t2)->verts[1] = quad->verts[2];
  (*t2)->verts[2] = quad->verts[3];
  (*t1)->boundary = ((quad->boundary & 4) ? 2 : 0) | ((quad->boundary & 4) ? 2 : 0);
}

int gl2psCompareDepth(const void *a, const void *b){
  GL2PSprimitive *q, *w;
  GLfloat diff;

  q = *(GL2PSprimitive**)a;
  w = *(GL2PSprimitive**)b;
  diff = q->depth - w->depth;
  if(diff > 0.){
    return 1;
  }
  else if(diff < 0.){
    return -1;
  }
  else{
    return 0;
  }
}

int gl2psTrianglesFirst(const void *a, const void *b){
  GL2PSprimitive *q, *w;

  q = *(GL2PSprimitive**)a;
  w = *(GL2PSprimitive**)b;
  return(q->type < w->type ? 1 : -1);
}

GLint gl2psFindRoot(GL2PSlist *primitives, GL2PSprimitive **root){
  GLint i, j, count, best = 1000000, index = 0;
  GL2PSprimitive *prim1, *prim2;
  GL2PSplane plane;
  GLint maxp;

  if(gl2ps->options & GL2PS_BEST_ROOT){
    *root = *(GL2PSprimitive**)gl2psListPointer(primitives, 0);
    maxp = gl2psListNbr(primitives);
    if(maxp > gl2ps->maxbestroot){
      maxp = gl2ps->maxbestroot;
    }
    for(i = 0; i < maxp; i++){
      prim1 = *(GL2PSprimitive**)gl2psListPointer(primitives, i);
      gl2psGetPlane(prim1, plane);
      count = 0;
      for(j = 0; j < gl2psListNbr(primitives); j++){
        if(j != i){
          prim2 = *(GL2PSprimitive**)gl2psListPointer(primitives, j);
          count += gl2psTestSplitPrimitive(prim2, plane); 
        }
        if(count > best) break;
      }
      if(count < best){
        best = count;
        index = i;
        *root = prim1;
        if(!count) return index;
      }
    }
    /* if(index) gl2psMsg(GL2PS_INFO, "GL2PS_BEST_ROOT was worth it: %d", index); */
    return index;
  }
  else{
    *root = *(GL2PSprimitive**)gl2psListPointer(primitives, 0);
    return 0;
  }
}

void gl2psFreePrimitive(void *a, void *b){
  GL2PSprimitive *q;
  
  q = *(GL2PSprimitive**)a;
  gl2psFree(q->verts);
  if(q->type == GL2PS_TEXT){
    gl2psFree(q->text->str);
    gl2psFree(q->text->fontname);
    gl2psFree(q->text);
  }
  if(q->type == GL2PS_PIXMAP){
    gl2psFree(q->image->pixels);
    gl2psFree(q->image);
  }
  gl2psFree(q);
}

void gl2psAddPrimitiveInList(GL2PSprimitive *prim, GL2PSlist *list){
  GL2PSprimitive *t1, *t2;

  if(prim->type != GL2PS_QUADRANGLE){
    gl2psListAdd(list, &prim);
  }
  else{
    gl2psDivideQuad(prim, &t1, &t2);
    gl2psListAdd(list, &t1);
    gl2psListAdd(list, &t2);
    gl2psFreePrimitive(&prim, NULL);
  }
  
}

void gl2psFreeBspTree(GL2PSbsptree **tree){
  if(*tree){
    if((*tree)->back) gl2psFreeBspTree(&(*tree)->back);
    if((*tree)->primitives){
      gl2psListAction((*tree)->primitives, gl2psFreePrimitive);
      gl2psListDelete((*tree)->primitives);
    }
    if((*tree)->front) gl2psFreeBspTree(&(*tree)->front);
    gl2psFree(*tree);
    *tree = NULL;
  }
}

GLboolean gl2psGreater(GLfloat f1, GLfloat f2){
  if(f1 > f2) return 1;
  else return 0;
}

GLboolean gl2psLess(GLfloat f1, GLfloat f2){
  if(f1 < f2) return 1;
  else return 0;
}

void gl2psBuildBspTree(GL2PSbsptree *tree, GL2PSlist *primitives){
  GL2PSprimitive *prim, *frontprim, *backprim;
  GL2PSlist *frontlist, *backlist;
  GLint i, index;

  tree->front = NULL;
  tree->back = NULL;
  tree->primitives = gl2psListCreate(1, 2, sizeof(GL2PSprimitive*));
  index = gl2psFindRoot(primitives, &prim);
  gl2psGetPlane(prim, tree->plane);
  gl2psAddPrimitiveInList(prim, tree->primitives);

  frontlist = gl2psListCreate(1, 2, sizeof(GL2PSprimitive*));
  backlist = gl2psListCreate(1, 2, sizeof(GL2PSprimitive*));

  for(i = 0; i < gl2psListNbr(primitives); i++){
    if(i != index){
      prim = *(GL2PSprimitive**)gl2psListPointer(primitives,i);
      switch(gl2psSplitPrimitive(prim, tree->plane, &frontprim, &backprim)){
      case GL2PS_COINCIDENT:
        gl2psAddPrimitiveInList(prim, tree->primitives);
        break;
      case GL2PS_IN_BACK_OF:
        gl2psAddPrimitiveInList(prim, backlist);
        break;
      case GL2PS_IN_FRONT_OF:
        gl2psAddPrimitiveInList(prim, frontlist);
        break;
      case GL2PS_SPANNING:
        gl2psAddPrimitiveInList(backprim, backlist);
        gl2psAddPrimitiveInList(frontprim, frontlist);
        gl2psFreePrimitive(&prim, NULL);
        break;
      }
    }
  }

  if(gl2psListNbr(tree->primitives)){
    gl2psListSort(tree->primitives, gl2psTrianglesFirst);
  }

  if(gl2psListNbr(frontlist)){
    gl2psListSort(frontlist, gl2psTrianglesFirst);
    tree->front = (GL2PSbsptree*)gl2psMalloc(sizeof(GL2PSbsptree));
    gl2psBuildBspTree(tree->front, frontlist);
  }
  else{
    gl2psListDelete(frontlist);
  }

  if(gl2psListNbr(backlist)){
    gl2psListSort(backlist, gl2psTrianglesFirst);
    tree->back = (GL2PSbsptree*)gl2psMalloc(sizeof(GL2PSbsptree));
    gl2psBuildBspTree(tree->back, backlist);
  }
  else{
    gl2psListDelete(backlist);
  }

  gl2psListDelete(primitives);
}

void gl2psTraverseBspTree(GL2PSbsptree *tree, GL2PSxyz eye, GLfloat epsilon,
                          GLboolean (*compare)(GLfloat f1, GLfloat f2),
                          void (*action)(void *data, void *dummy), int inverse){
  GLfloat result;

  if(!tree) return;

  result = gl2psComparePointPlane(eye, tree->plane);

  if(compare(result, epsilon)){
    gl2psTraverseBspTree(tree->back, eye, epsilon, compare, action, inverse);
    if(inverse){
      gl2psListActionInverse(tree->primitives, action);
    }
    else{
      gl2psListAction(tree->primitives, action);
    }
    gl2psTraverseBspTree(tree->front, eye, epsilon, compare, action, inverse);
  }
  else if(compare(-epsilon, result)){ 
    gl2psTraverseBspTree(tree->front, eye, epsilon, compare, action, inverse);
    if(inverse){
      gl2psListActionInverse(tree->primitives, action);
    }
    else{
      gl2psListAction(tree->primitives, action);
    }
    gl2psTraverseBspTree(tree->back, eye, epsilon, compare, action, inverse);
  }
  else{
    gl2psTraverseBspTree(tree->front, eye, epsilon, compare, action, inverse);
    gl2psTraverseBspTree(tree->back, eye, epsilon, compare, action, inverse);
  }
}

/********************************************************************* 
 *
 * 2D sorting routines (for occlusion culling) 
 *
 *********************************************************************/

GLint gl2psGetPlaneFromPoints(GL2PSxyz a, GL2PSxyz b, GL2PSplane plane){  
  GLfloat n; 

  plane[0] = b[1] - a[1];
  plane[1] = a[0] - b[0];
  n = (GLfloat)sqrt(plane[0]*plane[0] + plane[1]*plane[1]);
  plane[2] = 0.0F;
  if(n != 0.0){
    plane[0] /= n;
    plane[1] /= n;
    plane[3] = -plane[0]*a[0]-plane[1]*a[1]; 
    return 1;
  }
  else{
    plane[0] = -1.0F;
    plane[1] = 0.0F;
    plane[3] = a[0];
    return 0;
  }
}

void gl2psFreeBspImageTree(GL2PSbsptree2d **tree){
  if(*tree){
    if((*tree)->back)  gl2psFreeBspImageTree(&(*tree)->back);
    if((*tree)->front) gl2psFreeBspImageTree(&(*tree)->front);
    gl2psFree(*tree);
    *tree = NULL;
  }
}

GLint gl2psCheckPoint(GL2PSxyz point, GL2PSplane plane){
  GLfloat pt_dis;

  pt_dis = gl2psComparePointPlane(point, plane);
  if(pt_dis > GL2PS_EPSILON)        return GL2PS_POINT_INFRONT;
  else if(pt_dis < -GL2PS_EPSILON)  return GL2PS_POINT_BACK;
  else                              return GL2PS_POINT_COINCIDENT;
}

void gl2psAddPlanesInBspTreeImage(GL2PSprimitive *prim,
                                  GL2PSbsptree2d **tree){
  GLint ret = 0;
  GLint i;
  GLint offset = 0;
  GL2PSbsptree2d *head = NULL, *cur = NULL;

  if((*tree == NULL) && (prim->numverts > 2)){
    head = (GL2PSbsptree2d*)gl2psMalloc(sizeof(GL2PSbsptree2d));
    for(i = 0; i < prim->numverts-1; i++){
      if(!gl2psGetPlaneFromPoints(prim->verts[i].xyz,
                                  prim->verts[i+1].xyz,
                                  head->plane)){
        if(prim->numverts-i > 3){
          offset++;
        }
        else{
          gl2psFree(head);
          return;
        }
      }
      else{
        break;
      }
    }
    head->back = NULL;
    head->front = NULL;
    for(i = 2+offset; i < prim->numverts; i++){
      ret = gl2psCheckPoint(prim->verts[i].xyz, head->plane);
      if(ret != GL2PS_POINT_COINCIDENT) break;
    }
    switch(ret){
    case GL2PS_POINT_INFRONT :
      cur = head;
      for(i = 1+offset; i < prim->numverts-1; i++){
        if(cur->front == NULL){
          cur->front = (GL2PSbsptree2d*)gl2psMalloc(sizeof(GL2PSbsptree2d));
        }
        if(gl2psGetPlaneFromPoints(prim->verts[i].xyz,
                                   prim->verts[i+1].xyz,
                                   cur->front->plane)){
          cur = cur->front;
          cur->front = NULL;
          cur->back = NULL;
        }
      }
      if(cur->front == NULL){
        cur->front = (GL2PSbsptree2d*)gl2psMalloc(sizeof(GL2PSbsptree2d));
      }
      if(gl2psGetPlaneFromPoints(prim->verts[i].xyz,
                                 prim->verts[offset].xyz,
                                 cur->front->plane)){
        cur->front->front = NULL;
        cur->front->back = NULL;
      }
      else{
        gl2psFree(cur->front);
        cur->front = NULL;
      }
      break;
    case GL2PS_POINT_BACK :
      for(i = 0; i < 4; i++){
        head->plane[i] = -head->plane[i];
      }
      cur = head;
      for(i = 1+offset; i < prim->numverts-1; i++){
        if(cur->front == NULL){
          cur->front = (GL2PSbsptree2d*)gl2psMalloc(sizeof(GL2PSbsptree2d));
        }
        if(gl2psGetPlaneFromPoints(prim->verts[i+1].xyz,
                                   prim->verts[i].xyz,
                                   cur->front->plane)){
          cur = cur->front;
          cur->front = NULL;
          cur->back = NULL;
        }
      }
      if(cur->front == NULL){
        cur->front = (GL2PSbsptree2d*)gl2psMalloc(sizeof(GL2PSbsptree2d));
      }
      if(gl2psGetPlaneFromPoints(prim->verts[offset].xyz,
                                 prim->verts[i].xyz,
                                 cur->front->plane)){
        cur->front->front = NULL;
        cur->front->back = NULL;
      }
      else{
        gl2psFree(cur->front);
        cur->front = NULL;
      }
      break;
    default:
      gl2psFree(head);
      return;
    }
    (*tree) = head;
  }
}

GLint gl2psCheckPrimitive(GL2PSprimitive *prim, GL2PSplane plane){
  GLint i;
  GLint pos;

  pos = gl2psCheckPoint(prim->verts[0].xyz, plane);
  for(i = 1; i < prim->numverts; i++){
    pos |= gl2psCheckPoint(prim->verts[i].xyz, plane);
    if(pos == (GL2PS_POINT_INFRONT | GL2PS_POINT_BACK)) return GL2PS_SPANNING;
  }
  if(pos & GL2PS_POINT_INFRONT)   return GL2PS_IN_FRONT_OF;
  else if(pos & GL2PS_POINT_BACK) return GL2PS_IN_BACK_OF;
  else                            return GL2PS_COINCIDENT;
}

GL2PSprimitive* gl2psCreateSplitPrimitive2D(GL2PSprimitive *parent,
                                            GLshort numverts,
                                            GL2PSvertex *vertx){
  GLint i;
  GL2PSprimitive *child = (GL2PSprimitive*)gl2psMalloc(sizeof(GL2PSprimitive));

  switch(numverts){
  case 1 : child->type = GL2PS_POINT; break;
  case 2 : child->type = GL2PS_LINE; break;
  case 3 : child->type = GL2PS_TRIANGLE; break;
  case 4 : child->type = GL2PS_QUADRANGLE; break;
  }
  child->boundary = 0; /* not done! */
  child->depth = parent->depth;
  child->culled = parent->culled;
  child->dash = parent->dash;
  child->width = parent->width;
  child->numverts = numverts;
  child->verts = (GL2PSvertex *)gl2psMalloc(numverts * sizeof(GL2PSvertex));
  for(i = 0; i < numverts; i++){
    child->verts[i] = vertx[i];
  }
  return child;
}

void gl2psSplitPrimitive2D(GL2PSprimitive *prim, 
                           GL2PSplane plane, 
                           GL2PSprimitive **front, 
                           GL2PSprimitive **back){

  /* cur will hold the position of the current vertex
     prev will hold the position of the previous vertex
     prev0 will hold the position of the vertex number 0
     v1 and v2 represent the current and previous vertices, respectively
     flag is set if the current vertex should be checked against the plane */
  GLint cur = -1, prev = -1, i, v1 = 0, v2 = 0, flag = 1, prev0 = -1;
  
  /* list of vertices that will go in front and back primitive */
  GL2PSvertex *front_list = NULL, *back_list = NULL;
  
  /* number of vertices in front and back list */
  GLshort front_count = 0, back_count = 0;

  for(i = 0; i <= prim->numverts; i++){
    v1 = i;
    if(v1 == prim->numverts){
      if(prim->numverts < 3) break;
      v1 = 0;
      v2 = prim->numverts-1;
      cur = prev0;
    }
    else if(flag){
      cur = gl2psCheckPoint(prim->verts[v1].xyz, plane);
      if(i == 0){
        prev0 = cur;
      }
    } 
    if(((prev == -1) || (prev == cur) || (prev == 0) || (cur == 0)) &&
       (i < prim->numverts)){
      if(cur == GL2PS_POINT_INFRONT){
        front_count++;
        front_list = (GL2PSvertex*)gl2psRealloc(front_list,
                                                sizeof(GL2PSvertex)*front_count);
        front_list[front_count-1] = prim->verts[v1];
      }
      else if(cur == GL2PS_POINT_BACK){
        back_count++;
        back_list = (GL2PSvertex*)gl2psRealloc(back_list,
                                               sizeof(GL2PSvertex)*back_count);
        back_list[back_count-1] = prim->verts[v1];
      }
      else{
        front_count++;
        front_list = (GL2PSvertex*)gl2psRealloc(front_list,
                                                sizeof(GL2PSvertex)*front_count);
        front_list[front_count-1] = prim->verts[v1];
        back_count++;
        back_list = (GL2PSvertex*)gl2psRealloc(back_list,
                                               sizeof(GL2PSvertex)*back_count);
        back_list[back_count-1] = prim->verts[v1];
      }
      flag = 1;
    }
    else if((prev != cur) && (cur != 0) && (prev != 0)){
      if(v1 != 0){
        v2 = v1-1;
        i--;
      }
      front_count++;
      front_list = (GL2PSvertex*)gl2psRealloc(front_list,
                                              sizeof(GL2PSvertex)*front_count);
      gl2psCutEdge(&prim->verts[v2],
                   &prim->verts[v1],
                   plane,
                   &front_list[front_count-1]);
      back_count++;
      back_list = (GL2PSvertex*)gl2psRealloc(back_list,
                                             sizeof(GL2PSvertex)*back_count);
      back_list[back_count-1] = front_list[front_count-1];
      flag = 0;
    }
    prev = cur;
  }
  *front = gl2psCreateSplitPrimitive2D(prim, front_count, front_list);
  *back = gl2psCreateSplitPrimitive2D(prim, back_count, back_list);
  gl2psFree(front_list);
  gl2psFree(back_list);
}

GLint gl2psAddInBspImageTree(GL2PSprimitive *prim, GL2PSbsptree2d **tree){
  GLint ret = 0;
  GL2PSprimitive *frontprim = NULL, *backprim = NULL;
  
  /* FIXME: until we consider the actual extent of text strings and
     pixmaps, never cull them. Otherwise the whole string/pixmap gets
     culled as soon as the reference point is hidden */
  if(prim->type == GL2PS_PIXMAP || prim->type == GL2PS_TEXT){
    return 1;
  }

  if(*tree == NULL){
    if(!gl2ps->zerosurfacearea){
      gl2psAddPlanesInBspTreeImage(gl2ps->primitivetoadd, tree);
    }
    return 1;
  }
  else{
    switch(gl2psCheckPrimitive(prim, (*tree)->plane)){
    case GL2PS_IN_BACK_OF: return gl2psAddInBspImageTree(prim, &(*tree)->back);
    case GL2PS_IN_FRONT_OF: 
      if((*tree)->front != NULL) return gl2psAddInBspImageTree(prim, &(*tree)->front);
      else                       return 0;
    case GL2PS_SPANNING:
      gl2psSplitPrimitive2D(prim, (*tree)->plane, &frontprim, &backprim);
      ret = gl2psAddInBspImageTree(backprim, &(*tree)->back);
      if((*tree)->front != NULL){
        if(gl2psAddInBspImageTree(frontprim, &(*tree)->front)){
          ret = 1;
        }
      }
      gl2psFree(frontprim->verts);
      gl2psFree(frontprim);
      gl2psFree(backprim->verts);
      gl2psFree(backprim);
      return ret;
    case GL2PS_COINCIDENT:
      if((*tree)->back != NULL){
        gl2ps->zerosurfacearea = 1;
        ret = gl2psAddInBspImageTree(prim, &(*tree)->back);
        gl2ps->zerosurfacearea = 0;
        if(ret) return ret;
      }
      if((*tree)->front != NULL){
        gl2ps->zerosurfacearea = 1;
        ret = gl2psAddInBspImageTree(prim, &(*tree)->front);
        gl2ps->zerosurfacearea = 0;
        if(ret) return ret;
      }
      if(prim->type == GL2PS_LINE) return 1;
      else                         return 0;
    }
  }
  return 0;
}

void gl2psAddInImageTree(void *a, void *b){
  GL2PSprimitive *prim = *(GL2PSprimitive **)a;
  gl2ps->primitivetoadd = prim;
  if(!gl2psAddInBspImageTree(prim, &gl2ps->imagetree)){
    prim->culled = 1;
  }
}

/* Boundary contruction */

void gl2psAddBoundaryInList(GL2PSprimitive *prim, GL2PSlist *list){
  GL2PSprimitive *b;
  GLshort i;
  GL2PSxyz c;

  c[0] = c[1] = c[2] = 0.0F;
  for(i = 0; i < prim->numverts; i++){
    c[0] += prim->verts[i].xyz[0];
    c[1] += prim->verts[i].xyz[1];
  }
  c[0] /= prim->numverts;
  c[1] /= prim->numverts;

  for(i = 0; i < prim->numverts; i++){
    if(prim->boundary & (GLint)pow(2., i)){
      b = (GL2PSprimitive*)gl2psMalloc(sizeof(GL2PSprimitive));
      b->type = GL2PS_LINE;
      b->dash = prim->dash;
      b->depth = prim->depth; /* FIXME: this is wrong */
      b->culled = prim->culled;
      b->width = prim->width;
      b->boundary = 0;
      b->numverts = 2;
      b->verts = (GL2PSvertex *)gl2psMalloc(2 * sizeof(GL2PSvertex));

#if 0 /* FIXME: need to work on boundary offset... */
      v[0] = c[0] - prim->verts[i].xyz[0];
      v[1] = c[1] - prim->verts[i].xyz[1];
      v[2] = 0.0F;
      norm = gl2psNorm(v);
      v[0] /= norm;
      v[1] /= norm;
      b->verts[0].xyz[0] = prim->verts[i].xyz[0] +0.1*v[0];
      b->verts[0].xyz[1] = prim->verts[i].xyz[1] +0.1*v[1];
      b->verts[0].xyz[2] = prim->verts[i].xyz[2];
      v[0] = c[0] - prim->verts[gl2psGetIndex(i, prim->numverts)].xyz[0];
      v[1] = c[1] - prim->verts[gl2psGetIndex(i, prim->numverts)].xyz[1];
      norm = gl2psNorm(v);
      v[0] /= norm;
      v[1] /= norm;
      b->verts[1].xyz[0] = prim->verts[gl2psGetIndex(i, prim->numverts)].xyz[0] +0.1*v[0];
      b->verts[1].xyz[1] = prim->verts[gl2psGetIndex(i, prim->numverts)].xyz[1] +0.1*v[1];
      b->verts[1].xyz[2] = prim->verts[gl2psGetIndex(i, prim->numverts)].xyz[2];
#else
      b->verts[0].xyz[0] = prim->verts[i].xyz[0];
      b->verts[0].xyz[1] = prim->verts[i].xyz[1];
      b->verts[0].xyz[2] = prim->verts[i].xyz[2];
      b->verts[1].xyz[0] = prim->verts[gl2psGetIndex(i, prim->numverts)].xyz[0];
      b->verts[1].xyz[1] = prim->verts[gl2psGetIndex(i, prim->numverts)].xyz[1];
      b->verts[1].xyz[2] = prim->verts[gl2psGetIndex(i, prim->numverts)].xyz[2];
#endif

      b->verts[0].rgba[0] = 0.0F;
      b->verts[0].rgba[1] = 0.0F;
      b->verts[0].rgba[2] = 0.0F;
      b->verts[0].rgba[3] = 0.0F;
      b->verts[1].rgba[0] = 0.0F;
      b->verts[1].rgba[1] = 0.0F;
      b->verts[1].rgba[2] = 0.0F;
      b->verts[1].rgba[3] = 0.0F;
      gl2psListAdd(list, &b);
    }
  }

}

void gl2psBuildPolygonBoundary(GL2PSbsptree *tree){
  GLint i, n;
  GL2PSprimitive *prim;

  if(!tree) return;
  gl2psBuildPolygonBoundary(tree->back);
  n = gl2psListNbr(tree->primitives);
  for(i = 0; i < n; i++){
    prim = *(GL2PSprimitive**)gl2psListPointer(tree->primitives, i);
    if(prim->boundary) gl2psAddBoundaryInList(prim, tree->primitives);
  }
  gl2psBuildPolygonBoundary(tree->front);
}

/********************************************************************* 
 *
 * Feedback buffer parser
 *
 *********************************************************************/

void gl2psAddPolyPrimitive(GLshort type, GLshort numverts, 
                           GL2PSvertex *verts, GLint offset, 
                           char dash, GLfloat width,
                           char boundary){
  GLshort i;
  GLfloat factor, units, area, dZ, dZdX, dZdY, maxdZ;
  GL2PSprimitive *prim;

  prim = (GL2PSprimitive *)gl2psMalloc(sizeof(GL2PSprimitive));
  prim->type = type;
  prim->numverts = numverts;
  prim->verts = (GL2PSvertex *)gl2psMalloc(numverts * sizeof(GL2PSvertex));
  memcpy(prim->verts, verts, numverts * sizeof(GL2PSvertex));
  prim->boundary = boundary;
  prim->dash = dash;
  prim->width = width;
  prim->culled = 0;

  if(gl2ps->options & GL2PS_SIMPLE_LINE_OFFSET){

    if(type == GL2PS_LINE){
      if(gl2ps->sort == GL2PS_SIMPLE_SORT){
        prim->verts[0].xyz[2] -= GL2PS_SIMPLE_OFFSET_LARGE;
        prim->verts[1].xyz[2] -= GL2PS_SIMPLE_OFFSET_LARGE;
      }
      else{
        prim->verts[0].xyz[2] -= GL2PS_SIMPLE_OFFSET;
        prim->verts[1].xyz[2] -= GL2PS_SIMPLE_OFFSET;
      }
    }

  }
  else if(offset && type == GL2PS_TRIANGLE){

    /* FIXME: this needs some more work... */

    if(gl2ps->sort == GL2PS_SIMPLE_SORT){    
      factor = gl2ps->offset[0];
      units = gl2ps->offset[1];
    }
    else{
      factor = gl2ps->offset[0] / 800.0F;
      units = gl2ps->offset[1] / 800.0F;
    }

    area = 
      (prim->verts[1].xyz[0] - prim->verts[0].xyz[0]) * 
      (prim->verts[2].xyz[1] - prim->verts[1].xyz[1]) - 
      (prim->verts[2].xyz[0] - prim->verts[1].xyz[0]) * 
      (prim->verts[1].xyz[1] - prim->verts[0].xyz[1]);
    dZdX = 
      (prim->verts[2].xyz[1] - prim->verts[1].xyz[1]) *
      (prim->verts[1].xyz[2] - prim->verts[0].xyz[2]) -
      (prim->verts[1].xyz[1] - prim->verts[0].xyz[1]) *
      (prim->verts[2].xyz[2] - prim->verts[1].xyz[2]) / area;
    dZdY = 
      (prim->verts[1].xyz[0] - prim->verts[0].xyz[0]) *
      (prim->verts[2].xyz[2] - prim->verts[1].xyz[2]) -
      (prim->verts[2].xyz[0] - prim->verts[1].xyz[0]) *
      (prim->verts[1].xyz[2] - prim->verts[0].xyz[2]) / area;
    
    maxdZ = (GLfloat)sqrt(dZdX*dZdX + dZdY*dZdY);

    dZ = factor * maxdZ + units;

    prim->verts[0].xyz[2] += dZ;
    prim->verts[1].xyz[2] += dZ;
    prim->verts[2].xyz[2] += dZ;
  }

  prim->depth = 0.;
  if(gl2ps->sort == GL2PS_SIMPLE_SORT){
    for(i = 0; i < numverts; i++){
      prim->depth += prim->verts[i].xyz[2]; 
    }
    prim->depth /= (GLfloat)numverts;
  }
  
  gl2psListAdd(gl2ps->primitives, &prim);
}

GLint gl2psGetVertex(GL2PSvertex *v, GLfloat *p){
  GLint i;

  v->xyz[0] = p[0];
  v->xyz[1] = p[1];
  v->xyz[2] = GL2PS_DEPTH_FACT * p[2];

  if(gl2ps->colormode == GL_COLOR_INDEX && gl2ps->colorsize > 0){
    i = (GLint)(p[3] + 0.5);
    v->rgba[0] = gl2ps->colormap[i][0];
    v->rgba[1] = gl2ps->colormap[i][1];
    v->rgba[2] = gl2ps->colormap[i][2];
    v->rgba[3] = gl2ps->colormap[i][3];
    return 4;
  }
  else{
    v->rgba[0] = p[3];
    v->rgba[1] = p[4];
    v->rgba[2] = p[5];
    v->rgba[3] = p[6];
    return 7;
  }
}

void gl2psParseFeedbackBuffer(GLint used){
  char flag, dash = 0;
  GLshort boundary;
  GLint i, count, v, vtot, offset = 0;
  GLfloat lwidth = 1.0F, psize = 1.0F;
  GLfloat *current;
  GL2PSvertex vertices[3];

  current = gl2ps->feedback;
  boundary = gl2ps->boundary = 0;

  while(used > 0){

    if(boundary) gl2ps->boundary = 1;
    
    switch((GLint)*current){
    case GL_POINT_TOKEN :
      current ++;
      used --;
      i = gl2psGetVertex(&vertices[0], current);
      current += i;
      used    -= i;
      gl2psAddPolyPrimitive(GL2PS_POINT, 1, vertices, 0, dash, psize, 0);
      break;
    case GL_LINE_TOKEN :
    case GL_LINE_RESET_TOKEN :
      current ++;
      used --;
      i = gl2psGetVertex(&vertices[0], current);
      current += i;
      used    -= i;
      i = gl2psGetVertex(&vertices[1], current);
      current += i;
      used    -= i;
      gl2psAddPolyPrimitive(GL2PS_LINE, 2, vertices, 0, dash, lwidth, 0);
      break;
    case GL_POLYGON_TOKEN :
      count = (GLint)current[1];
      current += 2;
      used -= 2;
      v = vtot = 0;
      while(count > 0 && used > 0){
        i = gl2psGetVertex(&vertices[v], current);
        current += i;
        used    -= i;
        count --;
        vtot++;
        if(v == 2){
          if(boundary){
            if(!count && vtot == 2) flag = 1|2|4;
            else if(!count) flag = 2|4;
            else if(vtot == 2) flag = 1|2;
            else flag = 2;
          }
          else
            flag = 0;
          gl2psAddPolyPrimitive(GL2PS_TRIANGLE, 3, vertices, 
                                offset, dash, 1, flag);
          vertices[1] = vertices[2];
        }
        else
          v ++;
      }
      break;      
    case GL_BITMAP_TOKEN :
    case GL_DRAW_PIXEL_TOKEN :
    case GL_COPY_PIXEL_TOKEN :
      current ++;
      used --;
      i = gl2psGetVertex(&vertices[0], current);
      current += i;
      used    -= i;
      break;      
    case GL_PASS_THROUGH_TOKEN :
      switch((GLint)current[1]){
      case GL2PS_BEGIN_POLYGON_OFFSET_FILL : offset = 1; break;
      case GL2PS_END_POLYGON_OFFSET_FILL : offset = 0; break;
      case GL2PS_BEGIN_POLYGON_BOUNDARY : boundary = 1; break;
      case GL2PS_END_POLYGON_BOUNDARY : boundary = 0; break;
      case GL2PS_BEGIN_LINE_STIPPLE : dash = 4; break;
      case GL2PS_END_LINE_STIPPLE : dash = 0; break;
      case GL2PS_SET_POINT_SIZE : 
        current += 2; 
        used -= 2; 
        psize = current[1];
        break;
      case GL2PS_SET_LINE_WIDTH : 
        current += 2; 
        used -= 2; 
        lwidth = current[1];
        break;
      }
      current += 2; 
      used -= 2; 
      break;      
    default :
      gl2psMsg(GL2PS_WARNING, "Unknown token in buffer");
      current ++;
      used --;
      break;
    }
  }
}

/********************************************************************* 
 *
 * PostScript routines
 *
 *********************************************************************/

void gl2psGetRGB(GLfloat *pixels, GLsizei width, GLsizei height, GLuint x, GLuint y,
                 GLfloat *red, GLfloat *green, GLfloat *blue){
  /* OpenGL image is from down to up, PS image is up to down */
  GLfloat *pimag;
  pimag = pixels + 3 * (width * (height - 1 - y) + x);
  *red   = *pimag; pimag++;
  *green = *pimag; pimag++;
  *blue  = *pimag; pimag++;
}

void gl2psWriteByte(unsigned char byte){
  unsigned char h = byte / 16;
  unsigned char l = byte % 16;
  gl2psPrintf("%x%x", h, l);
}

void gl2psPrintPostScriptPixmap(GLfloat x, GLfloat y, GLsizei width, GLsizei height,
                                GLenum format, GLenum type, GLfloat *pixels){
  int nbhex, nbyte2, nbyte4, nbyte8;
  GLsizei row, col, col_max;
  GLfloat dr, dg, db;
  unsigned char red, green, blue, b, grey;

  /* FIXME: define an option for these? */
  int greyscale = 0; /* set to 1 to output greyscale image */
  int nbits = 8; /* number of bits per color compoment (2, 4 or 8) */

  if((width <= 0) || (height <= 0)) return;

  gl2psPrintf("gsave\n");
  gl2psPrintf("%.2f %.2f translate\n", x, y); 
  gl2psPrintf("%d %d scale\n", (int)width, (int)height); 

  if(greyscale){ /* greyscale, 8 bits per pixel */
    gl2psPrintf("/picstr %d string def\n", (int)width); 
    gl2psPrintf("%d %d %d\n", (int)width, (int)height, 8); 
    gl2psPrintf("[ %d 0 0 -%d 0 %d ]\n", (int)width, (int)height, (int)height); 
    gl2psPrintf("{ currentfile picstr readhexstring pop }\n");
    gl2psPrintf("image\n");
    for(row = 0; row < height; row++){
      for(col = 0; col < width; col++){ 
        gl2psGetRGB(pixels, width, height, col, row, &dr, &dg, &db);
        grey = (unsigned char)(255.0 * (0.30 * dr + 0.59 * dg + 0.11 * db));
        gl2psWriteByte(grey);
      }
      gl2psPrintf("\n");
    }
    nbhex = width * height * 2; 
    gl2psPrintf("%%%% nbhex digit          :%d\n", nbhex); 
  }
  else if(nbits == 2){ /* color, 2 bits for r and g and b; rgbs following each other */
    nbyte2 = (width * 3)/4;
    nbyte2 /=3;
    nbyte2 *=3;
    col_max = (nbyte2 * 4)/3;
    gl2psPrintf("/rgbstr %d string def\n", nbyte2); 
    gl2psPrintf("%d %d %d\n", (int)col_max, (int)height, 2); 
    gl2psPrintf("[ %d 0 0 -%d 0 %d ]\n", (int)col_max, (int)height, (int)height); 
    gl2psPrintf("{ currentfile rgbstr readhexstring pop }\n" );
    gl2psPrintf("false 3\n" );
    gl2psPrintf("colorimage\n" );
    for(row = 0; row < height; row++){
      for(col = 0; col < col_max; col+=4){
        gl2psGetRGB(pixels, width, height, col, row, &dr, &dg, &db);
        red = (unsigned char)(3.0 * dr);
        green = (unsigned char)(3.0 * dg);
        blue = (unsigned char)(3.0 * db);
        b = red;
        b = (b<<2)+green;
        b = (b<<2)+blue;
        gl2psGetRGB(pixels, width, height, col+1, row, &dr, &dg, &db);
        red = (unsigned char)(3.0 * dr);
        green = (unsigned char)(3.0 * dg);
        blue = (unsigned char)(3.0 * db);
        b = (b<<2)+red;
        gl2psWriteByte(b);
        b = green;
        b = (b<<2)+blue;
        gl2psGetRGB(pixels, width, height, col+2, row, &dr, &dg, &db);
        red = (unsigned char)(3.0 * dr);
        green = (unsigned char)(3.0 * dg);
        blue = (unsigned char)(3.0 * db);
        b = (b<<2)+red;
        b = (b<<2)+green;
        gl2psWriteByte(b);
        b = blue;
        gl2psGetRGB(pixels, width, height, col+3, row, &dr, &dg, &db);
        red = (unsigned char)(3.0 * dr);
        green = (unsigned char)(3.0 * dg);
        blue = (unsigned char)(3.0 * db);
        b = (b<<2)+red;
        b = (b<<2)+green;
        b = (b<<2)+blue;
        gl2psWriteByte(b);
      }
      gl2psPrintf("\n");
    }
  }
  else if(nbits == 4){ /* color, 4 bits for r and g and b; rgbs following each other */
    nbyte4 = (width  * 3)/2;
    nbyte4 /=3;
    nbyte4 *=3;
    col_max = (nbyte4 * 2)/3;
    gl2psPrintf("/rgbstr %d string def\n", nbyte4);
    gl2psPrintf("%d %d %d\n", (int)col_max, (int)height, 4);
    gl2psPrintf("[ %d 0 0 -%d 0 %d ]\n", (int)col_max, (int)height, (int)height);
    gl2psPrintf("{ currentfile rgbstr readhexstring pop }\n");
    gl2psPrintf("false 3\n");
    gl2psPrintf("colorimage\n");
    for(row = 0; row < height; row++){
      for(col = 0; col < col_max; col+=2){
        gl2psGetRGB(pixels, width, height, col, row, &dr, &dg, &db);
        red = (unsigned char)(15. * dr);
        green = (unsigned char)(15. * dg);
        gl2psPrintf("%x%x", red, green);
        blue = (unsigned char)(15. * db);
        gl2psGetRGB(pixels, width, height, col+1, row, &dr, &dg, &db);
        red = (unsigned char)(15. * dr);
        gl2psPrintf("%x%x",blue,red);
        green = (unsigned char)(15. * dg);
        blue = (unsigned char)(15. * db);
        gl2psPrintf("%x%x", green, blue);
      }
      gl2psPrintf("\n");
    }
  }
  else{ /* color, 8 bits for r and g and b; rgbs following each other */
    nbyte8 = width * 3;
    gl2psPrintf("/rgbstr %d string def\n", nbyte8);
    gl2psPrintf("%d %d %d\n", (int)width, (int)height, 8);
    gl2psPrintf("[ %d 0 0 -%d 0 %d ]\n", (int)width, (int)height, (int)height); 
    gl2psPrintf("{ currentfile rgbstr readhexstring pop }\n");
    gl2psPrintf("false 3\n");
    gl2psPrintf("colorimage\n");
    for(row = 0; row < height; row++){
      for(col = 0; col < width; col++){
        gl2psGetRGB(pixels, width, height, col, row, &dr, &dg, &db);
        red = (unsigned char)(255.0 * dr);
        gl2psWriteByte(red);
        green = (unsigned char)(255.0 * dg);
        gl2psWriteByte(green);
        blue = (unsigned char)(255.0 * db);
        gl2psWriteByte(blue);
      }
      gl2psPrintf("\n");
    }
  }

  gl2psPrintf("grestore\n");
}

void gl2psPrintPostScriptHeader(void){
  GLint index;
  GLfloat rgba[4];
  time_t now;

#ifdef GL2PS_HAVE_ZLIB
  char tmp[10] = {(char)0x1f,(char)0x8b /* magic numbers */,
                  8 /* compression method */, 0 /* flags */, 
                  0,0,0,0 /* time */, 2 /* xflags: max compression */,
                  (char)0x03 /* FIXME: OS code */};

  if(gl2ps->options & GL2PS_COMPRESS){
    gl2psSetupCompress();

    /* add the gzip file header */
    fwrite(tmp, 10, 1, gl2ps->stream);
  }
#endif  

  time(&now);

  if(gl2ps->format == GL2PS_PS){
    gl2psPrintf("%%!PS-Adobe-3.0\n");
  }
  else{
    gl2psPrintf("%%!PS-Adobe-3.0 EPSF-3.0\n");
  }

  gl2psPrintf("%%%%Title: %s\n"
              "%%%%Creator: GL2PS %d.%d.%d, (C) 1999-2003 Christophe Geuzaine <geuz@geuz.org>\n"
              "%%%%For: %s\n"
              "%%%%CreationDate: %s"
              "%%%%LanguageLevel: 3\n"
              "%%%%DocumentData: Clean7Bit\n"
              "%%%%Pages: 1\n",
              gl2ps->title, GL2PS_MAJOR_VERSION, GL2PS_MINOR_VERSION, GL2PS_PATCH_VERSION,
              gl2ps->producer, ctime(&now));

  if(gl2ps->format == GL2PS_PS){
    gl2psPrintf("%%%%Orientation: %s\n"
                "%%%%DocumentMedia: Default %d %d 0 () ()\n",
                (gl2ps->options & GL2PS_LANDSCAPE) ? "Landscape" : "Portrait",
                (gl2ps->options & GL2PS_LANDSCAPE) ? (int)gl2ps->viewport[3] :
                (int)gl2ps->viewport[2], 
                (gl2ps->options & GL2PS_LANDSCAPE) ? (int)gl2ps->viewport[2] : 
                (int)gl2ps->viewport[3]);
  }

  gl2psPrintf("%%%%BoundingBox: %d %d %d %d\n"
              "%%%%EndComments\n",
              (gl2ps->options & GL2PS_LANDSCAPE) ? (int)gl2ps->viewport[1] : 
              (int)gl2ps->viewport[0],
              (gl2ps->options & GL2PS_LANDSCAPE) ? (int)gl2ps->viewport[0] :
              (int)gl2ps->viewport[1],
              (gl2ps->options & GL2PS_LANDSCAPE) ? (int)gl2ps->viewport[3] : 
              (int)gl2ps->viewport[2],
              (gl2ps->options & GL2PS_LANDSCAPE) ? (int)gl2ps->viewport[2] :
              (int)gl2ps->viewport[3]);

  /* RGB color: r g b C (replace C by G in output to change from rgb to gray)
     Grayscale: r g b G
     Font choose: size fontname FC
     String primitive: (string) x y size fontname S
     Point primitive: x y size P
     Line width: width W
     Flat-shaded line: x2 y2 x1 y1 L
     Smooth-shaded line: x2 y2 r2 g2 b2 x1 y1 r1 g1 b1 SL
     Flat-shaded triangle: x3 y3 x2 y2 x1 y1 T
     Smooth-shaded triangle: x3 y3 r3 g3 b3 x2 y2 r2 g2 b2 x1 y1 r1 g1 b1 ST */

  gl2psPrintf("%%%%BeginProlog\n"
              "/gl2psdict 64 dict def gl2psdict begin\n"
              "0 setlinecap 0 setlinejoin\n"
              "/tryPS3shading %s def %% set to false to force subdivision\n"
              "/rThreshold %g def %% red component subdivision threshold\n"
              "/gThreshold %g def %% green component subdivision threshold\n"
              "/bThreshold %g def %% blue component subdivision threshold\n"
              "/BD { bind def } bind def\n"
              "/C  { setrgbcolor } BD\n"
              "/G  { 0.082 mul exch 0.6094 mul add exch 0.3086 mul add neg 1.0 add setgray } BD\n"
              "/W  { setlinewidth } BD\n"
              "/FC { findfont exch scalefont setfont } BD\n"
              "/S  { FC moveto show } BD\n"
              "/P  { newpath 0.0 360.0 arc closepath fill } BD\n"
              "/L  { newpath moveto lineto stroke } BD\n"
              "/SL { C moveto C lineto stroke } BD\n"
              "/T  { newpath moveto lineto lineto closepath fill } BD\n",
              (gl2ps->options & GL2PS_NO_PS3_SHADING) ? "false" : "true",
              gl2ps->threshold[0], gl2ps->threshold[1], gl2ps->threshold[2]);
  
  /* Smooth-shaded triangle with PostScript level 3 shfill operator:
        x3 y3 r3 g3 b3 x2 y2 r2 g2 b2 x1 y1 r1 g1 b1 STshfill */

  gl2psPrintf("/STshfill {\n"
              "      /b1 exch def /g1 exch def /r1 exch def /y1 exch def /x1 exch def\n"
              "      /b2 exch def /g2 exch def /r2 exch def /y2 exch def /x2 exch def\n"
              "      /b3 exch def /g3 exch def /r3 exch def /y3 exch def /x3 exch def\n"
              "      gsave << /ShadingType 4 /ColorSpace [/DeviceRGB]\n"
              "      /DataSource [ 0 x1 y1 r1 g1 b1 0 x2 y2 r2 g2 b2 0 x3 y3 r3 g3 b3 ] >>\n"
              "      shfill grestore } BD\n");

  /* Flat-shaded triangle with middle color:
        x3 y3 r3 g3 b3 x2 y2 r2 g2 b2 x1 y1 r1 g1 b1 Tm */

  gl2psPrintf(/* stack : x3 y3 r3 g3 b3 x2 y2 r2 g2 b2 x1 y1 r1 g1 b1 */
              "/Tm { 3 -1 roll 8 -1 roll 13 -1 roll add add 3 div\n" /* r = (r1+r2+r3)/3 */
              /* stack : x3 y3 g3 b3 x2 y2 g2 b2 x1 y1 g1 b1 r */
              "      3 -1 roll 7 -1 roll 11 -1 roll add add 3 div\n" /* g = (g1+g2+g3)/3 */
              /* stack : x3 y3 b3 x2 y2 b2 x1 y1 b1 r g b */
              "      3 -1 roll 6 -1 roll 9 -1 roll add add 3 div" /* b = (b1+b2+b3)/3 */
              /* stack : x3 y3 x2 y2 x1 y1 r g b */
              " C T } BD\n");

  /* Split triangle in four sub-triangles (at sides middle points) and call the
     STnoshfill procedure on each, interpolating the colors in RGB space:
        x3 y3 r3 g3 b3 x2 y2 r2 g2 b2 x1 y1 r1 g1 b1 STsplit
     (in procedure comments key: (Vi) = xi yi ri gi bi) */

  gl2psPrintf("/STsplit {\n"
              "      4 index 15 index add 0.5 mul\n" /* x13 = (x1+x3)/2 */
              "      4 index 15 index add 0.5 mul\n" /* y13 = (y1+y3)/2 */
              "      4 index 15 index add 0.5 mul\n" /* r13 = (r1+r3)/2 */
              "      4 index 15 index add 0.5 mul\n" /* g13 = (g1+g3)/2 */
              "      4 index 15 index add 0.5 mul\n" /* b13 = (b1+b3)/2 */
              "      5 copy 5 copy 25 15 roll\n"
              /* stack : (V3) (V13) (V13) (V13) (V2) (V1) */
              "      9 index 30 index add 0.5 mul\n" /* x23 = (x2+x3)/2 */
              "      9 index 30 index add 0.5 mul\n" /* y23 = (y2+y3)/2 */
              "      9 index 30 index add 0.5 mul\n" /* r23 = (r2+r3)/2 */
              "      9 index 30 index add 0.5 mul\n" /* g23 = (g2+g3)/2 */
              "      9 index 30 index add 0.5 mul\n" /* b23 = (b2+b3)/2 */
              "      5 copy 5 copy 35 5 roll 25 5 roll 15 5 roll\n"
              /* stack : (V3) (V13) (V23) (V13) (V23) (V13) (V23) (V2) (V1) */
              "      4 index 10 index add 0.5 mul\n" /* x12 = (x1+x2)/2 */
              "      4 index 10 index add 0.5 mul\n" /* y12 = (y1+y2)/2 */
              "      4 index 10 index add 0.5 mul\n" /* r12 = (r1+r2)/2 */
              "      4 index 10 index add 0.5 mul\n" /* g12 = (g1+g2)/2 */
              "      4 index 10 index add 0.5 mul\n" /* b12 = (b1+b2)/2 */
              "      5 copy 5 copy 40 5 roll 25 5 roll 15 5 roll 25 5 roll\n"
              /* stack : (V3) (V13) (V23) (V13) (V12) (V23) (V13) (V1) (V12) (V23) (V12) (V2) */
              "      STnoshfill STnoshfill STnoshfill STnoshfill } BD\n");
  
  /* Gouraud shaded triangle using recursive subdivision until the difference
     between corner colors does not exceed the thresholds:
        x3 y3 r3 g3 b3 x2 y2 r2 g2 b2 x1 y1 r1 g1 b1 STnoshfill  */

  gl2psPrintf("/STnoshfill {\n"
              "      2 index 8 index sub abs rThreshold gt\n" /* |r1-r2|>rth */
              "      { STsplit }\n"
              "      { 1 index 7 index sub abs gThreshold gt\n" /* |g1-g2|>gth */
              "        { STsplit }\n"
              "        { dup 6 index sub abs bThreshold gt\n" /* |b1-b2|>bth */
              "          { STsplit }\n"
              "          { 2 index 13 index sub abs rThreshold gt\n" /* |r1-r3|>rht */
              "            { STsplit }\n"
              "            { 1 index 12 index sub abs gThreshold gt\n" /* |g1-g3|>gth */
              "              { STsplit }\n"
              "              { dup 11 index sub abs bThreshold gt\n" /* |b1-b3|>bth */
              "                { STsplit }\n"
              "                { 7 index 13 index sub abs rThreshold gt\n" /* |r2-r3|>rht */
              "                  { STsplit }\n"
              "                  { 6 index 12 index sub abs gThreshold gt\n" /* |g2-g3|>gth */
              "                    { STsplit }\n"
              "                    { 5 index 11 index sub abs bThreshold gt\n" /* |b2-b3|>bth */
              "                      { STsplit }\n"
              "                      { Tm }\n" /* all colors sufficiently similar */
              "                      ifelse }\n"
              "                    ifelse }\n"
              "                  ifelse }\n"
              "                ifelse }\n"
              "              ifelse }\n"
              "            ifelse }\n"
              "          ifelse }\n"
              "        ifelse }\n"
              "      ifelse } BD\n");
  
  gl2psPrintf("tryPS3shading\n"
              "{ /shfill where\n"
              "  { /ST { STshfill } BD }\n"
              "  { /ST { STnoshfill } BD }\n"
              "  ifelse }\n"
              "{ /ST { STnoshfill } BD }\n"
              "ifelse\n");

  gl2psPrintf("end\n"
              "%%%%EndProlog\n"
              "%%%%BeginSetup\n"
              "/DeviceRGB setcolorspace\n"
              "gl2psdict begin\n"
              "%%%%EndSetup\n"
              "%%%%Page: 1 1\n"
              "%%%%BeginPageSetup\n");
  
  if(gl2ps->options & GL2PS_LANDSCAPE){
    gl2psPrintf("%d 0 translate 90 rotate\n",
                (int)gl2ps->viewport[3]);
  }

  gl2psPrintf("%%%%EndPageSetup\n"
              "mark\n"
              "gsave\n"
              "1.0 1.0 scale\n");
          
  if(gl2ps->options & GL2PS_DRAW_BACKGROUND){
    if(gl2ps->colormode == GL_RGBA || gl2ps->colorsize == 0){
      glGetFloatv(GL_COLOR_CLEAR_VALUE, rgba);
    }
    else{
      glGetIntegerv(GL_INDEX_CLEAR_VALUE, &index);
      rgba[0] = gl2ps->colormap[index][0];
      rgba[1] = gl2ps->colormap[index][1];
      rgba[2] = gl2ps->colormap[index][2];
      rgba[3] = 0.0F;
    }
    gl2psPrintf("%g %g %g C\n"
                "newpath %d %d moveto %d %d lineto %d %d lineto %d %d lineto\n"
                "closepath fill\n",
                rgba[0], rgba[1], rgba[2], 
                (int)gl2ps->viewport[0], (int)gl2ps->viewport[1], (int)gl2ps->viewport[2], 
                (int)gl2ps->viewport[1], (int)gl2ps->viewport[2], (int)gl2ps->viewport[3], 
                (int)gl2ps->viewport[0], (int)gl2ps->viewport[3]);
  }
}

void gl2psPrintPostScriptColor(GL2PSrgba rgba){
  if(!gl2psSameColor(gl2ps->lastrgba, rgba)){
    gl2psSetLastColor(rgba);
    gl2psPrintf("%g %g %g C\n", rgba[0], rgba[1], rgba[2]);
  }
}

void gl2psResetPostScriptColor(void){
  gl2ps->lastrgba[0] = gl2ps->lastrgba[1] = gl2ps->lastrgba[2] = -1.;
}

void gl2psPrintPostScriptPrimitive(void *a, void *b){
  GL2PSprimitive *prim;

  prim = *(GL2PSprimitive**)a;

  if((gl2ps->options & GL2PS_OCCLUSION_CULL) && prim->culled) return;

  switch(prim->type){
  case GL2PS_PIXMAP :
    gl2psPrintPostScriptPixmap(prim->verts[0].xyz[0], prim->verts[0].xyz[1],
                               prim->image->width, prim->image->height,
                               prim->image->format, prim->image->type,
                               prim->image->pixels);
    break;
  case GL2PS_TEXT :
    gl2psPrintPostScriptColor(prim->verts[0].rgba);
    gl2psPrintf("(%s) %g %g %d /%s S\n",
                prim->text->str, prim->verts[0].xyz[0], prim->verts[0].xyz[1],
                prim->text->fontsize, prim->text->fontname);
    break;
  case GL2PS_POINT :
    gl2psPrintPostScriptColor(prim->verts[0].rgba);
    gl2psPrintf("%g %g %g P\n", 
                prim->verts[0].xyz[0], prim->verts[0].xyz[1], 0.5*prim->width);
    break;
  case GL2PS_LINE :
    if(gl2ps->lastlinewidth != prim->width){
      gl2ps->lastlinewidth = prim->width;
      gl2psPrintf("%g W\n", gl2ps->lastlinewidth);
    }
    if(prim->dash){
      gl2psPrintf("[%d] 0 setdash\n", prim->dash);
    }
    if(!gl2psVertsSameColor(prim)){
      gl2psResetPostScriptColor();
      gl2psPrintf("%g %g %g %g %g %g %g %g %g %g SL\n",
                  prim->verts[1].xyz[0], prim->verts[1].xyz[1],
                  prim->verts[1].rgba[0], prim->verts[1].rgba[1],
                  prim->verts[1].rgba[2], prim->verts[0].xyz[0],
                  prim->verts[0].xyz[1], prim->verts[0].rgba[0],
                  prim->verts[0].rgba[1], prim->verts[0].rgba[2]);
    }
    else{
      gl2psPrintPostScriptColor(prim->verts[0].rgba);
      gl2psPrintf("%g %g %g %g L\n",
                  prim->verts[1].xyz[0], prim->verts[1].xyz[1],
                  prim->verts[0].xyz[0], prim->verts[0].xyz[1]);
    }
    if(prim->dash){
      gl2psPrintf("[] 0 setdash\n");
    }
    break;
  case GL2PS_TRIANGLE :
    if(!gl2psVertsSameColor(prim)){
      gl2psResetPostScriptColor();
      gl2psPrintf("%g %g %g %g %g %g %g %g %g %g %g %g %g %g %g ST\n",
                  prim->verts[2].xyz[0], prim->verts[2].xyz[1],
                  prim->verts[2].rgba[0], prim->verts[2].rgba[1],
                  prim->verts[2].rgba[2], prim->verts[1].xyz[0],
                  prim->verts[1].xyz[1], prim->verts[1].rgba[0],
                  prim->verts[1].rgba[1], prim->verts[1].rgba[2],
                  prim->verts[0].xyz[0], prim->verts[0].xyz[1],
                  prim->verts[0].rgba[0], prim->verts[0].rgba[1],
                  prim->verts[0].rgba[2]);
    }
    else{
      gl2psPrintPostScriptColor(prim->verts[0].rgba);
      gl2psPrintf("%g %g %g %g %g %g T\n",
                  prim->verts[2].xyz[0], prim->verts[2].xyz[1],
                  prim->verts[1].xyz[0], prim->verts[1].xyz[1],
                  prim->verts[0].xyz[0], prim->verts[0].xyz[1]);
    }
    break;
  case GL2PS_QUADRANGLE :
    gl2psMsg(GL2PS_WARNING, "There should not be any quad left to print");
    break;
  default :
    gl2psMsg(GL2PS_ERROR, "Unknown type of primitive to print");
    break;
  }
}

void gl2psPrintPostScriptFooter(void){
#ifdef GL2PS_HAVE_ZLIB
  int n;
  uLong crc, len;
  char tmp[8];
#endif
  
  gl2psPrintf("grestore\n"
              "showpage\n"
              "cleartomark\n"
              "%%%%PageTrailer\n"
              "%%%%Trailer\n"
              "end\n"
              "%%%%EOF\n");
  
#ifdef GL2PS_HAVE_ZLIB
  if(gl2ps->options & GL2PS_COMPRESS){
    if(Z_OK != gl2psDeflate()){
      gl2psMsg(GL2PS_ERROR, "Zlib deflate error");
    }
    else{
      /* determine the length of the header in the zlib stream */
      n = 2; /* CMF+FLG */
      if(gl2ps->compress->dest[1] & (1<<5)){
        n += 4; /* DICTID */
      }
      /* write the data, without the zlib header and footer */
      fwrite(gl2ps->compress->dest+n, gl2ps->compress->destLen-(n+4), 
             1, gl2ps->stream);
      /* add the gzip file footer */
      crc = crc32(0L, gl2ps->compress->start, gl2ps->compress->srcLen);
      for(n = 0; n < 4; ++n) {
        tmp[n] = (char)(crc & 0xff);
        crc >>= 8;
      }
      len = gl2ps->compress->srcLen;
      for(n = 4; n < 8; ++n) {
        tmp[n] = (char)(len & 0xff);
        len >>= 8;
      }
      fwrite(tmp, 8, 1, gl2ps->stream);
    }
    gl2psFreeCompress();
    gl2psFree(gl2ps->compress);
    gl2ps->compress = NULL;
  }
#endif 
}

void gl2psPrintPostScriptBeginViewport(GLint viewport[4]){
  GLint index;
  GLfloat rgba[4];
  int x = viewport[0], y = viewport[1], w = viewport[2], h = viewport[3];

  glRenderMode(GL_FEEDBACK);

  gl2psPrintf("gsave\n"
              "1.0 1.0 scale\n");
          
  if(gl2ps->options & GL2PS_DRAW_BACKGROUND){
    if(gl2ps->colormode == GL_RGBA || gl2ps->colorsize == 0){
      glGetFloatv(GL_COLOR_CLEAR_VALUE, rgba);
    }
    else{
      glGetIntegerv(GL_INDEX_CLEAR_VALUE, &index);
      rgba[0] = gl2ps->colormap[index][0];
      rgba[1] = gl2ps->colormap[index][1];
      rgba[2] = gl2ps->colormap[index][2];
      rgba[3] = 0.0F;
    }
    gl2psPrintf("%g %g %g C\n"
                "newpath %d %d moveto %d %d lineto %d %d lineto %d %d lineto\n"
                "closepath fill\n",
                rgba[0], rgba[1], rgba[2], 
                x, y, x+w, y, x+w, y+h, x, y+h);
    gl2psPrintf("newpath %d %d moveto %d %d lineto %d %d lineto %d %d lineto\n"
                "closepath clip\n",
                x, y, x+w, y, x+w, y+h, x, y+h);
  }

}

GLint gl2psPrintPostScriptEndViewport(void){
  GLint res;
  GLint gl2psPrintPrimitives(void);

  res = gl2psPrintPrimitives();
  gl2psPrintf("grestore\n");
  return res;
}

/********************************************************************* 
 *
 * LaTeX routines
 *
 *********************************************************************/

void gl2psPrintTeXHeader(void){
  char name[256];
  int i;

  if(gl2ps->filename && strlen(gl2ps->filename) < 256){
    for(i = strlen(gl2ps->filename)-1; i >= 0; i--){
      if(gl2ps->filename[i] == '.'){
        strncpy(name, gl2ps->filename, i);
        name[i] = '\0';
        break;
      }
    }
    if(i <= 0) strcpy(name, gl2ps->filename);
  }
  else{
    strcpy(name, "untitled");
  }

  fprintf(gl2ps->stream, 
          "\\setlength{\\unitlength}{1pt}\n"
          "\\begin{picture}(0,0)\n"
          "\\includegraphics{%s}\n"
          "\\end{picture}%%\n"
          "%s\\begin{picture}(%d,%d)(0,0)\n",
          name, (gl2ps->options & GL2PS_LANDSCAPE) ? "\\rotatebox{90}{" : "",
          (int)gl2ps->viewport[2], (int)gl2ps->viewport[3]);
}

void gl2psPrintTeXPrimitive(void *a, void *b){
  GL2PSprimitive *prim;

  prim = *(GL2PSprimitive**)a;

  switch(prim->type){
  case GL2PS_TEXT :
#if 0 /* old code: */
    fprintf(gl2ps->stream, "\\put(%g,%g){\\makebox(0,0)[lb]{%s}}\n",
            prim->verts[0].xyz[0], prim->verts[0].xyz[1], prim->text->str);
#endif
    fprintf(gl2ps->stream, "\\fontsize{%d}{0}\n\\selectfont", 
            prim->text->fontsize);
    fprintf(gl2ps->stream, "\\put(%g,%g){\\makebox(0,0)",
            prim->verts[0].xyz[0], prim->verts[0].xyz[1]);
    switch (prim->text->alignment) {
    case GL2PS_TEXT_CL:
      fprintf(gl2ps->stream, "[l]");
      break;
    case GL2PS_TEXT_CR:
      fprintf(gl2ps->stream, "[r]");
      break;
    case GL2PS_TEXT_B:
      fprintf(gl2ps->stream, "[b]");
      break;
    case GL2PS_TEXT_BL:
      fprintf(gl2ps->stream, "[bl]");
      break;
    case GL2PS_TEXT_BR:
      fprintf(gl2ps->stream, "[br]");
      break;
    case GL2PS_TEXT_T:
      fprintf(gl2ps->stream, "[t]");
      break;
    case GL2PS_TEXT_TL:
      fprintf(gl2ps->stream, "[tl]");
      break;
    case GL2PS_TEXT_TR:
      fprintf(gl2ps->stream, "[tr]");
      break;
    default:
      break;
    }
    fprintf(gl2ps->stream, "{\\textcolor[rgb]{%f,%f,%f}{",
            prim->verts[0].rgba[0], prim->verts[0].rgba[1], prim->verts[0].rgba[2]);
    fprintf(gl2ps->stream, "{%s}}}}\n", prim->text->str);
    break;
  default :
    break;
  }
}

void gl2psPrintTeXFooter(void){
  fprintf(gl2ps->stream, "\\end{picture}%s\n",
          (gl2ps->options & GL2PS_LANDSCAPE) ? "}" : "");
}

void gl2psPrintTeXBeginViewport(GLint viewport[4]){
}

GLint gl2psPrintTeXEndViewport(void){
  return GL2PS_SUCCESS;
}

/********************************************************************* 
 *
 * PDF routines
 *
 *********************************************************************/

int gl2psPrintPDFCompressorType(){
#ifdef GL2PS_HAVE_ZLIB
  if(gl2ps->options & GL2PS_COMPRESS){
    return fprintf(gl2ps->stream, "/Filter [/FlateDecode]\n");
  }
#endif
  return 0;
}

int gl2psPrintPDFStrokeColor(GL2PSrgba rgba){
  int offs = 0;
  int i;

  gl2psSetLastColor(rgba);
  for(i = 0; i < 3; ++i){
    if(GL2PS_ZERO(rgba[i]))
      offs += gl2psPrintf("%.0f ", 0.);
    else if(rgba[i] < 1e-4 || rgba[i] > 1e6) /* avoid %e formatting */
      offs += gl2psPrintf("%f ", rgba[i]);
    else
      offs += gl2psPrintf("%g ", rgba[i]);
  }
  offs += gl2psPrintf("RG\n");
  return offs;
}

int gl2psPrintPDFFillColor(GL2PSrgba rgba){
  int offs = 0;
  int i;
  
  for(i = 0; i < 3; ++i){
    if(GL2PS_ZERO(rgba[i]))
      offs += gl2psPrintf("%.0f ", 0.);
    else if(rgba[i] < 1e-4 || rgba[i] > 1e6) /* avoid %e formatting */
      offs += gl2psPrintf("%f ", rgba[i]);
    else
      offs += gl2psPrintf("%g ", rgba[i]);
  }
  offs += gl2psPrintf("rg\n");
  return offs;
}

int gl2psPrintPDFLineWidth(GLfloat lw){
  if(GL2PS_ZERO(lw))
    return gl2psPrintf("%.0f w\n", 0.);
  else if(lw < 1e-4 || lw > 1e6) /* avoid %e formatting */
    return gl2psPrintf("%f w\n", lw);
  else
    return gl2psPrintf("%g w\n", lw);
}

/* Print 1st PDF object - file info */

int gl2psPrintPDFInfo(){
  int offs;
  time_t now;
  struct tm *newtime;
  
  time(&now);
  newtime = gmtime(&now);
  
  offs = fprintf(gl2ps->stream,
                 "1 0 obj\n"
                 "<<\n"
                 "/Title (%s)\n"
                 "/Creator (%s)\n"
                 "/Producer (GL2PS %d.%d.%d, (C) 1999-2003 Christophe Geuzaine <geuz@geuz.org>)\n",
                 gl2ps->title, gl2ps->producer,
                 GL2PS_MAJOR_VERSION, GL2PS_MINOR_VERSION, GL2PS_PATCH_VERSION);
  
  if(!newtime){
    offs += fprintf(gl2ps->stream, 
                    ">>\n"
                    "endobj\n");
    return offs;
  }
  
  offs += fprintf(gl2ps->stream, 
                  "/CreationDate (D:%d%02d%02d%02d%02d%02d)\n"
                  ">>\n"
                  "endobj\n",
                  newtime->tm_year+1900, 
                  newtime->tm_mon+1, 
                  newtime->tm_mday,
                  newtime->tm_hour,
                  newtime->tm_min,
                  newtime->tm_sec);
  return offs;
}

/* Create catalog and page structure - 2nd and 3th PDF object */

int gl2psPrintPDFCatalog(){
  return fprintf(gl2ps->stream, 
                 "2 0 obj\n"
                 "<<\n"
                 "/Type /Catalog\n"
                 "/Pages 3 0 R\n"
                 ">>\n"
                 "endobj\n");
}

int gl2psPrintPDFPages(){
  return fprintf(gl2ps->stream, 
                 "3 0 obj\n"
                 "<<\n" 
                 "/Type /Pages\n"
                 "/Kids [6 0 R]\n"
                 "/Count 1\n"
                 ">>\n"
                 "endobj\n");
}

/* Open stream for data - graphical objects, fonts etc. PDF object 4*/

int gl2psOpenPDFDataStream(){
  int offs = 0;
  
  offs += fprintf(gl2ps->stream, 
                  "4 0 obj\n"
                  "<<\n" 
                  "/Length 5 0 R\n" );
  offs += gl2psPrintPDFCompressorType();
  offs += fprintf(gl2ps->stream, 
                  ">>\n"
                  "stream\n");
  return offs;
}

/* Stream setup - Graphics state, fill background if allowed */

int gl2psOpenPDFDataStreamWritePreface(){
  int offs;
  GLint index;
  GLfloat rgba[4];

  offs = gl2psPrintf("/GS1 gs\n");
  
  if(gl2ps->options & GL2PS_DRAW_BACKGROUND){
    if(gl2ps->colormode == GL_RGBA || gl2ps->colorsize == 0){
      glGetFloatv(GL_COLOR_CLEAR_VALUE, rgba);
    }
    else{
      glGetIntegerv(GL_INDEX_CLEAR_VALUE, &index);
      rgba[0] = gl2ps->colormap[index][0];
      rgba[1] = gl2ps->colormap[index][1];
      rgba[2] = gl2ps->colormap[index][2];
      rgba[3] = 0.0F;
    }
    offs += gl2psPrintPDFFillColor(rgba);
    offs += gl2psPrintf("%d %d %d %d re\n",
                    (int)gl2ps->viewport[0], (int)gl2ps->viewport[1],
                    (int)gl2ps->viewport[2], (int)gl2ps->viewport[3]);
    offs += gl2psPrintf("f\n");  
  }
  return offs;
}

/* Use the functions above to create the first part of the PDF*/

void gl2psPrintPDFHeader(){
  int offs;

#ifdef GL2PS_HAVE_ZLIB
  if(gl2ps->options & GL2PS_COMPRESS){
    gl2psSetupCompress();
  }
#endif  

  /* tlist, tidxlist, ilist and slist contain triangles, indexes for
     consecutive triangles, images and strings */
  gl2ps->tlist = gl2psListCreate(100, 100, sizeof(GL2PStriangle));
  gl2ps->tidxlist = gl2psListCreate(100, 100, sizeof(int));
  gl2ps->ilist = gl2psListCreate(100, 100, sizeof(GL2PSimage*));
  gl2ps->slist = gl2psListCreate(100, 100, sizeof(GL2PSstring*));
  
  gl2ps->lasttype = GL2PS_NOTYPE;
  gl2ps->consec_cnt = 0;
  gl2ps->consec_inner_cnt = 0;
  
  offs = fprintf(gl2ps->stream, "%%PDF-1.3\n");
  gl2ps->cref[0] = offs;
  
  offs += gl2psPrintPDFInfo();
  gl2ps->cref[1] = offs;
  
  offs += gl2psPrintPDFCatalog();
  gl2ps->cref[2] = offs;
  
  offs += gl2psPrintPDFPages();
  gl2ps->cref[3] = offs;
  
  offs += gl2psOpenPDFDataStream();
  gl2ps->cref[4] = offs; /* finished in gl2psPrintPDFFooter */
  gl2ps->streamlength = gl2psOpenPDFDataStreamWritePreface();
}

int gl2psFlushPDFTriangles(){
  int offs = 0;

  if(gl2ps->lasttype == GL2PS_TRIANGLE && !gl2ps->last_triangle_finished){
    gl2psListAdd(gl2ps->tidxlist, &gl2ps->consec_inner_cnt);
    offs = gl2psPrintf("/Sh%d sh\n", gl2ps->consec_cnt++);
    gl2ps->consec_inner_cnt = 0;
    gl2ps->streamlength += offs;
    gl2ps->last_triangle_finished = 1;
  }
  return offs;
}

int gl2psFlushPDFLines(){
  int offs = 0;

  if(gl2ps->lasttype == GL2PS_LINE && !gl2ps->last_line_finished){
    offs = gl2psPrintf("S\n");
    gl2ps->streamlength += offs;
    gl2ps->last_line_finished = 1;
  }
  return offs;
}

/* The central primitive drawing */

void gl2psPrintPDFPrimitive(void *a, void *b){
  GL2PSprimitive *prim;
  GL2PStriangle t;
  GL2PSimage* image;
  GL2PSstring* str;

  prim = *(GL2PSprimitive**)a;
  
  if((gl2ps->options & GL2PS_OCCLUSION_CULL) && prim->culled) return;
  
  if(prim->type != GL2PS_TRIANGLE)
    gl2psFlushPDFTriangles();
  if(prim->type != GL2PS_LINE)
    gl2psFlushPDFLines();
  
  switch(prim->type){
  case GL2PS_PIXMAP :
    image = gl2psCopyPixmap(prim->image);
    gl2psListAdd(gl2ps->ilist, &image);
    gl2ps->streamlength += gl2psPrintf("q\n"
                                       "%d 0 0 %d %f %f cm\n"
                                       "/Im%d Do\n"
                                       "Q\n",
                                       (int)prim->image->width, (int)prim->image->height,
                                       prim->verts[0].xyz[0], prim->verts[0].xyz[1],
                                       gl2psListNbr(gl2ps->ilist)-1);
    break;
  case GL2PS_TEXT :
    str = gl2psCopyText(prim->text);
    gl2psListAdd(gl2ps->slist, &str);
    gl2ps->streamlength += gl2psPrintPDFFillColor(prim->verts[0].rgba);
    gl2ps->streamlength += gl2psPrintf("BT\n"
                                       "/F%d %d Tf\n"
                                       "%f %f Td\n"
                                       "(%s) Tj\n"
                                       "ET\n",
                                       gl2psListNbr(gl2ps->slist)-1,
                                       prim->text->fontsize, prim->verts[0].xyz[0], 
                                       prim->verts[0].xyz[1], prim->text->str);
    break;
  case GL2PS_POINT :
    if(gl2ps->lastlinewidth != prim->width){
      gl2ps->lastlinewidth = prim->width;
      gl2ps->streamlength += gl2psPrintPDFLineWidth(gl2ps->lastlinewidth);
    }
    gl2ps->streamlength += gl2psPrintf("1 J\n");
    gl2ps->streamlength += gl2psPrintPDFStrokeColor(prim->verts[0].rgba);
    gl2ps->streamlength += gl2psPrintf("%f %f m %f %f l S\n",
                                   prim->verts[0].xyz[0], prim->verts[0].xyz[1],
                                   prim->verts[0].xyz[0], prim->verts[0].xyz[1]);
    gl2ps->streamlength += gl2psPrintf("0 J\n");
    break;
  case GL2PS_LINE :
    gl2ps->line_width_diff = gl2ps->lastlinewidth != prim->width;
    gl2ps->line_rgb_diff = !GL2PS_ZERO(gl2psColorDiff(prim->verts[0].rgba, gl2ps->lastrgba));
    
    if(gl2ps->line_width_diff || gl2ps->line_rgb_diff || prim->dash){
      gl2psFlushPDFLines();
    }
    if(gl2ps->line_width_diff){
      gl2ps->lastlinewidth = prim->width;
      gl2ps->streamlength += gl2psPrintPDFLineWidth(gl2ps->lastlinewidth);
    }
    if(gl2ps->line_rgb_diff){
      gl2ps->streamlength += gl2psPrintPDFStrokeColor(prim->verts[0].rgba);
    }
    if(prim->dash){
      gl2ps->streamlength += gl2psPrintf("[%d] 0 d\n", prim->dash);
    }
    gl2ps->streamlength += gl2psPrintf("%f %f m %f %f l \n",
                                       prim->verts[0].xyz[0], prim->verts[0].xyz[1],
                                       prim->verts[1].xyz[0], prim->verts[1].xyz[1]);
    gl2ps->last_line_finished = 0;
    
    if(prim->dash){
      gl2ps->streamlength += gl2psPrintf("S\n[] 0 d\n"); 
      gl2ps->last_line_finished = 1;
    }
    break;
  case GL2PS_TRIANGLE :
    t[0] = prim->verts[0];
    t[1] = prim->verts[1];
    t[2] = prim->verts[2];
    
    gl2psListAdd(gl2ps->tlist, t);
    ++gl2ps->consec_inner_cnt;
    gl2ps->last_triangle_finished = 0;
    break;
  case GL2PS_QUADRANGLE :
    gl2psMsg(GL2PS_WARNING, "There should not be any quad left to print");
    break;
  default :
    gl2psMsg(GL2PS_ERROR, "Unknown type of primitive to print");
    break;
  }
  gl2ps->lasttype = prim->type;
}

/* close stream and ... */

int gl2psClosePDFDataStream(){
  int offs = 0;
 
  offs += gl2psFlushPDFTriangles();
  offs += gl2psFlushPDFLines();

#ifdef GL2PS_HAVE_ZLIB
  if(gl2ps->options & GL2PS_COMPRESS){
    if(Z_OK != gl2psDeflate())
      gl2psMsg(GL2PS_ERROR, "Zlib deflate error");
    else
      fwrite(gl2ps->compress->dest, gl2ps->compress->destLen, 1, gl2ps->stream);
    gl2ps->streamlength += gl2ps->compress->destLen;
    
    offs += gl2ps->streamlength;
    gl2psFreeCompress();
  }
#endif 
  
  offs += fprintf(gl2ps->stream, 
                  "endstream\n"
                  "endobj\n");
  return offs;
}

/* ... write the now known length object */

int gl2psPrintPDFDataStreamLength(int val){
  return fprintf(gl2ps->stream,
                 "5 0 obj\n"
                 "%d\n"
                 "endobj\n", val);
}

/* Create named shader objects */

int gl2psPrintPDFShaderResources(int firstObject, int size){
  int offs = 0;
  int i;
  
  offs += fprintf(gl2ps->stream,
                  "/Shading\n"
                  "<<\n");
  for(i = 0; i < size; ++i){
    offs += fprintf(gl2ps->stream, "/Sh%d %d 0 R\n", i, firstObject+i);
  }
  offs += fprintf(gl2ps->stream, ">>\n");
  return offs;
}

/* Create named pixmap objects */

int gl2psPrintPDFPixmapResources(int firstObject, int size){
  int offs = 0;
  int i;
  
  offs += fprintf(gl2ps->stream,
                  "/XObject\n"
                  "<<\n");
  for(i = 0; i < size; ++i){
    offs += fprintf(gl2ps->stream, "/Im%d %d 0 R\n", i, firstObject + i);
  }
  offs += fprintf(gl2ps->stream, ">>\n");
  return offs;
}

/* Create named font objects */

int gl2psPrintPDFTextResources(int firstObject, int size){
  int offs = 0;
  int i;
  
  offs += fprintf(gl2ps->stream,
                  "/Font\n"
                  "<<\n");
  for(i = 0; i < size; ++i){
    offs += fprintf(gl2ps->stream, "/F%d %d 0 R\n", i, firstObject + i);
  }
  offs += fprintf(gl2ps->stream, ">>\n");
  return offs;
}

/* Put the info created before in PDF objects */

int gl2psPrintPDFSinglePage(){
  int offs;
        
  offs = fprintf(gl2ps->stream, 
                 "6 0 obj\n"
                 "<<\n" 
                 "/Type /Page\n"
                 "/Parent 3 0 R\n"
                 "/MediaBox [%d %d %d %d]\n",
                 (int)gl2ps->viewport[0], (int)gl2ps->viewport[1],
                 (int)gl2ps->viewport[2], (int)gl2ps->viewport[3]);
  
  if(gl2ps->options & GL2PS_LANDSCAPE)
    offs += fprintf(gl2ps->stream, "/Rotate -90\n");
  
  offs += fprintf(gl2ps->stream,
                  "/Contents 4 0 R\n"
                  "/Resources\n" 
                  "<<\n" 
                  "/ProcSet [/PDF /Text /ImageB /ImageC]  %%/ImageI\n"
                  "/ExtGState\n" 
                  "<<\n"
                  "/GS1 7 0 R\n"
                  ">>\n");
  
  offs += gl2psPrintPDFShaderResources(GL2PS_FIXED_XREF_ENTRIES + 1, 
                                       gl2psListNbr(gl2ps->tidxlist));
  offs += gl2psPrintPDFPixmapResources(GL2PS_FIXED_XREF_ENTRIES + 1
                                       + gl2psListNbr(gl2ps->tidxlist),
                                       gl2psListNbr(gl2ps->ilist));
  offs += gl2psPrintPDFTextResources(GL2PS_FIXED_XREF_ENTRIES + 1 
                                     + gl2psListNbr(gl2ps->tidxlist) 
                                     + gl2psListNbr(gl2ps->ilist), 
                                     gl2psListNbr(gl2ps->slist));
  offs += fprintf(gl2ps->stream,                        
                  ">>\n"
                  ">>\n"
                  "endobj\n");
  return offs;
}

/* Extended graphics state for shading */

int gl2psPrintPDFExtGState(){
  return fprintf(gl2ps->stream,
                 "7 0 obj\n"
                 "<<\n"
                 "/Type /ExtGState\n"
                 "/SA false\n"
                 "/SM 0.02\n"
                 "/OP false\n"
                 "/op false\n"
                 "/OPM 0\n"
                 "/BG2 /Default\n"
                 "/UCR2 /Default\n"
                 "/TR2 /Default\n"
                 ">>\n"
                 "endobj\n");
}

/* Put a triangles raw data in shader stream */

int gl2psPrintPDFShaderStreamData(GL2PStriangle triangle, 
                                  size_t (*action)(unsigned long data, size_t size)){
  int offs = 0;
  int i;
  unsigned long imap;
  GLfloat  diff, dx, dy;
  char edgeflag = 0;
  double dmax = ~1UL;
  
  dx = (GLfloat)(gl2ps->viewport[2] - gl2ps->viewport[0]);
  dy = (GLfloat)(gl2ps->viewport[3] - gl2ps->viewport[1]);
  
  for(i = 0; i < 3; ++i){
    offs += (*action)(edgeflag, 1);

    /* The Shader stream in PDF requires to be in a 'big-endian'
       order */
    
    if(fabs(dx*dy) < FLT_MIN){
      offs += (*action)(0, 4);
      offs += (*action)(0, 4);
    }
    else{
      diff = (triangle[i].xyz[0] - gl2ps->viewport[0]) / dx;
      if(diff > 1)
        diff = 1;
      else if(diff < 0)
        diff = 0;
      imap = (unsigned long)(diff * dmax);
      offs += (*action)(imap, 4);
      
      diff = (triangle[i].xyz[1] - gl2ps->viewport[1]) / dy;
      if(diff > 1)
        diff = 1;
      else if(diff < 0)
        diff = 0;
      imap = (unsigned long)(diff * dmax);
      offs += (*action)(imap, 4);
    }
    
    imap = (unsigned long)(triangle[i].rgba[0] * dmax);
    offs += (*action)(imap, 1);
    
    imap = (unsigned long)(triangle[i].rgba[1] * dmax);
    offs += (*action)(imap, 1);
    
    imap = (unsigned long)(triangle[i].rgba[2] * dmax);
    offs += (*action)(imap, 1);
  }
  return offs;
}


/* Writes shaded triangle */

int gl2psPrintPDFShader(int obj, GL2PSlist* triangles, int idx, int cnt ){
  int offs = 0;
  int vertexbytes = 1+4+4+1+1+1;
  int i, done = 0;
        
  offs += fprintf(gl2ps->stream,
                  "%d 0 obj\n"
                  "<< "
                  "/ShadingType 4 "
                  "/ColorSpace /DeviceRGB "
                  "/BitsPerCoordinate 32 "
                  "/BitsPerComponent 8 "
                  "/BitsPerFlag 8 "
                  "/Decode [%d %d %d %d 0 1 0 1 0 1] ",
                  obj,
                  (int)gl2ps->viewport[0], (int)gl2ps->viewport[2],
                  (int)gl2ps->viewport[1], (int)gl2ps->viewport[3]);
  
#ifdef GL2PS_HAVE_ZLIB
  if(gl2ps->options & GL2PS_COMPRESS){
    gl2psAllocCompress(vertexbytes * cnt * 3);

    for(i = 0; i < cnt; ++i)
      gl2psPrintPDFShaderStreamData((GL2PSvertex*)gl2psListPointer(triangles, idx+i),
                                    gl2psWriteBigEndianCompress);

    if(Z_OK == gl2psDeflate() && 23 + gl2ps->compress->destLen < gl2ps->compress->srcLen){
      offs += gl2psPrintPDFCompressorType();
      offs += fprintf(gl2ps->stream,
                      "/Length %d "
                      ">>\n"
                      "stream\n",
                      (int)gl2ps->compress->destLen);
      offs += gl2ps->compress->destLen * fwrite(gl2ps->compress->dest, gl2ps->compress->destLen, 
                                                1, gl2ps->stream);
      done = 1;
    }
    gl2psFreeCompress();
  }
#endif

  if(!done){
    /* no compression, or too long after compression, or compress error
       -> write non-compressed entry */
    offs += fprintf(gl2ps->stream,
                    "/Length %d "
                    ">>\n"
                    "stream\n",
                    vertexbytes * 3 * cnt);
    for(i = 0; i < cnt; ++i)
      offs += gl2psPrintPDFShaderStreamData((GL2PSvertex*)gl2psListPointer(triangles, idx+i),
                                            gl2psWriteBigEndian);
  }
  
  offs += fprintf(gl2ps->stream,
                  "\nendstream\n"
                  "endobj\n");

  return offs;
}

/* Writes all triangles and returns field of offsets for the PDF cross
   reference table */

int* gl2psPrintPDFShaderObjects(int firstObjnumber, int firstOffs){
  int size;
  int* offs;
  int i;
  int idx = 0;
  int tmp;
  
  size = gl2psListNbr(gl2ps->tidxlist);
  offs = (int*)gl2psMalloc(sizeof(int) * (size+1));
  
  offs[0] = firstOffs;
  
  for(i = 0; i < size; ++i){
    gl2psListRead(gl2ps->tidxlist, i, &tmp);
    firstOffs += gl2psPrintPDFShader(i+firstObjnumber, gl2ps->tlist, idx, tmp);
    offs[i+1] = firstOffs;
    idx += tmp;
  }
  return offs;
}

/* Similar groups of  functions for pixmaps and text */

int gl2psPrintPDFPixmapStreamData(GL2PSimage* im,
                                  size_t  (*action)(unsigned long data, size_t size)){
  int x, y;
  GLfloat r, g, b;
  
  for(y = 0; y < im->height; ++y)
    for(x = 0; x < im->width; ++x){
      gl2psGetRGB(im->pixels, im->width, im->height, x, y, &r, &g, &b);
                        (*action)((unsigned long)(r*255) << 24,1);
                        (*action)((unsigned long)(g*255) << 24,1);
                        (*action)((unsigned long)(b*255) << 24,1);
    }
  return 3 * im->width * im->height;
}

int gl2psPrintPDFPixmap(int obj, GL2PSimage* im){
  int offs = 0, done = 0;
  
  offs += fprintf(gl2ps->stream,
                  "%d 0 obj\n"
                  "<<\n"
                  "/Type /XObject\n"
                  "/Subtype /Image\n"
                  "/Width %d\n"
                  "/Height %d\n"
                  "/ColorSpace /DeviceRGB\n"
                  "/BitsPerComponent 8\n",
                  obj, (int)im->width, (int)im->height);
        
#ifdef GL2PS_HAVE_ZLIB
  if(gl2ps->options & GL2PS_COMPRESS){
    gl2psAllocCompress((int)(im->width * im->height * 3));
    
    gl2psPrintPDFPixmapStreamData(im, gl2psWriteBigEndianCompress);
    
    if(Z_OK == gl2psDeflate() && 23 + gl2ps->compress->destLen < gl2ps->compress->srcLen){
      offs += gl2psPrintPDFCompressorType();
      offs += fprintf(gl2ps->stream,
                      "/Length %d "
                      ">>\n"
                      "stream\n",
                      (int)gl2ps->compress->destLen);
      offs += gl2ps->compress->destLen * fwrite(gl2ps->compress->dest, gl2ps->compress->destLen,
                                                1, gl2ps->stream);
      done = 1;
    }
    gl2psFreeCompress();
  }
#endif
  
  if(!done){
    /* no compression, or too long after compression, or compress error
       -> write non-compressed entry */
    offs += fprintf(gl2ps->stream,
                    "/Length %d "
                    ">>\n"
                    "stream\n",
                    (int)(im->width * im->height * 3));
    offs += gl2psPrintPDFPixmapStreamData(im, gl2psWriteBigEndian);
  }
  
  offs += fprintf(gl2ps->stream,
                  "\nendstream\n"
                  "endobj\n");

  return offs;
}

int* gl2psPrintPDFPixmapObjects(int firstObjnumber, int firstOffs){
  int size;
  int* offs;
  int i;
  
  size = gl2psListNbr(gl2ps->ilist);
  offs = (int*)gl2psMalloc(sizeof(int) * (size+1));
  
  offs[0] = firstOffs;
  
  for(i = 0; i < size; ++i){
    firstOffs += gl2psPrintPDFPixmap(i+firstObjnumber, 
                                     *(GL2PSimage**)gl2psListPointer(gl2ps->ilist, i));
    offs[i+1] = firstOffs;
  }
  return offs;
}

int gl2psPrintPDFText(int obj, GL2PSstring* s, int fontnumber ){
  int offs = 0;
  
  offs += fprintf(gl2ps->stream,
                  "%d 0 obj\n"
                  "<<\n"
                  "/Type /Font\n"
                  "/Subtype /Type1\n"
                  "/Name /F%d\n"
                  "/BaseFont /%s\n"
                  "/Encoding /MacRomanEncoding\n"
                  ">>\n"
                  "endobj\n",
                  obj, fontnumber, s->fontname);
  return offs;
}

int* gl2psPrintPDFTextObjects(int firstObjnumber, int firstOffs){
  int size;
  int* offs;
  int i;
  
  size = gl2psListNbr(gl2ps->slist);
  offs = (int*)gl2psMalloc(sizeof(int) * (size+1));
  
  offs[0] = firstOffs;
  
  for(i = 0; i < size; ++i){
    firstOffs += gl2psPrintPDFText(i+firstObjnumber,
                                   *(GL2PSstring**)gl2psListPointer(gl2ps->slist, i),i);
    offs[i+1] = firstOffs;
  }
  return offs;
}

/* All variable data is written at this point and all required
   functioninality has been gathered: Writes file footer with cross
   reference table and trailer */

void gl2psPrintPDFFooter(){
  int offs;
  int i;
  int *shader_offs, *image_offs, *text_offs;
  int shader_size, image_size, text_size, objnumber, lastoffset;
  
  offs = gl2ps->cref[4] + gl2ps->streamlength; 
  offs += gl2psClosePDFDataStream();
  gl2ps->cref[4] = offs; 
  
  offs += gl2psPrintPDFDataStreamLength(gl2ps->streamlength);
  gl2ps->cref[5] = offs;
  gl2ps->streamlength = 0;
  
  offs += gl2psPrintPDFSinglePage();
  gl2ps->cref[6] = offs;
  
  offs += gl2psPrintPDFExtGState();
  
  shader_size = gl2psListNbr(gl2ps->tidxlist);
  image_size = gl2psListNbr(gl2ps->ilist);
  text_size = gl2psListNbr(gl2ps->slist);
  
  shader_offs = gl2psPrintPDFShaderObjects(GL2PS_FIXED_XREF_ENTRIES + 1, offs);
  image_offs = gl2psPrintPDFPixmapObjects(GL2PS_FIXED_XREF_ENTRIES + 1 + shader_size, 
                                          shader_offs[shader_size]);
  text_offs = gl2psPrintPDFTextObjects(GL2PS_FIXED_XREF_ENTRIES + 1 + shader_size + image_size,
                                       image_offs[image_size]);
  
  lastoffset = text_offs[text_size];
  objnumber = GL2PS_FIXED_XREF_ENTRIES + shader_size + image_size + text_size + 1;
  
  /* Start cross reference table. The file has to been opened in
     binary mode to preserve the 20 digit string length! */
  fprintf(gl2ps->stream,
          "xref\n"
          "0 %d\n"
          "%010d 65535 f \n", objnumber, 0);
  
  for(i = 0; i < GL2PS_FIXED_XREF_ENTRIES; ++i){
    fprintf(gl2ps->stream, "%010d 00000 n \n", gl2ps->cref[i]);
  }
  for(i = 0; i < shader_size; ++i){
    fprintf(gl2ps->stream, "%010d 00000 n \n", shader_offs[i]);
  }
  for(i = 0; i < image_size; ++i){
    fprintf(gl2ps->stream, "%010d 00000 n \n", image_offs[i]);
  }
  for(i = 0; i < text_size; ++i){
    fprintf(gl2ps->stream, "%010d 00000 n \n", text_offs[i]);
  }
  
  fprintf(gl2ps->stream,
          "trailer\n"
          "<<\n" 
          "/Size %d\n"
          "/Info 1 0 R\n"
          "/Root 2 0 R\n"
          ">>\n"
          "startxref\n%d\n"
          "%%%%EOF\n",
          objnumber, lastoffset);
  
  /* Free auxiliary lists and arrays */
  gl2psFree(shader_offs);
  gl2psFree(image_offs);
  gl2psFree(text_offs);
  gl2psListDelete(gl2ps->tlist);
  gl2psListDelete(gl2ps->tidxlist);
  for(i = 0; i < gl2psListNbr(gl2ps->ilist); ++i)
    gl2psFreePixmap(*(GL2PSimage**)gl2psListPointer(gl2ps->ilist, i));
  gl2psListDelete(gl2ps->ilist);
  for(i = 0; i < gl2psListNbr(gl2ps->slist); ++i)
    gl2psFreeText(*(GL2PSstring**)gl2psListPointer(gl2ps->slist, i));
  gl2psListDelete(gl2ps->slist);

#ifdef GL2PS_HAVE_ZLIB
  if(gl2ps->options & GL2PS_COMPRESS){
    gl2psFreeCompress();
    gl2psFree(gl2ps->compress);
    gl2ps->compress = NULL;
  }
#endif
}

/* PDF begin viewport */

void gl2psPrintPDFBeginViewport(GLint viewport[4]){
  int offs;
  GLint index;
  GLfloat rgba[4];
  int x = viewport[0], y = viewport[1], w = viewport[2], h = viewport[3];
  
  offs = 0;
  
  glRenderMode(GL_FEEDBACK);
  
  offs += gl2psPrintf("q\n");
  
  if(gl2ps->options & GL2PS_DRAW_BACKGROUND){
    if(gl2ps->colormode == GL_RGBA || gl2ps->colorsize == 0){
      glGetFloatv(GL_COLOR_CLEAR_VALUE, rgba);
    }
    else{
      glGetIntegerv(GL_INDEX_CLEAR_VALUE, &index);
      rgba[0] = gl2ps->colormap[index][0];
      rgba[1] = gl2ps->colormap[index][1];
      rgba[2] = gl2ps->colormap[index][2];
      rgba[3] = 0.;
    }
    offs += gl2psPrintf("%f %f %f rg\n"
                        "%d %d %d %d re\n"
                        "W\n"
                        "f\n",
                        rgba[0], rgba[1], rgba[2], x, y, w, h);
  }
  else{
    offs += gl2psPrintf("%d %d %d %d re\n"
                        "W\n"   
                        "n\n",
                        x, y, w, h);            
  }

  gl2ps->streamlength += offs;
}

GLint gl2psPrintPDFEndViewport(){
  GLint res;
  GLint gl2psPrintPrimitives(void);
  
  res = gl2psPrintPrimitives();
  res += gl2psFlushPDFTriangles();
  res += gl2psFlushPDFLines();

  gl2ps->streamlength += gl2psPrintf("Q\n");
  
  return res;
}

/********************************************************************* 
 *
 * General primitive printing routine
 *
 *********************************************************************/

GLint gl2psPrintPrimitives(void){
  GL2PSbsptree *root;
  GL2PSxyz eye = {0.0F, 0.0F, 100000.0F};
  GLint used;
  void (*pprim)(void *a, void *b) = 0;

  used = glRenderMode(GL_RENDER);

  if(used < 0){
    gl2psMsg(GL2PS_INFO, "OpenGL feedback buffer overflow");
    return GL2PS_OVERFLOW;
  }

  if(used == 0){
    return GL2PS_NO_FEEDBACK; /* Empty feedback buffer */
  }

  if(gl2ps->format == GL2PS_PS || 
     gl2ps->format == GL2PS_EPS ||
     gl2ps->format == GL2PS_PDF){
    gl2psParseFeedbackBuffer(used);
  }

  if(!gl2psListNbr(gl2ps->primitives)){
    return GL2PS_SUCCESS; /* Nothing to print */
  }

  switch(gl2ps->format){
  case GL2PS_TEX :
    pprim = gl2psPrintTeXPrimitive;
    break;
  case GL2PS_PS :
  case GL2PS_EPS :
    pprim = gl2psPrintPostScriptPrimitive;
    break;
  case GL2PS_PDF :
    pprim = gl2psPrintPDFPrimitive;
    break;
  }
  
  switch(gl2ps->sort){
  case GL2PS_NO_SORT :
    gl2psListAction(gl2ps->primitives, pprim);
    gl2psListAction(gl2ps->primitives, gl2psFreePrimitive);
    /* reset the primitive list, waiting for the next viewport */
    gl2psListReset(gl2ps->primitives);
    break;
  case GL2PS_SIMPLE_SORT :
    gl2psListSort(gl2ps->primitives, gl2psCompareDepth);
    if(gl2ps->options & GL2PS_OCCLUSION_CULL){
      gl2psListAction(gl2ps->primitives, gl2psAddInImageTree);
      gl2psFreeBspImageTree(&gl2ps->imagetree);
    }
    gl2psListActionInverse(gl2ps->primitives, pprim);
    gl2psListAction(gl2ps->primitives, gl2psFreePrimitive);
    /* reset the primitive list, waiting for the next viewport */
    gl2psListReset(gl2ps->primitives);
    break;
  case GL2PS_BSP_SORT :
    root = (GL2PSbsptree*)gl2psMalloc(sizeof(GL2PSbsptree));
    gl2psBuildBspTree(root, gl2ps->primitives);
    if(gl2ps->boundary) gl2psBuildPolygonBoundary(root);
    if(gl2ps->options & GL2PS_OCCLUSION_CULL){
      gl2psTraverseBspTree(root, eye, -(float)GL2PS_EPSILON, gl2psLess,
                           gl2psAddInImageTree, 1);
      gl2psFreeBspImageTree(&gl2ps->imagetree);
    }
    gl2psTraverseBspTree(root, eye, (float)GL2PS_EPSILON, gl2psGreater, 
                         pprim, 0);
    gl2psFreeBspTree(&root);
    /* reallocate the primitive list (it's been deleted by
       gl2psBuildBspTree) in case there is another viewport */
    gl2ps->primitives = gl2psListCreate(500, 500, sizeof(GL2PSprimitive*));
    break;
  default :
    gl2psMsg(GL2PS_ERROR, "Unknown sorting algorithm: %d", gl2ps->sort);
    return GL2PS_ERROR;
  }

  fflush(gl2ps->stream);

  return GL2PS_SUCCESS;
}

/********************************************************************* 
 *
 * Public routines
 *
 *********************************************************************/

GL2PSDLL_API GLint gl2psBeginPage(const char *title, const char *producer, 
                                  GLint viewport[4], GLint format, GLint sort,
                                  GLint options, GLint colormode,
                                  GLint colorsize, GL2PSrgba *colormap,
                                  GLint nr, GLint ng, GLint nb, GLint buffersize,
                                  FILE *stream, const char *filename){
  int i;

  gl2ps = (GL2PScontext*)gl2psMalloc(sizeof(GL2PScontext));
  gl2ps->maxbestroot = 10;
  gl2ps->format = format;
  gl2ps->title = title;
  gl2ps->producer = producer;
  gl2ps->filename = filename;
  gl2ps->sort = sort;
  gl2ps->options = options;
  gl2ps->compress = NULL;

  if(gl2ps->options & GL2PS_USE_CURRENT_VIEWPORT){
    glGetIntegerv(GL_VIEWPORT, gl2ps->viewport);
  }
  else{
    for(i = 0; i < 4; i++){
      gl2ps->viewport[i] = viewport[i];
    }
  }
  gl2ps->threshold[0] = nr ? 1.0F/(GLfloat)nr : 0.032F;
  gl2ps->threshold[1] = ng ? 1.0F/(GLfloat)ng : 0.017F;
  gl2ps->threshold[2] = nb ? 1.0F/(GLfloat)nb : 0.050F;
  gl2ps->colormode = colormode;
  gl2ps->buffersize = buffersize > 0 ? buffersize : 2048 * 2048;
  for(i = 0; i < 4; i++){
    gl2ps->lastrgba[i] = -1.0F;
  }
  gl2ps->lastlinewidth = -1.0F;
  gl2ps->imagetree = NULL;
  gl2ps->primitivetoadd = NULL;
  gl2ps->zerosurfacearea = 0;  

  if(gl2ps->colormode == GL_RGBA){
    gl2ps->colorsize = 0;
    gl2ps->colormap = NULL;
  }
  else if(gl2ps->colormode == GL_COLOR_INDEX){
    if(!colorsize || !colormap){
      gl2psMsg(GL2PS_ERROR, "Missing colormap for GL_COLOR_INDEX rendering");
      gl2psFree(gl2ps);
      gl2ps = NULL;
      return GL2PS_ERROR;
    }
    gl2ps->colorsize = colorsize;
    gl2ps->colormap = (GL2PSrgba*)gl2psMalloc(gl2ps->colorsize * sizeof(GL2PSrgba));
    memcpy(gl2ps->colormap, colormap, gl2ps->colorsize * sizeof(GL2PSrgba));
  }
  else{
    gl2psMsg(GL2PS_ERROR, "Unknown color mode in gl2psBeginPage");
    gl2psFree(gl2ps);
    gl2ps = NULL;
    return GL2PS_ERROR;
  }

  if(!stream){
    gl2psMsg(GL2PS_ERROR, "Bad file pointer");
    gl2psFree(gl2ps);
    gl2ps = NULL;
    return GL2PS_ERROR;
  }
  else{
    gl2ps->stream = stream;
    /* In case gl2psEndPage failed (e.g. due to a GL2PS_OVERFLOW) and
       we didn't reopen the stream before calling gl2psBeginPage
       again, we need to rewind the stream */
    rewind(gl2ps->stream);
  }

  /* only used for PDF output... */
  gl2ps->lasttype = -1;
  gl2ps->consec_cnt = 0;
  gl2ps->consec_inner_cnt = 1;
  gl2ps->line_width_diff = 1;
  gl2ps->line_rgb_diff = 1;
  gl2ps->last_line_finished = 0;
  gl2ps->last_triangle_finished = 0;

  switch(gl2ps->format){
  case GL2PS_TEX :
    gl2psPrintTeXHeader();
    break;
  case GL2PS_PS :
  case GL2PS_EPS :
    gl2psPrintPostScriptHeader();
    break;
  case GL2PS_PDF :
    gl2psPrintPDFHeader();
    break;
  default :
    gl2psMsg(GL2PS_ERROR, "Unknown output format: %d", gl2ps->format);
    gl2psFree(gl2ps);
    gl2ps = NULL;
    return GL2PS_ERROR;
  }

  gl2ps->primitives = gl2psListCreate(500, 500, sizeof(GL2PSprimitive*));
  gl2ps->feedback = (GLfloat*)gl2psMalloc(gl2ps->buffersize * sizeof(GLfloat));
  glFeedbackBuffer(gl2ps->buffersize, GL_3D_COLOR, gl2ps->feedback);
  glRenderMode(GL_FEEDBACK);  

  return GL2PS_SUCCESS;
}

GL2PSDLL_API GLint gl2psEndPage(void){
  GLint res;

  if(!gl2ps) return GL2PS_UNINITIALIZED;

  res = gl2psPrintPrimitives();

  /* print the footer even if gl2psPrintPrimitives didn't succeed, so
     that we end up with a valid file */
  switch(gl2ps->format){
  case GL2PS_TEX :
    gl2psPrintTeXFooter();
    break;
  case GL2PS_PS :
  case GL2PS_EPS :
    gl2psPrintPostScriptFooter();
    break;
  case GL2PS_PDF :
    gl2psPrintPDFFooter();
    break;
  }

  fflush(gl2ps->stream);

  gl2psListDelete(gl2ps->primitives);
  gl2psFree(gl2ps->colormap);
  gl2psFree(gl2ps->feedback);
  gl2psFree(gl2ps);
  gl2ps = NULL;

  return res;
}

GL2PSDLL_API GLint gl2psBeginViewport(GLint viewport[4]){
  if(!gl2ps) return GL2PS_UNINITIALIZED;

  switch(gl2ps->format){
  case GL2PS_PS :
  case GL2PS_EPS :
    gl2psPrintPostScriptBeginViewport(viewport);
    break;
  case GL2PS_PDF :
    gl2psPrintPDFBeginViewport(viewport);
    break;
  default :
    /* FIXME: handle other formats */
    break;
  }
  
  return GL2PS_SUCCESS;
}

GL2PSDLL_API GLint gl2psEndViewport(void){
  GLint res;

  if(!gl2ps) return GL2PS_UNINITIALIZED;

  switch(gl2ps->format){
  case GL2PS_PS :
  case GL2PS_EPS :
    res = gl2psPrintPostScriptEndViewport();
    break;
  case GL2PS_PDF :
    res = gl2psPrintPDFEndViewport();
    break;
  default :
    /* FIXME: handle other formats */
    res = GL2PS_SUCCESS;
    break;
  }

  return res;
}

GL2PSDLL_API GLint gl2psTextOpt(const char *str, const char *fontname, GLshort fontsize,
                                GLint alignment, GL2PSrgba rgba){
  GLfloat pos[4];
  GL2PSprimitive *prim;
  GLboolean valid;

  if(!gl2ps || !str) return GL2PS_UNINITIALIZED;

  if(gl2ps->options & GL2PS_NO_TEXT) return GL2PS_SUCCESS;

  glGetBooleanv(GL_CURRENT_RASTER_POSITION_VALID, &valid);
  if(!valid) return GL2PS_SUCCESS; /* the primitive is culled */

  glGetFloatv(GL_CURRENT_RASTER_POSITION, pos);

  prim = (GL2PSprimitive *)gl2psMalloc(sizeof(GL2PSprimitive));
  prim->type = GL2PS_TEXT;
  prim->boundary = 0;
  prim->numverts = 1;
  prim->verts = (GL2PSvertex *)gl2psMalloc(sizeof(GL2PSvertex));
  prim->verts[0].xyz[0] = pos[0];
  prim->verts[0].xyz[1] = pos[1];
  prim->verts[0].xyz[2] = GL2PS_DEPTH_FACT * pos[2];
  prim->depth = pos[2];
  prim->culled = 0;
  prim->dash = 0;
  prim->width = 1;
  if(rgba){
    prim->verts[0].rgba[0] = rgba[0];
    prim->verts[0].rgba[1] = rgba[1];
    prim->verts[0].rgba[2] = rgba[2];
    prim->verts[0].rgba[3] = rgba[3];
  }
  else{
    glGetFloatv(GL_CURRENT_RASTER_COLOR, prim->verts[0].rgba);
  }
  prim->text = (GL2PSstring*)gl2psMalloc(sizeof(GL2PSstring));
  prim->text->str = (char*)gl2psMalloc((strlen(str)+1)*sizeof(char));
  strcpy(prim->text->str, str); 
  prim->text->fontname = (char*)gl2psMalloc((strlen(fontname)+1)*sizeof(char));
  strcpy(prim->text->fontname, fontname);
  prim->text->fontsize = fontsize;
  prim->text->alignment = alignment;

  gl2psListAdd(gl2ps->primitives, &prim);

  return GL2PS_SUCCESS;
}

GL2PSDLL_API GLint gl2psText(const char *str, const char *fontname, GLshort fontsize){
  return gl2psTextOpt(str, fontname, fontsize, GL2PS_TEXT_BL, NULL);
}

GL2PSDLL_API GLint gl2psDrawPixels(GLsizei width, GLsizei height,
                                   GLint xorig, GLint yorig,
                                   GLenum format, GLenum type, 
                                   const void *pixels){
  int size;
  GLfloat pos[4];
  GL2PSprimitive *prim;
  GLboolean valid;

  if(!gl2ps || !pixels) return GL2PS_UNINITIALIZED;

  if((width <= 0) || (height <= 0)) return GL2PS_ERROR;

  if(gl2ps->options & GL2PS_NO_PIXMAP) return GL2PS_SUCCESS;

  if(format != GL_RGB || type != GL_FLOAT){
    gl2psMsg(GL2PS_ERROR, "gl2psDrawPixels only implemented for GL_RGB, GL_FLOAT pixels");
    return GL2PS_ERROR;
  }

  glGetBooleanv(GL_CURRENT_RASTER_POSITION_VALID, &valid);
  if(!valid) return GL2PS_SUCCESS; /* the primitive is culled */

  glGetFloatv(GL_CURRENT_RASTER_POSITION, pos);

  prim = (GL2PSprimitive *)gl2psMalloc(sizeof(GL2PSprimitive));
  prim->type = GL2PS_PIXMAP;
  prim->boundary = 0;
  prim->numverts = 1;
  prim->verts = (GL2PSvertex *)gl2psMalloc(sizeof(GL2PSvertex));
  prim->verts[0].xyz[0] = pos[0] + xorig;
  prim->verts[0].xyz[1] = pos[1] + yorig;
  prim->verts[0].xyz[2] = GL2PS_DEPTH_FACT * pos[2];
  prim->depth = pos[2];
  prim->culled = 0;
  prim->dash = 0;
  prim->width = 1;
  glGetFloatv(GL_CURRENT_RASTER_COLOR, prim->verts[0].rgba);
  prim->image = (GL2PSimage*)gl2psMalloc(sizeof(GL2PSimage));
  prim->image->width = width;
  prim->image->height = height;
  prim->image->format = format;
  prim->image->type = type;
  size = height*width*3*sizeof(GLfloat); /* FIXME: handle other types/formats */
  prim->image->pixels = (GLfloat*)gl2psMalloc(size);
  memcpy(prim->image->pixels, pixels, size);

  gl2psListAdd(gl2ps->primitives, &prim);

  return GL2PS_SUCCESS;
}

GL2PSDLL_API GLint gl2psEnable(GLint mode){
  if(!gl2ps) return GL2PS_UNINITIALIZED;

  switch(mode){
  case GL2PS_POLYGON_OFFSET_FILL :
    glPassThrough(GL2PS_BEGIN_POLYGON_OFFSET_FILL);
    glGetFloatv(GL_POLYGON_OFFSET_FACTOR, &gl2ps->offset[0]);
    glGetFloatv(GL_POLYGON_OFFSET_UNITS, &gl2ps->offset[1]);
    break;
  case GL2PS_POLYGON_BOUNDARY :
    glPassThrough(GL2PS_BEGIN_POLYGON_BOUNDARY);
    break;
  case GL2PS_LINE_STIPPLE :
    glPassThrough(GL2PS_BEGIN_LINE_STIPPLE);
    break;
  default :
    gl2psMsg(GL2PS_WARNING, "Unknown mode in gl2psEnable: %d", mode);
    return GL2PS_WARNING;
  }

  return GL2PS_SUCCESS;
}

GL2PSDLL_API GLint gl2psDisable(GLint mode){
  if(!gl2ps) return GL2PS_UNINITIALIZED;

  switch(mode){
  case GL2PS_POLYGON_OFFSET_FILL :
    glPassThrough(GL2PS_END_POLYGON_OFFSET_FILL);
    break;
  case GL2PS_POLYGON_BOUNDARY :
    glPassThrough(GL2PS_END_POLYGON_BOUNDARY);
    break;
  case GL2PS_LINE_STIPPLE :
    glPassThrough(GL2PS_END_LINE_STIPPLE);
    break;
  default :
    gl2psMsg(GL2PS_WARNING, "Unknown mode in gl2psDisable: %d", mode);
    return GL2PS_WARNING;
  }

  return GL2PS_SUCCESS;
}

GL2PSDLL_API GLint gl2psPointSize(GLfloat value){
  if(!gl2ps) return GL2PS_UNINITIALIZED;

  glPassThrough(GL2PS_SET_POINT_SIZE);
  glPassThrough(value);
  
  return GL2PS_SUCCESS;
}

GL2PSDLL_API GLint gl2psLineWidth(GLfloat value){
  if(!gl2ps) return GL2PS_UNINITIALIZED;

  glPassThrough(GL2PS_SET_LINE_WIDTH);
  glPassThrough(value);

  return GL2PS_SUCCESS;
}
