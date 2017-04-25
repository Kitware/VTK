/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkContinuousScatterplot.h"

#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkDataSetAttributes.h"
#include "vtkExecutive.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMassProperties.h"
#include "vtkMath.h"
#include "vtkMergePoints.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkSmartPointer.h"
#include "vtkTriangleFilter.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkContinuousScatterplot);

// Data structure to store the fragment faces.
// Each face of the fragment can be represented using a vtkIdList.
typedef std::vector<vtkSmartPointer<vtkIdList> >* Polytope;

//----------------------------------------------------------------------------
vtkContinuousScatterplot::vtkContinuousScatterplot()
{
  // value for floating comparison. Suppose two floating values a and b:
  // if fabs(a-b) <= dEpsilon : then a == b.
  // if (a-b) > dEpsilon : then a > b.
  // if (b-a) > dEpsilon : then b > a.
  this->Epsilon = 1.0e-6;
  // The number of output port is one.
  this->SetNumberOfOutputPorts(1);
  // Resolution of the output image is set to 100 as default.
  this->ResX = this->ResY = 100;
  // Create the vtkImageData object and pass to the output port.
  vtkImageData* output = vtkImageData::New();
  this->GetExecutive()->SetOutputData(0, output);
  output->Delete();
  this->Fields[0] = this->Fields[1] = NULL;
}
//----------------------------------------------------------------------------
void vtkContinuousScatterplot::SetField1(char* nm, vtkIdType xRes)
{
  this->Fields[0] = nm;
  this->ResX = xRes;
}
//----------------------------------------------------------------------------
void vtkContinuousScatterplot::SetField2(char* nm, vtkIdType yRes)
{
  this->Fields[1] = nm;
  this->ResY = yRes;
}
//----------------------------------------------------------------------------
void vtkContinuousScatterplot::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//----------------------------------------------------------------------------
int vtkContinuousScatterplot::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  return 1;
}
//----------------------------------------------------------------------------
int vtkContinuousScatterplot::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
  return 1;
}
//----------------------------------------------------------------------------
int vtkContinuousScatterplot::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the input and output ports information.
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input data set, which is required to be a vtkUnstructuredGrid.
  vtkUnstructuredGrid* input =
    vtkUnstructuredGrid::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  // get the output data set, which is required to be a vtkImageData.
  vtkImageData* output = vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // get the number of points and cells of the input grid.
  vtkIdType numCells = input->GetNumberOfCells();
  vtkIdType numPts = input->GetNumberOfPoints();

  // get the dataset statistics and check if it is not an empty grid.
  if (numCells < 1 || numPts < 1)
  {
    vtkErrorMacro("No input data.");
    return 1;
  }

  // check if the output image resolution is a valid number.
  if (this->ResX <= 0 || this->ResY <= 0)
  {
    vtkErrorMacro("The resolution of the output image has to be a positive number.");
    return 1;
  }

  // check if the names of the input scalar fields are specified.
  if (!this->Fields[0] || !this->Fields[1])
  {
    vtkErrorMacro("At least two fields need to be specified.");
    return 1;
  }

  // Input point data, which should include the arrays defining the range space.
  vtkDataSetAttributes* inPD = input->GetPointData();
  // current data array of the given array names from the user input.
  vtkSmartPointer<vtkDataArray> array;
  // scalar range in two fields, where fieldInterval[0] = f1_max - f1_min,
  // fieldInterval[1] = f2_max - f2_min
  float fieldInterval[2];

  // fragment width of two fields, where fragWidth[0] = fieldInterval[0] / resX,
  // fragWidth[0] = fieldInterval[1] / resY, (resX, resY) is the output image
  // resolution.
  float fragWidth[2];

  // Collect the first scalar field based on array names,
  array = inPD->GetArray(this->Fields[0]);
  // confirming that this field is present in the input.
  if (array)
  {
    // Collect field ranges for later use.
    fieldInterval[0] = array->GetRange()[1] - array->GetRange()[0];
    // Interval between cutting planes in this field.
    fragWidth[0] = (float)fieldInterval[0] / this->ResX;
  }
  else
  {
    vtkErrorMacro("Array not found in input point data: " << this->Fields[0]);
    return 1;
  }

  // Collect the second field data array.
  array = inPD->GetArray(this->Fields[1]);
  // confirming that this field is present in the input.
  if (array)
  {
    // The range interval of the field.
    fieldInterval[1] = array->GetRange()[1] - array->GetRange()[0];
    // Interval between cutting planes in this field.
    fragWidth[1] = (float)fieldInterval[1] / this->ResY;
  }
  else
  {
    vtkErrorMacro("Array not found in input point data: " << this->Fields[1]);
    return 1;
  }

  // Devide the tetrahedron into four faces. The index of each face.
  const int tetTemplate[4][3] = { { 0, 1, 2 }, { 0, 1, 3 }, { 0, 2, 3 }, { 1, 2, 3 } };

  // fragments of current cell: each cell is placed into the inputQ,
  // then fresh fragments from current slice put into outputQ.
  std::vector<Polytope> inputQ = std::vector<Polytope>();
  std::vector<Polytope> outputQ = std::vector<Polytope>();

  // structure for storing the vertices of cutting planes used to subdivide the cell.
  vtkSmartPointer<vtkIdList> cut = vtkSmartPointer<vtkIdList>::New();

  // Initially the points in the cutting plane are not ordered. These parameters are used
  // to compute convex hull for points lying on the cutting plane. The sorting process
  // utilizes the angular information between the plane edges.
  float theta[100];
  vtkIdType index[100];
  // number of points in the current cutting plane.
  int nrPoints;
  float base[3], vec[3];
  float baseNorm, ang;
  vtkIdType pnt, pnt0, pnt1;

  // min and max scalar value of the current field in a tetrahedron
  float maxCell, minCell;
  // min and max scalar values of the current field in the whole domain.
  float minField = 0;
  float maxField = 0;

  // first and last cutting planes' field values. This threshold is used to traverse
  // through all of the cutting planes in the range.
  float initThreshold, lastThreshold;

  // variables for walking around a cell face.
  vtkIdType newPointId, thisPointId, prevPointId;
  // number of the vertices in the current cell face.
  int nrFaceIds;

  // For each cutting plane, determine its intersecting position along the current edge.
  int mask;

  // scalar values of the end points of the current edge.
  float thisScalar, prevScalar;

  // if the cutting plane intersects in the middle of the given edge, these parameters
  // are used to compute these intersecting points using linear interpolation
  double delta, t, p[3], p0[3], p1[3], p2[3];

  // fragment: list of faces forming the fragments in the current cell.
  // residual: list of faces forming the rest of the cell (except fragments).
  // working: collect the residual faces for the next iteration of subdivision.
  Polytope fragment = NULL, residual = NULL, working = NULL;

  // structure for storing the current framgent vertices in each cell face.
  vtkSmartPointer<vtkIdList> fragmentFace = NULL;
  // structure for storing the vertices which are not belonging to the current
  // fragment in each cell face.
  vtkSmartPointer<vtkIdList> residualFace = NULL;

  // reading/writing cells from input, output and cell arrays.
  vtkSmartPointer<vtkIdList> cell = NULL;
  // reading/writing faces from input, output and cell arrays.
  vtkSmartPointer<vtkIdList> face = NULL;

  // Each fragment can be mapped to a 2-D bin based on its range values. The density
  // value in each bin equals to the total geometric volume of the fragments in this bin.
  // The following structure creates and initialise such a 2-D bin with a resolution
  // ResX * ResY.
  // float imageBin[this->ResX][this->ResY];
  float** imageBin = new float*[this->ResX];
  for (int xIndex = 0; xIndex < this->ResX; ++xIndex)
  {
    imageBin[xIndex] = new float[this->ResY];
    for (int yIndex = 0; yIndex < this->ResY; ++yIndex)
    {
      imageBin[xIndex][yIndex] = 0.0;
    }
  }
  // maximal volume value in the output image bin.
  float maxBinSize = 0;

  // The VTK filter for computing the fragment geometric volume. This filter only
  // accepts triangular mesh as input, while our fragments are stored as a polygonal
  // mesh. Therefore, we need a mesh conversion.
  vtkSmartPointer<vtkMassProperties> volume = vtkSmartPointer<vtkMassProperties>::New();
  // This VTK filter convert a polygonal mesh to a triangular mesh.
  vtkSmartPointer<vtkTriangleFilter> triangleFilter = vtkSmartPointer<vtkTriangleFilter>::New();
  // geometric volume of the current polyhedral fragments.
  float fragVolume;

  // mapped index of a fragment into the 2-D bin.
  vtkIdType binIndexFirst = 0, binIndexSecond = 0;

  // id for the newly interpolated fragment vertices. Since we only need to update point
  // structure, therefore this id is not of interest and can be ignored for the moment.
  vtkIdType ignored = 0;
  // point coordinates in the input grid
  vtkPoints* inputGridPoints = input->GetPoints();
  // point coordinates in the output fragments
  vtkSmartPointer<vtkPoints> newPoints = vtkSmartPointer<vtkPoints>::New();

  // Each fragment is composed of a number of points. This data structure contains
  // scalar values for all these points.
  vtkSmartPointer<vtkDataSetAttributes> newPointsPD = vtkSmartPointer<vtkDataSetAttributes>::New();

  // locator for inserting new interpolated points into the structure. This search
  // structure ensures that redundant points will not be added to the point collection.
  vtkSmartPointer<vtkMergePoints> pointLocator = vtkSmartPointer<vtkMergePoints>::New();

  // Each fragment will have a range value. This array records the range values
  // for all of the fragments. Since we iteratively subdivide the cell based on
  // two fields, therefore the first n elements in this array record the
  // range values of the fragments in the first field subdivision, and the following m
  // elements record the range values in the second field subdivision.
  vtkSmartPointer<vtkFloatArray> fragScalar = vtkSmartPointer<vtkFloatArray>::New();

  // Each fragment will have range values from two fields.
  float fragRangeValue[2];
  // number of fragments in the first field subdivision.
  size_t firstFragNr = 0;

  // data structure containing scalar values of the vertices of current tetrahedron of
  // the input grid.
  vtkSmartPointer<vtkDataSetAttributes> tetraPD = vtkSmartPointer<vtkDataSetAttributes>::New();

  // scalar values of two fields in the current tetrahedron of the input grid.
  // these two arrays are added to the tetrahedron point data.
  vtkSmartPointer<vtkFloatArray> tetraF1 = vtkSmartPointer<vtkFloatArray>::New(),
                                 tetraF2 = vtkSmartPointer<vtkFloatArray>::New();
  tetraF1->SetNumberOfComponents(1);
  tetraF1->SetNumberOfTuples(4);
  tetraF2->SetNumberOfComponents(1);
  tetraF2->SetNumberOfTuples(4);
  // add the two scalar arrays to the point data of the tetrahedron
  tetraPD->AddArray(tetraF1);
  tetraPD->AddArray(tetraF2);

  // structure containing the polygonal faces of polyhedral fragments
  vtkSmartPointer<vtkPolyData> polyhedra = vtkSmartPointer<vtkPolyData>::New();

  // estimate the total number of cells of the fragments in each input cell.
  // consider edge can be maximally subdivided to resX number
  // of fragments for the first field and resY number of fragments for the second field.
  // In total, there will be maximal resX * resY number of new points in each edge.
  int estOutputPointSize = this->ResX * this->ResY * 4;
  // Allocate the memory for the framgent points.
  newPoints->Allocate(estOutputPointSize);

  // main loop ...
  // For each tetrahedron in a gird
  for (vtkIdType tetraIndex = 0; tetraIndex < input->GetNumberOfCells(); tetraIndex++)
  {
    // current tetrahedron vertex list.
    cell = vtkSmartPointer<vtkIdList>::New();
    input->GetCellPoints(tetraIndex, cell);

    // Test if the current cell is a tetrahedron or not. If not, ignore this cell.
    if (input->GetCellType(tetraIndex) != VTK_TETRA || cell->GetNumberOfIds() != 4)
    {
      vtkWarningMacro("Current cell " << tetraIndex << " is not of a tetrahedron type.");
      continue;
    }

    // initialise the point structure for storing fragment vertices.
    newPoints->Reset();
    // initialise the search structure for new points insertion.
    pointLocator->InitPointInsertion(newPoints, input->GetCell(tetraIndex)->GetBounds());

    // initialise data structure containing the scalar values for the fragment vertices.
    newPointsPD->Initialize();
    // Set the storage for interpolating the field values of the fragment vertices.
    newPointsPD->InterpolateAllocate(tetraPD, estOutputPointSize, ignored, false);
    newPointsPD->CopyScalarsOn();

    // initialise data structure containing the scalar values of the whole fragment.
    fragScalar->Initialize();
    fragScalar->Allocate(this->ResX * this->ResY);
    // two components are needed to store the bivariate fields of the framgent.
    fragScalar->SetNumberOfComponents(2);

    // Initialise the scalar values in this tetrahedral cell.
    for (vtkIdType cellIndex = 0; cellIndex < cell->GetNumberOfIds(); cellIndex++)
    {
      int pointId = cell->GetId(cellIndex);
      pointLocator->InsertNextPoint(inputGridPoints->GetPoint(pointId));
      tetraF1->SetComponent(
        cellIndex, 0, inPD->GetArray(this->Fields[0])->GetComponent(pointId, 0));
      tetraF2->SetComponent(
        cellIndex, 0, inPD->GetArray(this->Fields[1])->GetComponent(pointId, 0));
    }

    // The scalar values of the framgent points are based on the interpolation of the
    // point data of the tetrahedral cell (tetraPD).
    for (vtkIdType cellPDIndex = 0; cellPDIndex < tetraPD->GetNumberOfTuples(); cellPDIndex++)
    {
      newPointsPD->CopyData(tetraPD, cellPDIndex, cellPDIndex);
    }

    /*
    cout << "ARRAY SUMMARY:\n" << endl;
    for (int i = 0; i < tetraPD->GetNumberOfTuples(); i++)
    {
    cout << i << "\t" << inPD->GetArray(this->Fields[0])->GetComponent(i,0)
              << "\t" << inPD->GetArray(this->Fields[1])->GetComponent(i,0)
              << "\t" << newPointsPD->GetArray(0)->GetComponent(i,0)
              << "\t" << newPointsPD->GetArray(1)->GetComponent(i,0) << endl;

    }

    */
    // Get the next cell from input, and place into the working queue.
    // We place into outputQ as each field takes the output of the
    // last step as its input, swapping queues BEFORE processing.
    // OutputQ : a list of faces of the output framgent
    Polytope ptp = new std::vector<vtkSmartPointer<vtkIdList> >();
    outputQ.push_back(ptp);

    // min and max range value of the current cell
    minCell = maxField;
    maxCell = minField;

    // for each point in the cell
    for (int fnr = 0; fnr < 4; fnr++)
    {
      // get the faces of the cell and push into outputQ structure.
      face = vtkSmartPointer<vtkIdList>::New();
      face->SetNumberOfIds(3);
      for (int pnr = 0; pnr < 3; pnr++)
      {
        face->SetId(pnr, tetTemplate[fnr][pnr]);
      }
      outputQ[0]->push_back(face);
    }

    // For each scalar field:
    for (size_t fieldNr = 0; fieldNr < 2; fieldNr++)
    {
      // Swap role of outputQ and inputQ
      std::swap(outputQ, inputQ);
      outputQ.clear();

      // If this is the second round subdivision, then we need to record the number
      // of fragments produced in the first round. This number is used to locate
      // the range values of the output fragment.
      if (fieldNr == 1)
      {
        firstFragNr = inputQ.size();
      }

      // min value of the whole field
      minField = inPD->GetArray(this->Fields[fieldNr])->GetRange()[0];
      // max value of the whole field
      maxField = inPD->GetArray(this->Fields[fieldNr])->GetRange()[1];
      // Initialize the value
      minCell = maxField;
      maxCell = minField;

      ////cout << "tet " << tetraIndex << ", field " << fieldNr << ", pnr: " <<
      ///newPointsPD->GetNumberOfTuples() << endl;
      ////cout << "min/max init: " << minCell << ", " << maxCell << endl;

      // obtain the minimal and maximal scalar values of the cell.
      for (int pnr = 0; pnr < newPointsPD->GetNumberOfTuples(); pnr++)
      {
        double fval = newPointsPD->GetArray((int)fieldNr)->GetComponent(pnr, 0);

        ////cout << "    fval: " << fval << endl;
        if (maxCell < fval)
        {
          maxCell = fval;
        }
        if (minCell > fval)
        {
          minCell = fval;
        }
      }

      ////cout << "D"  << endl;
      ////cout << "Cell min/max: " << minCell << " " << maxCell << endl;
      ////cout << "field min/max[0] " << minField << " " << maxField << endl;
      ////cout << "field widths " << fragWidth[0] << " " << fragWidth[1] << endl;

      // in each field, the smallest threshold of cutting plane to start with.
      // since each field is sliced uniformly, in other words, the interval between
      // fragment in each field is a constant. Therefore in each cell, the smallest
      // threshold to start with, should be the first fragment value of the field which
      // is greater or equal to the minimal range value of the cell.
      initThreshold =
        minField + (1 + floor((minCell - minField) / fragWidth[fieldNr])) * fragWidth[fieldNr];

      // Iterate through the faces of the current mesh.
      // 1. The first field is initially traversed and processed on the input grid.
      //    The inputQ should be the initial tetrahedron mesh. And the outputQ should be
      //    the collection of computed fragments with respect to the first field.
      // 2. The second field is then processed based on the outputQ of step 1. The inputQ
      //    now becomes the fragments in the first field subdivision. The OutputQ
      //    in this step should be the fragments of the bivariate fields.
      // Fragment: array records the faces of the fragments.
      // Residual: array records the faces of rest of the cell in addition to fragments.
      // Cut: array records the vertices of the current cutting plane.
      // For each faces of the input mesh.
      for (size_t cp = 0; cp < inputQ.size(); cp++)
      {
        // working[0] : faces of current input mesh.
        working = inputQ[cp];

        // initialise the first and last cutting planes in this cell.
        lastThreshold = initThreshold;

        // Traverse from the minimal to the maximal scalar values in a cell, every time
        // the threshold is increased by one fragmentWidth
        for (double threshold = initThreshold; threshold < maxCell; threshold += fragWidth[fieldNr])
        {
          // Initialise framgent face structure for the current cutting plane.
          if (fragment)
          {
            delete fragment;
          }
          if (residual)
          {
            delete residual;
          }
          fragment = new std::vector<vtkSmartPointer<vtkIdList> >();
          residual = new std::vector<vtkSmartPointer<vtkIdList> >();

          // Create the new cutting plane.
          cut = vtkSmartPointer<vtkIdList>::New();

          // Effectively, we start processing a new cell at this point.
          for (std::vector<vtkSmartPointer<vtkIdList> >::iterator faceIt = working->begin();
               faceIt != working->end(); ++faceIt)
          {
            fragmentFace = vtkSmartPointer<vtkIdList>::New();
            residualFace = vtkSmartPointer<vtkIdList>::New();

            // number of points in the current cell face
            nrFaceIds = (*faceIt)->GetNumberOfIds();

            // get the previous point id in the face
            prevPointId = (*faceIt)->GetId(nrFaceIds - 1);
            // the scalar value of the previous point in the face
            prevScalar = newPointsPD->GetArray(
              (int)fieldNr)->GetComponent(prevPointId, 0);

            // Walk around the edge, comparing the range values between the current
            // cutting plane and the edge end points. Classify the each end point of the
            // edge into three classes based on the comparison result. Three classes
            // includes:
            // fragmentFace: if current edge point belongs to the fragment.
            // cut: if current edge point belongs to the cutting plane.
            // residual: if current edge point belongs to neither of the above classes.
            // For each edge
            for (int i = 0; i < nrFaceIds; i++)
            {
              // get the current point Id in the face
              thisPointId = (*faceIt)->GetId(i);
              // get scalar value of the current point
              thisScalar = newPointsPD->GetArray(
                (int)fieldNr)->GetComponent(thisPointId, 0);

              ////cout <<  ">>> " << thisPointId << " " << thisScalar << " " << prevPointId << " "
              ///<< prevScalar << endl;

              // zero bitweise or to any value equals that value
              // threshold = minField + n * sw
              mask = 0;
              // thisScalar < threshold (0001)
              if (threshold - thisScalar > this->Epsilon)
                mask |= 1;
              // thisScalar > threshold (0010)
              if (thisScalar - threshold > this->Epsilon)
                mask |= 2;
              // prevScalar < threshold (0100)
              if (threshold - prevScalar > this->Epsilon)
                mask |= 4;
              // prevScalar > threshold (1000)
              if (prevScalar - threshold > this->Epsilon)
                mask |= 8;
              // thisScalar == threshold (0011)
              if (fabs(thisScalar - threshold) <= this->Epsilon)
                mask |= 3;
              // prevScalar == threshold (1100)
              if (fabs(prevScalar - threshold) <= this->Epsilon)
                mask |= 12;
              switch (mask)
              {
                // threshold == thisScalar
                case 3:
                case 7:
                // threshold == thisScalar,
                // threshold < preScalar
                case 11:
                // threshold == thisScalar
                // threshold == preScalar
                case 15:
                  // fragment face array insert current point
                  fragmentFace->InsertNextId(thisPointId);
                  // residual face array insert current point
                  residualFace->InsertNextId(thisPointId);
                  // if the current point is not in the cut array
                  if (cut->IsId(thisPointId) < 0)
                  {
                    cut->InsertNextId(thisPointId);
                  }
                  break;
                // threshold == thisScalar
                // threshold > preScalar
                case 5:
                // threshold > thisScalar
                // threshold == prevScalar
                case 13:
                  fragmentFace->InsertNextId(thisPointId);
                  break;
                // threshold < thisScalar
                // threshold < prevScalar
                case 10:
                // threshold < thisScalar
                // threshold == prevScalar
                case 14:
                  residualFace->InsertNextId(thisPointId);
                  break;
                // threshold > thisScalar
                // threshold < preScalar
                case 9:
                  // Moving to this point crosses the threshold hi-lo
                  // Insert interpolated point into both.
                  delta = prevScalar - thisScalar;
                  t = (threshold - thisScalar) / delta;
                  //  * PREV ------- T -------- THIS *
                  newPoints->GetPoint(thisPointId, p1);
                  newPoints->GetPoint(prevPointId, p2);
                  for (int j = 0; j < 3; j++)
                  {
                    p[j] = p1[j] + t * (p2[j] - p1[j]);
                  }
                  if (pointLocator->InsertUniquePoint(p, newPointId))
                  {
                    newPointsPD->InterpolateEdge(
                      newPointsPD, newPointId, thisPointId, prevPointId, t);
                  }
                  fragmentFace->InsertNextId(newPointId);
                  fragmentFace->InsertNextId(thisPointId);
                  residualFace->InsertNextId(newPointId);
                  // We have found a cut point, add it if first visit.
                  if (cut->IsId(newPointId) < 0)
                  {
                    cut->InsertNextId(newPointId);
                  }
                  break;
                // threshold < thisScalar
                // threshold > preScalar
                case 6:
                  // this >, prev <
                  // Moving to this point crosses the threshold lo-hi
                  // Insert interpolated point into both.
                  delta = thisScalar - prevScalar;
                  t = (threshold - prevScalar) / delta;
                  newPoints->GetPoint(thisPointId, p1);
                  newPoints->GetPoint(prevPointId, p2);
                  for (int j = 0; j < 3; j++)
                  {
                    p[j] = p2[j] + t * (p1[j] - p2[j]);
                  }
                  if (pointLocator->InsertUniquePoint(p, newPointId))
                  {
                    newPointsPD->InterpolateEdge(
                      newPointsPD, newPointId, prevPointId, thisPointId, t);
                  }
                  fragmentFace->InsertNextId(newPointId);
                  residualFace->InsertNextId(newPointId);
                  residualFace->InsertNextId(thisPointId);
                  if (cut->IsId(newPointId) < 0)
                  {
                    cut->InsertNextId(newPointId);
                  }
                  break;
                default:
                  vtkErrorMacro("Incomparable scalars " << prevScalar << ", " << thisScalar << ", "
                                                        << threshold);
              } // switch mask
              prevPointId = thisPointId;
              prevScalar = thisScalar;
            } // for-each edge
            // Output fragment and residue into new cells, as appropriate.
            // Test if the fragment and residual faces are well defined.
            if (fragmentFace->GetNumberOfIds() > 2)
            {
              fragmentFace->Squeeze();
              fragment->push_back(fragmentFace);
            }
            if (residualFace->GetNumberOfIds() > 2)
            {
              residualFace->Squeeze();
              residual->push_back(residualFace);
            }
          } // for each face.

          // We need to compute the face defined by the cut points.
          // We cannot guarantee that points in the cut-list are ordered wrt
          // polygon boundary, so we recompute an order by effectively working
          // the convex hull.
          // The cut-list is added to the framgent and residual array.
          if (cut->GetNumberOfIds() > 2)
          {
            nrPoints = cut->GetNumberOfIds();
            pnt0 = cut->GetId(0);
            pnt1 = cut->GetId(1);
            // 2.  Compute vector from p[0] to p[1].
            newPoints->GetPoint(pnt0, p0);
            newPoints->GetPoint(pnt1, p1);
            for (int i = 0; i < 3; i++)
            {
              base[i] = p1[i] - p0[i];
            }
            baseNorm = vtkMath::Norm(base);
            theta[0] = 0.0;
            index[0] = pnt1;
            // 3.  For the remaining points p, compute angle between p-p0 and base.
            for (int i = 2; i < nrPoints; i++)
            {
              newPoints->GetPoint(cut->GetId(i), p1);
              for (int j = 0; j < 3; j++)
              {
                vec[j] = p1[j] - p0[j];
              }
              theta[i - 1] = acos(vtkMath::Dot(base, vec) / (baseNorm * vtkMath::Norm(vec)));
              index[i - 1] = cut->GetId(i);
            }
            // 4.  Sort angles.
            for (int j = 1; j < nrPoints - 1; j++)
            {
              ang = theta[j];
              pnt = index[j];
              int i = j - 1;
              while (i >= 0 && theta[i] > ang)
              {
                theta[i + 1] = theta[i];
                index[i + 1] = index[i];
                i--;
              }
              theta[i + 1] = ang;
              index[i + 1] = pnt;
            }
            cut->Reset();
            cut->InsertNextId(pnt0);
            for (int i = 0; i < nrPoints - 1; i++)
            {
              cut->InsertNextId(index[i]);
            }
            fragment->push_back(cut);
            residual->push_back(cut);
          } // Generate cut plane.  // Generate cut plane.

          // OUTPUT PHASE  -----------------------------------------
          // Have completed traversing each face.
          // Now update output with new fragment.
          // Iterate through each edge and record for each fragment,  which are the
          // divided points included.
          // ------------------------------------------------------
          if (fragment->size() > 3)
          {
            // add the current framgent to the outpuQ structure.
            outputQ.push_back(fragment);
            fragment = NULL;
            // set threshold at which this fragment was created
            if (fieldNr)
            {
              // if this is the second field subdivision, we need to retrieve
              // the scalar value from the first field subdivision first. And insert the
              // retrieved first field value together with the current second field value
              // into the fragment scalar array.
              float firstFieldRange = fragScalar->GetComponent((vtkIdType)cp, 0);
              fragScalar->InsertNextTuple2(firstFieldRange, threshold);
            }
            else
            {
              // if this is the first field subdivisions, just insert the scalar value
              // into the array.
              fragScalar->InsertNextTuple2(threshold, 0);
            }
          }
          else
          {
            // Remove any partial fragments.
            while (fragment->size() > 0)
            {
              fragment->pop_back();
            }
          }
          // Faces defining the next working polyhedron are in the residual array.
          std::swap(working, residual);
          // Clear out anything left in residual
          while (residual->size() > 0)
          {
            residual->pop_back();
          }
          lastThreshold += fragWidth[fieldNr]; //  Track final threshold
        }                                      // for each threshold

        // If we are left with a residual, add it as a fragment.
        // However, note that final step in loop is to swap residual
        // with working, so need to look in the working list.
        if (working->size() > 3)
        {
          outputQ.push_back(working);
          // set threshold at which this fragment was created.
          if (fieldNr)
          {
            // if this is the second field subdivision, we need to retrieve
            // the scalar value from the first field subdivision first. And insert the
            // retrieved first field value together with the current second field value
            // into the fragment scalar array.
            float firstFieldRange = fragScalar->GetComponent((vtkIdType)cp, 0);
            fragScalar->InsertNextTuple2(firstFieldRange, lastThreshold);
          }
          else
          {
            // if this is the first field subdivisions, just insert the scalar value
            // into the array.
            fragScalar->InsertNextTuple2(lastThreshold, 0);
          }
        }
        else
        {
          while (working->size() > 0)
          {
            working->pop_back();
          }

          if(working)
          {
            delete working;
            working = nullptr;
          }
        }
      } // for each fragment of cell: faces, edges.
        // CutIds are the edges with current dividing fragments.
    }   // for each field

    // OUTPUT PHASE: ----------------------------------------------
    // Output generated fragments into main output dataset.
    // ------------------------------------------------------------
    // For each output framgent, we compute its geometric volume and aggregate over
    // the cells.
    for (size_t co = 0; co < outputQ.size(); co++)
    {
      // The current fragment needs to be converted to a polygonal mesh.
      // Array for recording the vertices of the polygonal mesh.
      polyhedra->Initialize();
      polyhedra->Allocate((vtkIdType)outputQ[co]->size());

      // for each face of the fragment
      vtkSmartPointer<vtkIdList> poly = vtkSmartPointer<vtkIdList>::New();
      for (std::vector<vtkSmartPointer<vtkIdList> >::iterator fc = outputQ[co]->begin();
           fc != outputQ[co]->end(); ++fc)
      {
        poly->Reset();
        for (int pnr = 0; pnr < (*fc)->GetNumberOfIds(); pnr++)
        {
          poly->InsertNextId((*fc)->GetId(pnr));
        }
        // insert the polygon face.
        polyhedra->InsertNextCell(VTK_POLYGON, poly);
      }
      polyhedra->SetPoints(newPoints);

      // convert the polygon faces to triangular faces.
      triangleFilter->SetInputData(polyhedra);
      triangleFilter->Update();

      if (triangleFilter->GetOutput()->GetNumberOfCells() > 0)
      {
        // Compute the volume of the fragment
        volume->SetInputData(triangleFilter->GetOutput());
        volume->Update();
        fragVolume = volume->GetVolume();
      }
      else
      {
        fragVolume = 0;
      }

      // Range values for the current fragment.
      fragRangeValue[0] = fragScalar->GetComponent((vtkIdType)(firstFragNr + co), 0);
      fragRangeValue[1] = fragScalar->GetComponent((vtkIdType)(firstFragNr + co), 1);

      // Map the current fragment into the 2-Dimensional bin based on its range values.
      binIndexFirst = (int)(this->ResX - 1) *
        (fragRangeValue[0] - inPD->GetArray(this->Fields[0])->GetRange()[0]) / fieldInterval[0];
      binIndexSecond = (int)(this->ResY - 1) *
        (fragRangeValue[1] - inPD->GetArray(this->Fields[1])->GetRange()[0]) / fieldInterval[1];

      ////cout << "biF: " << binIndexFirst << "\tbiS: " << binIndexSecond << "\t" << fragVolume <<
      ///endl;

      // aggregate the fragment volumes in each bin
      if (binIndexFirst >= 0 && binIndexFirst < this->ResX && binIndexSecond >= 0 &&
        binIndexSecond < this->ResY)
      {
        imageBin[binIndexFirst][binIndexSecond] += fragVolume;
      }

      // finding the largest volume in a bin.
      if (imageBin[binIndexFirst][binIndexSecond] > maxBinSize)
      {
        maxBinSize = imageBin[binIndexFirst][binIndexSecond];
      }

      // Clear faces from current polytope in output queue.
      while (outputQ[co]->size() > 0)
      {
        outputQ[co]->pop_back();
      }
    } // end for loop in outputQ

    // Release the memory for the local structure.
    while (outputQ.size() > 0)
    {
      Polytope plocal;
      plocal = outputQ.back();
      outputQ.pop_back();
      delete plocal;
    }
    outputQ.clear();
  } // For each cell

  if (fragment)
  {
    delete fragment;
    fragment = nullptr;
  }
  if (residual)
  {
    delete residual;
    residual = nullptr;
  }


  // Create the output image data.
  output->SetExtent(0, this->ResX - 1, 0, this->ResY - 1, 0, 0);
  output->SetOrigin(0, 0, 0);
  output->SetSpacing(1, 1, 1);
  output->AllocateScalars(VTK_DOUBLE, 1);

  // A scalar array is attached onto the output vtkImageData.
  // This array records the total volume of the fragments in each bin.
  vtkSmartPointer<vtkFloatArray> volumeArray = vtkSmartPointer<vtkFloatArray>::New();
  volumeArray->SetName("volume");
  volumeArray->SetNumberOfComponents(1);
  volumeArray->SetNumberOfTuples(this->ResX * this->ResY);

  // Pixel density are computed by the fragment volume.
  for (int xIndex = 0; xIndex < this->ResX; xIndex++)
  {
    for (int yIndex = 0; yIndex < this->ResY; yIndex++)
    {
      float size = imageBin[xIndex][yIndex];
      int idx = xIndex * this->ResY + yIndex;
      volumeArray->SetComponent(idx, 0, size);
      double* pixel = static_cast<double*>(output->GetScalarPointer(xIndex, yIndex, 0));
      if (imageBin[xIndex][yIndex] > 0)
      {
        pixel[0] = 255.0 * imageBin[xIndex][yIndex] / maxBinSize;
      }
      else
      {
        pixel[0] = 0;
      }
    }
  }
  output->GetPointData()->AddArray(volumeArray);
  output->Squeeze();

  for (int xIndex = 0; xIndex < this->ResX; ++xIndex)
  {
    delete[] imageBin[xIndex];
  }
  delete[] imageBin;

  return 1;
}
