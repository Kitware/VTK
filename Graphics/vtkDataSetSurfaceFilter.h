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
class vtkIdTypeArray;

//BTX
// Helper structure for hashing faces.
struct vtkFastGeomQuadStruct
{
  struct vtkFastGeomQuadStruct *Next;
  vtkIdType SourceId;
  int numPts;
  vtkIdType ptArray[4]; // actually a variable length array.  MUST be last
};
typedef struct vtkFastGeomQuadStruct vtkFastGeomQuad;
//ETX

class VTK_GRAPHICS_EXPORT vtkDataSetSurfaceFilter : public vtkPolyDataAlgorithm
{
public:
  static vtkDataSetSurfaceFilter *New();
  vtkTypeMacro(vtkDataSetSurfaceFilter,vtkPolyDataAlgorithm);
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

  // Description:
  // If on, the output polygonal dataset will have a celldata array that 
  // holds the cell index of the original 3D cell that produced each output
  // cell. This is useful for cell picking. The default is off to conserve 
  // memory. Note that PassThroughCellIds will be ignored if UseStrips is on,
  // since in that case each tringle strip can represent more than on of the
  // input cells.
  vtkSetMacro(PassThroughCellIds,int);
  vtkGetMacro(PassThroughCellIds,int);
  vtkBooleanMacro(PassThroughCellIds,int);
  vtkSetMacro(PassThroughPointIds,int);
  vtkGetMacro(PassThroughPointIds,int);
  vtkBooleanMacro(PassThroughPointIds,int);


  // Description:
  // Direct access methods that can be used to use the this class as an
  // algorithm without using it as a filter.
  virtual int StructuredExecute(vtkDataSet *input,
    vtkPolyData *output, vtkIdType *ext, vtkIdType *wholeExt);
#ifdef VTK_USE_64BIT_IDS
  virtual int StructuredExecute(vtkDataSet *input,
    vtkPolyData *output, int *ext32, int *wholeExt32)
    {
    vtkIdType ext[6]; vtkIdType wholeExt[6];
    for (int cc=0; cc < 6; cc++)
      {
      ext[cc] = ext32[cc];
      wholeExt[cc] = wholeExt32[cc];
      }
    return this->StructuredExecute(input, output, ext, wholeExt);
    }
#endif
  virtual int UnstructuredGridExecute(vtkDataSet *input, vtkPolyData *output);
  virtual int DataSetExecute(vtkDataSet *input, vtkPolyData *output);

protected:
  vtkDataSetSurfaceFilter();
  ~vtkDataSetSurfaceFilter();

  int UseStrips;
  
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);


  // Helper methods.
  void ExecuteFaceStrips(vtkDataSet *input, vtkPolyData *output,
                         int maxFlag, vtkIdType *ext,
                         int aAxis, int bAxis, int cAxis,
                         vtkIdType *wholeExt);
  void ExecuteFaceQuads(vtkDataSet *input, vtkPolyData *output,
                        int maxFlag, vtkIdType *ext,
                        int aAxis, int bAxis, int cAxis,
                        vtkIdType *wholeExt);

  void InitializeQuadHash(vtkIdType numPoints);
  void DeleteQuadHash();
  virtual void InsertQuadInHash(vtkIdType a, vtkIdType b, vtkIdType c, vtkIdType d,
                        vtkIdType sourceId);
  virtual void InsertTriInHash(vtkIdType a, vtkIdType b, vtkIdType c,
                       vtkIdType sourceId, vtkIdType faceId = -1);
  virtual void InsertPolygonInHash(vtkIdType* ids, int numpts,
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
  void InitFastGeomQuadAllocation(vtkIdType numberOfCells);
  vtkFastGeomQuad* NewFastGeomQuad(int numPts);
  void DeleteAllFastGeomQuads();
  // -----
  vtkIdType FastGeomQuadArrayLength;
  vtkIdType NumberOfFastGeomQuadArrays;
  unsigned char** FastGeomQuadArrays;  // store this data as an array of bytes
  // These indexes allow us to find the next available face.
  vtkIdType NextArrayIndex;
  vtkIdType NextQuadIndex;

  int PieceInvariant;

  int PassThroughCellIds;
  void RecordOrigCellId(vtkIdType newIndex, vtkIdType origId);
  virtual void RecordOrigCellId(vtkIdType newIndex, vtkFastGeomQuad *quad);
  vtkIdTypeArray *OriginalCellIds;

  int PassThroughPointIds;
  void RecordOrigPointId(vtkIdType newIndex, vtkIdType origId);
  vtkIdTypeArray *OriginalPointIds;

private:
  vtkDataSetSurfaceFilter(const vtkDataSetSurfaceFilter&);  // Not implemented.
  void operator=(const vtkDataSetSurfaceFilter&);  // Not implemented.
};

#endif
