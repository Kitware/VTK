/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAMReXParticlesReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAMReXParticlesReader.h"

#include "vtkAOSDataArrayTemplate.h"
#include "vtkCellArray.h"
#include "vtkCommand.h"
#include "vtkDataArraySelection.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSOADataArrayTemplate.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtksys/FStream.hxx"
#include "vtksys/SystemTools.hxx"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <vector>

using vtksystools = vtksys::SystemTools;

namespace
{
// returns empty string on failure.
std::string ReadAndBroadCastFile(const std::string& filename, vtkMultiProcessController* controller)
{
  std::string contents;
  if (controller == nullptr || controller->GetLocalProcessId() == 0)
  {
    vtksys::ifstream stream(filename.c_str(), std::ios::binary);
    if (stream)
    {
      stream.seekg(0, std::ios::end);
      int flength = static_cast<int>(stream.tellg());
      stream.seekg(0, std::ios::beg);
      if (controller)
      {
        controller->Broadcast(&flength, 1, 0);
      }
      char* data = new char[flength + 1 + (flength + 1) % 8]; // padded for better alignment.
      stream.read(data, flength);
      if (controller)
      {
        controller->Broadcast(data, flength, 0);
      }
      data[flength] = '\0';
      contents = data;
      delete[] data;
      data = nullptr;
    }
  }
  else if (controller && controller->GetLocalProcessId() > 0)
  {
    int flength(0);
    controller->Broadcast(&flength, 1, 0);
    char* data = new char[flength + 1 + (flength + 1) % 8]; // padded for better alignment.
    if (controller)
    {
      controller->Broadcast(data, flength, 0);
    }
    data[flength] = '\0';
    contents = data;
    delete[] data;
    data = nullptr;
  }
  return contents;
}
}

#define AMREX_PRINT(os, indent, var) os << indent << #var << ": " << var << endl

