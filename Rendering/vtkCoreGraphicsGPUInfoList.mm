/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCoreGraphicsGPUInfoList.mm

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#import "vtkCoreGraphicsGPUInfoList.h"

#include "vtkGPUInfoListArray.h"
#include "vtkObjectFactory.h"
#include <assert.h>

//#import "CGDirectDisplay.h" // for CGGetActiveDisplayList()
#import <ApplicationServices/ApplicationServices.h>

vtkCxxRevisionMacro(vtkCoreGraphicsGPUInfoList, "1.2");
vtkStandardNewMacro(vtkCoreGraphicsGPUInfoList);

// ----------------------------------------------------------------------------
// Description:
// Build the list of vtkInfoGPU if not done yet.
// \post probed: IsProbed()
void vtkCoreGraphicsGPUInfoList::Probe()
{
  if(!this->Probed)
    {
    this->Probed=true;
    this->Array=new vtkGPUInfoListArray;
    
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
        vtkGPUInfo *info=vtkGPUInfo::New();
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
vtkCoreGraphicsGPUInfoList::vtkCoreGraphicsGPUInfoList()
{
}

// ----------------------------------------------------------------------------
vtkCoreGraphicsGPUInfoList::~vtkCoreGraphicsGPUInfoList()
{  
}

// ----------------------------------------------------------------------------
void vtkCoreGraphicsGPUInfoList::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
