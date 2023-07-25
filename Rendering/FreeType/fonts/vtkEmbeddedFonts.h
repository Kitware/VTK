// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkEmbeddedFonts_h
#define vtkEmbeddedFonts_h

#include "vtkABINamespace.h"

#include <stddef.h>

// -----------------------------------------------------------------------
// VTK: Arial Normal
// Gothic L Book
// Contributed by URW

VTK_ABI_NAMESPACE_BEGIN
extern size_t face_arial_buffer_length;
extern unsigned char face_arial_buffer[];

// VTK: Arial Bold
// Gothic L Demi
// Contributed by URW

extern size_t face_arial_bold_buffer_length;
extern unsigned char face_arial_bold_buffer[];

// VTK: Arial Bold Italic
// Gothic L Demi Oblique
// Contributed by URW

extern size_t face_arial_bold_italic_buffer_length;
extern unsigned char face_arial_bold_italic_buffer[];

// VTK: Arial Italic
// Gothic L Book Oblique
// Contributed by URW

extern size_t face_arial_italic_buffer_length;
extern unsigned char face_arial_italic_buffer[];

// -----------------------------------------------------------------------
// VTK: Courier Normal
// Courier 10 Pitch (c0419bt_.pfb)
// Contributed by Bitstream (XFree86)

extern size_t face_courier_buffer_length;
extern unsigned char face_courier_buffer[];

// VTK: Courier Bold
// Courier 10 Pitch Bold (c0583bt_.pfb)
// Contributed by Bitstream (XFree86)

extern size_t face_courier_bold_buffer_length;
extern unsigned char face_courier_bold_buffer[];

// VTK: Courier Bold Italic
// Courier 10 Pitch Bold Italic (c0611bt_.pfb)
// Contributed by Bitstream (XFree86)

extern size_t face_courier_bold_italic_buffer_length;
extern unsigned char face_courier_bold_italic_buffer[];

// VTK: Courier Italic
// Courier 10 Pitch Regular Italic (c0582bt_.pfb)
// Contributed by Bitstream (XFree86)

extern size_t face_courier_italic_buffer_length;
extern unsigned char face_courier_italic_buffer[];

// -----------------------------------------------------------------------
// VTK: Times Normal
// Bitstream Charter (c0648bt_.pfb)
// Contributed by Bitstream (XFree86)

extern size_t face_times_buffer_length;
extern unsigned char face_times_buffer[];

// VTK: Times Bold
// Bitstream Charter Bold (c0632bt_.pfb)
// Contributed by Bitstream (XFree86)

extern size_t face_times_bold_buffer_length;
extern unsigned char face_times_bold_buffer[];

// VTK: Times Bold Italic
// Bitstream Charter Bold Italic (c0633bt_.pfb)
// Contributed by Bitstream (XFree86)

extern size_t face_times_bold_italic_buffer_length;
extern unsigned char face_times_bold_italic_buffer[];

// VTK: Times Italic
// Bitstream Charter Regular Italic (c0649bt_.pfb)
// Contributed by Bitstream (XFree86)

extern size_t face_times_italic_buffer_length;
extern unsigned char face_times_italic_buffer[];

VTK_ABI_NAMESPACE_END
#endif
