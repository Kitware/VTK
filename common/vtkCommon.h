/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCommon.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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

#include "vtkAGraymap.h"
#include "vtkAPixmap.h"
#include "vtkBitArray.h"
#include "vtkBitScalars.h"
#include "vtkBitmap.h"
#include "vtkByteSwap.h"
#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkCellList.h"
#include "vtkCollection.h"
#include "vtkColorScalars.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkEdgeTable.h"
#include "vtkFloatArray.h"
#include "vtkFloatNormals.h"
#include "vtkFloatPoints.h"
#include "vtkFloatScalars.h"
#include "vtkFloatTCoords.h"
#include "vtkFloatTensors.h"
#include "vtkFloatVectors.h"
#include "vtkGraymap.h"
#include "vtkHexahedron.h"
#include "vtkIdList.h"
#include "vtkImageCache.h"
#include "vtkImageCachedSource.h"
#include "vtkImageData.h"
#include "vtkImageRegion.h"
#include "vtkImageSimpleCache.h"
#include "vtkImageSource.h"
#include "vtkImplicitFunction.h"
#include "vtkIndent.h"
#include "vtkIntArray.h"
#include "vtkIntPoints.h"
#include "vtkIntScalars.h"
#include "vtkLine.h"
#include "vtkLinkList.h"
#include "vtkLocator.h"
#include "vtkLogLookupTable.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkNormals.h"
#include "vtkObject.h"
#include "vtkPixel.h"
#include "vtkPixmap.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkPointLocator.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyLine.h"
#include "vtkPolyVertex.h"
#include "vtkPolygon.h"
#include "vtkQuad.h"
#include "vtkReferenceCount.h"
#include "vtkScalars.h"
#include "vtkShortArray.h"
#include "vtkShortScalars.h"
#include "vtkSource.h"
#include "vtkStack.h"
#include "vtkStructuredData.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredPoints.h"
#include "vtkTCoords.h"
#include "vtkTensors.h"
#include "vtkTetra.h"
#include "vtkTimeStamp.h"
#include "vtkTransform.h"
#include "vtkTriangle.h"
#include "vtkTriangleStrip.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedCharScalars.h"
#include "vtkUnsignedShortArray.h"
#include "vtkUnsignedShortScalars.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUserDefined.h"
#include "vtkVectors.h"
#include "vtkVertex.h"
#include "vtkVoidArray.h"
#include "vtkVoxel.h"
#include "vtkWindowLevelLookupTable.h"
