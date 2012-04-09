/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkJavaAwt.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// for use with JAWT
#include "jawt_md.h"

#define VTK_JAVA_DEBUG


#if defined(_WIN32) || defined(WIN32)
#define WIN32_JAWT_LOCK_HACK
#endif

#if defined(WIN32_JAWT_LOCK_HACK)
#define WJLH_MAX_COUNT (32)
#define WJLH_HASH_FUNC(E,C,H) {\
  jclass cls = E->GetObjectClass(C);\
  jmethodID mid = E->GetMethodID(cls, "hashCode", "()I");\
  H = E->CallIntMethod(C, mid); }
# include "vtkWindows.h"
int WJLH_init_check = 0;
# include <map> // STL Header
std::map<int,int> WJLH_lock_map;
#endif

extern "C" JNIEXPORT jint  JNICALL 
Java_vtk_vtkPanel_RenderCreate(JNIEnv *env, jobject canvas, jobject id0)
{
#if defined(WIN32_JAWT_LOCK_HACK)
  int hash;
  WJLH_HASH_FUNC(env, canvas, hash);
  WJLH_lock_map[hash] = 0;
#endif

  JAWT awt;
  JAWT_DrawingSurface* ds;
  JAWT_DrawingSurfaceInfo* dsi;
  jint lock;

  // get the render window pointer
  vtkRenderWindow *temp0;
  temp0 = (vtkRenderWindow *)(vtkJavaGetPointerFromObject(env,id0));
  
  /* Get the AWT */
  awt.version = JAWT_VERSION_1_3;
  if (JAWT_GetAWT(env, &awt) == JNI_FALSE) 
    {
#ifndef VTK_JAVA_DEBUG
    printf("AWT Not found\n");
#endif
    return 1;
    }
  
  /* Get the drawing surface */
  ds = awt.GetDrawingSurface(env, canvas);
  if (ds == NULL) 
    {
#ifndef VTK_JAVA_DEBUG
    printf("NULL drawing surface\n");
#endif
    return 1;
    }
  
  /* Lock the drawing surface */
  lock = ds->Lock(ds);
  if((lock & JAWT_LOCK_ERROR) != 0) 
    {
#ifndef VTK_JAVA_DEBUG
    printf("Error locking surface\n");
#endif
    awt.FreeDrawingSurface(ds);
    return 1;
    }

  /* Get the drawing surface info */
  dsi = ds->GetDrawingSurfaceInfo(ds);
  if (dsi == NULL) 
    {
    printf("Error getting surface info\n");
    ds->Unlock(ds);
    awt.FreeDrawingSurface(ds);
    return 1;
    }
  
// Here is the win32 drawing code
#if defined(_WIN32) || defined(WIN32)
  temp0->Finalize();
  JAWT_Win32DrawingSurfaceInfo* dsi_win;
  dsi_win = (JAWT_Win32DrawingSurfaceInfo*)dsi->platformInfo;
  temp0->SetWindowId((void *)dsi_win->hwnd);
  temp0->SetDisplayId((void *)dsi_win->hdc);
  // also set parent id to avoid border sizes being added
  temp0->SetParentId((void *)dsi_win->hdc);
// use mac code
#elif defined(__APPLE__)
  JAWT_MacOSXDrawingSurfaceInfo* dsi_mac;
  dsi_mac = (JAWT_MacOSXDrawingSurfaceInfo*)dsi->platformInfo;
  temp0->SetWindowId(dsi_mac->cocoaViewRef);
// otherwise use X11 code
#else
  JAWT_X11DrawingSurfaceInfo* dsi_x11;
  dsi_x11 = (JAWT_X11DrawingSurfaceInfo*)dsi->platformInfo;
  temp0->SetDisplayId((void *)dsi_x11->display);
  temp0->SetWindowId((void *)dsi_x11->drawable);
  temp0->SetParentId((void *)dsi_x11->display);
#endif
  
  /* Free the drawing surface info */
  ds->FreeDrawingSurfaceInfo(dsi);
  
  /* Unlock the drawing surface */
  ds->Unlock(ds);
  
  /* Free the drawing surface */
  awt.FreeDrawingSurface(ds);

#if defined(WIN32_JAWT_LOCK_HACK)
if (WJLH_init_check == 0)
{
  WJLH_init_check = 1;
}
  WJLH_lock_map[hash] = 1;
#endif
  return 0;

}


