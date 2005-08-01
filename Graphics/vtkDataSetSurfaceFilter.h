/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetSurfaceFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDataSetSurfaceFilter - Extracts outer (polygonal) surface.
// .SECTION Description
// vtkDataSetSurfaceFilter is a faster version of vtkGeometry filter, but it 
// does not have an option to select bounds.  It may use more memory than
// vtkGeometryFilter.  It only has one option: whether to use triangle strips 
// when the input type is structured.

// .SECTION See Also
// vtkGeometryFilter vtkStructuredGridGeometryFilter.

#ifndef __vtkDataSetSurfaceFilter_h
#define __vtkDataSetSurfaceFilter_h

#include "vtkPolyDataAlgorithm.h"


class vtkPointData;
class vtkPoints;
//BTX
struct vtkFastGeomQuadStruct;
typedef struct vtkFastGeomQuadStruct vtkFastGeomQuad;
//ETX

class VTK_GRAPHICS_EXPORT vtkDataSetSurfaceFilter : public vtkPolyDataAlgorithm
{
public:
  static vtkDataSetSurfaceFilter *New();
  vtkTypeRevisionMacro(vtkDataSetSurfaceFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // When input is structured data, this flag will generate faces with
  // triangle strips.  This should render faster and use less memory, but no
  // cell data is copied.  By default, UseStrips is Off.
  vtkSetMacro(UseStrips, int);
  vtkGetMacro(UseStrips, int);
  vtkBooleanMacro(UseStrips, int);

  // Description:
  // If PieceInvariant is true, vtkDataSetSurfaceFilter requests
  // 1 ghost level from input in order to remove internal surface
  // that are between processes. False by default.
  vtkSetMacro(PieceInvariant, int);
  vtkGetMacro(PieceInvariant, int);

protected:
  vtkDataSetSurfaceFilter();
  ~vtkDataSetSurfaceFilter();

  int UseStrips;
  
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);
  int StructuredExecute(vtkDataSet *input, vtkPolyData *output, int *ext,
                         vtkInformation *inInfo);
  int UnstructuredGridExecute(vtkDataSet *input, vtkPolyData *output);
  int DataSetExecute(vtkDataSet *input, vtkPolyData *output);

  // Helper methods.
  void ExecuteFaceStrips(vtkDataSet *input, vtkPolyData *output,
                         int maxFlag, int *ext,
                         int aAxis, int bAxis, int cAxis,
                         vtkInformation *inInfo);
  void ExecuteFaceQuads(vtkDataSet *input, vtkPolyData *output,
                        int maxFlag, int *ext,
                        int aAxis, int bAxis, int cAxis,
                        vtkInformation *inInfo);

  void InitializeQuadHash(vtkIdType numPoints);
  void DeleteQuadHash();
  void InsertQuadInHash(vtkIdType a, vtkIdType b, vtkIdType c, vtkIdType d,
                        vtkIdType sourceId);
  void InsertTriInHash(vtkIdType a, vtkIdType b, vtkIdType c,
                       vtkIdType sourceId);
  void InsertPentaInHash(vtkIdType a, vtkIdType b,
                       vtkIdType c, vtkIdType d,
                       vtkIdType e, 
                       vtkIdType sourceId);
  void InsertHexInHash(vtkIdType a, vtkIdType b,
                       vtkIdType c, vtkIdType d,
                       vtkIdType e, vtkIdType f,
                       vtkIdType sourceId);
  void InitQuadHashTraversal();
  vtkFastGeomQuad *GetNextVisibleQuadFromHash();

  vtkFastGeomQuad **QuadHash;
  vtkIdType QuadHashLength;
  vtkFastGeomQuad *QuadHashTraversal;
  vtkIdType QuadHashTraversalIndex;

  vtkIdType *PointMap;
  vtkIdType GetOutputPointId(vtkIdType inPtId, vtkDataSet *input, 
                             vtkPoints *outPts, vtkPointData *outPD);
  
  vtkIdType NumberOfNewCells;
  
  // Better memory allocation for faces (hash)
  void InitFastGeomQuadAllocation(int numberOfCells);
  vtkFastGeomQuad* NewFastGeomQuad();
  void DeleteAllFastGeomQuads();
  // -----
  int FastGeomQuadArrayLength;
  int NumberOfFastGeomQuadArrays;
  vtkFastGeomQuad** FastGeomQuadArrays;
  // These indexes allow us to find the next available face.
  int NextArrayIndex;
  int NextQuadIndex;

  int PieceInvariant;

private:
  vtkDataSetSurfaceFilter(const vtkDataSetSurfaceFilter&);  // Not implemented.
  void operator=(const vtkDataSetSurfaceFilter&);  // Not implemented.
};

#endif
