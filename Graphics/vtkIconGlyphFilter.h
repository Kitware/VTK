/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIconGlyphFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright 2004 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/

#ifndef __vtkIconGlyphFilter_h
#define __vtkIconGlyphFilter_h

#include "vtkPolyDataAlgorithm.h"


class VTK_GRAPHICS_EXPORT vtkIconGlyphFilter : public vtkPolyDataAlgorithm
{
public:

  // Description
  static vtkIconGlyphFilter *New();
  vtkTypeRevisionMacro(vtkIconGlyphFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the Width and Height, in pixels, of an icon in the icon sheet
  vtkSetVector2Macro(IconSize,int);
  vtkGetVectorMacro(IconSize,int,2);

  // Description:
  // Specify the Width and Height, in pixels, of an icon in the icon sheet
  vtkSetVector2Macro(IconSheetSize,int);
  vtkGetVectorMacro(IconSheetSize,int,2);

protected:
  vtkIconGlyphFilter();
  ~vtkIconGlyphFilter();

  virtual int RequestData(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *);

  int IconSize[2]; // Size in pixels of an icon in an icon sheet
  int IconSheetSize[2]; // Size in pixels of the icon sheet

private:
  vtkIconGlyphFilter(const vtkIconGlyphFilter&);  // Not implemented.
  void operator=(const vtkIconGlyphFilter&);  // Not implemented.

  void IconConvertIndex(int id, int & j, int & k);
};

inline void vtkIconGlyphFilter::IconConvertIndex(int id, int & j, int & k)
{
  int dimX = this->IconSheetSize[0]/this->IconSize[0];
  int dimY = this->IconSheetSize[1]/this->IconSize[1];

  j = id - dimX * (int)(id/dimX);
  k = dimY - (int)(id/dimX) - 1;
}

#endif
