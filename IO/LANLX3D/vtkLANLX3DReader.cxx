// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) 2021, Los Alamos National Laboratory
// SPDX-FileCopyrightText: Copyright (c) 2021. Triad National Security, LLC
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-LANL-Triad-USGov
#include "vtkLANLX3DReader.h"

// typical VTK boilerplate
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

// specific to this class
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkPointData.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

// common X3D reader code
#include "X3D_reader.hxx"
#include "X3D_tokens.hxx"

// for file detection
#include <sstream>
#include <sys/stat.h>

VTK_ABI_NAMESPACE_BEGIN
//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkLANLX3DReader);

//----------------------------------------------------------------------------
vtkLANLX3DReader::vtkLANLX3DReader()
{
  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
vtkLANLX3DReader::~vtkLANLX3DReader()
{
  this->SetFileName(nullptr);
}

//----------------------------------------------------------------------------
void vtkLANLX3DReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkLANLX3DReader::RequestInformation(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  // we can handle pieces
  vtkInformation* out_info = outputVector->GetInformationObject(0);
  out_info->Set(vtkAlgorithm::CAN_HANDLE_PIECE_REQUEST(), 1);

  return 1;
}

//----------------------------------------------------------------------------
int vtkLANLX3DReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  //
  // VTK stuff
  //
  vtkInformation* out_info = outputVector->GetInformationObject(0);
  // multiblock data set is required, because there are no multipiece data
  // set algorithms or filters
  vtkMultiBlockDataSet* output =
    vtkMultiBlockDataSet::SafeDownCast(out_info->Get(vtkDataObject::DATA_OBJECT()));
  output->SetNumberOfBlocks(1);

  // accepted pattern for VTK distributed data now is (per processor)
  // where n is number of files, and p is number of processors
  // 1 multiblock -> 1 multipiece -> 0 or 1 or (n/p) data sets
  //
  // pieces = processor and number of pieces = number of processors
  int piece = out_info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int n_pieces = out_info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  // this is the "real" data set result, as we have multiple file pieces
  vtkMultiPieceDataSet* mpds = vtkMultiPieceDataSet::New();
  output->SetBlock(0, mpds);

  // put other data into scope (on the stack) for goto
  X3D::Reader* x3d = nullptr;
  int return_code = 1;
  int first_file_piece, end_file_piece, global_first_file;
  bool has_numbered_files;
  std::string fn(this->FileName);

  if (this->FileName == nullptr)
  {
    vtkErrorMacro(<< "Fatal error. FileName is not set.");
    return_code = 0;
    goto X3D_EXIT_POINT; // yes, goto -- early return/break/guard vars is an
                         // annoying/bad code pattern for cleaning up in
                         // structured code and prone to error -- use goto
                         // to put all clean up in ONE place, rather than
                         // replication or putting silly guard variables
                         //
                         // Dijkstra was wrong
  }

  //
  // read the number of pieces on disk
  //

  // check that it ends with x3d or not
  // NOTE not unicode compliant! (but I don't think any of VTK is)
  if (fn.size() > 4 && (fn[fn.size() - 1] == 'd' || fn[fn.size() - 1] == 'D') &&
    (fn[fn.size() - 2] == '3') && (fn[fn.size() - 3] == 'x' || fn[fn.size() - 3] == 'X') &&
    (fn[fn.size() - 4] == '.'))
  {
    has_numbered_files = false;
    first_file_piece = 1;
    end_file_piece = 2;
  }
  // check that it ends with numbers and x3d
  // NOTE not unicode compliant! (but I don't think any of VTK is)
  else if (fn.size() > 10 && (fn[fn.size() - 1] > 47 && fn[fn.size() - 1] < 58) &&
    (fn[fn.size() - 2] > 47 && fn[fn.size() - 2] < 58) &&
    (fn[fn.size() - 3] > 47 && fn[fn.size() - 3] < 58) &&
    (fn[fn.size() - 4] > 47 && fn[fn.size() - 4] < 58) &&
    (fn[fn.size() - 5] > 47 && fn[fn.size() - 5] < 58) && (fn[fn.size() - 6] == '.') &&
    (fn[fn.size() - 7] == 'd' || fn[fn.size() - 7] == 'D') && (fn[fn.size() - 8] == '3') &&
    (fn[fn.size() - 9] == 'x' || fn[fn.size() - 9] == 'X') && (fn[fn.size() - 10] == '.'))
  {
    has_numbered_files = true;

    std::string base = fn.substr(0, fn.size() - 5);
    first_file_piece = std::stoi(fn.substr(fn.size() - 5));
    end_file_piece = first_file_piece + 1;
    fn = base;

    // if we want to read all of them
    if (this->ReadAllPieces)
    {
      // count upwards until we run out of files
      struct stat buffer;
      std::stringstream construct;
      construct << base << std::setw(5) << std::setfill('0') << end_file_piece;

      while (stat(construct.str().c_str(), &buffer) == 0)
      {
        end_file_piece = end_file_piece + 1;
        construct.str("");
        construct.clear();
        construct << base << std::setw(5) << std::setfill('0') << end_file_piece;
      }
    }
  }
  // incorrectly formatted filename
  else
  {
    vtkErrorMacro(<< "Fatal error. X3D file name is not formatted correctly: Needs to end in "
                     "'.x3d' or '.x3d.NNNNN'.");
    return_code = 0;
    goto X3D_EXIT_POINT;
  }

  // now we know how many file pieces we have
  // vtkMultiPiece data set allows us to represent each file with
  // an indepedent unstructured grid
  //
  // it is set to the number of file pieces we have in total
  mpds->SetNumberOfPieces(end_file_piece - first_file_piece);

  // return early if we have more VTK pieces than *actual* X3D file pieces
  // e.g., generate empty data on more processors than what we have on disk
  if (piece >= (end_file_piece - first_file_piece))
  {
    return_code = 1;
    goto X3D_EXIT_POINT;
  }

  // different cases for assigning files to pieces (processors)
  global_first_file = first_file_piece;
  if (has_numbered_files && n_pieces > 1)
  {
    // fewer processors than files -- assign them to processors
    // (the reason for the need for multipiece data set)
    if (n_pieces < (end_file_piece - first_file_piece))
    {
      double scale = (double)(end_file_piece - first_file_piece) / (double)n_pieces;
      int bias = first_file_piece;
      first_file_piece = (int)(piece * scale) + bias;
      // make sure we account for numerical error and get *all* files
      // i.e., if it's the end, leave end_file_piece as the last actual file
      // (the last processor sucks up any numerical error, off by 1-ish)
      if (piece + 1 < n_pieces)
      {
        end_file_piece = (int)((piece + 1) * scale) + bias;
      }
    }
    // equal (or more) number of pieces and processors -- but we
    // exited early if there were more
    else
    {
      first_file_piece = first_file_piece + piece;
      end_file_piece = first_file_piece + 1;
    }
  }

  //
  // read the data, looping over files assigned to this piece
  //
  try
  {
    for (int f = first_file_piece; f < end_file_piece; f++)
    {
      // if we have numbered files, construct the filename
      if (has_numbered_files)
      {
        std::stringstream construct;
        construct << fn << std::setw(5) << std::setfill('0') << f;
        x3d = new X3D::Reader(construct.str());
      }
      else
      {
        x3d = new X3D::Reader(fn);
      }
      X3D::Header header = x3d->header();

      // check if X3D processor matches file piece number
      int processor = header["process"];
      if (processor != f)
      {
        vtkErrorMacro(
          << "Warning in X3D header: 'process' does not match file number. Visualization may be "
             "wrong. Further fatal errors may occur in the X3D parser.");
      }

      // only support dim 2 or 3
      int dimension = header["numdim"];
      if (dimension != 2 && dimension != 3)
      {
        vtkErrorMacro(<< "Fatal error in X3D header: No VTK reader support for 'numdim' = "
                      << dimension);
        return_code = 0;
        goto X3D_EXIT_POINT;
      }

      // read the rest
      X3D::Materials matnames = x3d->matnames();
      X3D::Materials mateos = x3d->mateos();
      X3D::Materials matopc = x3d->matopc();
      X3D::Nodes nodes = x3d->nodes();
      X3D::Faces faces = x3d->faces();
      X3D::Cells cells = x3d->cells();
      X3D::ConstrainedNodes slaved_nodes = x3d->constrained_nodes();
      X3D::SharedNodes ghost_nodes = x3d->shared_nodes();
      X3D::CellData cell_data = x3d->cell_data();
      X3D::NodeData node_data = x3d->node_data();
      delete x3d;
      x3d = nullptr; // delete the reader

      //
      // translate x3d file mesh into VTK UG
      //
      vtkUnstructuredGrid* ug = vtkUnstructuredGrid::New();
      // set our file piece/multipiece number
      mpds->SetPiece(f - global_first_file, ug);

      //
      // translate points
      //
      size_t n_points = nodes.size();

      {
        vtkPoints* points = vtkPoints::New();
        points->SetNumberOfPoints(n_points);
        vtkIdTypeArray* pid = vtkIdTypeArray::New();
        pid->SetNumberOfValues(n_points);

        for (size_t i = 0; i < n_points; i++)
        {
          points->SetPoint(i, nodes[i][0], nodes[i][1], nodes[i][2]);
          pid->SetValue(i, header["process"]);
        }

        // set points
        ug->SetPoints(points);
        points->Delete();

        // set pid data
        pid->SetName("partition_number");
        ug->GetPointData()->AddArray(pid);
        pid->Delete();
      }

      //
      // translate cells
      //
      size_t n_cells = cells.size();

      // both 2D and 3D cells are boundary represented
      // 1D edges for 2D faces and 2D faces for 3D cells
      if (dimension == 2)
      {
        vtkCellArray* cell_list = vtkCellArray::New();
        vtkIdTypeArray* id_array = vtkIdTypeArray::New();
        vtkIdTypeArray* n_neighbors = vtkIdTypeArray::New();
        n_neighbors->SetNumberOfValues(n_cells);

        for (size_t i = 0; i < n_cells; i++)
        {
          size_t n_points_cell = cells[i].size();
          id_array->InsertNextValue(n_points_cell);

          size_t neighbors = 0;
          for (size_t j = 0; j < n_points_cell; j++)
          {
            size_t face_id = cells[i][j] - 1;
            // same winding direction as VTK, CCW -- take the first vertex
            // of a 1D edge
            id_array->InsertNextValue(faces[face_id].node_id[0] - 1);

            // count number of neighbor faces
            if (faces[face_id].neighbor_process_id != 0)
            {
              neighbors = neighbors + 1;
            }
          }
          n_neighbors->SetValue(i, neighbors);
        }

        // set cells
        cell_list->SetCells(n_cells, id_array);
        ug->SetCells(VTK_POLYGON, cell_list);
        id_array->Delete();
        cell_list->Delete();

        // set neighbors
        n_neighbors->SetName("number_of_neighbors");
        ug->GetCellData()->AddArray(n_neighbors);
        n_neighbors->Delete();
      }
      // dimension is 3
      else
      {
        vtkCellArray* cell_list = vtkCellArray::New();
        vtkIdTypeArray* id_array = vtkIdTypeArray::New();
        vtkIdTypeArray* n_neighbors = vtkIdTypeArray::New();
        n_neighbors->SetNumberOfValues(n_cells);

        for (size_t i = 0; i < n_cells; i++)
        {
          size_t n_faces = cells[i].size();
          size_t length_index = id_array->GetNumberOfValues();
          id_array->InsertNextValue(0); // unknown length, have to fixup later
          id_array->InsertNextValue(n_faces);

          size_t neighbors = 0;
          size_t length = 1;
          for (size_t j = 0; j < n_faces; j++)
          {
            size_t face_id = cells[i][j] - 1;

            size_t n_points_face = faces[face_id].node_id.size();
            id_array->InsertNextValue(n_points_face);
            length = length + 1 + n_points_face;

            for (size_t k = 0; k < n_points_face; k++)
            {
              // VTK's polyhedron is a boundary representation cell, too
              // just insert all of the 2D polygon faces
              id_array->InsertNextValue(faces[face_id].node_id[k] - 1);
            }

            // count number of owned faces and neighbor faces
            if (faces[face_id].neighbor_process_id != 0)
            {
              neighbors = neighbors + 1;
            }
          }
          n_neighbors->SetValue(i, neighbors);

          // go back and fixup length
          id_array->SetValue(length_index, length);
        }

        // set cells
        cell_list->SetCells(n_cells, id_array);
        ug->SetCells(VTK_POLYHEDRON, cell_list);
        id_array->Delete();
        cell_list->Delete();

        // set neighbors
        n_neighbors->SetName("number_of_neighbors");
        ug->GetCellData()->AddArray(n_neighbors);
        n_neighbors->Delete();
      }

      //
      // translate slave node attribute data
      //
      if (!slaved_nodes.empty())
      {
        vtkIdTypeArray* n_masters = vtkIdTypeArray::New();
        n_masters->SetNumberOfValues(n_points);
        n_masters->Fill(0);
        vtkIdTypeArray* n_slaves = vtkIdTypeArray::New();
        n_slaves->SetNumberOfValues(n_points);
        n_slaves->Fill(0);

        for (size_t i = 0; i < slaved_nodes.size(); i++)
        {
          int vertex = slaved_nodes[i].vertex_id - 1;
          size_t nm = slaved_nodes[i].master.size();
          n_masters->SetValue(vertex, nm);
          for (size_t j = 0; j < nm; j++)
          {
            size_t master = slaved_nodes[i].master[j] - 1;
            n_slaves->SetValue(master, n_slaves->GetValue(master) + 1);
          }
        }

        // set point data
        n_masters->SetName("number_of_masters");
        n_slaves->SetName("number_of_slaves");
        ug->GetPointData()->AddArray(n_masters);
        ug->GetPointData()->AddArray(n_slaves);
        n_masters->Delete();
        n_slaves->Delete();
      }

      //
      // translate ghost node data
      //
      if (!ghost_nodes.empty())
      {
        ug->AllocatePointGhostArray();
        vtkUnsignedCharArray* ghosts = ug->GetPointGhostArray();
        vtkIdTypeArray* owner = vtkIdTypeArray::New();
        owner->SetNumberOfValues(n_points);
        owner->Fill(0);

        for (size_t i = 0; i < ghost_nodes.size(); i++)
        {
          int point_id = ghost_nodes[i][0] - 1;

          // if the current piece does not match the owner, it's a VTK ghost
          if (ghost_nodes[i][1] != processor)
          {
            ghosts->SetValue(
              point_id, ghosts->GetValue(point_id) | vtkDataSetAttributes::DUPLICATEPOINT);
            owner->SetValue(point_id, ghost_nodes[i][1]);
          }
        }

        // set owner data
        owner->SetName("owning_partition");
        ug->GetPointData()->AddArray(owner);
        owner->Delete();
      }

      //
      // translate cell attribute data
      //

      // matid
      {
        vtkIntArray* matid = vtkIntArray::New();
        matid->SetNumberOfValues(n_cells);
        for (size_t i = 0; i < n_cells; i++)
        {
          matid->SetValue(i, cell_data.matid[i]);
        }

        // add matid
        matid->SetName("matid");
        ug->GetCellData()->AddArray(matid);
        matid->Delete();
      }

      // partelm
      {
        vtkIntArray* partelm = vtkIntArray::New();
        partelm->SetNumberOfValues(n_cells);
        for (size_t i = 0; i < n_cells; i++)
        {
          partelm->SetValue(i, cell_data.partelm[i]);
        }

        // add partelm
        partelm->SetName("partelm");
        ug->GetCellData()->AddArray(partelm);
        partelm->Delete();
      }

      // all others
      for (std::vector<std::string>::iterator iter = cell_data.names.begin();
           iter != cell_data.names.end(); iter++)
      {
        // skip matid and partelm
        if (*iter == "matid" || *iter == "partelm")
        {
          continue;
        }

        // attr
        vtkDoubleArray* attr = vtkDoubleArray::New();
        attr->SetNumberOfValues(n_cells);
        std::vector<double>& data = cell_data.fields[*iter];
        for (size_t i = 0; i < n_cells; i++)
        {
          attr->SetValue(i, data[i]);
        }

        // add attr
        attr->SetName(iter->c_str());
        ug->GetCellData()->AddArray(attr);
        attr->Delete();
      }

      //
      // translate point attribute data
      //

      // node attribute data are 3-vectors
      for (std::vector<std::string>::iterator iter = node_data.names.begin();
           iter != node_data.names.end(); iter++)
      {
        // attr
        vtkDoubleArray* attr = vtkDoubleArray::New();
        attr->SetNumberOfComponents(3);
        attr->SetNumberOfTuples(n_points);
        std::vector<X3D::Node>& data = node_data.fields[*iter];
        for (size_t i = 0; i < n_points; i++)
        {
          attr->SetTuple3(i, data[i][0], data[i][1], data[i][2]);
        }

        // add attr
        attr->SetName(iter->c_str());
        ug->GetPointData()->AddArray(attr);
        attr->Delete();
      }

      // all done with this file
      ug->Delete();
    }
  } // end of try { for {

  // if parser fails
  catch (const X3D::ReadError& e)
  {
    vtkErrorMacro(<< "Fatal error in X3D parsing: " << e.what());
    return_code = 0;
  }
  catch (const X3D::ScanError& e)
  {
    vtkErrorMacro(<< "Fatal error in X3D parsing: " << e.what());
    return_code = 0;
  }
  catch (...)
  {
    vtkErrorMacro(<< "Fatal error. Caught unknown exception in X3D parser.");
    return_code = 0;
  }

  // done -- one exit point because return/breaking out/guard booleans is/are
  // an annoying code pattern -- as well as try/catch, which should be
  // replaced with typed maybe/either/none
X3D_EXIT_POINT:
  delete x3d;
  mpds->Delete();
  return return_code;
}
VTK_ABI_NAMESPACE_END