class vtkAMReXParticlesReader::AMReXParticleHeader
{
  template <typename RealType, typename IntType>
  bool ReadParticles(
    vtkPolyData* pd, const int count, istream& ifp, const vtkAMReXParticlesReader* self) const
  {
    auto selection = self->GetPointDataArraySelection();

    // read integer data.
    vtkNew<vtkAOSDataArrayTemplate<IntType> > istuff;
    if (this->is_checkpoint)
    {
      istuff->SetNumberOfComponents(this->num_int);
      istuff->SetNumberOfTuples(count);
      if (!ifp.read(reinterpret_cast<char*>(istuff->GetPointer(0)),
            count * this->num_int * sizeof(IntType)))
      {
        return false;
      }
    }

    // read real data.
    vtkNew<vtkAOSDataArrayTemplate<RealType> > rstuff;
    rstuff->SetNumberOfComponents(this->num_real);
    rstuff->SetNumberOfTuples(count);
    if (!ifp.read(reinterpret_cast<char*>(rstuff->GetPointer(0)),
          count * this->num_real * sizeof(RealType)))
    {
      return false;
    }

    // Split out istuff and rstuff into separate arrays.
    if (this->num_int > 0)
    {
      std::vector<IntType*> iptrs(this->num_int, nullptr);
      std::vector<vtkIdType*> idtypeptrs(this->num_int, nullptr);

      for (int cc = 0; cc < this->num_int; ++cc)
      {
        // need to determine is the 1st two array have standard names.
        const std::string& name = (cc < this->num_int_base)
          ? this->int_base_component_names[cc]
          : this->int_component_names[cc - this->num_int_base];
        if (selection->GetArraySetting(name.c_str()) == 0)
        {
          continue;
        }

        // we'll handle "id" array separately.
        if (name == "id")
        {
          vtkNew<vtkIdTypeArray> idarray;
          idarray->SetName(name.c_str());
          idarray->SetNumberOfComponents(1);
          idarray->SetNumberOfTuples(count);
          pd->GetPointData()->AddArray(idarray);
          idtypeptrs[cc] = idarray->GetPointer(0);
        }
        else
        {
          vtkNew<vtkAOSDataArrayTemplate<IntType> > iarray;
          iarray->SetName(name.c_str());
          iarray->SetNumberOfComponents(1);
          iarray->SetNumberOfTuples(count);
          pd->GetPointData()->AddArray(iarray);
          iptrs[cc] = iarray->GetPointer(0);
        }
      }
      const IntType* isource = istuff->GetPointer(0);
      for (int cc = 0; cc < count; ++cc)
      {
        for (int kk = 0; kk < this->num_int; ++kk, ++isource)
        {
          if (iptrs[kk] != nullptr)
          {
            iptrs[kk][cc] = *isource;
          }
          else if (idtypeptrs[kk] != nullptr)
          {
            idtypeptrs[kk][cc] = static_cast<vtkIdType>(*isource);
          }
        }
      }
    }

    if (this->num_real > 0)
    {
      std::vector<RealType*> rptrs(this->num_real, nullptr);

      assert(this->num_real_base == this->dim);
      vtkNew<vtkAOSDataArrayTemplate<RealType> > coords;
      coords->SetName("Points");
      coords->SetNumberOfComponents(3);
      coords->SetNumberOfTuples(count);
      if (this->num_real_base < 3)
      {
        // fill with 0, since this->dim may be less than 3.
        std::fill_n(coords->GetPointer(0), 3 * count, 0.0);
      }

      vtkNew<vtkPoints> pts;
      pts->SetData(coords);
      pd->SetPoints(pts);

      rptrs[0] = coords->GetPointer(0);
      for (int cc = this->num_real_base; cc < this->num_real; ++cc)
      {
        const auto& name = this->real_component_names[cc - this->num_real_base];
        if (selection->GetArraySetting(name.c_str()) == 0)
        {
          continue;
        }
        vtkNew<vtkAOSDataArrayTemplate<RealType> > rarray;
        rarray->SetName(name.c_str());
        rarray->SetNumberOfComponents(1);
        rarray->SetNumberOfTuples(count);
        pd->GetPointData()->AddArray(rarray);
        rptrs[cc] = rarray->GetPointer(0);
      }

      const RealType* rsource = rstuff->GetPointer(0);
      for (int cc = 0; cc < count; ++cc)
      {
        for (int kk = 0; kk < this->num_real_base; ++kk)
        {
          rptrs[0][this->num_real_base * cc + kk] = *rsource++;
        }
        for (int kk = this->num_real_base; kk < this->num_real; ++kk, ++rsource)
        {
          if (rptrs[kk] != nullptr)
          {
            rptrs[kk][cc] = *rsource;
          }
        }
      }
    }

    //// Now build connectivity information.
    // vtkNew<vtkCellArray> verts;
    // verts->Allocate(verts->EstimateSize(count, 1));
    // for (vtkIdType cc=0; cc < count; ++cc)
    //{
    //  verts->InsertNextCell(1, &cc);
    //}
    // pd->SetVerts(verts);
    return true;
  }

  // seems like the DATA_<filenumber> files can be written with differing number
  // of leading zeros. That being the case, we try a few options starting the
  // most recent successful match.
  // Returns empty string if failed to find a valid filename.
  std::string GetDATAFileName(
    const std::string& plotfilename, const std::string& ptype, int level, int filenumber) const
  {
    std::string fname =
      this->GetDATAFileName(plotfilename, ptype, level, filenumber, this->DataFormatZeroFill);
    if (!fname.empty())
    {
      return fname;
    }

    for (int cc = 7; cc >= 0; --cc)
    {
      fname = this->GetDATAFileName(plotfilename, ptype, level, filenumber, cc);
      if (!fname.empty())
      {
        this->DataFormatZeroFill = cc;
        return fname;
      }
    }

    return std::string();
  }

  // Returns empty string if failed to find a valid filename.
  std::string GetDATAFileName(const std::string& plotfilename, const std::string& ptype, int level,
    int filenumber, int zerofill) const
  {
    std::ostringstream str;
    str << plotfilename << "/" << ptype << "/Level_" << level << "/DATA_" << std::setfill('0')
        << std::setw(zerofill) << filenumber;
    return (vtksys::SystemTools::FileExists(str.str(), /*isFile*/ true)) ? str.str()
                                                                         : std::string();
  }