// Lock must be called prior to render or anything which might
// cause vtkRenderWindow to make an XLib call or to call Render(). 
// The Lock() and UnLock() functions are necessary for drawing in
// JAWT, but they also provide a form of mutex locking so that multiple
// java threads are prevented from accessing X at the same time.  The only
// requirement JAWT has is that all operations on a JAWT_DrawingSurface 
// MUST be performed from the same thread as the call to GetDrawingSurface.
extern "C" JNIEXPORT jint  JNICALL 
Java_vtk_vtkPanel_Lock(JNIEnv *env, 
                       jobject canvas)
{
  JAWT awt;
  JAWT_DrawingSurface* ds;
  jint lock;

  /* Get the AWT */
  awt.version = JAWT_VERSION_1_3;
  if (JAWT_GetAWT(env, &awt) == JNI_FALSE) 
    {
#ifndef VTK_JAVA_DEBUG
    printf("AWT Not found\n");
#endif
    return 1;
    }
  
  /* Get the drawing surface */
  ds = awt.GetDrawingSurface(env, canvas);
  if (ds == NULL) 
    {
#ifndef VTK_JAVA_DEBUG
    printf("NULL drawing surface\n");
#endif
    return 1;
    }

#if defined(WIN32_JAWT_LOCK_HACK)
  int hash;
  WJLH_HASH_FUNC(env, canvas, hash);
  if (WJLH_init_check && WJLH_lock_map[hash] > WJLH_MAX_COUNT)
  {
    env->MonitorEnter(canvas);      
  }
  else
  {
#endif
  /* Lock the drawing surface */
  lock = ds->Lock(ds);
  if((lock & JAWT_LOCK_ERROR) != 0) 
    {
#ifndef VTK_JAVA_DEBUG
    printf("Error locking surface\n");
#endif
    awt.FreeDrawingSurface(ds);
    return 1;
    }
#if defined(WIN32_JAWT_LOCK_HACK)
  }
#endif

  return 0;

}

// UnLock() must be called after a Lock() and execution of a
// function which might change the drawing surface.  See Lock().
extern "C" JNIEXPORT jint  JNICALL 
Java_vtk_vtkPanel_UnLock(JNIEnv *env, 
                         jobject canvas)
{
  JAWT awt;
  JAWT_DrawingSurface* ds;

  /* Get the AWT */
  awt.version = JAWT_VERSION_1_3;
  if (JAWT_GetAWT(env, &awt) == JNI_FALSE) 
    {
#ifndef VTK_JAVA_DEBUG
    printf("AWT Not found\n");
#endif
    return 1;
    }
  
  /* Get the drawing surface */
  ds = awt.GetDrawingSurface(env, canvas);
  if (ds == NULL) 
    {
#ifndef VTK_JAVA_DEBUG
    printf("NULL drawing surface\n");
#endif
    return 1;
    }

#if defined(WIN32_JAWT_LOCK_HACK)
  int hash;
  WJLH_HASH_FUNC(env, canvas, hash);
  if (WJLH_init_check && WJLH_lock_map[hash] > WJLH_MAX_COUNT)
  {
    env->MonitorExit(canvas);
  }
  else
  {
    if (WJLH_init_check) WJLH_lock_map[hash]++;
#endif
  /* Unlock the drawing surface */
  ds->Unlock(ds);
#if defined(WIN32_JAWT_LOCK_HACK)
  }
#endif
  
  /* Free the drawing surface */
  awt.FreeDrawingSurface(ds);

  return 0;
}
