// only do this when JAWT is to be used
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

extern "C" JNIEXPORT void  JNICALL 
    Java_vtkPanel_RenderInternal(JNIEnv *env, jobject canvas, jobject id0)
{
  JAWT awt;
  JAWT_DrawingSurface* ds;
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

  temp0->Render();
  
  /* Unlock the drawing surface */
  ds->Unlock(ds);
  
  /* Free the drawing surface */
  awt.FreeDrawingSurface(ds);
}

extern "C" JNIEXPORT void  JNICALL 
    Java_vtkPanel_SetSizeInternal(JNIEnv *env, 
                                  jobject canvas,
                                  jobject id0, jint id1,jint id2)
{
  JAWT awt;
  JAWT_DrawingSurface* ds;
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

  int      temp1;
  int      temp2;
  temp1 = id1;
  temp2 = id2;

  temp0->SetSize(temp1,temp2);
  
  /* Unlock the drawing surface */
  ds->Unlock(ds);
  
  /* Free the drawing surface */
  awt.FreeDrawingSurface(ds);
}


// This function is provided to lock the render window and execute the
// method represented by methodString of the object represented by anObject.
// This is necessary to wrap any vtk filter which might call Render in
// its Execute method (for example vtkRenderLargeImage).  The method passed
// in must be void and zero-argument (this could be extended later to handle
// more generic methods).
extern "C" JNIEXPORT void  JNICALL 
    Java_vtkPanel_LockAndExecuteVoidMethod(JNIEnv *env, 
                                           jobject canvas,
                                           jobject renderWindow, 
                                           jobject anObject,
                                           jstring methodString)
{
  JAWT awt;
  JAWT_DrawingSurface* ds;
  jint lock;

  jclass clazz;
  jmethodID mid;
  jboolean isCopy;

  // get the render window pointer
  vtkRenderWindow *temp0;
  temp0 = (vtkRenderWindow *)(vtkJavaGetPointerFromObject(env,renderWindow,(char *) "vtkRenderWindow"));


  // convert jstring to const char*
  const char* utf_string = env->GetStringUTFChars(methodString, &isCopy);

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

  // get object class containing method
  clazz = env->GetObjectClass(anObject);

  // get void method id 
  mid = env->GetMethodID(clazz, utf_string, "()V");
  if (mid == 0)
    {
      printf("Can't get void methodID %s for object\n", utf_string);
      ds->Unlock(ds);
      awt.FreeDrawingSurface(ds);
      return;
    }

  env->CallVoidMethod(anObject, mid);

  temp0->Render();
  
  /* Unlock the drawing surface */
  ds->Unlock(ds);
  
  /* Free the drawing surface */
  awt.FreeDrawingSurface(ds);
}