  mutable int DataFormatZeroFill;

public:
  struct GridInfo
  {
    int which;
    int count;
    vtkTypeInt64 where;
  };

  // the names are deliberately kept consistent with am
  std::vector<std::string> real_component_names;
  std::vector<std::string> int_component_names;
  std::vector<std::string> int_base_component_names;
  size_t int_type;
  size_t real_type;
  int dim;
  int num_int_base;
  int num_real_base;
  int num_real_extra;
  int num_int_extra;
  int num_int;
  int num_real;
  bool is_checkpoint;
  vtkIdType num_particles;
  vtkIdType max_next_id;
  int finest_level;
  int num_levels;
  std::vector<int> grids_per_level;
  std::vector<std::vector<GridInfo> > grids;

  AMReXParticleHeader()
    : DataFormatZeroFill(5)
    , real_component_names()
    , int_component_names()
    , int_base_component_names()
    , int_type(0)
    , real_type(0)
    , dim(0)
    , num_int_base(0)
    , num_real_base(0)
    , num_real_extra(0)
    , num_int_extra(0)
    , num_int(0)
    , num_real(0)
    , is_checkpoint(false)
    , num_particles(0)
    , max_next_id(0)
    , finest_level(0)
    , num_levels(0)
    , grids_per_level()
    , grids()
  {
  }

  void PrintSelf(ostream& os, vtkIndent indent)
  {
    AMREX_PRINT(os, indent, real_type);
    AMREX_PRINT(os, indent, int_type);
    AMREX_PRINT(os, indent, dim);
    AMREX_PRINT(os, indent, num_int_base);
    AMREX_PRINT(os, indent, num_real_base);
    AMREX_PRINT(os, indent, num_real_extra);
    AMREX_PRINT(os, indent, num_int_extra);
    AMREX_PRINT(os, indent, num_int);
    AMREX_PRINT(os, indent, num_real);
    AMREX_PRINT(os, indent, is_checkpoint);
    AMREX_PRINT(os, indent, num_particles);
    AMREX_PRINT(os, indent, max_next_id);
    AMREX_PRINT(os, indent, finest_level);
    AMREX_PRINT(os, indent, num_levels);
    os << indent << "grids_per_level: " << endl;
    for (const int& gpl : this->grids_per_level)
    {
      os << indent.GetNextIndent() << gpl << endl;
    }
    os << indent << "grids: " << endl;
    int level = 0;
    for (const auto& grids_level : this->grids)
    {
      os << indent.GetNextIndent() << "level: " << level << endl;
      for (const auto& ginfo : grids_level)
      {
        os << indent.GetNextIndent().GetNextIndent() << "which: " << ginfo.which
           << " count: " << ginfo.count << " where: " << ginfo.where << endl;
      }
      level++;
    }

    os << indent << "real_component_names: " << endl;
    for (const auto& name : this->real_component_names)
    {
      os << indent.GetNextIndent() << name << endl;
    }
    os << indent << "int_component_names: " << endl;
    for (const auto& name : this->int_component_names)
    {
      os << indent.GetNextIndent() << name << endl;
    }
  }

