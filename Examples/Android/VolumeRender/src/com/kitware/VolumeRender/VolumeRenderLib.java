/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
