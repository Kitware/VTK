/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeProVG500Mapper.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkVolumeProVG500Mapper - Superclass for VG500 board
//
// .SECTION Description
// vtkVolumeProVG500Mapper is the superclass for VolumePRO volume rendering 
// mappers based on the VG500 chip. Subclasses are for underlying graphics 
// languages. Users should not create subclasses directly - 
// a vtkVolumeProMapper will automatically create the object of the right 
// type.
//
// This class is not included in the contrib Makefile.in by default. If you
// want to add this class to your vtk build, you need to have the vli header
// and library files, and you will need to perform the following steps:
//
// 1. Make sure you are building with the contrib kit. On Unix add
//    --with-contrib to your configure line, on Windows check the
//   contrib box on PCMaker.
//
// 2. Edit the Makefile.in in contrib. Add the following three classes to
//    the CONCRETE list of classes: vtkVolumeProMapper, 
//    vtkVolumeProVG500Mapper, vtkOpenGLVolumeProVG500Mapper. Please be
//    certain that there are no spaces after the "\" that separates lines.
//
// 3. Either edit vtkVolumeProMapper.h and specify the include path
//    for vli.h, or copy the vli.h file to your contrib directory.
//
// 4. On Windows - add the vli.lib file to the Extra Linker Flags under
//    the Advanced Options. On Unix - add the vli shared object to the
//    KIT_LIBS in the Makefile.in in contrib.
//  
// 5. On Windows - make sure vli.dll is somewhere in your path before you
//    run vtk. You can put it in your vtkbin/lib or vtkbin/Debug/lib if
//    you want. On Unix - make sure the vli shared object is in your
//    shared library path before you run.
//
// 6. Reconfigure and rebuild vtk. You should now be able to create a
//    vtkVolumeProMapper which, if you have a VolumePRO board and the
//    device driver is running, should connect to the hardware and render
//    your volumes quickly.
//
// For more information on the VolumePRO hardware, please see:
//
//   http://www.3dvolumegraphics.com/3dvolumegraphics/product/index.htm
//
// If you encouter any problems with this class, please inform Kitware, Inc.
// at kitware@kitware.com.
//
//
// .SECTION See Also
// vtkVolumeMapper vtkVolumeProMapper vtkOpenGLVolumeProVG500Mapper
//

#ifndef __vtkVolumeProVG500Mapper_h
#define __vtkVolumeProVG500Mapper_h

#include "vtkVolumeProMapper.h"

class VTK_EXPORT vtkVolumeProVG500Mapper : public vtkVolumeProMapper
{
public:
  const char *GetClassName() {return "vtkVolumeProVG500Mapper";};
  static vtkVolumeProVG500Mapper *New();

protected:
  vtkVolumeProVG500Mapper() {};
  ~vtkVolumeProVG500Mapper() {};
};


#endif



