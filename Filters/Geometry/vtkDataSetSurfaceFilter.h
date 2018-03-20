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
/**
 * @class   vtkDataSetSurfaceFilter
 * @brief   Extracts outer (polygonal) surface.
 *
 * vtkDataSetSurfaceFilter is a faster version of vtkGeometry filter, but it
 * does not have an option to select bounds.  It may use more memory than
 * vtkGeometryFilter.  It only has one option: whether to use triangle strips
 * when the input type is structured.
 *
 * @sa
 * vtkGeometryFilter vtkStructuredGridGeometryFilter.
*/

#ifndef vtkDataSetSurfaceFilter_h
#define vtkDataSetSurfaceFilter_h

#include "vtkFiltersGeometryModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkPointData;
class vtkPoints;
class vtkIdTypeArray;
class vtkStructuredGrid;

// Helper structure for hashing faces.
struct vtkFastGeomQuadStruct
{
  struct vtkFastGeomQuadStruct *Next;
  vtkIdType SourceId;
  int numPts;
  vtkIdType* ptArray;
};
typedef struct vtkFastGeomQuadStruct vtkFastGeomQuad;

class VTKFILTERSGEOMETRY_EXPORT vtkDataSetSurfaceFilter : public vtkPolyDataAlgorithm
{
public:
  static vtkDataSetSurfaceFilter *New();
  vtkTypeMacro(vtkDataSetSurfaceFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * When input is structured data, this flag will generate faces with
   * triangle strips.  This should render faster and use less memory, but no
   * cell data is copied.  By default, UseStrips is Off.
   */
  vtkSetMacro(UseStrips, vtkTypeBool);
  vtkGetMacro(UseStrips, vtkTypeBool);
  vtkBooleanMacro(UseStrips, vtkTypeBool);
  //@}

  //@{
  /**
   * If PieceInvariant is true, vtkDataSetSurfaceFilter requests
   * 1 ghost level from input in order to remove internal surface
   * that are between processes. False by default.
   */
  vtkSetMacro(PieceInvariant, int);
  vtkGetMacro(PieceInvariant, int);
  //@}

  //@{
  /**
   * If on, the output polygonal dataset will have a celldata array that
   * holds the cell index of the original 3D cell that produced each output
   * cell. This is useful for cell picking. The default is off to conserve
   * memory. Note that PassThroughCellIds will be ignored if UseStrips is on,
   * since in that case each tringle strip can represent more than on of the
   * input cells.
   */
  vtkSetMacro(PassThroughCellIds,vtkTypeBool);
  vtkGetMacro(PassThroughCellIds,vtkTypeBool);
  vtkBooleanMacro(PassThroughCellIds,vtkTypeBool);
  vtkSetMacro(PassThroughPointIds,vtkTypeBool);
  vtkGetMacro(PassThroughPointIds,vtkTypeBool);
  vtkBooleanMacro(PassThroughPointIds,vtkTypeBool);
  //@}

  //@{
  /**
   * If PassThroughCellIds or PassThroughPointIds is on, then these ivars
   * control the name given to the field in which the ids are written into.  If
   * set to nullptr, then vtkOriginalCellIds or vtkOriginalPointIds (the default)
   * is used, respectively.
   */
  vtkSetStringMacro(OriginalCellIdsName);
  virtual const char *GetOriginalCellIdsName()
  {
    return (  this->OriginalCellIdsName
              ? this->OriginalCellIdsName : "vtkOriginalCellIds");
  }
  vtkSetStringMacro(OriginalPointIdsName);
  virtual const char *GetOriginalPointIdsName()
  {
    return (  this->OriginalPointIdsName
            ? this->OriginalPointIdsName : "vtkOriginalPointIds");
  }
  //@}

  //@{
  /**
   * If the input is an unstructured grid with nonlinear faces, this parameter
   * determines how many times the face is subdivided into linear faces.  If 0,
   * the output is the equivalent of its linear counterpart (and the midpoints
   * determining the nonlinear interpolation are discarded).  If 1 (the
   * default), the nonlinear face is triangulated based on the midpoints.  If
   * greater than 1, the triangulated pieces are recursively subdivided to reach
   * the desired subdivision.  Setting the value to greater than 1 may cause
   * some point data to not be passed even if no nonlinear faces exist.  This
   * option has no effect if the input is not an unstructured grid.
   */
  vtkSetMacro(NonlinearSubdivisionLevel, int);
  vtkGetMacro(NonlinearSubdivisionLevel, int);
  //@}

  //@{
  /**
   * Direct access methods that can be used to use the this class as an
   * algorithm without using it as a filter.
   */
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
  virtual int UnstructuredGridExecute(vtkDataSet *input,
                                      vtkPolyData *output);
  virtual int DataSetExecute(vtkDataSet *input, vtkPolyData *output);
  virtual int StructuredWithBlankingExecute(vtkStructuredGrid *input, vtkPolyData *output);
  virtual int UniformGridExecute(
      vtkDataSet *input, vtkPolyData *output,
      vtkIdType *ext, vtkIdType *wholeExt, bool extractface[6] );
#ifdef VTK_USE_64BIT_IDS
  virtual int UniformGridExecute(vtkDataSet *input,
    vtkPolyData *output, int *ext32, int *wholeExt32, bool extractface[6] )
  {
    vtkIdType ext[6]; vtkIdType wholeExt[6];
    for (int cc=0; cc < 6; cc++)
    {
      ext[cc] = ext32[cc];
      wholeExt[cc] = wholeExt32[cc];
    }
    return this->UniformGridExecute(input, output, ext, wholeExt, extractface);
  }
#endif
  //@}

protected:
  vtkDataSetSurfaceFilter();
  ~vtkDataSetSurfaceFilter() override;

  vtkTypeBool UseStrips;

  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;
  int FillInputPortInformation(int port, vtkInformation *info) override;


  // Helper methods.

  /**
   * Estimates the total number of points & cells on the surface to render
   * ext -- the extent of the structured data in question (in)
   * wholeExt -- the global extent of the structured data (in)
   * numPoints -- the estimated number of points (out)
   * numCells -- the estimated number of cells (out)
   */
  void EstimateStructuredDataArraySizes(
      vtkIdType *ext, vtkIdType *wholeExt,
      vtkIdType &numPoints, vtkIdType &numCells );

  void ExecuteFaceStrips(vtkDataSet *input, vtkPolyData *output,
                         int maxFlag, vtkIdType *ext,
                         int aAxis, int bAxis, int cAxis,
                         vtkIdType *wholeExt);

  void ExecuteFaceQuads(vtkDataSet *input, vtkPolyData *output,
      int maxFlag, vtkIdType *ext, int aAxis, int bAxis, int cAxis,
      vtkIdType *wholeExt, bool checkVisibility );

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

  class vtkEdgeInterpolationMap;

  vtkEdgeInterpolationMap *EdgeMap;
  vtkIdType GetInterpolatedPointId(vtkIdType edgePtA, vtkIdType edgePtB,
                                   vtkDataSet *input, vtkCell *cell,
                                   double pcoords[3], vtkPoints *outPts,
                                   vtkPointData *outPD);

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

  vtkTypeBool PassThroughCellIds;
  void RecordOrigCellId(vtkIdType newIndex, vtkIdType origId);
  virtual void RecordOrigCellId(vtkIdType newIndex, vtkFastGeomQuad *quad);
  vtkIdTypeArray *OriginalCellIds;
  char *OriginalCellIdsName;

  vtkTypeBool PassThroughPointIds;
  void RecordOrigPointId(vtkIdType newIndex, vtkIdType origId);
  vtkIdTypeArray *OriginalPointIds;
  char *OriginalPointIdsName;

  int NonlinearSubdivisionLevel;

private:
  vtkDataSetSurfaceFilter(const vtkDataSetSurfaceFilter&) = delete;
  void operator=(const vtkDataSetSurfaceFilter&) = delete;
};

#endif