  bool Parse(const std::string& headerData)
  {
    std::istringstream hstream(headerData);
    std::string version;
    hstream >> version;
    if (version.empty())
    {
      vtkGenericWarningMacro("Failed to read version string.");
      return false;
    }

    this->int_type = 32;

    // What do our version strings mean? (from ParticleContainer::Restart)
    // "Version_One_Dot_Zero" -- hard-wired to write out in double precision.
    // "Version_One_Dot_One" -- can write out either as either single or double precision.
    // Appended to the latter version string are either "_single" or "_double" to
    // indicate how the particles were written.
    // "Version_Two_Dot_Zero" -- this is the AMReX particle file format
    if (version.find("Version_One_Dot_Zero") != std::string::npos)
    {
      this->real_type = 64;
    }
    else if (version.find("Version_One_Dot_One") != std::string::npos ||
      version.find("Version_Two_Dot_Zero") != std::string::npos)
    {
      if (version.find("_single") != std::string::npos)
      {
        this->real_type = 32;
      }
      else if (version.find("_double") != std::string::npos)
      {
        this->real_type = 64;
      }
      else
      {
        vtkGenericWarningMacro("Bad version string: " << version);
        return false;
      }
    }
    else
    {
      vtkGenericWarningMacro("Bad version string: " << version);
      return false;
    }

    hstream >> this->dim;
    if (this->dim != 1 && this->dim != 2 && this->dim != 3)
    {
      vtkGenericWarningMacro("dim must be 1, 2, or 3.");
      return false;
    }

    this->num_int_base = 2;
    this->num_real_base = this->dim;

    hstream >> this->num_real_extra;
    if (this->num_real_extra < 0 || this->num_real_extra > 1024)
    {
      vtkGenericWarningMacro("potentially incorrect num_real_extra=" << this->num_real_extra);
      return false;
    }
    this->real_component_names.resize(this->num_real_extra);
    for (int cc = 0; cc < this->num_real_extra; ++cc)
    {
      hstream >> this->real_component_names[cc];
    }

    hstream >> this->num_int_extra;
    if (this->num_int_extra < 0 || this->num_int_extra > 1024)
    {
      vtkGenericWarningMacro("potentially incorrect num_int_extra=" << this->num_int_extra);
      return false;
    }
    this->int_component_names.resize(this->num_int_extra);
    for (int cc = 0; cc < this->num_int_extra; ++cc)
    {
      hstream >> this->int_component_names[cc];
    }

    this->num_real = this->num_real_base + this->num_real_extra;
    this->num_int = this->num_int_base + this->num_int_extra;

    hstream >> this->is_checkpoint;
    hstream >> this->num_particles;
    if (this->num_particles < 0)
    {
      vtkGenericWarningMacro("num_particles must be >=0");
      return false;
    }

    hstream >> this->max_next_id;
    if (this->max_next_id <= 0)
    {
      vtkGenericWarningMacro("max_next_id must be > 0");
      return false;
    }

    hstream >> this->finest_level;
    if (this->finest_level < 0)
    {
      vtkGenericWarningMacro("finest_level must be >= 0");
      return false;
    }

    this->num_levels = this->finest_level + 1;

    if (!this->is_checkpoint)
    {
      this->num_int_base = 0;
      this->num_int_extra = 0;
      this->num_int = 0;
    }
    else
    {
      this->int_base_component_names.push_back("id");
      this->int_base_component_names.push_back("cpu");
    }

    this->grids_per_level.resize(this->num_levels, 0);
    for (int lev = 0; lev < this->num_levels; ++lev)
    {
      hstream >> this->grids_per_level[lev];
      assert(this->grids_per_level[lev] > 0);
    }

    this->grids.resize(this->num_levels);
    for (int lev = 0; lev < this->num_levels; ++lev)
    {
      auto& grids_lev = this->grids[lev];
      grids_lev.resize(this->grids_per_level[lev]);
      for (int grid_num = 0; grid_num < this->grids_per_level[lev]; ++grid_num)
      {
        hstream >> grids_lev[grid_num].which >> grids_lev[grid_num].count >>
          grids_lev[grid_num].where;
      }
    }
    return true;
  }

  bool ReadGrid(int level, int idx, vtkPolyData* pd, const vtkAMReXParticlesReader* self) const
  {
    assert(level < this->num_levels && idx < this->grids_per_level[level]);

    auto& gridInfo = this->grids[level][idx];
    if (gridInfo.count == 0)
    {
      // empty grid.
      return true;
    }

    const std::string& fname =
      this->GetDATAFileName(self->PlotFileName, self->ParticleType, level, gridInfo.which);
    vtksys::ifstream ifp(fname.c_str(), std::ios::binary);
    if (!ifp.good())
    {
      return false;
    }

    ifp.seekg(gridInfo.where, std::ios::beg);

    if (this->real_type == 32 && this->int_type == 32)
    {
      return this->ReadParticles<float, vtkTypeInt32>(pd, gridInfo.count, ifp, self);
    }
    else if (this->real_type == 64 && this->int_type == 32)
    {
      return this->ReadParticles<double, vtkTypeInt32>(pd, gridInfo.count, ifp, self);
    }
    else
    {
      return false;
    }
  }

