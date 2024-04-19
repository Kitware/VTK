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

#include "metaTube.h"


#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE
{
#endif

TubePnt::TubePnt(int dim)
{
  m_NDims = static_cast<unsigned int>(dim);

  m_ID = -1;

  m_X = new float[m_NDims];
  unsigned int i = 0;
  for (i = 0; i < m_NDims; i++)
  {
    m_X[i] = 0;
  }

  for (i = 0; i < 4; i++)
  {
    m_Color[i] = 1;
  }

  m_Mark = false;

  m_T = new float[m_NDims];
  m_V1 = new float[m_NDims];
  m_V2 = new float[m_NDims];
  for (unsigned int ii = 0; ii < m_NDims; ii++)
  {
    m_T[ii] = 0;
    m_V1[ii] = 0;
    m_V2[ii] = 0;
  }
  m_Alpha1 = 0;
  m_Alpha2 = 0;
  m_Alpha3 = 0;

  m_R = 0;
  m_Medialness = 0;
  m_Ridgeness = 0;
  m_Branchness = 0;
  m_Curvature = 0;
  m_Levelness = 0;
  m_Roundness = 0;
  m_Intensity = 0;

  m_ExtraFields.clear();
}

TubePnt::TubePnt(const TubePnt * _tubePnt)
{
  TubePnt::CopyInfo(_tubePnt);
}

TubePnt::~TubePnt()
{
  delete[] m_X;
  delete[] m_T;
  delete[] m_V1;
  delete[] m_V2;

  m_ExtraFields.clear();
}

void
TubePnt::CopyInfo(const TubePnt * _tubePnt)
{
  delete[] m_X;
  delete[] m_T;
  delete[] m_V1;
  delete[] m_V2;

  m_ExtraFields.clear();

  m_NDims = _tubePnt->m_NDims;

  m_X = new float[m_NDims];
  m_T = new float[m_NDims];
  m_V1 = new float[m_NDims];
  m_V2 = new float[m_NDims];
  for (unsigned int i = 0; i < m_NDims; ++i)
  {
    m_X[i] = _tubePnt->m_X[i];
    m_T[i] = _tubePnt->m_T[i];
    m_V1[i] = _tubePnt->m_V1[i];
    m_V2[i] = _tubePnt->m_V2[i];
  }
  m_Alpha1 = _tubePnt->m_Alpha1;
  m_Alpha2 = _tubePnt->m_Alpha2;
  m_Alpha3 = _tubePnt->m_Alpha3;
  m_R = _tubePnt->m_R;
  m_Medialness = _tubePnt->m_Medialness;
  m_Ridgeness = _tubePnt->m_Ridgeness;
  m_Branchness = _tubePnt->m_Branchness;
  m_Curvature = _tubePnt->m_Curvature;
  m_Levelness = _tubePnt->m_Levelness;
  m_Roundness = _tubePnt->m_Roundness;
  m_Intensity = _tubePnt->m_Intensity;

  for (unsigned int i = 0; i < 4; ++i)
  {
    m_Color[i] = _tubePnt->m_Color[i];
  }
  m_Mark = _tubePnt->m_Mark;

  auto it = _tubePnt->m_ExtraFields.begin();
  auto itEnd = _tubePnt->m_ExtraFields.end();
  while (it != itEnd)
  {
    m_ExtraFields.push_back(*it);
    ++it;
  }
}
const TubePnt::FieldListType &
TubePnt::GetExtraFields() const
{
  return m_ExtraFields;
}

size_t
TubePnt::GetNumberOfExtraFields() const
{
  return m_ExtraFields.size();
}

void
TubePnt::SetNumberOfExtraFields(int size)
{
  m_ExtraFields.resize(static_cast<unsigned long>(size));
}

void
TubePnt::SetField(int indx, const char * name, float value)
{
  FieldType field(name, value);
  m_ExtraFields[indx] = field;
}

void
TubePnt::SetField(const char * name, float value)
{
  int indx = this->GetFieldIndex(name);
  if (indx >= 0)
  {
    m_ExtraFields[indx].second = value;
  }
  else
  {
    this->AddField(name, value);
  }
}

void
TubePnt::AddField(const char * name, float value)
{
  int indx = this->GetFieldIndex(name);
  if (indx != -1)
  {
    m_ExtraFields[indx].second = value;
  }
  else
  {
    FieldType field(name, value);
    m_ExtraFields.push_back(field);
  }
}

int
TubePnt::GetFieldIndex(const char * name) const
{
  unsigned int count = 0;
  auto         it = m_ExtraFields.begin();
  auto         itEnd = m_ExtraFields.end();
  while (it != itEnd)
  {
    if (!strcmp((*it).first.c_str(), name))
    {
      return count;
    }
    ++it;
    ++count;
  }
  return -1;
}

float
TubePnt::GetField(int indx) const
{
  if (indx >= 0 && static_cast<size_t>(indx) < m_ExtraFields.size())
  {
    return m_ExtraFields[indx].second;
  }
  return -1;
}

float
TubePnt::GetField(const char * name) const
{
  auto it = m_ExtraFields.begin();
  auto itEnd = m_ExtraFields.end();
  while (it != itEnd)
  {
    if (!strcmp((*it).first.c_str(), name))
    {
      return (*it).second;
    }
    ++it;
  }
  return -1;
}

/** MetaTube Constructors */
MetaTube::MetaTube()
{
  META_DEBUG_PRINT( "MetaTube()" );
  MetaTube::Clear();
}


MetaTube::MetaTube(const char * _headerName)
{
  META_DEBUG_PRINT( "MetaTube()" );
  MetaTube::Clear();
  MetaTube::Read(_headerName);
}


MetaTube::MetaTube(const MetaTube * Tube)
{
  META_DEBUG_PRINT( "MetaTube()" );
  MetaTube::Clear();
  CopyInfo(Tube);
}


MetaTube::MetaTube(unsigned int dim)
  : MetaObject(dim)
{
  META_DEBUG_PRINT( "MetaTube()" );
  MetaTube::Clear();
}

/** Destructor */
MetaTube::~MetaTube()
{
  // Delete the list of pointers to PointObject.
  auto it = m_PointList.begin();
  while (it != m_PointList.end())
  {
    TubePnt * pnt = *it;
    ++it;
    delete pnt;
  }
  m_PointList.clear();
  MetaObject::M_Destroy();
}

//
void
MetaTube::PrintInfo() const
{
  MetaObject::PrintInfo();

  std::cout << "PointDim = " << m_PointDim << std::endl;

  std::cout << "NPoints = " << m_NPoints << std::endl;

  char str[255];
  MET_TypeToString(m_ElementType, str);
  std::cout << "ElementType = " << str << std::endl;

  std::cout << "ParentPoint = " << m_ParentPoint << std::endl;

  if (m_Root)
  {
    std::cout << "Root = "
              << "True" << std::endl;
  }
  else
  {
    std::cout << "Root = "
              << "False" << std::endl;
  }

  std::cout << "Artery = " << m_Artery << std::endl;
}

void
MetaTube::CopyInfo(const MetaTube * _object)
{
  Clear();

  MetaObject::CopyInfo(_object);

  auto it = _object->GetPoints().begin();
  while (it != _object->GetPoints().end())
  {
    auto * pnt = new TubePnt(*it);
    m_PointList.push_back(pnt);
    ++it;
  }

  m_ParentPoint = _object->ParentPoint();
  m_Artery = _object->Artery();
  m_Root = _object->Root();
}

/** Clear Tube information */
void
MetaTube::Clear()
{
  META_DEBUG_PRINT( "MetaTube: Clear" );

  MetaObject::Clear();

  strcpy(m_ObjectTypeName, "Tube");
  strcpy(m_ObjectSubTypeName, "");

  m_ElementType = MET_FLOAT;

  m_ParentPoint = -1;
  m_Root = false;
  m_Artery = true;

  // Delete the list of pointers to PointObjects.
  auto it = m_PointList.begin();
  while (it != m_PointList.end())
  {
    TubePnt * pnt = *it;
    ++it;
    delete pnt;
  }
  m_PointList.clear();

  m_NPoints = 0;
  if (m_NDims == 2)
  {
    m_PointDim = "id x y red green blue alpha mark r rn mn bn cv lv ro in tx ty v1x v1y a1 a2";
  }
  else
  {
    m_PointDim = "id x y z red green blue alpha mark r rn mn bn cv lv ro in tx ty tz v1x v1y v1z v2x v2y v2z a1 a2 a3";
  }
}

const char *
MetaTube::PointDim() const
{
  return m_PointDim.c_str();
}

void
MetaTube::PointDim(const char * pntDim)
{
  m_PointDim = pntDim;
}

void
MetaTube::NPoints(int npnt)
{
  m_NPoints = npnt;
}

int
MetaTube::NPoints() const
{
  return m_NPoints;
}

/** Set Read fields */
void
MetaTube::M_SetupReadFields()
{
  META_DEBUG_PRINT( "MetaTube: M_SetupReadFields" );

  MetaObject::M_SetupReadFields();

  MET_FieldRecordType * mF;

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "ParentPoint", MET_INT, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "Root", MET_STRING, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "Artery", MET_STRING, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "ElementType", MET_STRING, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "PointDim", MET_STRING, true);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "NPoints", MET_INT, true);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "Points", MET_NONE, true);
  mF->terminateRead = true;
  m_Fields.push_back(mF);
}

