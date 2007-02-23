/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLWriterC.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __vtkXMLWriterC_h
#define __vtkXMLWriterC_h

#include "vtkType.h" /* For scalar and vtkDataObject type enumerations.  */

/* Define the export macro.  */
#if defined(_WIN32) && defined(VTK_BUILD_SHARED_LIBS)
# if defined(vtkIO_EXPORTS)
#  define VTK_XML_EXPORT __declspec(dllexport)
# else
#  define VTK_XML_EXPORT __declspec(dllimport)
# endif
#else
# define VTK_XML_EXPORT
#endif

#ifdef __cplusplus
extern "C"
{
#endif /*cplusplus*/

/**
 * vtkXMLWriterC is an opaque structure holding the state of an
 * individual writer object.  It can be used to write VTK XML files.
 */
typedef struct vtkXMLWriterC_s vtkXMLWriterC;

/**
 * Create a new instance of vtkXMLWriterC.  Returns the object or NULL
 * on failure.
 */
VTK_XML_EXPORT
vtkXMLWriterC* vtkXMLWriterC_New();

/**
 * Delete the writer object.
 *
 * This should not be called between Start and Stop calls.
 */
VTK_XML_EXPORT
void vtkXMLWriterC_Delete(vtkXMLWriterC* self);

/**
 * Set the VTK data object type that will be written.  This
 * initializes an empty data object of the given type.
 *
 * This must be set before setting geometry or data information can
 * can be set only once per writer object.
 */
VTK_XML_EXPORT
void vtkXMLWriterC_SetDataObjectType(vtkXMLWriterC* self, int objType);

/**
 * Set the VTK writer data mode to either:
 * - Ascii
 * - Binary
 * - Appended (default)
 *
 * This may be used only after SetDataObjectType has been called.
 */
VTK_XML_EXPORT
void vtkXMLWriterC_SetDataModeType(vtkXMLWriterC* self, int datamodetype);

/**
 * Set the extent of a structured data set.
 *
 * This may be used only after SetDataObjectType has been called with
 * a structured data object type.
 */
VTK_XML_EXPORT
void vtkXMLWriterC_SetExtent(vtkXMLWriterC* self, int extent[6]);

/**
 * Set the points of a point data set.  For structured data, the
 * number of points must match number of points indicated by the
 * extent.
 *
 * Use dataType to specify the scalar type used in the given array.
 * The data array must have numPoints*3 entries specifying 3-D points.
 *
 * This may not be used for data objects with implicit points.  It may
 * not be called before SetDataObjectType or between Start and Stop
 * calls.
 */
VTK_XML_EXPORT
void vtkXMLWriterC_SetPoints(vtkXMLWriterC* self, int dataType,
                             void* data, vtkIdType numPoints);

/**
 * Set the origin of an image data set.
 *
 * This may only be used for image data.  It may not be called before
 * SetDataObjectType or between Start and Stop calls.
 */
VTK_XML_EXPORT
void vtkXMLWriterC_SetOrigin(vtkXMLWriterC* self, double origin[3]);

/**
 * Set the spacing of an image data set.
 *
 * This may only be used for image data.  It may not be called before
 * SetDataObjectType or between Start and Stop calls.
 */
VTK_XML_EXPORT
void vtkXMLWriterC_SetSpacing(vtkXMLWriterC* self, double spacing[3]);

/**
 * Set the coordinates along one axis of a rectilinear grid data set.
 *
 * Specify axis 0 for X, 1 for Y, and 2 for Z.  Use dataType to
 * specify the scalar type used in the given data array.  Use
 * numCoordinates to specify the number of such values in the array.
 * The number of values must match that specified by the extent for
 * the given axis.
 *
 * This may only be used for rectilinear grids.  It may not be called
 * before SetDataObjectType or between Start and Stop calls.
 */
VTK_XML_EXPORT
void vtkXMLWriterC_SetCoordinates(vtkXMLWriterC* self, int axis,
                                  int dataType, void* data,
                                  vtkIdType numCoordinates);

/**
 * Set a cell array on the data object to be written.  All cells must
 * have the same type.
 *
 * For unstructured grid data objects, the cellType can be any type.
 * For polygonal data objects, the cellType must be VTK_VERTEX,
 * VTK_POLY_VERTEX, VTK_LINE, VTK_POLY_LINE, VTK_TRIANGLE,
 * VTK_TRIANGLE_STRIP, or cyclically connected simple cell type such
 * as VTK_POLYGON.
 *
 * The cells array must have cellsSize entries.  Each cell uses N+1
 * entries where N is the number of points in the cell.  The layout of
 * the array for each cell is "[N,id1,id2,...,idN]".  The total number
 * of cells must be ncells.
 *
 * This may only be used for unstructured grid and polygonal data
 * types.  It may not be called before SetDataObjectType or between
 * Start and Stop calls.
 */
VTK_XML_EXPORT
void vtkXMLWriterC_SetCellsWithType(vtkXMLWriterC* self,
                                    int cellType, vtkIdType ncells,
                                    vtkIdType* cells, vtkIdType cellsSize);

/**
 * Set a cell array on the data object to be written.  Each cell can
 * have its own type.
 *
 * The cellTypes array specifies the type of each cell, and has ncells
 * entries.  The cells array must have cellsSize entries.  Each cell
 * uses N+1 entries where N is the number of points in the cell.  The
 * layout of the array for each cell is "[N,id1,id2,...,idN]".  The
 * total number of cells must be ncells.
 *
 * This may only be used for unstructured grid data objects.  It may
 * not be called before SetDataObjectType or between Start and Stop
 * calls.
 */
VTK_XML_EXPORT
void vtkXMLWriterC_SetCellsWithTypes(vtkXMLWriterC* self,
                                     int* cellTypes, vtkIdType ncells,
                                     vtkIdType* cells, vtkIdType cellsSize);

/**
 * Set a point or cell data array by name.
 *
 * The name of the array is required and should describe the purpose
 * of the data.  Use dataType to specify the scalar type used in the
 * given data array.  Use numTuples to specify the number of tuples
 * and numComponents to specify the number of scalar components in
 * each tuple.
 *
 * The data array must have exactly numTuples*numComponents entries.
 * For SetPointData, numTuples must be equal to the number of points
 * indicated by SetExtent and/or SetPoints.  For SetCellData,
 * numTuples must be equal to the total number of cells set by
 * SetCells.
 *
 * The role can be one of "SCALARS", "VECTORS", "NORMALS", "TENSORS",
 * or "TCOORDS" and specifies that the array should be designated as
 * the active array for the named role.  Other values for role are
 * ignored.
 *
 * This may be used for all data types.  It may not be called before
 * SetDataObjectType but may be called between Start and Stop calls.
 */
VTK_XML_EXPORT
void vtkXMLWriterC_SetPointData(vtkXMLWriterC* self, const char* name,
                                int dataType, void* data,
                                vtkIdType numTuples, int numComponents,
                                const char* role);
VTK_XML_EXPORT
void vtkXMLWriterC_SetCellData(vtkXMLWriterC* self, const char* name,
                               int dataType, void* data,
                               vtkIdType numTuples, int numComponents,
                               const char* role);

/**
 * Set the name of the file into which the data are to be written.
 *
 * This may be used for all data types.  It may not be called before
 * SetDataObjectType or between Start and Stop calls.
 */
VTK_XML_EXPORT
void vtkXMLWriterC_SetFileName(vtkXMLWriterC* self, const char* fileName);

/**
 * Write the data to a file immediately.  This is not used when
 * writing time-series data.  Returns 1 for success and 0 for failure.
 *
 * This may only be called after SetFileName and SetDataObjectType.
 */
VTK_XML_EXPORT
int vtkXMLWriterC_Write(vtkXMLWriterC* self);

/**
 * Set the number of time steps that will be written between upcoming
 * Start and Stop calls.  This is used when writing time-series data.
 *
 * This may be used for all data types.  It may not be called before
 * SetDataObjectType or between Start and Stop calls.
 */
VTK_XML_EXPORT
void vtkXMLWriterC_SetNumberOfTimeSteps(vtkXMLWriterC* self, int numTimeSteps);

/**
 * Start writing a time-series to the output file.
 *
 * This may only be called after SetFileName, SetDataObjectType, and
 * SetNumberOfTimeSteps.  It may not be called a second time until
 * after an intervening call to Stop.
 */
VTK_XML_EXPORT
void vtkXMLWriterC_Start(vtkXMLWriterC* self);

/**
 * Write one time step of a time-series to the output file.  The
 * current data set by SetPointData and SetCellData will be written.
 *
 * Use timeValue to specify the time associated with the time step
 * being written.
 *
 * This may only be called after Start has been called.  It should be
 * called NumberOfTimeSteps times before calling Stop.
 */
VTK_XML_EXPORT
void vtkXMLWriterC_WriteNextTimeStep(vtkXMLWriterC* self, double timeValue);

/**
 * Stop writing a time-series to the output file.
 *
 * This may only be called after Start and NumberOfTimeSteps calls to
 * WriteNextTimeStep.
 */
VTK_XML_EXPORT
void vtkXMLWriterC_Stop(vtkXMLWriterC* self);

#ifdef __cplusplus
} /* extern "C" */
#endif /*cplusplus*/

/* Done with export macro.  */
#undef VTK_XML_EXPORT

#endif
