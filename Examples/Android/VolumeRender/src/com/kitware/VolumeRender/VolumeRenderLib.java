// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (C) 2007 The Android Open Source Project
// SPDX-License-Identifier: BSD-3-Clause
package com.kitware.VolumeRender;
import android.view.KeyEvent;


// Wrapper for native library

public class VolumeRenderLib
{

    static
    {
      System.loadLibrary("VolumeRender");
    }

    /**
     * @param width the current view width
     * @param height the current view height
     */
     public static native long init(int width, int height);
     public static native void render(long udp);
     public static native void onKeyEvent(long udp, boolean down, int keyCode,
                                          int metaState,
                                          int repeatCount);
     public static native void onMotionEvent(long udp,
        int action,
        int eventPointer,
        int numPtrs,
        float [] xPos, float [] yPos, int [] ids,
        int metaState);
}