void
MetaTube::M_SetupWriteFields()
{
  MetaObject::M_SetupWriteFields();

  MET_FieldRecordType * mF;

  if (m_ParentPoint >= 0 && m_ParentID >= 0)
  {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "ParentPoint", MET_INT, m_ParentPoint);
    m_Fields.push_back(mF);
  }

  if (m_Root)
  {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "Root", MET_STRING, strlen("True"), "True");
    m_Fields.push_back(mF);
  }
  else
  {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "Root", MET_STRING, strlen("False"), "False");
    m_Fields.push_back(mF);
  }

  if (m_Artery)
  {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "Artery", MET_STRING, strlen("True"), "True");
    m_Fields.push_back(mF);
  }
  else
  {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "Artery", MET_STRING, strlen("False"), "False");
    m_Fields.push_back(mF);
  }

  if (m_NDims == 2)
  {
    m_PointDim = "id x y red green blue alpha mark r rn mn bn cv lv ro in tx ty v1x v1y a1 a2";
  }
  else
  {
    m_PointDim = "id x y z red green blue alpha mark r rn mn bn cv lv ro in tx ty tz v1x v1y v1z v2x v2y v2z a1 a2 a3";
  }

  // All the points in the tube have the same number of fields
  const TubePnt::FieldListType & extraList = (*(m_PointList.begin()))->GetExtraFields();
  auto                           itFields = extraList.begin();
  auto                           itFieldsEnd = extraList.end();
  while (itFields != itFieldsEnd)
  {
    m_PointDim += " ";
    m_PointDim += (*itFields).first;
    ++itFields;
  }

  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "PointDim", MET_STRING, m_PointDim.size(), m_PointDim.c_str());
  m_Fields.push_back(mF);

  m_NPoints = static_cast<int>(m_PointList.size());
  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "NPoints", MET_INT, m_NPoints);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "Points", MET_NONE);
  m_Fields.push_back(mF);
}

