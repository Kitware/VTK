/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHoudiniPolyDataWriter.h"

#include <algorithm>

#include "vtkAbstractArray.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkInformation.h"
#include "vtkIntArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkLongArray.h"
#include "vtkLongLongArray.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSetGet.h"
#include "vtkSignedCharArray.h"
#include "vtkShortArray.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedLongLongArray.h"
#include "vtkUnsignedShortArray.h"

vtkStandardNewMacro(vtkHoudiniPolyDataWriter);

namespace
{
  // Houdini geometry files store point/cell data in-line with the point/cell
  // definition. So, the point data access pattern is to write a point's
  // coordinates, followed by its data values for each point data attribute.
  // This storage pattern differs from VTK's, where all points are
  // logically held in a contiguous memory block, followed by all of the values
  // for a single data attribute. To accommodate this discrepancy in data
  // access, we construct a facade for point/cell attributes that allows us to
  // stream all of the values associated with a single point/cell.

  struct AttributeBase
  {
    virtual ~AttributeBase() {}
    virtual void StreamHeader(std::ostream&) const = 0;
    virtual void StreamData(std::ostream&, vtkIdType) const = 0;
  };

  template <int AttributeId>
  struct AttributeTrait;

#define DefineAttributeTrait(attId, attType, attName, vtkArray, attDefault) \
  template <>                                                           \
  struct AttributeTrait<attId>                                          \
  {                                                                     \
    typedef attType Type;                                               \
    typedef vtkArray vtkArrayType;                                      \
    std::string Name() const { return std::string(attName); }           \
    attType Default() const { return static_cast<attType>(attDefault); } \
    static void Get(vtkIdType index, attType* in, vtkArray* array)      \
    { array->GetTypedTuple(index, in); }                                \
    static void Stream(std::ostream& out, attType t) { out << t; }      \
  }

  DefineAttributeTrait(VTK_DOUBLE, double, "float", vtkDoubleArray, 0.0);
  DefineAttributeTrait(VTK_FLOAT, float, "float", vtkFloatArray, 0.0);
  DefineAttributeTrait(VTK_LONG_LONG, long long, "int", vtkLongLongArray, 0);
  DefineAttributeTrait(VTK_UNSIGNED_LONG_LONG, unsigned long long, "int", vtkUnsignedLongLongArray, 0);
  DefineAttributeTrait(VTK_ID_TYPE, vtkIdType, "int", vtkIdTypeArray, 0);
  DefineAttributeTrait(VTK_LONG, long, "int", vtkLongArray, 0);
  DefineAttributeTrait(VTK_UNSIGNED_LONG, unsigned long, "int", vtkUnsignedLongArray, 0);
  DefineAttributeTrait(VTK_INT, int, "int", vtkIntArray, 0);
  DefineAttributeTrait(VTK_UNSIGNED_INT, unsigned int, "int", vtkUnsignedIntArray, 0);
  DefineAttributeTrait(VTK_SHORT, short, "int", vtkShortArray, 0);
  DefineAttributeTrait(VTK_UNSIGNED_SHORT, unsigned short, "int", vtkUnsignedShortArray, 0);

#undef DefineAttributeTrait

  template <>
  struct AttributeTrait<VTK_CHAR>
  {
    typedef char Type;
    typedef vtkCharArray vtkArrayType;
    std::string Name() const { return std::string("int"); }
    int Default() const { return static_cast<int>('0'); }
    static void Get(vtkIdType index, char* in, vtkCharArray* array)
    { array->GetTypedTuple(index, in); }
    static void Stream(std::ostream& out, char t)
    {
      out << static_cast<int>(t);
    }
  };

  template <>
  struct AttributeTrait<VTK_SIGNED_CHAR>
  {
    typedef signed char Type;
    typedef vtkSignedCharArray vtkArrayType;
    std::string Name() const { return std::string("int"); }
    int Default() const { return static_cast<int>('0'); }
    static void Get(vtkIdType index, signed char* in, vtkSignedCharArray* array)
    { array->GetTypedTuple(index, in); }
    static void Stream(std::ostream& out, signed char t)
    {
      out << static_cast<int>(t);
    }
  };

  template <>
  struct AttributeTrait<VTK_UNSIGNED_CHAR>
  {
    typedef unsigned char Type;
    typedef vtkUnsignedCharArray vtkArrayType;
    std::string Name() const { return std::string("int"); }
    int Default() const { return static_cast<int>('0'); }
    static void Get(vtkIdType index, unsigned char* in,
                    vtkUnsignedCharArray* array)
    { array->GetTypedTuple(index, in); }
    static void Stream(std::ostream& out, unsigned char t)
    {
      out << static_cast<int>(t);
    }
  };

