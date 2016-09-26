/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCocoaTkUtilities.mm

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef MAC_OSX_TK
#define MAC_OSX_TK 1
#endif

#import <Cocoa/Cocoa.h>
#import "vtkCocoaMacOSXSDKCompatibility.h" // Needed to support old SDKs

#import "vtkCocoaTkUtilities.h"
#import "vtkObjectFactory.h"
#import "vtkTcl.h"
#import "tkMacOSXInt.h"

vtkStandardNewMacro(vtkCocoaTkUtilities);

void vtkCocoaTkUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// Getting an NSView from a Tk widget is strictly internal to Tk, so we
// have to duplicate that functionality here.  Hopefully this will be
// included in the distributed PrivateHeaders in later releases of Tk.
void* vtkCocoaTkUtilities::GetDrawableView(Tk_Window window)
{
  MacDrawable *macWin = reinterpret_cast<TkWindow *>(window)->privatePtr;

  if (!macWin)
  {
    return nil;
  }
  else if (macWin->toplevel && (macWin->toplevel->flags & TK_EMBEDDED))
  {
    // can't handle embedded window, but not sure if this will ever happen
    return nil;
  }
  else if (macWin->toplevel)
  {
    macWin = macWin->toplevel;
  }

  TkMacOSXMakeRealWindowExist(macWin->winPtr);
  NSView *result = macWin->view;

  return reinterpret_cast<void *>(result);
}