/** Return the position given the name of the field */
int
MetaTube::M_GetPosition(const char * name, std::vector<bool> & used) const
{
  auto it = m_Positions.begin();
  auto itUsed = used.begin();
  auto itEnd = m_Positions.end();
  while (it != itEnd)
  {
    if (!strcmp((*it).first.c_str(), name))
    {
      *itUsed = true;
      return (*it).second;
    }
    ++it;
    ++itUsed;
  }

  return -1;
}

void
MetaTube::M_SetFloatIntoBinaryData(float val, char * _data, int i) const
{
  MET_SwapByteIfSystemMSB(&val, MET_FLOAT);
  MET_DoubleToValue(static_cast<double>(val), m_ElementType, _data, i);
}

float
MetaTube::M_GetFloatFromBinaryData(size_t pos, const char * _data, size_t readSize)
{
  if (pos < readSize)
  {
    float        tf;
    char * const num = reinterpret_cast<char *>(&tf);
    size_t       posChar = pos * sizeof(float);
    for (size_t k = 0; k < sizeof(float) && posChar + k < readSize; k++)
    {
      num[k] = _data[posChar + k];
    }
    MET_SwapByteIfSystemMSB(&tf, MET_FLOAT);
    return tf;
  }
  return -1;
}

bool
MetaTube::M_Read()
{
  META_DEBUG_PRINT( "MetaTube: M_Read: Loading Header" );

  if (!MetaObject::M_Read())
  {
    std::cout << "MetaTube: M_Read: Error parsing file" << std::endl;
    return false;
  }

  META_DEBUG_PRINT( "MetaTube: M_Read: Parsing Header" );

  MET_FieldRecordType * mF;

  mF = MET_GetFieldRecord("ParentPoint", &m_Fields);
  if (mF && mF->defined)
  {
    m_ParentPoint = static_cast<int>(mF->value[0]);
  }

  m_Root = false;
  mF = MET_GetFieldRecord("Root", &m_Fields);
  if (mF && mF->defined)
  {
    if (*(reinterpret_cast<char *>(mF->value)) == 'T' || *(reinterpret_cast<char *>(mF->value)) == 't' || *(reinterpret_cast<char *>(mF->value)) == '1')
    {
      m_Root = true;
    }
    else
    {
      m_Root = false;
    }
  }

  m_Artery = true;
  mF = MET_GetFieldRecord("Artery", &m_Fields);
  if (mF && mF->defined)
  {
    if (*(reinterpret_cast<char *>(mF->value)) == 'T' || *(reinterpret_cast<char *>(mF->value)) == 't')
    {
      m_Artery = true;
    }
    else
    {
      m_Artery = false;
    }
  }

  mF = MET_GetFieldRecord("NPoints", &m_Fields);
  if (mF->defined)
  {
    m_NPoints = static_cast<int>(mF->value[0]);
  }

  mF = MET_GetFieldRecord("ElementType", &m_Fields);
  if (mF && mF->defined)
  {
    MET_StringToType(reinterpret_cast<char *>(mF->value), &m_ElementType);
  }

  mF = MET_GetFieldRecord("PointDim", &m_Fields);
  if (mF->defined)
  {
    m_PointDim = reinterpret_cast<char *>(mF->value);
  }


  int     pntDim;
  char ** pntVal = nullptr;
  char    pointDim[4096];

  for (unsigned t = 0; t < this->m_PointDim.size(); t++)
  {
    pointDim[t] = this->m_PointDim[t];
  }
  pointDim[m_PointDim.size()] = '\0';

  MET_StringToWordArray(pointDim, &pntDim, &pntVal);

  META_DEBUG_PRINT( "MetaTube: Parsing point dim" );

  m_Positions.clear();
  std::vector<bool> positionUsed;
  for (int i = 0; i < pntDim; i++)
  {
    PositionType p(pntVal[i], static_cast<const unsigned int &>(i));
    m_Positions.push_back(p);
    positionUsed.push_back(false);
  }

  for (int i = 0; i < pntDim; i++)
  {
    delete[] pntVal[i];
  }
  delete[] pntVal;

  float v[100];

  if (m_Event)
  {
    m_Event->StartReading(static_cast<unsigned int>(m_NPoints));
  }

  int posId = M_GetPosition("id", positionUsed);
  int posX = M_GetPosition("x", positionUsed);
  int posY = M_GetPosition("y", positionUsed);
  int posZ = M_GetPosition("z", positionUsed);
  int posRed = M_GetPosition("red", positionUsed);
  int posGreen = M_GetPosition("green", positionUsed);
  int posBlue = M_GetPosition("blue", positionUsed);
  int posAlpha = M_GetPosition("alpha", positionUsed);
  int posMark = M_GetPosition("mark", positionUsed);
  if (posMark == -1)
  {
    posMark = M_GetPosition("mk", positionUsed);
  }
  int posR = M_GetPosition("r", positionUsed);
  if (posR == -1)
  {
    posR = M_GetPosition("R", positionUsed);
  }
  if (posR == -1)
  {
    posR = M_GetPosition("radius", positionUsed);
  }
  if (posR == -1)
  {
    posR = M_GetPosition("Radius", positionUsed);
  }
  if (posR == -1)
  {
    posR = M_GetPosition("rad", positionUsed);
  }
  if (posR == -1)
  {
    posR = M_GetPosition("Rad", positionUsed);
  }
  if (posR == -1)
  {
    posR = M_GetPosition("s", positionUsed);
  }
  if (posR == -1)
  {
    posR = M_GetPosition("S", positionUsed);
  }
  int posRn = M_GetPosition("rn", positionUsed);
  int posMn = M_GetPosition("mn", positionUsed);
  int posBn = M_GetPosition("bn", positionUsed);
  int posCv = M_GetPosition("cv", positionUsed);
  int posLv = M_GetPosition("lv", positionUsed);
  int posRo = M_GetPosition("ro", positionUsed);
  int posIn = M_GetPosition("in", positionUsed);
  int posTx = M_GetPosition("tx", positionUsed);
  int posTy = M_GetPosition("ty", positionUsed);
  int posTz = M_GetPosition("tz", positionUsed);
  int posV1x = M_GetPosition("v1x", positionUsed);
  int posV1y = M_GetPosition("v1y", positionUsed);
  int posV1z = M_GetPosition("v1z", positionUsed);
  int posV2x = M_GetPosition("v2x", positionUsed);
  int posV2y = M_GetPosition("v2y", positionUsed);
  int posV2z = M_GetPosition("v2z", positionUsed);
  int posA1 = M_GetPosition("a1", positionUsed);
  int posA2 = M_GetPosition("a2", positionUsed);
  int posA3 = M_GetPosition("a3", positionUsed);

  if (m_BinaryData)
  {
    int elementSize;
    MET_SizeOfType(m_ElementType, &elementSize);
    int readSize = m_NPoints * pntDim * elementSize;

    char * _data = new char[readSize];
    m_ReadStream->read(_data, readSize);

    int gc = static_cast<int>(m_ReadStream->gcount());
    if (gc != readSize)
    {
      std::cout << "MetaLine: m_Read: data not read completely" << std::endl;
      std::cout << "   ideal = " << readSize << " : actual = " << gc << std::endl;
      delete[] _data;
      return false;
    }

    for (int j = 0; j < m_NPoints; j++)
    {
      auto * pnt = new TubePnt(m_NDims);

      auto pntPos = static_cast<size_t>(j * pntDim);

      if (posId >= 0)
      {
        pnt->m_ID = static_cast<int>(M_GetFloatFromBinaryData(pntPos + posId, _data, static_cast<size_t>(readSize)));
      }
      if (posX >= 0)
      {
        pnt->m_X[0] = M_GetFloatFromBinaryData(pntPos + posX, _data, static_cast<size_t>(readSize));
      }
      if (posY >= 0)
      {
        pnt->m_X[1] = M_GetFloatFromBinaryData(pntPos + posY, _data, static_cast<size_t>(readSize));
      }
      if (m_NDims > 2 && posZ >= 0)
      {
        pnt->m_X[2] = M_GetFloatFromBinaryData(pntPos + posZ, _data, static_cast<size_t>(readSize));
      }
      if (posRed >= 0)
      {
        pnt->m_Color[0] = M_GetFloatFromBinaryData(pntPos + posRed, _data, static_cast<size_t>(readSize));
      }
      if (posGreen >= 0)
      {
        pnt->m_Color[1] = M_GetFloatFromBinaryData(pntPos + posGreen, _data, static_cast<size_t>(readSize));
      }
      if (posBlue >= 0)
      {
        pnt->m_Color[2] = M_GetFloatFromBinaryData(pntPos + posBlue, _data, static_cast<size_t>(readSize));
      }
      if (posAlpha >= 0)
      {
        pnt->m_Color[3] = M_GetFloatFromBinaryData(pntPos + posAlpha, _data, static_cast<size_t>(readSize));
      }
      if (posMark >= 0)
      {
        if (M_GetFloatFromBinaryData(pntPos + posMark, _data, static_cast<size_t>(readSize)) != 0)
        {
          pnt->m_Mark = true;
        }
        else
        {
          pnt->m_Mark = false;
        }
      }
      if (posR != -1)
      {
        pnt->m_R = M_GetFloatFromBinaryData(pntPos + posR, _data, static_cast<size_t>(readSize));
      }
      if (posMn != -1)
      {
        pnt->m_Medialness = M_GetFloatFromBinaryData(pntPos + posMn, _data, static_cast<size_t>(readSize));
      }
      if (posRn != -1)
      {
        pnt->m_Ridgeness = M_GetFloatFromBinaryData(pntPos + posRn, _data, static_cast<size_t>(readSize));
      }
      if (posBn != -1)
      {
        pnt->m_Branchness = M_GetFloatFromBinaryData(pntPos + posBn, _data, static_cast<size_t>(readSize));
      }
      if (posCv != -1)
      {
        pnt->m_Curvature = M_GetFloatFromBinaryData(pntPos + posCv, _data, static_cast<size_t>(readSize));
      }
      if (posLv != -1)
      {
        pnt->m_Levelness = M_GetFloatFromBinaryData(pntPos + posLv, _data, static_cast<size_t>(readSize));
      }
      if (posRo != -1)
      {
        pnt->m_Roundness = M_GetFloatFromBinaryData(pntPos + posRo, _data, static_cast<size_t>(readSize));
      }
      if (posIn != -1)
      {
        pnt->m_Intensity = M_GetFloatFromBinaryData(pntPos + posIn, _data, static_cast<size_t>(readSize));
      }
      if (posTx != -1)
      {
        pnt->m_T[0] = M_GetFloatFromBinaryData(pntPos + posTx, _data, static_cast<size_t>(readSize));
      }
      if (posTy != -1)
      {
        pnt->m_T[1] = M_GetFloatFromBinaryData(pntPos + posTy, _data, static_cast<size_t>(readSize));
      }
      if (m_NDims > 2 && posTz != -1)
      {
        pnt->m_T[2] = M_GetFloatFromBinaryData(pntPos + posTz, _data, static_cast<size_t>(readSize));
      }
      if (posV1x != -1)
      {
        pnt->m_V1[0] = M_GetFloatFromBinaryData(pntPos + posV1x, _data, static_cast<size_t>(readSize));
      }
      if (posV1y != -1)
      {
        pnt->m_V1[1] = M_GetFloatFromBinaryData(pntPos + posV1y, _data, static_cast<size_t>(readSize));
      }
      if (m_NDims > 2 && posV1z != -1)
      {
        pnt->m_V1[2] = M_GetFloatFromBinaryData(pntPos + posV1z, _data, static_cast<size_t>(readSize));
      }
      if (posV2x != -1)
      {
        pnt->m_V2[0] = M_GetFloatFromBinaryData(pntPos + posV2x, _data, static_cast<size_t>(readSize));
      }
      if (posV2y != -1)
      {
        pnt->m_V2[1] = M_GetFloatFromBinaryData(pntPos + posV2y, _data, static_cast<size_t>(readSize));
      }
      if (m_NDims > 2 && posV2z != -1)
      {
        pnt->m_V2[2] = M_GetFloatFromBinaryData(pntPos + posV2z, _data, static_cast<size_t>(readSize));
      }
      if (posA1 != -1)
      {
        pnt->m_Alpha1 = M_GetFloatFromBinaryData(pntPos + posA1, _data, static_cast<size_t>(readSize));
      }
      if (posA2 != -1)
      {
        pnt->m_Alpha2 = M_GetFloatFromBinaryData(pntPos + posA2, _data, static_cast<size_t>(readSize));
      }
      if (posA3 != -1)
      {
        pnt->m_Alpha3 = M_GetFloatFromBinaryData(pntPos + posA3, _data, static_cast<size_t>(readSize));
      }

      std::vector<PositionType>::const_iterator itFields = m_Positions.begin();
      auto                                      itUsed = positionUsed.begin();
      std::vector<PositionType>::const_iterator itFieldsEnd = m_Positions.end();
      while (itFields != itFieldsEnd)
      {
        if (!(*itUsed))
        {
          int pos = M_GetPosition((*itFields).first.c_str(), positionUsed);
          (*itUsed) = false;
          if (pos >= 0)
          {
            float tf = M_GetFloatFromBinaryData(pntPos + pos, _data, static_cast<size_t>(readSize));
            pnt->AddField((*itFields).first.c_str(), tf);
          }
        }
        ++itFields;
        ++itUsed;
      }

      m_PointList.push_back(pnt);
    }
    char c = ' ';
    while ((c != '\n') && (!m_ReadStream->eof()))
    {
      c = static_cast<char>(m_ReadStream->get());
      // to avoid unrecognize charactere
    }
    delete[] _data;
  }
  else
  {
    for (int j = 0; j < m_NPoints; j++)
    {
      if (m_Event)
      {
        m_Event->SetCurrentIteration(static_cast<unsigned int>(j + 1));
      }

      for (int k = 0; k < pntDim; k++)
      {
        *m_ReadStream >> v[k];
        m_ReadStream->get();
      }

      auto * pnt = new TubePnt(m_NDims);

      if (posId >= 0)
      {
        pnt->m_ID = static_cast<int>(v[posId]);
      }
      if (posX >= 0)
      {
        pnt->m_X[0] = v[posX];
      }
      if (posY >= 0)
      {
        pnt->m_X[1] = v[posY];
      }
      if (m_NDims > 2 && posZ >= 0)
      {
        pnt->m_X[2] = v[posZ];
      }
      if (posRed >= 0)
      {
        pnt->m_Color[0] = v[posRed];
      }
      if (posGreen >= 0)
      {
        pnt->m_Color[1] = v[posGreen];
      }
      if (posBlue >= 0)
      {
        pnt->m_Color[2] = v[posBlue];
      }
      if (posAlpha >= 0)
      {
        pnt->m_Color[3] = v[posAlpha];
      }
      if (posMark >= 0)
      {
        pnt->m_Mark = (v[posMark] != 0) ? true : false;
      }
      if (posR >= 0)
      {
        pnt->m_R = v[posR];
      }
      if (posRn >= 0)
      {
        pnt->m_Ridgeness = v[posRn];
      }
      if (posMn >= 0)
      {
        pnt->m_Medialness = v[posMn];
      }
      if (posBn >= 0)
      {
        pnt->m_Branchness = v[posBn];
      }
      if (posCv >= 0)
      {
        pnt->m_Curvature = v[posCv];
      }
      if (posLv >= 0)
      {
        pnt->m_Levelness = v[posLv];
      }
      if (posRo >= 0)
      {
        pnt->m_Roundness = v[posRo];
      }
      if (posIn >= 0)
      {
        pnt->m_Intensity = v[posIn];
      }
      if (posTx >= 0)
      {
        pnt->m_T[0] = v[posTx];
      }
      if (posTy >= 0)
      {
        pnt->m_T[1] = v[posTy];
      }
      if (m_NDims > 2 && posTz >= 0)
      {
        pnt->m_T[2] = v[posTz];
      }
      if (posV1x >= 0)
      {
        pnt->m_V1[0] = v[posV1x];
      }
      if (posV1y >= 0)
      {
        pnt->m_V1[1] = v[posV1y];
      }
      if (m_NDims > 2 && posV1z >= 0)
      {
        pnt->m_V1[2] = v[posV1z];
      }
      if (posV2x >= 0)
      {
        pnt->m_V2[0] = v[posV2x];
      }
      if (posV2y >= 0)
      {
        pnt->m_V2[1] = v[posV2y];
      }
      if (m_NDims > 2 && posV2z >= 0)
      {
        pnt->m_V2[2] = v[posV2z];
      }
      if (posA1 >= 0)
      {
        pnt->m_Alpha1 = v[posA1];
      }
      if (posA2 >= 0)
      {
        pnt->m_Alpha2 = v[posA2];
      }
      if (posA3 >= 0)
      {
        pnt->m_Alpha3 = v[posA3];
      }

      std::vector<PositionType>::const_iterator itFields = m_Positions.begin();
      auto                                      itUsed = positionUsed.begin();
      std::vector<PositionType>::const_iterator itFieldsEnd = m_Positions.end();
      while (itFields != itFieldsEnd)
      {
        if (!(*itUsed))
        {
          std::string fldstr = (*itFields).first;
          pnt->AddField(fldstr.c_str(), v[this->M_GetPosition(fldstr.c_str(), positionUsed)]);
          (*itUsed) = false;
        }
        ++itFields;
        ++itUsed;
      }

      m_PointList.push_back(pnt);
    }

    const std::string objectType = MET_ReadType(*m_ReadStream);
    if (objectType.empty())
    {
      // to avoid unrecognized characters
      char c = ' ';
      while ((c != '\n') && (m_ReadStream->good()))
      {
        c = static_cast<char>(m_ReadStream->get());
      }
    }
  }

  if (m_Event)
  {
    m_Event->StopReading();
  }

  return true;
}