  template <>
  struct AttributeTrait<VTK_STRING>
  {
    typedef vtkStdString Type;
    typedef vtkStringArray vtkArrayType;
    std::string Name() const { return std::string("string"); }
    vtkStdString Default() const { return vtkStdString("None"); }
    static void Get(vtkIdType index, vtkStdString* in, vtkStringArray* array)
    {
      assert(array->GetNumberOfComponents() == 1);
      *in = array->GetValue(index);
    }
    static void Stream(std::ostream& out, const vtkStdString& t)
    {
      std::size_t i = 0;
      out << "\'";
      for (; i < (t.size() < 32 ? t.size() : 32); i++)
      {
        out << t[i];
      }
      for (; i < 32; i++)
      {
        out << " ";
      }
      out << "\'";
    }
  };

  template <int AttributeId>
  struct Attribute : public AttributeBase
  {
    typedef typename AttributeTrait<AttributeId>::vtkArrayType vtkArrayType;

    Attribute(vtkAbstractArray* array) : AttributeBase()
    {
      this->Array = vtkArrayType::SafeDownCast(array);
      assert(this->Array != NULL);
      this->Value.resize(this->Array->GetNumberOfComponents());
    }

    void StreamHeader(std::ostream& out) const VTK_OVERRIDE
    {
      std::string s = this->Array->GetName();
      std::replace(s.begin(), s.end(), ' ', '_');
      std::replace(s.begin(), s.end(), '\t', '-');

      AttributeTrait<AttributeId> trait;
      out << s << " " << this->Array->GetNumberOfComponents() << " "
          << trait.Name() << " " << trait.Default();
      for (int i = 1; i < this->Array->GetNumberOfComponents(); i++)
      {
        out << " ";
        AttributeTrait<AttributeId>::Stream(out, trait.Default());
      }
    }

    void StreamData(std::ostream& out, vtkIdType index) const VTK_OVERRIDE
    {
      assert(index < this->Array->GetNumberOfTuples());

      AttributeTrait<AttributeId>::Get(index, &this->Value[0], this->Array);
      AttributeTrait<AttributeId>::Stream(out, this->Value[0]);

      for (int i = 1; i < this->Array->GetNumberOfComponents(); i++)
      {
          out << " ";
        AttributeTrait<AttributeId>::Stream(out, this->Value[i]);
      }
    }

  protected:
    mutable std::vector<typename AttributeTrait<AttributeId>::Type> Value;
    mutable vtkArrayType* Array;
  };

  class Attributes
  {
  public:
    typedef std::vector<AttributeBase*>::iterator AttIt;

    class Header
    {
      friend class Attributes;
      Header(Attributes* atts) : Atts(atts) {}
      void operator=(const Attributes::Header&) VTK_DELETE_FUNCTION;

      friend ostream& operator<<(ostream& out, const Attributes::Header& header)
      {
        for (Attributes::AttIt it=header.Atts->AttVec.begin();
             it != header.Atts->AttVec.end(); ++it)
        {
          (*it)->StreamHeader(out);
          out << endl;
        }
        return out;
      }
    public:
      Attributes* Atts;
    };

    class Component
    {
      friend class Attributes;

      Component(Attributes* atts, vtkIdType index) : Atts(atts),
                                                     Index(index) {}

      Attributes* Atts;
      vtkIdType Index;

      friend ostream& operator<<(ostream& out,
                                 const Attributes::Component& component)
      {
        for (Attributes::AttIt it=component.Atts->AttVec.begin();
             it != component.Atts->AttVec.end(); ++it)
        {
          (*it)->StreamData(out, component.Index);

          if (it + 1 != component.Atts->AttVec.end())
          {
            out << " ";
          }
        }
        return out;
      }
    };

    Attributes() : Hdr(NULL) { this->Hdr.Atts = this; }
    virtual ~Attributes()
    {
      for (AttIt it=this->AttVec.begin(); it != this->AttVec.end(); ++it)
      {
        delete *it;
      }
    }

    Header& GetHeader() { return this->Hdr; }

    Component operator[](vtkIdType i)
    {
      return Attributes::Component(this, i);
    }

    template <int TypeId>
    void AddAttribute(vtkAbstractArray* array)
    {
      this->AttVec.push_back(new Attribute<TypeId>(array));
    }

    Header Hdr;
    std::vector<AttributeBase*> AttVec;
  };

  template <int TypeId>
  void AddAttribute(Attributes& atts, vtkAbstractArray* array)
  {
    atts.AddAttribute<TypeId>(array);
  }
}

//----------------------------------------------------------------------------
vtkHoudiniPolyDataWriter::vtkHoudiniPolyDataWriter()
{
  this->FileName = NULL;
}

