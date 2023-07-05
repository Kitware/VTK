// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (C) 2010 The Android Open Source Project
// SPDX-License-Identifier: BSD-3-Clause AND Apache-2.0
package com.kitware.JavaVTK;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.WindowManager;

import java.io.File;


public class JavaVTKActivity extends Activity
{
    JavaVTKView mView;

    @Override protected void onCreate(Bundle icicle)
    {
        super.onCreate(icicle);
        mView = new JavaVTKView(getApplication());
        this.setContentView(mView);
    }

    @Override protected void onPause()
    {
        super.onPause();
        this.mView.onPause();
    }

    @Override protected void onResume()
    {
        super.onResume();
        this.mView.onResume();
    }
}
