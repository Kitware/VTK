/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEmbeddedFonts.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkEmbeddedFonts_h
#define __vtkEmbeddedFonts_h

#include <stddef.h>

// -----------------------------------------------------------------------
// VTK: Arial Normal
// Nimbus Sans L Regular
// n019003l.pfb -URW-Nimbus Sans L-medium-r-normal--0-0-0-0-p-0-iso8859-1
// Contributed by URW

extern size_t face_arial_buffer_length;
extern unsigned char face_arial_buffer[];

// VTK: Arial Bold
// Nimbus Sans L Bold
// n019004l.pfb -URW-Nimbus Sans L-bold-r-normal--0-0-0-0-p-0-iso8859-1
// Contributed by URW

extern size_t face_arial_bold_buffer_length;
extern unsigned char face_arial_bold_buffer[];

// VTK: Arial Bold Italic
// Nimbus Sans L Bold Italic
// n019024l.pfb -URW-Nimbus Sans L-bold-o-normal--0-0-0-0-p-0-iso8859-1
// Contributed by URW

extern size_t face_arial_bold_italic_buffer_length;
extern unsigned char face_arial_bold_italic_buffer[];

// VTK: Arial Italic
// Nimbus Sans L Regular Italic
// n019023l.pfb -URW-Nimbus Sans L-medium-o-normal--0-0-0-0-p-0-iso8859-1
// Contributed by URW

extern size_t face_arial_italic_buffer_length;
extern unsigned char face_arial_italic_buffer[];

// -----------------------------------------------------------------------
// VTK: Courier Normal
// Courier 10 Pitch
// c0419bt_.pfb -bitstream-courier-medium-r-normal--0-0-0-0-m-0-iso8859-1
// Contributed by Bitstream (XFree86)

extern size_t face_courier_buffer_length;
extern unsigned char face_courier_buffer[];

// VTK: Courier Bold
// Courier 10 Pitch Bold
// c0583bt_.pfb -bitstream-courier-bold-r-normal--0-0-0-0-m-0-iso8859-1
// Contributed by Bitstream (XFree86)

extern size_t face_courier_bold_buffer_length;
extern unsigned char face_courier_bold_buffer[];

// VTK: Courier Bold Italic
// Courier 10 Pitch Bold Italic
// c0611bt_.pfb -bitstream-courier-bold-i-normal--0-0-0-0-m-0-iso8859-1
// Contributed by Bitstream (XFree86)

extern size_t face_courier_bold_italic_buffer_length;
extern unsigned char face_courier_bold_italic_buffer[];

// VTK: Courier Italic
// Courier 10 Pitch Regular Italic
// c0582bt_.pfb -bitstream-courier-medium-i-normal--0-0-0-0-m-0-iso8859-1
// Contributed by Bitstream (XFree86)

extern size_t face_courier_italic_buffer_length;
extern unsigned char face_courier_italic_buffer[];

// -----------------------------------------------------------------------
// VTK: Times Normal
// Bitstream Charter
// c0648bt_.pfb -bitstream-charter-medium-r-normal--0-0-0-0-p-0-iso8859-1
// Contributed by Bitstream (XFree86)

extern size_t face_times_buffer_length;
extern unsigned char face_times_buffer[];

// VTK: Times Bold
// Bitstream Charter Bold
// c0632bt_.pfb -bitstream-charter-bold-r-normal--0-0-0-0-p-0-iso8859-1
// Contributed by Bitstream (XFree86)

extern size_t face_times_bold_buffer_length;
extern unsigned char face_times_bold_buffer[];

// VTK: Times Bold Italic
// Bitstream Charter Bold Italic
// c0633bt_.pfb -bitstream-charter-bold-i-normal--0-0-0-0-p-0-iso8859-1
// Contributed by Bitstream (XFree86)

extern size_t face_times_bold_italic_buffer_length;
extern unsigned char face_times_bold_italic_buffer[];

// VTK: Times Italic
// Bitstream Charter Regular Italic
// c0649bt_.pfb -bitstream-charter-medium-i-normal--0-0-0-0-p-0-iso8859-1
// Contributed by Bitstream (XFree86)

extern size_t face_times_italic_buffer_length;
extern unsigned char face_times_italic_buffer[];

#endif
