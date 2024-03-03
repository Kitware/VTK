/*============================================================================
  MetaIO
  Copyright 2000-2010 Insight Software Consortium

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
#ifdef _MSC_VER
#  pragma warning(disable : 4702)
#  pragma warning(disable : 4284)
#endif

#include "metaMesh.h"

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE
{
#endif

union bufferAlignedUnion
{
  char              character;
  unsigned char     ucharacter;
  short             shortint;
  unsigned short    ushortint;
  int               integer;
  unsigned int      uinteger;
  long int          linteger;
  unsigned long int ulinteger;
  float             floatingpoint;
  double            doublefloatingpoint;
};

MeshPoint::MeshPoint(int dim)
{
  m_Dim = static_cast<unsigned int>(dim);
  m_X = new float[m_Dim];
  for (unsigned int i = 0; i < m_Dim; i++)
  {
    m_X[i] = 0;
  }
}

MeshPoint::~MeshPoint()
{
  delete[] m_X;
}

MeshCell::MeshCell(int dim)
{
  m_Dim = static_cast<unsigned int>(dim);
  m_Id = -1;
  m_PointsId = new int[m_Dim];
  for (unsigned int i = 0; i < m_Dim; i++)
  {
    m_PointsId[i] = -1;
  }
}

MeshCell::~MeshCell()
{
  delete[] m_PointsId;
}


//
// MetaMesh Constructors
//
MetaMesh::MetaMesh()
{
  META_DEBUG_PRINT( "MetaMesh()" );
  m_NPoints = 0;

  for (auto & i : m_CellListArray)
  {
    i = nullptr;
  }
  MetaMesh::Clear();
}

//
MetaMesh::MetaMesh(const char * _headerName)
{
  META_DEBUG_PRINT( "MetaMesh()" );
  m_NPoints = 0;

  for (auto & i : m_CellListArray)
  {
    i = nullptr;
  }
  MetaMesh::Clear();
  MetaMesh::Read(_headerName);
}

//
MetaMesh::MetaMesh(const MetaMesh * _mesh)
{
  META_DEBUG_PRINT( "MetaMesh()" );
  m_NPoints = 0;
  for (auto & i : m_CellListArray)
  {
    i = nullptr;
  }
  MetaMesh::Clear();
  MetaMesh::CopyInfo(_mesh);
}


//
MetaMesh::MetaMesh(unsigned int dim)
  : MetaObject(dim)
{
  META_DEBUG_PRINT( "MetaMesh()" );
  m_NPoints = 0;
  for (auto & i : m_CellListArray)
  {
    i = nullptr;
  }
  MetaMesh::Clear();
}

/** Destructor */
MetaMesh::~MetaMesh()
{
  MetaMesh::Clear();
  for (auto & i : m_CellListArray)
  {
    delete i;
    i = nullptr;
  }

  MetaObject::M_Destroy();
}

//
void
MetaMesh::PrintInfo() const
{
  MetaObject::PrintInfo();
  std::cout << "PointDim = " << m_PointDim << std::endl;
  std::cout << "NPoints = " << m_NPoints << std::endl;
  char str[255];
  MET_TypeToString(m_PointType, str);
  std::cout << "PointType = " << str << std::endl;
  MET_TypeToString(m_PointDataType, str);
  std::cout << "PointDataType = " << str << std::endl;
  MET_TypeToString(m_CellDataType, str);
  std::cout << "CellDataType = " << str << std::endl;
}

void
MetaMesh::CopyInfo(const MetaObject * _object)
{
  MetaObject::CopyInfo(_object);
}

int
MetaMesh::NPoints() const
{
  return m_NPoints;
}

int
MetaMesh::NCells() const
{
  return m_NCells;
}

int
MetaMesh::NCellLinks() const
{
  return m_NCellLinks;
}

/** Clear tube information */
void
MetaMesh::Clear()
{
  META_DEBUG_PRINT( "MetaMesh: Clear" );

  MetaObject::Clear();

  strcpy(m_ObjectTypeName, "Mesh");

  META_DEBUG_PRINT( "MetaMesh: Clear: m_NPoints" );

  // Delete the list of pointers to points.
  auto it_pnt = m_PointList.begin();
  while (it_pnt != m_PointList.end())
  {
    MeshPoint * pnt = *it_pnt;
    ++it_pnt;
    delete pnt;
  }

  // Delete the list of pointers to celllinks
  auto it_celllinks = m_CellLinks.begin();
  while (it_celllinks != m_CellLinks.end())
  {
    MeshCellLink * link = *it_celllinks;
    ++it_celllinks;
    delete link;
  }

  // Delete the list of pointers to pointdata
  auto it_pointdata = m_PointData.begin();
  while (it_pointdata != m_PointData.end())
  {
    MeshDataBase * data = *it_pointdata;
    ++it_pointdata;
    delete data;
  }

  // Delete the list of pointers to celldata
  auto it_celldata = m_CellData.begin();
  while (it_celldata != m_CellData.end())
  {
    MeshDataBase * data = *it_celldata;
    ++it_celldata;
    delete data;
  }

  // Initialize the new array
  for (auto & i : m_CellListArray)
  {
    if (i)
    {
      // Delete the list of pointers to cells.
      auto it_cell = i->begin();
      while (it_cell != i->end())
      {
        MeshCell * cell = *it_cell;
        ++it_cell;
        delete cell;
      }
      delete i;
    }
    i = new CellListType;
  }

  m_PointList.clear();
  m_PointData.clear();
  m_CellData.clear();

  m_NPoints = 0;
  m_NCells = 0;
  m_NCellLinks = 0;
  m_NCellData = 0;
  m_NPointData = 0;
  strcpy(m_PointDim, "ID x y ...");
  m_PointType = MET_FLOAT;
  m_PointDataType = MET_FLOAT;
  m_CellDataType = MET_FLOAT;
}