bool
MetaTube::M_Write()
{
  if (!MetaObject::M_Write())
  {
    std::cout << "MetaTube: M_Write: Error parsing file" << std::endl;
    return false;
  }

  int     pntDim;
  char ** pntVal = nullptr;
  char    pointDim[4096];

  for (unsigned t = 0; t < this->m_PointDim.size(); t++)
  {
    pointDim[t] = this->m_PointDim[t];
  }
  pointDim[m_PointDim.size()] = '\0';

  MET_StringToWordArray(pointDim, &pntDim, &pntVal);

  META_DEBUG_PRINT( "MetaTube: Parsing point dim" );

  m_Positions.clear();
  std::vector<bool> positionUsed;
  for (int i = 0; i < pntDim; i++)
  {
    PositionType p(pntVal[i], static_cast<const unsigned int &>(i));
    m_Positions.push_back(p);
    positionUsed.push_back(false);
  }

  for (int i = 0; i < pntDim; i++)
  {
    delete[] pntVal[i];
  }
  delete[] pntVal;

  int posId = M_GetPosition("id", positionUsed);
  int posX = M_GetPosition("x", positionUsed);
  int posY = M_GetPosition("y", positionUsed);
  int posZ = M_GetPosition("z", positionUsed);
  int posRed = M_GetPosition("red", positionUsed);
  int posGreen = M_GetPosition("green", positionUsed);
  int posBlue = M_GetPosition("blue", positionUsed);
  int posAlpha = M_GetPosition("alpha", positionUsed);
  int posMark = M_GetPosition("mark", positionUsed);
  if (posMark == -1)
  {
    posMark = M_GetPosition("mk", positionUsed);
  }
  int posR = M_GetPosition("r", positionUsed);
  if (posR == -1)
  {
    posR = M_GetPosition("R", positionUsed);
  }
  if (posR == -1)
  {
    posR = M_GetPosition("radius", positionUsed);
  }
  if (posR == -1)
  {
    posR = M_GetPosition("Radius", positionUsed);
  }
  if (posR == -1)
  {
    posR = M_GetPosition("rad", positionUsed);
  }
  if (posR == -1)
  {
    posR = M_GetPosition("Rad", positionUsed);
  }
  if (posR == -1)
  {
    posR = M_GetPosition("s", positionUsed);
  }
  if (posR == -1)
  {
    posR = M_GetPosition("S", positionUsed);
  }
  int posRn = M_GetPosition("rn", positionUsed);
  int posMn = M_GetPosition("mn", positionUsed);
  int posBn = M_GetPosition("bn", positionUsed);
  int posCv = M_GetPosition("cv", positionUsed);
  int posLv = M_GetPosition("lv", positionUsed);
  int posRo = M_GetPosition("ro", positionUsed);
  int posIn = M_GetPosition("in", positionUsed);
  int posTx = M_GetPosition("tx", positionUsed);
  int posTy = M_GetPosition("ty", positionUsed);
  int posTz = M_GetPosition("tz", positionUsed);
  int posV1x = M_GetPosition("v1x", positionUsed);
  int posV1y = M_GetPosition("v1y", positionUsed);
  int posV1z = M_GetPosition("v1z", positionUsed);
  int posV2x = M_GetPosition("v2x", positionUsed);
  int posV2y = M_GetPosition("v2y", positionUsed);
  int posV2z = M_GetPosition("v2z", positionUsed);
  int posA1 = M_GetPosition("a1", positionUsed);
  int posA2 = M_GetPosition("a2", positionUsed);
  int posA3 = M_GetPosition("a3", positionUsed);

  int                                       extraCount = 0;
  std::vector<PositionType>::const_iterator itFields = m_Positions.begin();
  auto                                      itUsed = positionUsed.begin();
  std::vector<PositionType>::const_iterator itFieldsEnd = m_Positions.end();
  while (itFields != itFieldsEnd)
  {
    if (!(*itUsed))
    {
      ++extraCount;
    }
    ++itFields;
    ++itUsed;
  }


  if (m_BinaryData)
  {
    auto it = m_PointList.begin();
    auto itEnd = m_PointList.end();

    int elementSize;
    MET_SizeOfType(m_ElementType, &elementSize);

    // Allocates memory specifically needed to hold a TubePoint
    int    dataSize = (m_NDims * (2 + m_NDims) + 14 + extraCount) * m_NPoints * elementSize;
    char * data = new char[dataSize];
    // NDIMS * 2: x, alpha
    // NDims * NDims: t, v1, [v2]
    // 14: id, red, green, blue, alpha, mark, r, rn, mn, bn, cv, lv, ro, in
    int dataPos = 0;
    while (it != itEnd)
    {
      std::vector<PositionType>::const_iterator l_itFields = m_Positions.begin();
      std::vector<PositionType>::const_iterator l_itFieldsEnd = m_Positions.end();
      while (l_itFields != l_itFieldsEnd)
      {
        if (l_itFields->second == posId)
        {
          M_SetFloatIntoBinaryData(static_cast<float>((*it)->m_ID), data, dataPos++);
        }
        else if (l_itFields->second == posX)
        {
          M_SetFloatIntoBinaryData((*it)->m_X[0], data, dataPos++);
        }
        else if (l_itFields->second == posY)
        {
          M_SetFloatIntoBinaryData((*it)->m_X[1], data, dataPos++);
        }
        else if (m_NDims > 2 && l_itFields->second == posZ)
        {
          M_SetFloatIntoBinaryData((*it)->m_X[2], data, dataPos++);
        }
        else if (l_itFields->second == posRed)
        {
          M_SetFloatIntoBinaryData((*it)->m_Color[0], data, dataPos++);
        }
        else if (l_itFields->second == posGreen)
        {
          M_SetFloatIntoBinaryData((*it)->m_Color[1], data, dataPos++);
        }
        else if (l_itFields->second == posBlue)
        {
          M_SetFloatIntoBinaryData((*it)->m_Color[2], data, dataPos++);
        }
        else if (l_itFields->second == posAlpha)
        {
          M_SetFloatIntoBinaryData((*it)->m_Color[3], data, dataPos++);
        }
        else if (l_itFields->second == posR)
        {
          M_SetFloatIntoBinaryData((*it)->m_R, data, dataPos++);
        }
        else if (l_itFields->second == posRn)
        {
          M_SetFloatIntoBinaryData((*it)->m_Ridgeness, data, dataPos++);
        }
        else if (l_itFields->second == posMn)
        {
          M_SetFloatIntoBinaryData((*it)->m_Medialness, data, dataPos++);
        }
        else if (l_itFields->second == posBn)
        {
          M_SetFloatIntoBinaryData((*it)->m_Branchness, data, dataPos++);
        }
        else if (l_itFields->second == posCv)
        {
          M_SetFloatIntoBinaryData((*it)->m_Curvature, data, dataPos++);
        }
        else if (l_itFields->second == posRo)
        {
          M_SetFloatIntoBinaryData((*it)->m_Roundness, data, dataPos++);
        }
        else if (l_itFields->second == posLv)
        {
          M_SetFloatIntoBinaryData((*it)->m_Levelness, data, dataPos++);
        }
        else if (l_itFields->second == posIn)
        {
          M_SetFloatIntoBinaryData((*it)->m_Intensity, data, dataPos++);
        }
        else if (l_itFields->second == posMark)
        {
          M_SetFloatIntoBinaryData((*it)->m_Mark ? 1.0f : 0.0f, data, dataPos++);
        }
        else if (l_itFields->second == posTx)
        {
          M_SetFloatIntoBinaryData((*it)->m_T[0], data, dataPos++);
        }
        else if (l_itFields->second == posTy)
        {
          M_SetFloatIntoBinaryData((*it)->m_T[1], data, dataPos++);
        }
        else if (m_NDims > 2 && l_itFields->second == posTz)
        {
          M_SetFloatIntoBinaryData((*it)->m_T[2], data, dataPos++);
        }
        else if (l_itFields->second == posV1x)
        {
          M_SetFloatIntoBinaryData((*it)->m_V1[0], data, dataPos++);
        }
        else if (l_itFields->second == posV1y)
        {
          M_SetFloatIntoBinaryData((*it)->m_V1[1], data, dataPos++);
        }
        else if (m_NDims > 2 && l_itFields->second == posV1z)
        {
          M_SetFloatIntoBinaryData((*it)->m_V1[2], data, dataPos++);
        }
        else if (l_itFields->second == posV2x)
        {
          M_SetFloatIntoBinaryData((*it)->m_V2[0], data, dataPos++);
        }
        else if (l_itFields->second == posV2y)
        {
          M_SetFloatIntoBinaryData((*it)->m_V2[1], data, dataPos++);
        }
        else if (m_NDims > 2 && l_itFields->second == posV2z)
        {
          M_SetFloatIntoBinaryData((*it)->m_V2[2], data, dataPos++);
        }
        else if (l_itFields->second == posA1)
        {
          M_SetFloatIntoBinaryData((*it)->m_Alpha1, data, dataPos++);
        }
        else if (l_itFields->second == posA2)
        {
          M_SetFloatIntoBinaryData((*it)->m_Alpha2, data, dataPos++);
        }
        else if (l_itFields->second == posA3)
        {
          M_SetFloatIntoBinaryData((*it)->m_Alpha3, data, dataPos++);
        }
        else
        {
          int indx = (*it)->GetFieldIndex(l_itFields->first.c_str());
          if (indx >= 0)
          {
            M_SetFloatIntoBinaryData((*it)->GetField(indx), data, dataPos++);
          }
          else
          {
            std::cerr << "Cannot find value for field " << l_itFields->first << std::endl;
          }
        }
        ++l_itFields;
      }

      ++it;
    }

    m_WriteStream->write(data, dataSize);
    m_WriteStream->write("\n", 1);

    delete[] data;
  }
  else
  {
    PointListType::const_iterator it = m_PointList.begin();
    PointListType::const_iterator itEnd = m_PointList.end();

    int dataPos = 0;
    while (it != itEnd)
    {
      std::vector<PositionType>::const_iterator l_itFields = m_Positions.begin();
      std::vector<PositionType>::const_iterator l_itFieldsEnd = m_Positions.end();
      while (l_itFields != l_itFieldsEnd)
      {
        if (l_itFields->second == posId)
        {
          *m_WriteStream << (*it)->m_ID << " ";
          dataPos++;
        }
        else if (l_itFields->second == posX)
        {
          *m_WriteStream << (*it)->m_X[0] << " ";
          dataPos++;
        }
        else if (l_itFields->second == posY)
        {
          *m_WriteStream << (*it)->m_X[1] << " ";
          dataPos++;
        }
        else if (m_NDims > 2 && l_itFields->second == posZ)
        {
          *m_WriteStream << (*it)->m_X[2] << " ";
          dataPos++;
        }
        else if (l_itFields->second == posRed)
        {
          *m_WriteStream << (*it)->m_Color[0] << " ";
          dataPos++;
        }
        else if (l_itFields->second == posGreen)
        {
          *m_WriteStream << (*it)->m_Color[1] << " ";
          dataPos++;
        }
        else if (l_itFields->second == posBlue)
        {
          *m_WriteStream << (*it)->m_Color[2] << " ";
          dataPos++;
        }
        else if (l_itFields->second == posAlpha)
        {
          *m_WriteStream << (*it)->m_Color[3] << " ";
          dataPos++;
        }
        else if (l_itFields->second == posR)
        {
          *m_WriteStream << (*it)->m_R << " ";
          dataPos++;
        }
        else if (l_itFields->second == posRn)
        {
          *m_WriteStream << (*it)->m_Ridgeness << " ";
          dataPos++;
        }
        else if (l_itFields->second == posMn)
        {
          *m_WriteStream << (*it)->m_Medialness << " ";
          dataPos++;
        }
        else if (l_itFields->second == posBn)
        {
          *m_WriteStream << (*it)->m_Branchness << " ";
          dataPos++;
        }
        else if (l_itFields->second == posCv)
        {
          *m_WriteStream << (*it)->m_Curvature << " ";
          dataPos++;
        }
        else if (l_itFields->second == posRo)
        {
          *m_WriteStream << (*it)->m_Roundness << " ";
          dataPos++;
        }
        else if (l_itFields->second == posLv)
        {
          *m_WriteStream << (*it)->m_Levelness << " ";
          dataPos++;
        }
        else if (l_itFields->second == posIn)
        {
          *m_WriteStream << (*it)->m_Intensity << " ";
          dataPos++;
        }
        else if (l_itFields->second == posMark)
        {
          if ((*it)->m_Mark)
          {
            *m_WriteStream << "1 ";
          }
          else
          {
            *m_WriteStream << "0 ";
          }
          dataPos++;
        }
        else if (l_itFields->second == posTx)
        {
          *m_WriteStream << (*it)->m_T[0] << " ";
          dataPos++;
        }
        else if (l_itFields->second == posTy)
        {
          *m_WriteStream << (*it)->m_T[1] << " ";
          dataPos++;
        }
        else if (m_NDims > 2 && l_itFields->second == posTz)
        {
          *m_WriteStream << (*it)->m_T[2] << " ";
          dataPos++;
        }
        else if (l_itFields->second == posV1x)
        {
          *m_WriteStream << (*it)->m_V1[0] << " ";
          dataPos++;
        }
        else if (l_itFields->second == posV1y)
        {
          *m_WriteStream << (*it)->m_V1[1] << " ";
          dataPos++;
        }
        else if (m_NDims > 2 && l_itFields->second == posV1z)
        {
          *m_WriteStream << (*it)->m_V1[2] << " ";
          dataPos++;
        }
        else if (l_itFields->second == posV2x)
        {
          *m_WriteStream << (*it)->m_V2[0] << " ";
          dataPos++;
        }
        else if (l_itFields->second == posV2y)
        {
          *m_WriteStream << (*it)->m_V2[1] << " ";
          dataPos++;
        }
        else if (m_NDims > 2 && l_itFields->second == posV2z)
        {
          *m_WriteStream << (*it)->m_V2[2] << " ";
          dataPos++;
        }
        else if (l_itFields->second == posA1)
        {
          *m_WriteStream << (*it)->m_Alpha1 << " ";
          dataPos++;
        }
        else if (l_itFields->second == posA2)
        {
          *m_WriteStream << (*it)->m_Alpha2 << " ";
          dataPos++;
        }
        else if (l_itFields->second == posA3)
        {
          *m_WriteStream << (*it)->m_Alpha3 << " ";
          dataPos++;
        }
        else
        {
          int indx = (*it)->GetFieldIndex(l_itFields->first.c_str());
          if (indx >= 0)
          {
            *m_WriteStream << (*it)->GetField(indx) << " ";
            dataPos++;
          }
          else
          {
            std::cerr << "Cannot find value for field " << l_itFields->first << std::endl;
          }
        }
        ++l_itFields;
      }

      *m_WriteStream << std::endl;
      ++it;
    }
  }
  return true;
}

#if (METAIO_USE_NAMESPACE)
};
#endif
