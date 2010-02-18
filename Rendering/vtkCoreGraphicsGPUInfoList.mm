//=============================================================================
//   This file is part of VTKEdge. See vtkedge.org for more information.
//
//   Copyright (c) 2008 Kitware, Inc.
//
//   VTKEdge may be used under the terms of the GNU General Public License 
//   version 3 as published by the Free Software Foundation and appearing in 
//   the file LICENSE.txt included in the top level directory of this source
//   code distribution. Alternatively you may (at your option) use any later 
//   version of the GNU General Public License if such license has been 
//   publicly approved by Kitware, Inc. (or its successors, if any).
//
//   VTKEdge is distributed "AS IS" with NO WARRANTY OF ANY KIND, INCLUDING
//   THE WARRANTIES OF DESIGN, MERCHANTABILITY, AND FITNESS FOR A PARTICULAR
//   PURPOSE. See LICENSE.txt for additional details.
//
//   VTKEdge is available under alternative license terms. Please visit
//   vtkedge.org or contact us at kitware@kitware.com for further information.
//
//=============================================================================
#import "vtkKWECoreGraphicsGPUInfoList.h"

#include "vtkKWEGPUInfoListArray.h"
#include "vtkObjectFactory.h"
#include <assert.h>

//#import "CGDirectDisplay.h" // for CGGetActiveDisplayList()
#import <ApplicationServices/ApplicationServices.h>

vtkCxxRevisionMacro(vtkKWECoreGraphicsGPUInfoList, "1.1");
vtkStandardNewMacro(vtkKWECoreGraphicsGPUInfoList);

// ----------------------------------------------------------------------------
// Description:
// Build the list of vtkKWEInfoGPU if not done yet.
// \post probed: IsProbed()
void vtkKWECoreGraphicsGPUInfoList::Probe()
{
  if(!this->Probed)
    {
    this->Probed=true;
    this->Array=new vtkKWEGPUInfoListArray;
    
    CGError err=CGDisplayNoErr;
    CGDirectDisplayID *displays=NULL;
    CGDisplayCount dspCount=0;
    // How many active displays do we have?
    err=CGGetActiveDisplayList(0,NULL,&dspCount);
    if(dspCount>0)
      {
      // Allocate enough memory to hold all the display IDs we have
      displays=static_cast<CGDirectDisplayID *>(calloc(static_cast<size_t>(dspCount),sizeof(CGDirectDisplayID)));
      // Get the list of active displays
      err=CGGetActiveDisplayList(dspCount,
                                 displays,
                                 &dspCount);
      
      size_t c=dspCount; // there are `c' GPUS.
      this->Array->v.resize(c);
      size_t i=0;
      while(i<c)
        {
        vtkKWEGPUInfo *info=vtkKWEGPUInfo::New();
        this->Array->v[i]=info;
        
        io_service_t dspPort=CGDisplayIOServicePort(displays[i]);
        
        // Ask IOKit for the VRAM size property
        CFTypeRef typeCode=
          IORegistryEntryCreateCFProperty(dspPort,
                                          CFSTR(kIOFBMemorySizeKey),
                                          kCFAllocatorDefault,
                                          kNilOptions);
        
        // Ensure we have valid data from IOKit
        if(typeCode!=0 && CFGetTypeID(typeCode) == CFNumberGetTypeID())
          {
          // If so, convert the CFNumber into a plain unsigned long
          long ramSize;
          CFNumberGetValue(static_cast<const __CFNumber *>(typeCode), kCFNumberSInt32Type,&ramSize);
          info->SetDedicatedVideoMemory(ramSize);
          }
        if(typeCode)
          {
          CFRelease(typeCode);
          }
        ++i;
        }
      free(displays);
      }
    else
      {
      this->Array->v.resize(0);
      }
    }
  assert("post: probed" && this->IsProbed());
}

// ----------------------------------------------------------------------------
vtkKWECoreGraphicsGPUInfoList::vtkKWECoreGraphicsGPUInfoList()
{
}

// ----------------------------------------------------------------------------
vtkKWECoreGraphicsGPUInfoList::~vtkKWECoreGraphicsGPUInfoList()
{  
}

// ----------------------------------------------------------------------------
void vtkKWECoreGraphicsGPUInfoList::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
