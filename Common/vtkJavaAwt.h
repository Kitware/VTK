/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkJavaAwt.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// for use with JAWT
#include "jawt_md.h"

extern "C" JNIEXPORT void  JNICALL 
    Java_vtkPanel_RenderCreate(JNIEnv *env, jobject canvas, jobject id0)
{
  JAWT awt;
  JAWT_DrawingSurface* ds;
  JAWT_DrawingSurfaceInfo* dsi;
  jint lock;

  // get the render window pointer
  vtkRenderWindow *temp0;
  temp0 = (vtkRenderWindow *)(vtkJavaGetPointerFromObject(env,id0,(char *) "vtkRenderWindow"));
  
  /* Get the AWT */
  awt.version = JAWT_VERSION_1_3;
  if (JAWT_GetAWT(env, &awt) == JNI_FALSE) 
    {
    printf("AWT Not found\n");
    return;
    }
  
  /* Get the drawing surface */
  ds = awt.GetDrawingSurface(env, canvas);
  if (ds == NULL) 
    {
    printf("NULL drawing surface\n");
    return;
    }
  
  /* Lock the drawing surface */
  lock = ds->Lock(ds);
  if((lock & JAWT_LOCK_ERROR) != 0) 
    {
    printf("Error locking surface\n");
    awt.FreeDrawingSurface(ds);
    return;
    }

  /* Get the drawing surface info */
  dsi = ds->GetDrawingSurfaceInfo(ds);
  if (dsi == NULL) 
    {
    printf("Error getting surface info\n");
    ds->Unlock(ds);
    awt.FreeDrawingSurface(ds);
    return;
    }
  
// Here is the win32 drawing code
#if defined(_WIN32) || defined(WIN32)
  JAWT_Win32DrawingSurfaceInfo* dsi_win;
  dsi_win = (JAWT_Win32DrawingSurfaceInfo*)dsi->platformInfo;
  temp0->SetWindowId((void *)dsi_win->hwnd);
  
// otherwise use X11 code
#else
  JAWT_X11DrawingSurfaceInfo* dsi_x11;
  dsi_x11 = (JAWT_X11DrawingSurfaceInfo*)dsi->platformInfo;
  temp0->SetDisplayId((void *)dsi_x11->display);
  temp0->SetWindowId((void *)dsi_x11->drawable);
#endif

  temp0->Render();
  
  /* Free the drawing surface info */
  ds->FreeDrawingSurfaceInfo(dsi);
  
  /* Unlock the drawing surface */
  ds->Unlock(ds);
  
  /* Free the drawing surface */
  awt.FreeDrawingSurface(ds);
}


// Lock must be called prior to render or anything which might
// cause vtkRenderWindow to make an XLib call or to call Render(). 
// The Lock() and UnLock() functions are necessary for drawing in
// JAWT, but they also provide a form of mutex locking so that multiple
// java threads are prevented from accessing X at the same time.  The only
// requirement JAWT has is that all operations on a JAWT_DrawingSurface 
// MUST be performed from the same thread as the call to GetDrawingSurface.
extern "C" JNIEXPORT void  JNICALL 
    Java_vtkPanel_Lock(JNIEnv *env, 
                       jobject canvas)
{
  JAWT awt;
  JAWT_DrawingSurface* ds;
  jint lock;

  /* Get the AWT */
  awt.version = JAWT_VERSION_1_3;
  if (JAWT_GetAWT(env, &awt) == JNI_FALSE) 
      {
          printf("AWT Not found\n");
          return;
      }
  
  /* Get the drawing surface */
  ds = awt.GetDrawingSurface(env, canvas);
  if (ds == NULL) 
      {
          printf("NULL drawing surface\n");
          return;
      }
  
  /* Lock the drawing surface */
  lock = ds->Lock(ds);
  if((lock & JAWT_LOCK_ERROR) != 0) 
    {
    printf("Error locking surface\n");
    awt.FreeDrawingSurface(ds);
    return;
    }

}

// UnLock() must be called after a Lock() and execution of a
// function which might change the drawing surface.  See Lock().
extern "C" JNIEXPORT void  JNICALL 
    Java_vtkPanel_UnLock(JNIEnv *env, 
                         jobject canvas)
{
  JAWT awt;
  JAWT_DrawingSurface* ds;

  /* Get the AWT */
  awt.version = JAWT_VERSION_1_3;
  if (JAWT_GetAWT(env, &awt) == JNI_FALSE) 
      {
          printf("AWT Not found\n");
          return;
      }
  
  /* Get the drawing surface */
  ds = awt.GetDrawingSurface(env, canvas);
  if (ds == NULL) 
      {
          printf("NULL drawing surface\n");
          return;
      }
  
  /* Unlock the drawing surface */
  ds->Unlock(ds);
  
  /* Free the drawing surface */
  awt.FreeDrawingSurface(ds);
}

