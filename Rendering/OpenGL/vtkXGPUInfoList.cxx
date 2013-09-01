/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXGPUInfoList.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXGPUInfoList.h"

#include "vtkGPUInfoListArray.h"

#include "vtkObjectFactory.h"
#include <cassert>

#include <X11/Xlib.h> // Display structure, XOpenDisplay(), XScreenCount()

#include "vtkToolkits.h"
#ifdef VTK_USE_NVCONTROL
#include "NVCtrlLib.h" // needs NVCtrl.h (NV_CTRL_VIDEO_RAM, XNVCTRLQueryAttribute() )
#endif

vtkStandardNewMacro(vtkXGPUInfoList);

// ----------------------------------------------------------------------------
// Description:
// Build the list of vtkInfoGPU if not done yet.
// \post probed: IsProbed()
void vtkXGPUInfoList::Probe()
{
  if(!this->Probed)
    {
    this->Probed=true;
    this->Array=new vtkGPUInfoListArray;
    bool found=false;

#ifdef VTK_USE_NVCONTROL
    // see sample code in nvidia-settings-1.0/samples/nv-control-info.c
    Display *dpy=XOpenDisplay(NULL); // we use the environment variable DISPLAY
    if(dpy!=NULL)
      {
      int eventBase;
      int errorBase;
      if(XNVCTRLQueryExtension(dpy,&eventBase,&errorBase)==True)
        {
        int screenCount=XScreenCount(dpy);
        int nvScreenCount=0;
        int i=0;
        while(i<screenCount)
          {
          if(XNVCTRLIsNvScreen(dpy,i))
            {
            ++nvScreenCount;
            }
          ++i;
          }
        found=nvScreenCount>0;
        if(found)
          {
          this->Array->v.resize(nvScreenCount);
          int j=0;
          i=0;
          while(i<screenCount)
            {
            if(XNVCTRLIsNvScreen(dpy,i))
              {
              int ramSize;
              Bool status=XNVCTRLQueryAttribute(dpy,i,0,
                                                NV_CTRL_VIDEO_RAM,&ramSize);
              if(!status)
                {
                ramSize=0;
                }
              vtkGPUInfo *info=vtkGPUInfo::New();
              info->SetDedicatedVideoMemory(static_cast<vtkIdType>(ramSize)*1024); // ramSize is in KB
              this->Array->v[j]=info;
              ++j;
              }
            ++i;
            }
          }
        }
      XCloseDisplay(dpy);
      }
#endif // #ifdef VTK_USE_NVCONTROL
    if(!found)
      {
      this->Array->v.resize(0); // no GPU.
      }
    }
  assert("post: probed" && this->IsProbed());
}


// ----------------------------------------------------------------------------
vtkXGPUInfoList::vtkXGPUInfoList()
{
}

// ----------------------------------------------------------------------------
vtkXGPUInfoList::~vtkXGPUInfoList()
{
}

// ----------------------------------------------------------------------------
void vtkXGPUInfoList::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