//----------------------------------------------------------------------------
vtkHoudiniPolyDataWriter::~vtkHoudiniPolyDataWriter()
{
  this->SetFileName(NULL);
}

//----------------------------------------------------------------------------
void vtkHoudiniPolyDataWriter::WriteData()
{
  // Grab the input data
  vtkPolyData* input = vtkPolyData::SafeDownCast(this->GetInput());
  if (!input)
  {
    vtkErrorMacro(<< "Missing input polydata!");
    return;
  }

  // Open the file for streaming
  std::ofstream file(this->FileName, std::ofstream::out);

  if (file.fail())
  {
    vtkErrorMacro(<< "Unable to open file: "<< this->FileName);
    return;
  }

  vtkIdType nPrims = 0;
  {
    nPrims += input->GetNumberOfVerts();
    nPrims += input->GetNumberOfLines();
    nPrims += input->GetNumberOfPolys();

    vtkCellArray* stripArray = input->GetStrips();
    vtkIdType nPts, *pts;

    stripArray->InitTraversal();
    while (stripArray->GetNextCell(nPts, pts))
    {
      nPrims += nPts - 2;
    }
  }

  // Write generic header info
  file << "PGEOMETRY V2" << endl;
  file << "NPoints " << input->GetNumberOfPoints() << " "
       << "NPrims " << nPrims << endl;
  file << "NPointGroups " << 0 << " NPrimGroups " << 0 << endl;
  file << "NPointAttrib " << input->GetPointData()->GetNumberOfArrays()<< " "
       << "NVertexAttrib " << 0 << " "
       << "NPrimAttrib " << input->GetCellData()->GetNumberOfArrays() << " "
       << "NAttrib " << 0 << endl;

#define vtkHoudiniTemplateMacroCase(typeN, atts, arr)           \
  case typeN: { AddAttribute<typeN>(atts, arr); }; break
#define vtkHoudiniTemplateMacro(atts, arr)                              \
  vtkHoudiniTemplateMacroCase(VTK_DOUBLE, atts, arr);                   \
  vtkHoudiniTemplateMacroCase(VTK_FLOAT, atts, arr);                    \
  vtkHoudiniTemplateMacroCase(VTK_LONG_LONG, atts, arr);                \
  vtkHoudiniTemplateMacroCase(VTK_UNSIGNED_LONG_LONG, atts, arr);       \
  vtkHoudiniTemplateMacroCase(VTK_ID_TYPE, atts, arr);                  \
  vtkHoudiniTemplateMacroCase(VTK_LONG, atts, arr);                     \
  vtkHoudiniTemplateMacroCase(VTK_UNSIGNED_LONG, atts, arr);            \
  vtkHoudiniTemplateMacroCase(VTK_INT, atts, arr);                      \
  vtkHoudiniTemplateMacroCase(VTK_UNSIGNED_INT, atts, arr);             \
  vtkHoudiniTemplateMacroCase(VTK_SHORT, atts, arr);                    \
  vtkHoudiniTemplateMacroCase(VTK_UNSIGNED_SHORT, atts, arr);           \
  vtkHoudiniTemplateMacroCase(VTK_CHAR, atts, arr);                     \
  vtkHoudiniTemplateMacroCase(VTK_SIGNED_CHAR, atts, arr);              \
  vtkHoudiniTemplateMacroCase(VTK_UNSIGNED_CHAR, atts, arr)

  // Construct Attributes instance for points
  Attributes pointAttributes;
  for (vtkIdType i = 0; i < input->GetPointData()->GetNumberOfArrays(); i++)
  {
    vtkAbstractArray* array = input->GetPointData()->GetAbstractArray(i);
    switch (array->GetDataType())
    {
      vtkHoudiniTemplateMacro(pointAttributes, array);
#if 0
    case VTK_STRING: { AddAttribute<VTK_STRING>(pointAttributes, array); }; break;
#endif
    default:
      vtkGenericWarningMacro(<<"Unsupported data type!");
    }
  }

  // Write point attributes header info
  if (input->GetPointData()->GetNumberOfArrays() != 0)
  {
    file << "PointAttrib" << endl;
    file << pointAttributes.GetHeader();
  }

  // Write point data
  vtkPoints* points = input->GetPoints();
  double xyz[3];
  for (vtkIdType i = 0; i < input->GetNumberOfPoints(); i++)
  {
    points->GetPoint(i, xyz);
    file << xyz[0] << " " << xyz[1] << " " << xyz[2] << " "
         << 1;
    if (input->GetPointData()->GetNumberOfArrays() != 0)
    {
      file << " (" << pointAttributes[i] << ")";
    }
    file << endl;
  }

  // Construct Attributes instance for cells
  Attributes cellAttributes;
  for (vtkIdType i = 0; i < input->GetCellData()->GetNumberOfArrays(); i++)
  {
    vtkAbstractArray* array = input->GetCellData()->GetAbstractArray(i);
    switch (array->GetDataType())
    {
      vtkHoudiniTemplateMacro(cellAttributes, array);
#if 0
    case VTK_STRING: { AddAttribute<VTK_STRING>(cellAttributes, array); }; break;
#endif
    default:
      vtkGenericWarningMacro(<<"Unsupported data type!");
    }
  }

#undef vtkHoudiniTemplateMacro
#undef vtkHoudiniTemplateMacroCase

  // Write cell attributes header info
  if (input->GetCellData()->GetNumberOfArrays() != 0 &&
      input->GetNumberOfCells() != 0)
  {
    file << "PrimitiveAttrib" << endl;
    file << cellAttributes.GetHeader();
  }

  if (input->GetNumberOfVerts() != 0)
  {
    // Write vertex data as a particle system
    vtkCellArray* vertArray = input->GetVerts();
    vtkIdType nPts, *pts, cellId;

    if (input->GetNumberOfVerts() > 1)
    {
      file << "Run " << input->GetNumberOfVerts() << " Part" << endl;
    }
    else
    {
      file << "Part ";
    }
    cellId = 0;

    vertArray->InitTraversal();
    while (vertArray->GetNextCell(nPts, pts))
    {
      file << nPts;
      for (vtkIdType i = 0; i < nPts; i++)
      {
        file << " " << pts[i];
      }
      if (input->GetCellData()->GetNumberOfArrays() != 0)
      {
        file << " [" << cellAttributes[cellId] << "]";
      }
      file << endl;
      cellId++;
    }
  }

  if (input->GetNumberOfLines() != 0)
  {
    // Write line data as open polygons
    file << "Run " << input->GetNumberOfLines() << " Poly" <<endl;

    vtkCellArray* lineArray = input->GetLines();
    vtkIdType nPts, *pts, cellId;

    cellId = input->GetNumberOfVerts();

    lineArray->InitTraversal();
    while (lineArray->GetNextCell(nPts, pts))
    {
      file << nPts << " : " << pts[0];
      for (vtkIdType i = 1; i < nPts; i++)
      {
        file << " " << pts[i];
      }
      if (input->GetCellData()->GetNumberOfArrays() != 0)
      {
        file << " [" << cellAttributes[cellId++] << "]";
      }
      file << endl;
    }
  }

  if (input->GetNumberOfPolys() != 0)
  {
    // Write polygon data
    file << "Run " << input->GetNumberOfPolys() << " Poly" <<endl;

    vtkCellArray* polyArray = input->GetPolys();
    vtkIdType nPts, *pts, cellId;

    cellId = (input->GetNumberOfVerts() + input->GetNumberOfLines());

    polyArray->InitTraversal();
    while (polyArray->GetNextCell(nPts, pts))
    {
      file << nPts << " < " << pts[0];
      for (vtkIdType i = 1; i < nPts; i++)
      {
        file << " " << pts[i];
      }
      if (input->GetCellData()->GetNumberOfArrays() != 0)
      {
        file << " [" << cellAttributes[cellId++] << "]";
      }
      file << endl;
    }
  }

  if (input->GetNumberOfStrips() != 0)
  {
    // Write triangle strip data as polygons
    vtkCellArray* stripArray = input->GetStrips();
    vtkIdType nPts, *pts, cellId;

    cellId = (input->GetNumberOfVerts() +
              input->GetNumberOfLines() +
              input->GetNumberOfPolys());

    stripArray->InitTraversal();
    while (stripArray->GetNextCell(nPts, pts))
    {
      if (nPts > 3)
      {
        file << "Run " << nPts - 2 << " Poly" << endl;
      }
      else
      {
        file << "Poly ";
      }

      for (vtkIdType i = 2; i < nPts; i++)
      {
        if (i%2 == 0)
        {
          file << "3 < "
               << pts[i - 2] << " " << pts[i - 1] << " " << pts[i];
        }
        else
        {
          file << "3 < "
               << pts[i - 1] << " " << pts[i - 2] << " " << pts[i];
        }
        if (input->GetCellData()->GetNumberOfArrays() != 0)
        {
          file << " [" << cellAttributes[cellId] << "]";
        }
        file << endl;
      }
      cellId++;
    }
  }

  file << "beginExtra" << endl;
  file << "endExtra" << endl;

  file.close();
}

//----------------------------------------------------------------------------
int vtkHoudiniPolyDataWriter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  return 1;
}

//----------------------------------------------------------------------------
void vtkHoudiniPolyDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: "
     << (this->FileName? this->FileName:"(none)") << "\n";
}