/** Set Read fields */
void
MetaMesh::M_SetupReadFields()
{
  META_DEBUG_PRINT( "MetaMesh: M_SetupReadFields" );

  MetaObject::M_SetupReadFields();

  MET_FieldRecordType * mF;
  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "NCellTypes", MET_INT, true);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "PointDim", MET_STRING, true);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "NPoints", MET_INT, true);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "PointType", MET_STRING, true);
  mF->required = true;
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "PointDataType", MET_STRING, true);
  mF->required = true;
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "CellDataType", MET_STRING, true);
  mF->required = true;
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "Points", MET_NONE, true);
  mF->terminateRead = true;
  m_Fields.push_back(mF);
}

void
MetaMesh::M_SetupWriteFields()
{
  MetaObject::M_SetupWriteFields();

  MET_FieldRecordType * mF;

  char s[255];
  mF = new MET_FieldRecordType;
  MET_TypeToString(m_PointType, s);
  MET_InitWriteField(mF, "PointType", MET_STRING, strlen(s), s);
  m_Fields.push_back(mF);

  // Find the pointDataType
  if (!m_PointData.empty())
  {
    m_PointDataType = (*m_PointData.begin())->GetMetaType();
  }

  char s1[255];
  mF = new MET_FieldRecordType;
  MET_TypeToString(m_PointDataType, s1);
  MET_InitWriteField(mF, "PointDataType", MET_STRING, strlen(s1), s1);
  m_Fields.push_back(mF);

  char s2[255];
  mF = new MET_FieldRecordType;
  MET_TypeToString(m_CellDataType, s2);
  MET_InitWriteField(mF, "CellDataType", MET_STRING, strlen(s2), s2);
  m_Fields.push_back(mF);

  unsigned int numberOfCellTypes = 0;
  for (auto & i : m_CellListArray)
  {
    if (!i->empty())
    {
      numberOfCellTypes++;
    }
  }
  if (numberOfCellTypes)
  {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "NCellTypes", MET_INT, numberOfCellTypes);
    m_Fields.push_back(mF);
  }

  if (strlen(m_PointDim) > 0)
  {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "PointDim", MET_STRING, strlen(m_PointDim), m_PointDim);
    m_Fields.push_back(mF);
  }

  m_NPoints = static_cast<int>(m_PointList.size());
  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "NPoints", MET_INT, m_NPoints);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "Points", MET_NONE);
  m_Fields.push_back(mF);
}


