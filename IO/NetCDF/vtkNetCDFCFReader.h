// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNetCDFCFReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

/**
 * @class   vtkNetCDFCFReader
 *
 *
 *
 * Reads netCDF files that follow the CF convention.  Details on this convention
 * can be found at <http://cf-pcmdi.llnl.gov/>.
 *
*/

#ifndef vtkNetCDFCFReader_h
#define vtkNetCDFCFReader_h

#include "vtkIONetCDFModule.h" // For export macro
#include "vtkNetCDFReader.h"

#include "vtkStdString.h" // Used for ivars.

class vtkImageData;
class vtkPoints;
class vtkRectilinearGrid;
class vtkStructuredGrid;
class vtkUnstructuredGrid;

class VTKIONETCDF_EXPORT vtkNetCDFCFReader : public vtkNetCDFReader
{
public:
  vtkTypeMacro(vtkNetCDFCFReader, vtkNetCDFReader);
  static vtkNetCDFCFReader *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  //@{
  /**
   * If on (the default), then 3D data with latitude/longitude dimensions
   * will be read in as curvilinear data shaped like spherical coordinates.
   * If false, then the data will always be read in Cartesian coordinates.
   */
  vtkGetMacro(SphericalCoordinates, int);
  vtkSetMacro(SphericalCoordinates, int);
  vtkBooleanMacro(SphericalCoordinates, int);
  //@}

  //@{
  /**
   * The scale and bias of the vertical component of spherical coordinates.  It
   * is common to write the vertical component with respect to something other
   * than the center of the sphere (for example, the surface).  In this case, it
   * might be necessary to scale and/or bias the vertical height.  The height
   * will become height*scale + bias.  Keep in mind that if the positive
   * attribute of the vertical dimension is down, then the height is negated.
   * By default the scale is 1 and the bias is 0 (that is, no change).  The
   * scaling will be adjusted if it results in invalid (negative) vertical
   * values.
   */
  vtkGetMacro(VerticalScale, double);
  vtkSetMacro(VerticalScale, double);
  vtkGetMacro(VerticalBias, double);
  vtkSetMacro(VerticalBias, double);
  //@}

  //@{
  /**
   * Set/get the data type of the output.  The index used is taken from the list
   * of VTK data types in vtkType.h.  Valid types are VTK_IMAGE_DATA,
   * VTK_RECTILINEAR_GRID, VTK_STRUCTURED_GRID, and VTK_UNSTRUCTURED_GRID.  In
   * addition you can set the type to -1 (the default), and this reader will
   * pick the data type best suited for the dimensions being read.
   */
  vtkGetMacro(OutputType, int);
  virtual void SetOutputType(int type);
  void SetOutputTypeToAutomatic() { this->SetOutputType(-1); }
  void SetOutputTypeToImage() { this->SetOutputType(VTK_IMAGE_DATA); }
  void SetOutputTypeToRectilinear() {this->SetOutputType(VTK_RECTILINEAR_GRID);}
  void SetOutputTypeToStructured() { this->SetOutputType(VTK_STRUCTURED_GRID); }
  void SetOutputTypeToUnstructured() {
    this->SetOutputType(VTK_UNSTRUCTURED_GRID);
  }
  //@}

  /**
   * Returns true if the given file can be read.
   */
  static int CanReadFile(const char *filename);

protected:
  vtkNetCDFCFReader();
  ~vtkNetCDFCFReader();

  int SphericalCoordinates;

  double VerticalScale;
  double VerticalBias;

  int OutputType;

  virtual int RequestDataObject(vtkInformation *request,
                                vtkInformationVector **inputVector,
                                vtkInformationVector *outputVector);

