/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkJavaUtil.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/

#include <string.h>
#include <jni.h>

extern JNIEXPORT void vtkJavaAddObjectToHash(JNIEnv *env,jobject obj,void *anInstance,
				   void *tcFunc, int);
extern JNIEXPORT void *vtkJavaGetPointerFromObject(JNIEnv *env,jobject obj, 
					 char *result_type);
extern JNIEXPORT void vtkJavaDeleteObjectFromHash(JNIEnv *env, int id);
extern JNIEXPORT jobject vtkJavaGetObjectFromPointer(void *ptr);
extern JNIEXPORT int  vtkJavaShouldIDeleteObject(JNIEnv *env,jobject obj);
extern JNIEXPORT char *vtkJavaUTFToChar(JNIEnv *env, jstring in);
extern JNIEXPORT jstring vtkJavaMakeJavaString(JNIEnv *env, const char *in);

extern JNIEXPORT jarray vtkJavaMakeJArrayOfDoubleFromFloat(JNIEnv *env,
						 float *arr, int size);
extern JNIEXPORT jarray vtkJavaMakeJArrayOfDoubleFromDouble(JNIEnv *env, 
						  double *arr, int size);
extern JNIEXPORT jarray vtkJavaMakeJArrayOfIntFromInt(JNIEnv *env, int *arr, int size);

// this is the void pointer parm passed to the vtk callback routines on
// behalf of the Java interface for callbacks.
struct vtkJavaVoidFuncArg 
{
  JNIEnv * uenv;
  jobject  uobj;
  jmethodID mid;
} ;

extern JNIEXPORT void vtkJavaVoidFunc(void *);
extern JNIEXPORT void vtkJavaVoidFuncArgDelete(void *);