bool
MetaMesh::M_Read()
{

  META_DEBUG_PRINT( "MetaMesh: M_Read: Loading Header" );

  if (!MetaObject::M_Read())
  {
    std::cout << "MetaMesh: M_Read: Error parsing file" << std::endl;
    return false;
  }

  META_DEBUG_PRINT( "MetaMesh: M_Read: Parsing Header" );

  MET_FieldRecordType * mF;

  unsigned int numberOfCellTypes = 0;
  mF = MET_GetFieldRecord("NCellTypes", &m_Fields);
  if (mF->defined)
  {
    numberOfCellTypes = static_cast<unsigned int>(mF->value[0]);
  }

  mF = MET_GetFieldRecord("NPoints", &m_Fields);
  if (mF->defined)
  {
    m_NPoints = static_cast<int>(mF->value[0]);
  }

  mF = MET_GetFieldRecord("PointType", &m_Fields);
  if (mF->defined)
  {
    MET_StringToType(reinterpret_cast<char *>(mF->value), &m_PointType);
  }

  mF = MET_GetFieldRecord("PointDataType", &m_Fields);
  if (mF->defined)
  {
    MET_StringToType(reinterpret_cast<char *>(mF->value), &m_PointDataType);
  }

  mF = MET_GetFieldRecord("CellDataType", &m_Fields);
  if (mF->defined)
  {
    MET_StringToType(reinterpret_cast<char *>(mF->value), &m_CellDataType);
  }

  mF = MET_GetFieldRecord("PointDim", &m_Fields);
  if (mF->defined)
  {
    strcpy(m_PointDim, reinterpret_cast<char *>(mF->value));
  }

  if (m_BinaryData)
  {
    int elementSize;
    MET_SizeOfType(m_PointType, &elementSize);
    int readSize = m_NPoints * (m_NDims)*elementSize + m_NPoints * sizeof(int);

    char * _data = new char[readSize];
    m_ReadStream->read(_data, readSize);

    int gc = static_cast<int>(m_ReadStream->gcount());
    if (gc != readSize)
    {
      std::cout << "MetaMesh: m_Read: Points not read completely" << std::endl;
      std::cout << "   ideal = " << readSize << " : actual = " << gc << std::endl;
      delete[] _data;
      return false;
    }

    int i = 0;
    for (int j = 0; j < m_NPoints; j++)
    {
      auto * pnt = new MeshPoint(m_NDims);
      {
        int          td;
        char * const num = reinterpret_cast<char *>(&td);
        for (unsigned int k = 0; k < sizeof(int); k++)
        {
          num[k] = _data[i + k];
        }
        MET_SwapByteIfSystemMSB(&td, MET_INT);
        pnt->m_Id = td;
        i += sizeof(int);
      }

      for (int d = 0; d < m_NDims; d++)
      {
        bufferAlignedUnion alignedBuffer{};
        char * const       num = reinterpret_cast<char *>(&alignedBuffer);
        for (unsigned int k = 0; k < static_cast<unsigned int>(elementSize); k++)
        {
          num[k] = _data[i + k];
        }
        i += elementSize;

        if (m_PointType == MET_CHAR)
        {
          pnt->m_X[d] = alignedBuffer.character;
        }
        else if (m_PointType == MET_UCHAR)
        {
          pnt->m_X[d] = alignedBuffer.ucharacter;
        }
        else if (m_PointType == MET_SHORT)
        {
          short val = alignedBuffer.shortint;
          MET_SwapByteIfSystemMSB(&val, MET_SHORT);
          pnt->m_X[d] = val;
        }
        else if (m_PointType == MET_USHORT)
        {
          unsigned short val = alignedBuffer.ushortint;
          MET_SwapByteIfSystemMSB(&val, MET_USHORT);
          pnt->m_X[d] = val;
        }
        else if (m_PointType == MET_INT)
        {
          int val = alignedBuffer.integer;
          MET_SwapByteIfSystemMSB(&val, MET_INT);
          pnt->m_X[d] = static_cast<float>(val);
        }
        else if (m_PointType == MET_UINT)
        {
          unsigned int val = alignedBuffer.uinteger;
          MET_SwapByteIfSystemMSB(&val, MET_UINT);
          pnt->m_X[d] = static_cast<float>(val);
        }
        else if (m_PointType == MET_LONG)
        {
          long val = alignedBuffer.linteger;
          MET_SwapByteIfSystemMSB(&val, MET_LONG);
          pnt->m_X[d] = static_cast<float>(val);
        }
        else if (m_PointType == MET_ULONG)
        {
          unsigned long val = alignedBuffer.ulinteger;
          MET_SwapByteIfSystemMSB(&val, MET_ULONG);
          pnt->m_X[d] = static_cast<float>(val);
        }
        else if (m_PointType == MET_FLOAT)
        {
          float val = alignedBuffer.floatingpoint;
          MET_SwapByteIfSystemMSB(&val, MET_FLOAT);
          pnt->m_X[d] = val;
        }
        else if (m_PointType == MET_DOUBLE)
        {
          double val = alignedBuffer.doublefloatingpoint;
          MET_SwapByteIfSystemMSB(&val, MET_DOUBLE);
          pnt->m_X[d] = static_cast<float>(val);
        }
      }
      m_PointList.push_back(pnt);
    }
    delete[] _data;
  }
  else
  {
    for (int j = 0; j < m_NPoints; j++)
    {
      auto * pnt = new MeshPoint(m_NDims);

      float v[10];
      for (int k = 0; k < m_NDims + 1; k++)
      {
        *m_ReadStream >> v[k];
        m_ReadStream->get();
      }

      int d;
      pnt->m_Id = static_cast<int>(v[0]);
      for (d = 0; d < m_NDims; d++)
      {
        pnt->m_X[d] = v[d + 1];
      }
      m_PointList.push_back(pnt);
    }

    char c = ' ';
    while ((c != '\n') && (!m_ReadStream->eof()))
    {
      c = static_cast<char>(m_ReadStream->get()); // to avoid unrecognize charactere
    }
  }

  // Now reading the cells
  for (unsigned int nCellType = 0; nCellType < numberOfCellTypes; nCellType++)
  {
    MetaObject::ClearFields();
    mF = new MET_FieldRecordType;
    MET_InitReadField(mF, "CellType", MET_STRING, true);
    m_Fields.push_back(mF);

    mF = new MET_FieldRecordType;
    MET_InitReadField(mF, "NCells", MET_INT, true);
    m_Fields.push_back(mF);

    mF = new MET_FieldRecordType;
    MET_InitReadField(mF, "Cells", MET_NONE, true);
    mF->terminateRead = true;
    m_Fields.push_back(mF);

    if (!MET_Read(*m_ReadStream, &m_Fields))
    {
      std::cout << "MetaObject: Read: MET_Read Failed" << std::endl;
      return false;
    }

    mF = MET_GetFieldRecord("NCells", &m_Fields);
    if (mF->defined)
    {
      m_NCells = static_cast<int>(mF->value[0]);
    }

    MET_CellGeometry celltype = MET_VERTEX_CELL;

    mF = MET_GetFieldRecord("CellType", &m_Fields);
    if (mF->defined)
    {
      for (int j = 0; j < MET_NUM_CELL_TYPES; j++)
      {
        if (!strncmp(reinterpret_cast<char *>(mF->value), MET_CellTypeName[j], 3))
        {
          celltype = static_cast<MET_CellGeometry>(j);
        }
      }
    }

    if (m_BinaryData)
    {
      auto totalcellsize = static_cast<unsigned int>((MET_CellSize[celltype] + 1) * m_NCells);
      int          readSize = totalcellsize * sizeof(int);

      char * _data = new char[readSize];
      m_ReadStream->read(_data, readSize);

      int gc = static_cast<int>(m_ReadStream->gcount());
      if (gc != readSize)
      {
        std::cout << "MetaMesh: m_Read: Cells not read completely" << std::endl;
        std::cout << "   ideal = " << readSize << " : actual = " << gc << std::endl;
        delete[] _data;
        return false;
      }

      int i = 0;
      for (int j = 0; j < m_NCells; j++)
      {
        int    n = MET_CellSize[celltype];
        auto * cell = new MeshCell(n);
        {
          int          td;
          char * const num = reinterpret_cast<char *>(&td);
          for (unsigned int k = 0; k < sizeof(int); k++)
          {
            num[k] = _data[i + k];
          }
          MET_SwapByteIfSystemMSB(&td, MET_INT);
          cell->m_Id = td;
          i += sizeof(int);
        }

        for (int d = 0; d < n; d++)
        {
          int          val;
          char * const num = reinterpret_cast<char *>(&val);
          for (unsigned int k = 0; k < static_cast<unsigned int>(sizeof(int)); k++)
          {
            num[k] = _data[i + k];
          }
          i += sizeof(int);

          MET_SwapByteIfSystemMSB(&val, MET_INT);
          cell->m_PointsId[d] = val;
        }
        m_CellListArray[celltype]->push_back(cell);
      }
      delete[] _data;
    }
    else
    {
      for (int j = 0; j < m_NCells; j++)
      {
        int    n = MET_CellSize[celltype];
        auto * cell = new MeshCell(MET_CellSize[celltype]);

        int v;
        *m_ReadStream >> v;
        m_ReadStream->get();
        cell->m_Id = v;

        for (int k = 0; k < n; k++)
        {
          *m_ReadStream >> v;
          m_ReadStream->get();
          cell->m_PointsId[k] = v;
        }
        m_CellListArray[celltype]->push_back(cell);
      }


      char c = ' ';
      while ((c != '\n') && (!m_ReadStream->eof()))
      {
        c = static_cast<char>(m_ReadStream->get()); // to avoid unrecognized characters
      }
    }
  }

  std::streampos pos = m_ReadStream->tellg();

  // Now reading the cell links
  MetaObject::ClearFields();

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "NCellLinks", MET_INT, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "CellLinksSize", MET_INT, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "CellLinks", MET_NONE, false);
  mF->terminateRead = true;
  m_Fields.push_back(mF);

  if (!MET_Read(*m_ReadStream, &m_Fields, '=', false, false))
  {
    std::cout << "MetaObject: Read: MET_Read Failed" << std::endl;
    return false;
  }

  mF = MET_GetFieldRecord("NCellLinks", &m_Fields);
  if (mF->defined)
  {
    m_NCellLinks = static_cast<int>(mF->value[0]);
  }

  unsigned int totalCellLink = 0;
  mF = MET_GetFieldRecord("CellLinksSize", &m_Fields);
  if (m_BinaryData)
  {
    if (mF->defined)
    {
      totalCellLink = static_cast<unsigned int>(mF->value[0]);
    }
  }

  if (m_BinaryData)
  {
    int readSize = totalCellLink * sizeof(int);

    char * _data = new char[readSize];
    m_ReadStream->read(_data, readSize);

    int gc = static_cast<int>(m_ReadStream->gcount());
    if (gc != readSize)
    {
      std::cout << "MetaMesh: m_Read: Cell Link not read completely" << std::endl;
      std::cout << "   ideal = " << readSize << " : actual = " << gc << std::endl;
      delete[] _data;
      return false;
    }
    int i = 0;
    for (int j = 0; j < m_NCellLinks; j++)
    {
      auto * link = new MeshCellLink();
      {
        int          td;
        char * const num = reinterpret_cast<char *>(&td);
        for (unsigned int k = 0; k < sizeof(int); k++)
        {
          num[k] = _data[i + k];
        }
        MET_SwapByteIfSystemMSB(&td, MET_INT);
        link->m_Id = td;
      }

      i += sizeof(int);
      {
        int n;
        {
          char * const num = reinterpret_cast<char *>(&n);
          for (unsigned int k = 0; k < sizeof(int); k++)
          {
            num[k] = _data[i + k];
          }
          MET_SwapByteIfSystemMSB(&n, MET_INT);
          i += sizeof(int);
        }

        for (int d = 0; d < n; d++)
        {
          int          td;
          char * const num = reinterpret_cast<char *>(&td);
          for (unsigned int k = 0; k < sizeof(int); k++)
          {
            num[k] = _data[i + k];
          }
          MET_SwapByteIfSystemMSB(&td, MET_INT);
          link->m_Links.push_back(td);
          i += sizeof(int);
        }
      }
      m_CellLinks.push_back(link);
    }
    delete[] _data;
  }
  else
  {
    for (int j = 0; j < m_NCellLinks; j++)
    {
      int    v;
      auto * link = new MeshCellLink();

      *m_ReadStream >> v;
      m_ReadStream->get();
      link->m_Id = v;

      *m_ReadStream >> v;
      m_ReadStream->get();
      int count = v;

      for (int i = 0; i < count; i++)
      {
        *m_ReadStream >> v;
        m_ReadStream->get();
        link->m_Links.push_back(v);
      }
      m_CellLinks.push_back(link);
    }

    if (m_NCellLinks > 0)
    {
      char c = ' ';
      while ((c != '\n') && (!m_ReadStream->eof()))
      {
        c = static_cast<char>(m_ReadStream->get()); // to avoid unrecognized characters
      }
    }
  }

  if (m_NCellLinks == 0)
  {
    m_ReadStream->clear();
    m_ReadStream->seekg(pos, std::ios::beg);
  }
  pos = m_ReadStream->tellg();

  // Now reading the point data
  MetaObject::ClearFields();

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "NPointData", MET_INT, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "PointDataSize", MET_INT, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "PointData", MET_NONE, false);
  mF->terminateRead = true;
  m_Fields.push_back(mF);

  if (!MET_Read(*m_ReadStream, &m_Fields, '=', false, false))
  {
    std::cout << "MetaObject: Read: MET_Read Failed" << std::endl;
    return false;
  }

  mF = MET_GetFieldRecord("NPointData", &m_Fields);
  if (mF->defined)
  {
    m_NPointData = static_cast<int>(mF->value[0]);
  }

  unsigned int pointDataSize = 0;
  mF = MET_GetFieldRecord("PointDataSize", &m_Fields);
  if (mF->defined)
  {
    pointDataSize = static_cast<unsigned int>(mF->value[0]);
  }

  char * _data = new char[pointDataSize];
  m_ReadStream->read(_data, pointDataSize);

  auto gc = static_cast<unsigned int>(m_ReadStream->gcount());
  if (gc != pointDataSize)
  {
    std::cout << "MetaMesh: m_Read: PointData not read completely" << std::endl;
    std::cout << "   ideal = " << pointDataSize << " : actual = " << gc << std::endl;
    delete[] _data;
    return false;
  }
  int i = 0;

  for (int j = 0; j < m_NPointData; j++)
  {
    MeshDataBase * pd;

    int td;
    {
      char * const num = reinterpret_cast<char *>(&td);
      for (unsigned int k = 0; k < sizeof(int); k++)
      {
        num[k] = _data[i + k];
      }
      MET_SwapByteIfSystemMSB(&td, MET_INT);
      i += sizeof(int);
    }

    int elementSize;
    MET_SizeOfType(m_PointDataType, &elementSize);

    bufferAlignedUnion alignedBuffer{};
    char * const       num = reinterpret_cast<char *>(&alignedBuffer);
    for (unsigned int k = 0; k < static_cast<unsigned int>(elementSize); k++)
    {
      num[k] = _data[i + k];
    }
    i += elementSize;

    if (m_PointDataType == MET_CHAR)
    {
      char val = alignedBuffer.character;
      pd = new MeshData<char>();
      dynamic_cast<MeshData<char> *>(pd)->m_Data = val;
    }
    else if (m_PointDataType == MET_UCHAR)
    {
      unsigned char val = alignedBuffer.ucharacter;
      pd = new MeshData<unsigned char>();
      dynamic_cast<MeshData<unsigned char> *>(pd)->m_Data = val;
    }
    else if (m_PointDataType == MET_SHORT)
    {
      short val = alignedBuffer.shortint;
      pd = new MeshData<short>();
      MET_SwapByteIfSystemMSB(&val, MET_SHORT);
      dynamic_cast<MeshData<short> *>(pd)->m_Data = val;
    }
    else if (m_PointDataType == MET_USHORT)
    {
      unsigned short val = alignedBuffer.ushortint;
      pd = new MeshData<unsigned short>();
      MET_SwapByteIfSystemMSB(&val, MET_USHORT);
      dynamic_cast<MeshData<unsigned short> *>(pd)->m_Data = val;
    }
    else if (m_PointDataType == MET_INT)
    {
      int val = alignedBuffer.integer;
      pd = new MeshData<int>();
      MET_SwapByteIfSystemMSB(&val, MET_INT);
      dynamic_cast<MeshData<int> *>(pd)->m_Data = val;
    }
    else if (m_PointDataType == MET_UINT)
    {
      unsigned int val = alignedBuffer.uinteger;
      pd = new MeshData<unsigned int>();
      MET_SwapByteIfSystemMSB(&val, MET_UINT);
      dynamic_cast<MeshData<unsigned int> *>(pd)->m_Data = val;
    }
    else if (m_PointDataType == MET_LONG)
    {
      long val = alignedBuffer.linteger;
      pd = new MeshData<long>();
      MET_SwapByteIfSystemMSB(&val, MET_LONG);
      dynamic_cast<MeshData<long> *>(pd)->m_Data = val;
    }
    else if (m_PointDataType == MET_ULONG)
    {
      unsigned long val = alignedBuffer.ulinteger;
      pd = new MeshData<unsigned long>();
      MET_SwapByteIfSystemMSB(&val, MET_ULONG);
      dynamic_cast<MeshData<unsigned long> *>(pd)->m_Data = val;
    }
    else if (m_PointDataType == MET_FLOAT)
    {
      float val = alignedBuffer.floatingpoint;
      pd = new MeshData<float>();
      MET_SwapByteIfSystemMSB(&val, MET_FLOAT);
      dynamic_cast<MeshData<float> *>(pd)->m_Data = val;
    }
    else if (m_PointDataType == MET_DOUBLE)
    {
      double val = alignedBuffer.doublefloatingpoint;
      pd = new MeshData<double>();
      MET_SwapByteIfSystemMSB(&val, MET_DOUBLE);
      dynamic_cast<MeshData<double> *>(pd)->m_Data = val;
    }
    else // assume double
    {
      std::cerr << "Warning: Mesh point data type not known - assuming double" << std::endl;
      double val = alignedBuffer.doublefloatingpoint;
      pd = new MeshData<double>();
      MET_SwapByteIfSystemMSB(&val, MET_DOUBLE);
      dynamic_cast<MeshData<double> *>(pd)->m_Data = val;
    }
    pd->m_Id = td;
    m_PointData.push_back(pd);
  }
  delete[] _data;
  _data = nullptr;

  // If no point data, reset the pointer to the stream to the previous position
  if (m_NPointData == 0)
  {
    m_ReadStream->clear();
    m_ReadStream->seekg(pos, std::ios::beg);
  }
  pos = m_ReadStream->tellg();

  // Now reading the cell data
  MetaObject::ClearFields();

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "NCellData", MET_INT, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "CellDataSize", MET_INT, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "CellData", MET_NONE, false);
  mF->terminateRead = true;
  m_Fields.push_back(mF);

  if (!MET_Read(*m_ReadStream, &m_Fields, '=', false, false))
  {
    std::cout << "MetaObject: Read: MET_Read Failed" << std::endl;
    return false;
  }


  mF = MET_GetFieldRecord("NCellData", &m_Fields);
  if (mF->defined)
  {
    m_NCellData = static_cast<int>(mF->value[0]);
  }

  unsigned int cellDataSize = 0;
  mF = MET_GetFieldRecord("CellDataSize", &m_Fields);
  if (mF->defined)
  {
    cellDataSize = static_cast<unsigned int>(mF->value[0]);
  }

  char * _celldata = new char[cellDataSize];
  m_ReadStream->read(_celldata, cellDataSize);

  auto gcCell = static_cast<unsigned int>(m_ReadStream->gcount());
  if (gcCell != cellDataSize)
  {
    std::cout << "MetaMesh: m_Read: data not read completely" << std::endl;
    std::cout << "   ideal = " << cellDataSize << " : actual = " << gcCell << std::endl;
    delete[] _data;
    delete[] _celldata;
    return false;
  }

  i = 0;
  for (int j = 0; j < m_NCellData; j++)
  {
    MeshDataBase * cd;
    int            td;
    {
      char * const num = reinterpret_cast<char *>(&td);
      for (unsigned int k = 0; k < sizeof(int); k++)
      {
        num[k] = _celldata[i + k];
      }
      MET_SwapByteIfSystemMSB(&td, MET_INT);
    }
    i += sizeof(int);

    int elementSize;
    MET_SizeOfType(m_CellDataType, &elementSize);
    bufferAlignedUnion alignedBuffer{};
    char * const       num = reinterpret_cast<char *>(&alignedBuffer);
    for (unsigned int k = 0; k < static_cast<unsigned int>(elementSize); k++)
    {
      num[k] = _celldata[i + k];
    }
    i += elementSize;

    if (m_CellDataType == MET_CHAR)
    {
      char val = alignedBuffer.character;
      cd = new MeshData<char>();
      dynamic_cast<MeshData<char> *>(cd)->m_Data = val;
    }
    else if (m_CellDataType == MET_UCHAR)
    {
      unsigned char val = alignedBuffer.ucharacter;
      cd = new MeshData<unsigned char>();
      dynamic_cast<MeshData<unsigned char> *>(cd)->m_Data = val;
    }
    else if (m_CellDataType == MET_SHORT)
    {
      short val = alignedBuffer.shortint;
      cd = new MeshData<short>();
      MET_SwapByteIfSystemMSB(&val, MET_SHORT);
      dynamic_cast<MeshData<short> *>(cd)->m_Data = val;
    }
    else if (m_CellDataType == MET_USHORT)
    {
      unsigned short val = alignedBuffer.ushortint;
      cd = new MeshData<unsigned short>();
      MET_SwapByteIfSystemMSB(&val, MET_USHORT);
      dynamic_cast<MeshData<unsigned short> *>(cd)->m_Data = val;
    }
    else if (m_CellDataType == MET_INT)
    {
      int val = alignedBuffer.integer;
      cd = new MeshData<int>();
      MET_SwapByteIfSystemMSB(&val, MET_INT);
      dynamic_cast<MeshData<int> *>(cd)->m_Data = val;
    }
    else if (m_CellDataType == MET_UINT)
    {
      unsigned int val = alignedBuffer.uinteger;
      cd = new MeshData<unsigned int>();
      MET_SwapByteIfSystemMSB(&val, MET_UINT);
      dynamic_cast<MeshData<unsigned int> *>(cd)->m_Data = val;
    }
    else if (m_CellDataType == MET_LONG)
    {
      long val = alignedBuffer.linteger;
      cd = new MeshData<long>();
      MET_SwapByteIfSystemMSB(&val, MET_LONG);
      dynamic_cast<MeshData<long> *>(cd)->m_Data = val;
    }
    else if (m_CellDataType == MET_ULONG)
    {
      unsigned long val = alignedBuffer.ulinteger;
      cd = new MeshData<unsigned long>();
      MET_SwapByteIfSystemMSB(&val, MET_ULONG);
      dynamic_cast<MeshData<unsigned long> *>(cd)->m_Data = static_cast<long>(val);
    }
    else if (m_CellDataType == MET_FLOAT)
    {
      float val = alignedBuffer.floatingpoint;
      cd = new MeshData<float>();
      MET_SwapByteIfSystemMSB(&val, MET_FLOAT);
      dynamic_cast<MeshData<float> *>(cd)->m_Data = val;
    }
    else if (m_CellDataType == MET_DOUBLE)
    {
      double val = alignedBuffer.doublefloatingpoint;
      cd = new MeshData<double>();
      MET_SwapByteIfSystemMSB(&val, MET_DOUBLE);
      dynamic_cast<MeshData<double> *>(cd)->m_Data = val;
    }
    else
    {
      std::cerr << "Warning: Mesh point data type not known - assuming double" << std::endl;
      double val = alignedBuffer.doublefloatingpoint;
      cd = new MeshData<double>();
      MET_SwapByteIfSystemMSB(&val, MET_DOUBLE);
      dynamic_cast<MeshData<double> *>(cd)->m_Data = val;
    }
    cd->m_Id = td;
    m_CellData.push_back(cd);
  }

  delete[] _celldata;

  // If no cell data, reset the pointer to the stream to the previous position
  if (m_NCellData == 0)
  {
    m_ReadStream->clear();
    m_ReadStream->seekg(pos, std::ios::beg);
  }

  return true;
}

