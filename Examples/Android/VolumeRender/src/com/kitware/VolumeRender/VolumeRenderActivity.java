// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (C) 2007 The Android Open Source Project
// SPDX-License-Identifier: BSD-3-Clause AND Apache-2.0
package com.kitware.VolumeRender;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.WindowManager;

import java.io.File;


public class VolumeRenderActivity extends Activity
{
    VolumeRenderView mView;

    @Override protected void onCreate(Bundle icicle)
    {
    super.onCreate(icicle);
    mView = new VolumeRenderView(getApplication());
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