  void PopulatePointArraySelection(vtkDataArraySelection* selection) const
  {
    for (auto& aname : this->int_base_component_names)
    {
      selection->AddArray(aname.c_str());
    }
    for (auto& aname : this->int_component_names)
    {
      selection->AddArray(aname.c_str());
    }
    for (auto& aname : this->real_component_names)
    {
      selection->AddArray(aname.c_str());
    }
  }
};

vtkStandardNewMacro(vtkAMReXParticlesReader);
vtkCxxSetObjectMacro(vtkAMReXParticlesReader, Controller, vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkAMReXParticlesReader::vtkAMReXParticlesReader()
  : Controller(nullptr)
  , PlotFileName()
  , PlotFileNameMTime()
  , MetaDataMTime()
  , ParticleType("particles")
  , Header(nullptr)
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->SetController(vtkMultiProcessController::GetGlobalController());
  this->PointDataArraySelection->AddObserver(
    vtkCommand::ModifiedEvent, this, &vtkAMReXParticlesReader::Modified);
}

//----------------------------------------------------------------------------
vtkAMReXParticlesReader::~vtkAMReXParticlesReader()
{
  this->SetController(nullptr);
  delete this->Header;
}

//----------------------------------------------------------------------------
void vtkAMReXParticlesReader::SetPlotFileName(const char* fname)
{
  const std::string filename(fname == nullptr ? "" : fname);
  if (this->PlotFileName != filename)
  {
    this->PlotFileName = filename;
    this->PlotFileNameMTime.Modified();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkAMReXParticlesReader::SetParticleType(const std::string& str)
{
  if (this->ParticleType != str)
  {
    this->ParticleType = str;
    this->PlotFileNameMTime.Modified(); // since we need to re-read metadata.
    this->Modified();
  }
}

//----------------------------------------------------------------------------
vtkDataArraySelection* vtkAMReXParticlesReader::GetPointDataArraySelection() const
{
  return this->PointDataArraySelection;
}

//----------------------------------------------------------------------------
int vtkAMReXParticlesReader::CanReadFile(const char* fname, const char* particleType)
{
  if (fname && vtksystools::FileIsDirectory(fname))
  {
    if (!vtksystools::FileExists(std::string(fname) + "/Header", true))
    {
      return false;
    }

    if (particleType == nullptr)
    {
      // may be should check for existence of subdirectories that could
      // potentially contain particles?
      return true;
    }

    // now let's confirm it has "particles" directory.
    const std::string particles = std::string(fname) + "/" + particleType;
    if (vtksystools::FileIsDirectory(particles))
    {
      const std::string header(particles + "/Header");
      if (vtksystools::FileExists(header, /*isFile*/ true))
      {
        vtksys::ifstream ifp(header.c_str(), std::ios::binary);
        if (ifp)
        {
          std::string header_line;
          if (std::getline(ifp, header_line))
          {
            return (header_line == "Version_Two_Dot_Zero_double" ||
              header_line == "Version_Two_Dot_Zero_float");
          }
        }
      }
    }
  }
  return 0;
}

//----------------------------------------------------------------------------
const char* vtkAMReXParticlesReader::GetPlotFileName() const
{
  return (this->PlotFileName.empty() ? nullptr : this->PlotFileName.c_str());
}

//----------------------------------------------------------------------------
void vtkAMReXParticlesReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PlotFileName: " << this->PlotFileName << endl;
  if (this->Header)
  {
    os << indent << "Header: " << endl;
    this->Header->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Header: nullptr" << endl;
  }
  os << indent << "PointDataArraySelection: " << endl;
  this->PointDataArraySelection->PrintSelf(os, indent.GetNextIndent());
}

//----------------------------------------------------------------------------
int vtkAMReXParticlesReader::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // read meta-data to fill up point array selection information.
  if (!this->ReadMetaData())
  {
    return 0;
  }

  auto outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
  return this->Superclass::RequestInformation(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkAMReXParticlesReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  if (!this->ReadMetaData())
  {
    return 0;
  }
  assert(this->Header);

  // we could use a smarter strategy, but for now, we'll stick to a very simply
  // distribution strategy: each level is distributed among requested pieces
  // in a contiguous fashion.
  auto* outInfo = outputVector->GetInformationObject(0);
  using sddp = vtkStreamingDemandDrivenPipeline;
  int update_piece = 0, update_num_pieces = 1;
  if (outInfo->Has(sddp::UPDATE_PIECE_NUMBER()) && outInfo->Has(sddp::UPDATE_NUMBER_OF_PIECES()))
  {
    update_piece = outInfo->Get(sddp::UPDATE_PIECE_NUMBER());
    update_num_pieces = outInfo->Get(sddp::UPDATE_NUMBER_OF_PIECES());
  }

  vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::GetData(outputVector, 0);

  const auto& header = (*this->Header);

  // Let's do as many blocks as levels and distribute each level among pieces.
  output->SetNumberOfBlocks(header.num_levels);

  for (int cc = 0; cc < header.num_levels; ++cc)
  {
    vtkNew<vtkMultiPieceDataSet> piece;
    output->SetBlock(cc, piece);
    output->GetMetaData(cc)->Set(
      vtkCompositeDataSet::NAME(), (std::string("Level ") + std::to_string(cc)).c_str());
    this->ReadLevel(cc, piece, update_piece, update_num_pieces);
  }

  return 1;
}

//----------------------------------------------------------------------------
bool vtkAMReXParticlesReader::ReadMetaData()
{
  if (this->MetaDataMTime > this->PlotFileNameMTime)
  {
    return true;
  }

  delete this->Header;
  this->Header = nullptr;

  if (this->PlotFileName.empty())
  {
    vtkErrorMacro("PlotFileName must be specified.");
    return false;
  }

  if (this->ParticleType.empty())
  {
    vtkErrorMacro("ParticleType must be specified.");
    return false;
  }

  const std::string hdrFileName = this->PlotFileName + "/" + this->ParticleType + "/Header";
  const auto headerData = ::ReadAndBroadCastFile(hdrFileName, this->Controller);
  if (headerData.empty())
  {
    return false;
  }

  auto headerPtr = new AMReXParticleHeader();
  if (!headerPtr->Parse(headerData))
  {
    delete headerPtr;
    return false;
  }

  this->Header = headerPtr;
  this->Header->PopulatePointArraySelection(this->PointDataArraySelection);
  this->MetaDataMTime.Modified();
  return true;
}

//----------------------------------------------------------------------------
bool vtkAMReXParticlesReader::ReadLevel(
  const int level, vtkMultiPieceDataSet* levelDS, const int piece_idx, const int num_pieces) const
{
  assert(level >= 0 && this->Header != nullptr && piece_idx >= 0 && num_pieces >= 1);

  auto& header = (*this->Header);

  assert(level < header.num_levels);

  const int& num_grids = header.grids_per_level[level];
  const int quotient = num_grids / num_pieces;
  const int remainder = num_grids % num_pieces;

  const int start_grid_idx = (piece_idx * quotient) + ((piece_idx < remainder) ? 1 : 0);
  const int grids_count = quotient + ((piece_idx < remainder) ? 1 : 0);

  levelDS->SetNumberOfPieces(num_grids);
  for (int cc = start_grid_idx; cc < start_grid_idx + grids_count; ++cc)
  {
    vtkNew<vtkPolyData> pd;
    if (header.ReadGrid(level, cc, pd, this) == false)
    {
      vtkGenericWarningMacro("Failed to read grid for level " << level << ", index " << cc);
      return false;
    }
    levelDS->SetPiece(cc, pd);
  }

  return true;
}