bool
MetaMesh::M_Write()
{
  if (!MetaObject::M_Write())
  {
    std::cout << "MetaMesh: M_Write: Error parsing file" << std::endl;
    return false;
  }

  /** Then copy all points */
  if (m_BinaryData)
  {
    PointListType::const_iterator it = m_PointList.begin();
    PointListType::const_iterator itEnd = m_PointList.end();
    int                           elementSize;
    MET_SizeOfType(m_PointType, &elementSize);

    const size_t dataSize = (m_NDims)*m_NPoints * elementSize + m_NPoints * sizeof(int);
    char * data = new char[dataSize];
    int    i = 0;
    int    d;
    while (it != itEnd)
    {
      int pntId = (*it)->m_Id;
      MET_SwapByteIfSystemMSB(&pntId, MET_INT);
      MET_DoubleToValueN(static_cast<double>(pntId), MET_INT, data, dataSize, i++);

      for (d = 0; d < m_NDims; d++)
      {
        float pntX = (*it)->m_X[d];
        MET_SwapByteIfSystemMSB(&pntX, MET_FLOAT);
        MET_DoubleToValueN(static_cast<double>(pntX), m_PointType, data, dataSize, i++);
      }
      ++it;
    }
    m_WriteStream->write(data, (m_NDims + 1) * m_NPoints * elementSize);
    m_WriteStream->write("\n", 1);
    delete[] data;
  }
  else
  {
    PointListType::const_iterator it = m_PointList.begin();
    PointListType::const_iterator itEnd = m_PointList.end();
    int                           d;
    while (it != itEnd)
    {
      *m_WriteStream << (*it)->m_Id << " ";
      for (d = 0; d < m_NDims; d++)
      {
        *m_WriteStream << (*it)->m_X[d] << " ";
      }
      *m_WriteStream << std::endl;
      ++it;
    }
  }

  // Loop trough the array of cell types and write them if they exists
  for (unsigned int i = 0; i < MET_NUM_CELL_TYPES; i++)
  {
    if (!m_CellListArray[i]->empty())
    {
      // clear the fields and add new fields for a new write
      MetaObject::ClearFields();
      MET_FieldRecordType * mF;
      if (strlen(MET_CellTypeName[i]) > 0)
      {
        mF = new MET_FieldRecordType;
        MET_InitWriteField(mF, "CellType", MET_STRING, strlen(MET_CellTypeName[i]), MET_CellTypeName[i]);
        m_Fields.push_back(mF);
      }

      m_NCells = static_cast<int>(m_CellListArray[i]->size());
      mF = new MET_FieldRecordType;
      MET_InitWriteField(mF, "NCells", MET_INT, m_NCells);
      m_Fields.push_back(mF);

      mF = new MET_FieldRecordType;
      MET_InitWriteField(mF, "Cells", MET_NONE);
      m_Fields.push_back(mF);


      if (!MetaObject::M_Write())
      {
        std::cout << "MetaMesh: M_Write: Error parsing file" << std::endl;
        return false;
      }

      /** Then copy all cells */
      if (m_BinaryData)
      {
        auto         totalCellsSize = static_cast<unsigned int>(m_CellListArray[i]->size() * (MET_CellSize[i] + 1));
        const size_t dataSize = totalCellsSize * sizeof(int);
        char *       data = new char[dataSize];
        unsigned int d;
        int          j = 0;
        CellListType::const_iterator it = m_CellListArray[i]->begin();
        CellListType::const_iterator itEnd = m_CellListArray[i]->end();
        while (it != itEnd)
        {
          int cellId = (*it)->m_Id;
          MET_SwapByteIfSystemMSB(&cellId, MET_INT);
          MET_DoubleToValueN(static_cast<double>(cellId), MET_INT, data, dataSize, j++);

          for (d = 0; d < (*it)->m_Dim; d++)
          {
            int pntId = (*it)->m_PointsId[d];
            MET_SwapByteIfSystemMSB(&pntId, MET_INT);
            MET_DoubleToValueN(static_cast<double>(pntId), MET_INT, data, dataSize, j++);
          }
          ++it;
        }
        m_WriteStream->write(data, totalCellsSize * sizeof(int));
        m_WriteStream->write("\n", 1);
        delete[] data;
      }
      else
      {
        CellListType::const_iterator it = m_CellListArray[i]->begin();
        CellListType::const_iterator itEnd = m_CellListArray[i]->end();

        unsigned int d;
        while (it != itEnd)
        {
          *m_WriteStream << (*it)->m_Id << " ";
          for (d = 0; d < (*it)->m_Dim; d++)
          {
            *m_WriteStream << (*it)->m_PointsId[d] << " ";
          }

          *m_WriteStream << std::endl;
          ++it;
        }
      }
    }
  }

  // Now write the cell links
  if (!m_CellLinks.empty())
  {
    MetaObject::ClearFields();
    m_NCellLinks = static_cast<int>(m_CellLinks.size());
    MET_FieldRecordType * mF;
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "NCellLinks", MET_INT, m_NCellLinks);
    m_Fields.push_back(mF);

    int cellLinksSize = 0;
    if (m_BinaryData)
    {
      CellLinkListType::const_iterator it = m_CellLinks.begin();
      CellLinkListType::const_iterator itEnd = m_CellLinks.end();
      while (it != itEnd)
      {
        cellLinksSize += static_cast<int>(2 + (*it)->m_Links.size());
        ++it;
      }
      mF = new MET_FieldRecordType;
      MET_InitWriteField(mF, "CellLinksSize", MET_INT, cellLinksSize);
      m_Fields.push_back(mF);
    }


    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "CellLinks", MET_NONE);
    m_Fields.push_back(mF);


    if (!MetaObject::M_Write())
    {
      std::cout << "MetaMesh: M_Write: Error parsing file" << std::endl;
      return false;
    }

    /** Then copy all cell links */
    if (m_BinaryData)
    {
      const size_t                     dataSize = cellLinksSize * sizeof(int);
      char *                           data = new char[dataSize];
      int                              j = 0;
      CellLinkListType::const_iterator it = m_CellLinks.begin();
      CellLinkListType::const_iterator itEnd = m_CellLinks.end();
      while (it != itEnd)
      {
        int clId = (*it)->m_Id;
        MET_SwapByteIfSystemMSB(&clId, MET_INT);
        MET_DoubleToValueN(static_cast<double>(clId), MET_INT, data, dataSize, j++);

        int linkSize = static_cast<int>((*it)->m_Links.size());
        MET_SwapByteIfSystemMSB(&linkSize, MET_INT);
        MET_DoubleToValueN(static_cast<double>(linkSize), MET_INT, data, dataSize, j++);

        std::list<int>::const_iterator it2 = (*it)->m_Links.begin();
        std::list<int>::const_iterator it2End = (*it)->m_Links.end();
        while (it2 != it2End)
        {
          int links = (*it2);
          MET_SwapByteIfSystemMSB(&links, MET_INT);
          MET_DoubleToValueN(static_cast<double>(links), MET_INT, data, dataSize, j++);
          ++it2;
        }
        ++it;
      }
      m_WriteStream->write(data, cellLinksSize * sizeof(int));
      m_WriteStream->write("\n", 1);
      delete[] data;
    }
    else
    {
      CellLinkListType::const_iterator it = m_CellLinks.begin();
      CellLinkListType::const_iterator itEnd = m_CellLinks.end();

      while (it != itEnd)
      {
        *m_WriteStream << (*it)->m_Id << " ";
        *m_WriteStream << (*it)->m_Links.size() << " ";
        std::list<int>::const_iterator it2 = (*it)->m_Links.begin();
        std::list<int>::const_iterator it2End = (*it)->m_Links.end();
        while (it2 != it2End)
        {
          *m_WriteStream << (*it2) << " ";
          ++it2;
        }
        *m_WriteStream << std::endl;
        ++it;
      }
    }
  }

  // Now write the point data
  // Point Data type is the same for the whole mesh
  if (!m_PointData.empty())
  {
    MetaObject::ClearFields();
    m_NPointData = static_cast<int>(m_PointData.size());
    MET_FieldRecordType * mF;
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "NPointData", MET_INT, m_NPointData);
    m_Fields.push_back(mF);

    int                               pointDataSize = 0;
    PointDataListType::const_iterator it = m_PointData.begin();
    PointDataListType::const_iterator itEnd = m_PointData.end();
    while (it != itEnd)
    {
      pointDataSize += (*it)->GetSize();
      ++it;
    }

    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "PointDataSize", MET_INT, pointDataSize);
    m_Fields.push_back(mF);

    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "PointData", MET_NONE);
    m_Fields.push_back(mF);

    if (!MetaObject::M_Write())
    {
      std::cout << "MetaMesh: M_Write: Error parsing file" << std::endl;
      return false;
    }

    // Then copy all Point data :
    // Always binary to be compatible with everything
    it = m_PointData.begin();
    itEnd = m_PointData.end();
    while (it != itEnd)
    {
      (*it)->Write(m_WriteStream);
      ++it;
    }
    m_WriteStream->write("\n", 1);
  }

  // Now write the cell data
  // Cell Data type is the same for the whole mesh
  if (!m_CellData.empty())
  {
    MetaObject::ClearFields();
    m_NCellData = static_cast<int>(m_CellData.size());
    MET_FieldRecordType * mF;
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "NCellData", MET_INT, m_NCellData);
    m_Fields.push_back(mF);

    int                              cellDataSize = 0;
    CellDataListType::const_iterator it = m_CellData.begin();
    CellDataListType::const_iterator itEnd = m_CellData.end();
    while (it != itEnd)
    {
      cellDataSize += (*it)->GetSize();
      ++it;
    }

    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "CellDataSize", MET_INT, cellDataSize);
    m_Fields.push_back(mF);

    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "CellData", MET_NONE);
    m_Fields.push_back(mF);


    if (!MetaObject::M_Write())
    {
      std::cout << "MetaMesh: M_Write: Error parsing file" << std::endl;
      return false;
    }

    // Then copy all Cell data :
    // Always binary to be compatible with everything
    it = m_CellData.begin();
    itEnd = m_CellData.end();
    while (it != itEnd)
    {
      (*it)->Write(m_WriteStream);
      ++it;
    }
    m_WriteStream->write("\n", 1);
  }
  return true;
}

#if (METAIO_USE_NAMESPACE)
};
#endif
