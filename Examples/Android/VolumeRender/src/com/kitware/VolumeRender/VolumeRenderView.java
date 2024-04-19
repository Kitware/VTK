// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (C) 2008-2009 The Android Open Source Project
// SPDX-License-Identifier: BSD-3-Clause AND Apache-2.0
package com.kitware.VolumeRender;

import android.content.Context;
import android.graphics.PixelFormat;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.opengles.GL10;

/**
 * A simple GLSurfaceView sub-class that demonstrate how to perform
 * OpenGL ES 2.0 rendering into a GL Surface. Note the following important
 * details:
 *
 * - The class must use a custom context factory to enable 2.0 rendering.
 *   See ContextFactory class definition below.
 *
 * - The class must use a custom EGLConfigChooser to be able to select
 *   an EGLConfig that supports 2.0. This is done by providing a config
 *   specification to eglChooseConfig() that has the attribute
 *   EGL10.ELG_RENDERABLE_TYPE containing the EGL_OPENGL_ES2_BIT flag
 *   set. See ConfigChooser class definition below.
 *
 * - The class must select the surface's format, then choose an EGLConfig
 *   that matches it exactly (with regards to red/green/blue/alpha channels
 *   bit depths). Failure to do so would result in an EGL_BAD_MATCH error.
 */
class VolumeRenderView extends GLSurfaceView
{
    private static String TAG = "VolumeRenderView";
    private Renderer myRenderer;

    public VolumeRenderView(Context context)
    {
        super(context);
        setFocusable(true);
        setFocusableInTouchMode(true);

        setEGLContextClientVersion(3);

        /* Set the renderer responsible for frame rendering */
        this.myRenderer = new Renderer();
        this.setRenderer(myRenderer);
        this.setRenderMode(RENDERMODE_WHEN_DIRTY);
    }

    private static void checkEglError(String prompt, EGL10 egl)
    {
        int error;
        while ((error = egl.eglGetError()) != EGL10.EGL_SUCCESS)
            {
            Log.e(TAG, String.format("%s: EGL error: 0x%x", prompt, error));
            }
    }

    private static class Renderer implements GLSurfaceView.Renderer
    {
        private long vtkContext;

        public void onDrawFrame(GL10 gl)
        {
            VolumeRenderLib.render(vtkContext);
        }

        // forward events to VTK for it to handle
        public void onKeyEvent(boolean down, KeyEvent ke)
        {
            VolumeRenderLib.onKeyEvent(vtkContext, down, ke.getKeyCode(),
                ke.getMetaState(),
                ke.getRepeatCount());
        }

        // forward events to VTK for it to handle
        public void onMotionEvent(final MotionEvent me)
        {
            try {
                int numPtrs = me.getPointerCount();
                float [] xPos = new float[numPtrs];
                float [] yPos = new float[numPtrs];
                int [] ids = new int[numPtrs];
                for (int i = 0; i < numPtrs; ++i)
                    {
                        ids[i] = me.getPointerId(i);
                        xPos[i] = me.getX(i);
                        yPos[i] = me.getY(i);
                    }

                int actionIndex = me.getActionIndex();
                int actionMasked = me.getActionMasked();
                int actionId = me.getPointerId(actionIndex);

                if (actionMasked != 2)
                {
                    Log.e(TAG, "Got action " + actionMasked);
                }
                VolumeRenderLib.onMotionEvent(vtkContext,
                                         actionMasked,
                                         actionId,
                                         numPtrs, xPos, yPos, ids,
                                         me.getMetaState());
            } catch (IllegalArgumentException e) {
                Log.e(TAG, "Bogus motion event");
            }
        }

        public void onSurfaceChanged(GL10 gl, int width, int height)
        {
            vtkContext = VolumeRenderLib.init(width, height);
        }

        public void onSurfaceCreated(GL10 gl, EGLConfig config)
        {
            // Do nothing.
        }
    }

    // forward events to rendering thread for it to handle
    public boolean onKeyUp(int keyCode, KeyEvent event)
    {
        final KeyEvent keyEvent = event;
        queueEvent(new Runnable()
            {
            public void run()
                {
                myRenderer.onKeyEvent(false, keyEvent);
                }
            });
        return true;
    }

    // forward events to rendering thread for it to handle
    public boolean onKeyDown(int keyCode, KeyEvent event)
    {
        final KeyEvent keyEvent = event;
        queueEvent(new Runnable()
            {
            public void run()
                {
                myRenderer.onKeyEvent(true, keyEvent);
                }
            });
        return true;
    }

    // forward events to rendering thread for it to handle
    public boolean onTouchEvent(MotionEvent event)
    {
        final MotionEvent motionEvent = event;
        queueEvent(new Runnable()
            {
            public void run()
                {
                myRenderer.onMotionEvent(motionEvent);
                }
            });
        return true;
    }
}
