/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCocoaWindow.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCocoaWindow - Access Cocoa NSWindow context
//
// .SECTION Description
// This class is a subclass of Cocoa's NSWindow; it uses Objective-C++.
// This class overrides NSWindow's close method to quit the application
// when the close box is pressed by the user. A vtkCocoaWindow is created
// automatically by vtk by default, but the programmer can override this
// behaviour, see vtkCocoaRenderWindow's SetWindowId().
// Note that this class previously contained much more functionality
// but that functionality has been moved to other classes.
 

// .SECTION See Also
// vtkCocoaGLView vtkCocoaRenderWindow

#import <Cocoa/Cocoa.h>

@interface vtkCocoaWindow : NSWindow
{
}

- (void)close;
@end
