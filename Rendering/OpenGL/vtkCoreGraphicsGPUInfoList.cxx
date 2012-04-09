/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCoreGraphicsGPUInfoList.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCoreGraphicsGPUInfoList.h"

#include "vtkGPUInfoListArray.h"
#include "vtkObjectFactory.h"

#include <assert.h>
#include <ApplicationServices/ApplicationServices.h>

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
        
        // Note: the QA1168 Apple sample code is wrong as it uses
        // kIOFBMemorySizeKey. Also it does not work in 64-bit because it
        // used "long".
        // Our method is to get the value of property "VRAM,totalsize"
        // We cannot (yet) distinguish between dedicated video memory 
        // (for example 512MB for a nVidia GeForce 9600M GT) and
        // dedicated system memory (for example 256MB for a nVidia GeForce
        // 9400M).
        
        // Look for property
        CFTypeRef typeCode = IORegistryEntrySearchCFProperty(
          dspPort,kIOServicePlane,CFSTR("VRAM,totalsize"),kCFAllocatorDefault,
          kIORegistryIterateRecursively | kIORegistryIterateParents);
        
        if(typeCode!=0)
          {
          if(CFGetTypeID(typeCode)==CFDataGetTypeID())
            {
            // Get the property and interpret it.
            const UInt8 *v=CFDataGetBytePtr(static_cast<CFDataRef>(typeCode));
            int ramSize=*(reinterpret_cast<const int *>(v));
            info->SetDedicatedVideoMemory(ramSize);
            }
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