  virtual int RequestInformation(vtkInformation *request,
                                 vtkInformationVector **inputVector,
                                 vtkInformationVector *outputVector);

  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector);

  //@{
  /**
   * Interprets the special conventions of COARDS.
   */
  virtual int ReadMetaData(int ncFD);
  virtual int IsTimeDimension(int ncFD, int dimId);
  virtual vtkSmartPointer<vtkDoubleArray> GetTimeValues(int ncFD, int dimId);
  //@}

  class vtkDimensionInfo {
  public:
    vtkDimensionInfo() { };
    vtkDimensionInfo(int ncFD, int id);
    const char *GetName() const { return this->Name.c_str(); }
    enum UnitsEnum {
      UNDEFINED_UNITS,
      TIME_UNITS,
      LATITUDE_UNITS,
      LONGITUDE_UNITS,
      VERTICAL_UNITS
    };
    UnitsEnum GetUnits() const { return this->Units; }
    vtkSmartPointer<vtkDoubleArray> GetCoordinates() {return this->Coordinates;}
    vtkSmartPointer<vtkDoubleArray> GetBounds() { return this->Bounds; }
    bool GetHasRegularSpacing() const { return this->HasRegularSpacing; }
    double GetOrigin() const { return this->Origin; }
    double GetSpacing() const { return this->Spacing; }
    vtkSmartPointer<vtkStringArray> GetSpecialVariables() const {
      return this->SpecialVariables;
    }
  protected:
    vtkStdString Name;
    int DimId;
    vtkSmartPointer<vtkDoubleArray> Coordinates;
    vtkSmartPointer<vtkDoubleArray> Bounds;
    UnitsEnum Units;
    bool HasRegularSpacing;
    double Origin, Spacing;
    vtkSmartPointer<vtkStringArray> SpecialVariables;
    int LoadMetaData(int ncFD);
  };
  class vtkDimensionInfoVector;
  friend class vtkDimensionInfoVector;
  vtkDimensionInfoVector *DimensionInfo;
  vtkDimensionInfo *GetDimensionInfo(int dimension);

  class vtkDependentDimensionInfo {
  public:
    vtkDependentDimensionInfo() : Valid(false) { };
    vtkDependentDimensionInfo(int ncFD, int varId, vtkNetCDFCFReader *parent);
    bool GetValid() const { return this->Valid; }
    bool GetHasBounds() const { return this->HasBounds; }
    bool GetCellsUnstructured() const { return this->CellsUnstructured; }
    vtkSmartPointer<vtkIntArray> GetGridDimensions() const {
      return this->GridDimensions;
    }
    vtkSmartPointer<vtkDoubleArray> GetLongitudeCoordinates() const {
      return this->LongitudeCoordinates;
    }
    vtkSmartPointer<vtkDoubleArray> GetLatitudeCoordinates() const {
      return this->LatitudeCoordinates;
    }
    vtkSmartPointer<vtkStringArray> GetSpecialVariables() const {
      return this->SpecialVariables;
    }
  protected:
    bool Valid;
    bool HasBounds;
    bool CellsUnstructured;
    vtkSmartPointer<vtkIntArray> GridDimensions;
    vtkSmartPointer<vtkDoubleArray> LongitudeCoordinates;
    vtkSmartPointer<vtkDoubleArray> LatitudeCoordinates;
    vtkSmartPointer<vtkStringArray> SpecialVariables;
    int LoadMetaData(int ncFD, int varId, vtkNetCDFCFReader *parent);
    int LoadCoordinateVariable(int ncFD, int varId, vtkDoubleArray *coords);
    int LoadBoundsVariable(int ncFD, int varId, vtkDoubleArray *coords);
    int LoadUnstructuredBoundsVariable(int ncFD, int varId,
                                       vtkDoubleArray *coords);
  };
  friend class vtkDependentDimensionInfo;
  class vtkDependentDimensionInfoVector;
  friend class vtkDependentDimensionInfoVector;
  vtkDependentDimensionInfoVector *DependentDimensionInfo;

  // Finds the dependent dimension information for the given set of dimensions.
  // Returns NULL if no information has been recorded.
  vtkDependentDimensionInfo *FindDependentDimensionInfo(vtkIntArray *dims);

  /**
   * Given the list of dimensions, identify the longitude, latitude, and
   * vertical dimensions.  -1 is returned for any not found.  The results depend
   * on the values in this->DimensionInfo.
   */
  virtual void IdentifySphericalCoordinates(vtkIntArray *dimensions,
                                            int &longitudeDim,
                                            int &latitudeDim,
                                            int &verticalDim);

  enum CoordinateTypesEnum {
    COORDS_UNIFORM_RECTILINEAR,
    COORDS_NONUNIFORM_RECTILINEAR,
    COORDS_REGULAR_SPHERICAL,
    COORDS_2D_EUCLIDEAN,
    COORDS_2D_SPHERICAL,
    COORDS_EUCLIDEAN_4SIDED_CELLS,
    COORDS_SPHERICAL_4SIDED_CELLS,
    COORDS_EUCLIDEAN_PSIDED_CELLS,
    COORDS_SPHERICAL_PSIDED_CELLS
  };

  /**
   * Based on the given dimensions and the current state of the reader, returns
   * how the coordinates should be interpreted.  The returned value is one of
   * the CoordinateTypesEnum identifiers.
   */
  CoordinateTypesEnum CoordinateType(vtkIntArray *dimensions);

  /**
   * Returns false for spherical dimensions, which should use cell data.
   */
  virtual bool DimensionsAreForPointData(vtkIntArray *dimensions);

  /**
   * Convenience function that takes piece information and then returns a set of
   * extents to load based on this->WholeExtent.  The result is returned in
   * extent.
   */
  void ExtentForDimensionsAndPiece(int pieceNumber,
                                   int numberOfPieces,
                                   int ghostLevels,
                                   int extent[6]);

  /**
   * Overridden to retrieve stored extent for unstructured data.
   */
  virtual void GetUpdateExtentForOutput(vtkDataSet *output, int extent[6]);

  //@{
  /**
   * Internal methods for setting rectilinear coordinates.
   */
  void AddRectilinearCoordinates(vtkImageData *imageOutput);
  void AddRectilinearCoordinates(vtkRectilinearGrid *rectilinearOutput);
  void FakeRectilinearCoordinates(vtkRectilinearGrid *rectilinearOutput);
  void Add1DRectilinearCoordinates(vtkPoints *points, const int extent[6]);
  void Add2DRectilinearCoordinates(vtkPoints *points, const int extent[6]);
  void Add1DRectilinearCoordinates(vtkStructuredGrid *structuredOutput);
  void Add2DRectilinearCoordinates(vtkStructuredGrid *structuredOutput);
  void FakeStructuredCoordinates(vtkStructuredGrid *structuredOutput);
  void Add1DRectilinearCoordinates(vtkUnstructuredGrid *unstructuredOutput,
                                   const int extent[6]);
  void Add2DRectilinearCoordinates(vtkUnstructuredGrid *unstructuredOutput,
                                   const int extent[6]);
  //@}

  //@{
  /**
   * Internal methods for setting spherical coordinates.
   */
  void Add1DSphericalCoordinates(vtkPoints *points, const int extent[6]);
  void Add2DSphericalCoordinates(vtkPoints *points, const int extent[6]);
  void Add1DSphericalCoordinates(vtkStructuredGrid *structuredOutput);
  void Add2DSphericalCoordinates(vtkStructuredGrid *structuredOutput);
  void Add1DSphericalCoordinates(vtkUnstructuredGrid *unstructuredOutput,
                                 const int extent[6]);
  void Add2DSphericalCoordinates(vtkUnstructuredGrid *unstructuredOutput,
                                 const int extent[6]);
  //@}

  /**
   * Internal method for building unstructred cells that match structured cells.
   */
  void AddStructuredCells(vtkUnstructuredGrid *unstructuredOutput,
                          const int extent[6]);

  //@{
  /**
   * Internal methods for creating unstructured cells.
   */
  void AddUnstructuredRectilinearCoordinates(
                                        vtkUnstructuredGrid *unstructuredOutput,
                                        const int extent[6]);
  void AddUnstructuredSphericalCoordinates(
                                        vtkUnstructuredGrid *unstructuredOutput,
                                        const int extent[6]);
  //@}


private:
  vtkNetCDFCFReader(const vtkNetCDFCFReader &) VTK_DELETE_FUNCTION;
  void operator=(const vtkNetCDFCFReader &) VTK_DELETE_FUNCTION;
};

#endif //vtkNetCDFCFReader_h

