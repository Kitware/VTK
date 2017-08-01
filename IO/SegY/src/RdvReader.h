/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlane.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef SEGYVISUALIZER2D_RDVREADER_H
#define SEGYVISUALIZER2D_RDVREADER_H


#include <vtkPolyData.h>
using namespace std;
class RdvReader {
public:
    bool Read(string path, vtkPolyData* polyData);
};


#endif //SEGYVISUALIZER2D_RDVREADER_H
