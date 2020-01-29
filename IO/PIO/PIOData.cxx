#include <PIOData.h>
#include <iostream>
#include <vtksys/FStream.hxx>

PIO_DATA::PIO_DATA(const char* piofile, const std::list<std::string>* fields_to_read,
  bool _defer_read_data, const std::set<const char*, Cstring_less>* rdata,
  const std::set<const char*, Cstring_less>* cdata)
{
  this->Infile = nullptr;
  buf = 0;
  size_buf = 0;
  pio_field = 0;
  name = 0;
  pio_dandt = 0;
  verbose = false;
  defer_read_data = _defer_read_data;
  // Add known data type field
  AddRealData("controller_r8");
  AddRealData("matdef");
  AddRealData("ist");
  AddRealData("irt");
  AddRealData("frac_mass_c");
  AddRealData("frac_mass_m");
  AddRealData("frac_vol_c");
  AddRealData("frac_vol_m");
  AddRealData("frac_eng_c");
  AddRealData("frac_eng_m");
  AddRealData("chunk_nummat");
  AddRealData("chunk_mat");
  AddRealData("chunk_vol");
  AddRealData("chunk_eng");
  AddRealData("cell_momentum");
  AddRealData("old_numpe");
  AddRealData("strength_num");
  AddRealData("strength_nm");
  AddRealData("global_numcell");
  AddRealData("cell_center");
  AddRealData("amhc_i");
  AddRealData("amhc_r8");
  AddRealData("amhc_l");
  AddRealData("frac_mass");
  AddRealData("frac_vol");
  AddRealData("frac_eng");
  AddRealData("cell_level");
  AddRealData("cell_index");
  AddRealData("cell_mother");
  AddRealData("cell_daughter");
  AddRealData("vcell");
  AddRealData("mass");
  AddRealData("pres");
  AddRealData("tev");
  AddRealData("rade");
  AddRealData("sound");
  AddRealData("cell_energy");
  AddRealData("numm");
  AddRealData("idents");
  AddRealData("numt");
  AddRealData("teos_t");
  AddRealData("numprs");
  AddRealData("teos_p");
  AddRealData("teos_r");
  AddRealData("teos_e");
  AddRealData("npmin_t");
  AddRealData("npmax_t");
  AddRealData("pmin_t");
  AddRealData("pmax_t");
  AddRealData("npmax_t");
  AddRealData("npmax_t");
  AddRealData("pmax_t");
  AddRealData("pmax_t");
  if (rdata != 0)
  {
    std::set<const char*, Cstring_less>::const_iterator q;
    for (q = rdata->begin(); q != rdata->end(); ++q)
      AddRealData(*q);
  }

  AddCharData("matident");
  AddCharData("hist_prbnm");
  if (cdata != 0)
  {
    std::set<const char*, Cstring_less>::const_iterator q;
    for (q = cdata->begin(); q != cdata->end(); ++q)
      AddCharData(*q);
  }

  if (piofile != 0)
  {
    if (!read(piofile, fields_to_read))
    {
      if (pio_field)
        delete[] pio_field;
      pio_field = 0;
    }
    else
      insert_VAR_MAP_pairs();
  }
} // End PIO_DATA::PIO_DATA

PIO_DATA::~PIO_DATA()
{
  if (buf)
    delete[] buf;
  buf = 0;
  size_buf = 0;
  if (pio_field)
  {
    for (int i = 0; i < pio_num; ++i)
    {
      if (pio_field[i].data)
        delete[] pio_field[i].data;
      pio_field[i].data = 0;
      if (pio_field[i].cdata)
        delete[] pio_field[i].cdata;
      pio_field[i].cdata = 0;
      pio_field[i].cdata_len = 0;
      if (pio_field[i].pio_name)
      {
        pio_field[i].pio_name[0] = '\0';
        delete[] pio_field[i].pio_name;
      }
    }
    delete[] pio_field;
    pio_field = 0;
  }
  if (name)
    delete[] name;
  name = 0;
  if (pio_dandt)
    delete[] pio_dandt;
  pio_dandt = 0;
  VarMMap.clear();
  delete this->Infile;
  this->Infile = nullptr;
  std::set<const char*, Cstring_less>::const_iterator q;
  for (q = RealData.begin(); q != RealData.end(); ++q)
    delete[] * q;
  RealData.clear();
  for (q = CharData.begin(); q != CharData.end(); ++q)
    delete[] * q;
  CharData.clear();
} // End PIO_DATA::~PIO_DATA()

bool PIO_DATA::read(const char* piofile, const std::list<std::string>* fields_to_read)
{
  bool status;
  if (piofile == 0)
  {
    std::cerr << "PIO_DATA::read - file name not given" << std::endl;
    return false;
  }
  delete this->Infile;
  this->Infile = new vtksys::ifstream(piofile, std::ios::binary);
  if (this->Infile->fail())
  {
    delete this->Infile;
    this->Infile = nullptr;
    return false;
  }
  status = read(fields_to_read);
  if (!defer_read_data)
  {
    delete this->Infile;
    this->Infile = nullptr;
  }
  return status;
} // End PIO_DATA::read

static bool is_a_string(char* c, size_t len)
{
  if (c[0] == '\0') // No empty strings should exist
    return false;
  for (size_t j = 0; j < len; ++j)
  {
    if (!isascii(c[j]))
      return false;
    if (!isalnum(c[j]) && !ispunct(c[j]) && !isspace(c[j]))
      return false;
  }
  return true;
}

bool PIO_DATA::read(const std::list<std::string>* fields_to_read)
{
  double two;

  this->Infile->seekg(0, std::ios::beg);
  name = read_pio_char_string(8);
  if (strcmp(name, "pio_file") != 0)
  {
    delete this->Infile;
    this->Infile = nullptr;
    return false;
  }
  this->Infile->read((char*)&two, sizeof(two));
  reverse_endian = (two != 2.0) ? true : false;
  read_pio_word(PIO_VERSION);
  read_pio_word(PIO_NAME_LENGTH);
  read_pio_word(PIO_HEADER_LENGTH);
  read_pio_word(PIO_INDEX_LENGTH);
  pio_dandt = read_pio_char_string(16);
  read_pio_word(pio_num);
  pio_position = sizeof(double) * read_pio_word(pio_position);
  read_pio_word(pio_signature);
  if (pio_num <= 0)
  {
    pio_field = 0;
    delete this->Infile;
    this->Infile = nullptr;
    return true;
  }
  pio_field = new PIO_FIELD[pio_num];
  memset((void*)pio_field, 0, pio_num * sizeof(PIO_FIELD));
  this->Infile->seekg(pio_position, std::ios::beg);

  for (int i = 0; i < pio_num; ++i)
  {
    int64_t num_read = PIO_INDEX_LENGTH * sizeof(double);
    pio_field[i].pio_name = read_pio_char_string(PIO_NAME_LENGTH);
    num_read -= PIO_NAME_LENGTH;
    read_pio_word(pio_field[i].index);
    num_read -= sizeof(double);
    read_pio_word(pio_field[i].length);
    num_read -= sizeof(double);
    pio_field[i].position = sizeof(double) * read_pio_word(pio_field[i].position);
    num_read -= sizeof(double);
    read_pio_word(pio_field[i].chksum);
    num_read -= sizeof(double);
    this->Infile->seekg(num_read, std::ios::cur);
    pio_field[i].read_field_data = read_field(pio_field[i].pio_name, fields_to_read);
  }

  matident_len = sizeof(double);
  timertype_len = 2 * sizeof(double);
  for (int i = 0; i < pio_num; ++i)
  {
    if ((pio_field[i].length > 0) && (strcmp(pio_field[i].pio_name, "MATIDENT_LEN") == 0))
    {
      this->Infile->seekg(pio_field[i].position, std::ios::beg);
      double dtmp;
      read_pio_word(dtmp);
      matident_len = (size_t)dtmp;
    }
    if ((pio_field[i].length > 0) && (strcmp(pio_field[i].pio_name, "TIMERTYPE_LEN") == 0))
    {
      this->Infile->seekg(pio_field[i].position, std::ios::beg);
      double dtmp;
      read_pio_word(dtmp);
      timertype_len = (size_t)dtmp;
    }
    if ((pio_field[i].length > 0) && pio_field[i].read_field_data && !defer_read_data)
      ReadPioFieldData(pio_field[i]);
  }
  return true;
} // End PIO_DATA::read

bool PIO_DATA::set_scalar_field(std::valarray<int>& v, const char* fieldname)
{
  if (VarMMap.count(fieldname) != 1)
  {
    v.resize(0);
    return false;
  }
  PIO_FIELD* Pio_field = VarMMap.equal_range(fieldname).first->second;
  bool free_data = (Pio_field->data == 0);
  const double* cl = GetPIOData(*Pio_field);
  if (cl != 0)
  {
    int64_t length = Pio_field->length;
    if (v.size() < size_t(length))
      v.resize(length);
    for (int64_t i = 0; i < length; ++i)
      v[i] = int(cl[i]);
    if (free_data)
      FreePIOData(*Pio_field);
    if (verbose)
      std::cerr << "Set integer scalar field " << fieldname << "\n";
    return true;
  }
  v.resize(0);
  return false;
} // End PIO_DATA::set_scalar_field

bool PIO_DATA::set_scalar_field(std::valarray<int64_t>& v, const char* fieldname)
{
  if (VarMMap.count(fieldname) != 1)
  {
    v.resize(0);
    return false;
  }
  PIO_FIELD* Pio_field = VarMMap.equal_range(fieldname).first->second;
  bool free_data = (Pio_field->data == 0);
  const double* cl = GetPIOData(*Pio_field);
  if (cl != 0)
  {
    int64_t length = Pio_field->length;
    if (v.size() < size_t(length))
      v.resize(length);
    for (int64_t i = 0; i < length; ++i)
      v[i] = int64_t(cl[i]);
    if (free_data)
      FreePIOData(*Pio_field);
    if (verbose)
      std::cerr << "Set int64_t scalar field " << fieldname << "\n";
    return true;
  }
  v.resize(0);
  return false;
} // End PIO_DATA::set_scalar_field

bool PIO_DATA::set_scalar_field(std::valarray<uint64_t>& v, const char* fieldname)
{
  if (VarMMap.count(fieldname) != 1)
  {
    v.resize(0);
    return false;
  }
  PIO_FIELD* Pio_field = VarMMap.equal_range(fieldname).first->second;
  bool free_data = (Pio_field->data == 0);
  const double* cl = GetPIOData(*Pio_field);
  if (cl != 0)
  {
    int64_t length = Pio_field->length;
    if (v.size() < size_t(length))
      v.resize(length);
    for (int64_t i = 0; i < length; ++i)
      v[i] = uint64_t(cl[i]);
    if (free_data)
      FreePIOData(*Pio_field);
    if (verbose)
      std::cerr << "Set uint64_t scalar field " << fieldname << "\n";
    return true;
  }
  v.resize(0);
  return false;
} // End PIO_DATA::set_scalar_field

bool PIO_DATA::set_scalar_field(std::valarray<double>& v, const char* fieldname)
{
  if (VarMMap.count(fieldname) != 1)
  {
    v.resize(0);
    return false;
  }
  PIO_FIELD* Pio_field = VarMMap.equal_range(fieldname).first->second;
  bool free_data = (Pio_field->data == 0);
  const double* cl = GetPIOData(*Pio_field);
  if (cl != 0)
  {
    int64_t length = Pio_field->length;

    const double* cell_active = 0;
    PIO_FIELD* Pf = 0;
    bool FreePf = false;
    if (VarMMap.count("cell_active") == 1)
    {
      Pf = VarMMap.equal_range("cell_active").first->second;
      if (Pf->length == length)
      {
        FreePf = (Pf->data == 0);
        cell_active = GetPIOData(*Pf);
      }
    }
    if (v.size() < size_t(length))
      v.resize(length);
    for (int64_t i = 0; i < length; ++i)
    {
      if (cell_active && (cell_active[i] == 0.0))
        v[i] = HUGE_VAL;
      else
        v[i] = cl[i];
    }
    if (free_data)
      FreePIOData(*Pio_field);
    if (FreePf)
      FreePIOData(*Pf);
    if (verbose)
      std::cerr << "Set double scalar field " << fieldname << "\n";
    return true;
  }
  v.resize(0);
  return false;
} // End PIO_DATA::set_scalar_field

bool PIO_DATA::set_vector_field(std::valarray<std::valarray<double> >& v, const char* fieldname)
{
  uint32_t numdim = static_cast<uint32_t>(VarMMap.count(fieldname));
  if (numdim <= 0)
  {
    v.resize(0);
    return false;
  }
  if (v.size() < numdim)
    v.resize(numdim);
  const double* cell_active = 0;
  int64_t cell_active_length = 0;
  PIO_FIELD* Pf = 0;
  bool FreePf = false;
  if (VarMMap.count("cell_active") == 1)
  {
    Pf = VarMMap.equal_range("cell_active").first->second;
    FreePf = (Pf->data == 0);
    cell_active = GetPIOData(*Pf);
    cell_active_length = Pf->length;
  }
  VMP b = VarMMap.equal_range(fieldname);
  VMI ii = b.first;
  for (uint32_t i = 0; (i < numdim) && (ii != b.second); ++i, ++ii)
  {
    PIO_FIELD* Pio_field = ii->second;
    bool free_data = (Pio_field->data == 0);
    const double* cl = GetPIOData(*Pio_field);
    if (cl != 0)
    {
      int64_t length = Pio_field->length;
      if (v[i].size() < size_t(length))
        v[i].resize(length);
      for (int64_t j = 0; j < length; ++j)
      {
        if (cell_active && (cell_active_length == length) && (cell_active[j] == 0.0))
          v[i][j] = HUGE_VAL;
        else
          v[i][j] = cl[j];
      }
      if (free_data)
        FreePIOData(*Pio_field);
    }
    else
    {
      for (uint32_t j = 0; j < i; ++j)
        v[j].resize(0);
      v.resize(0);
      if (FreePf)
        FreePIOData(*Pf);
      return false;
    }
  }
  if (FreePf)
    FreePIOData(*Pf);
  if (verbose)
    std::cerr << "Set double vector field " << fieldname << "\n";
  return true;
} // End PIO_DATA::set_vector_field

void PIO_DATA::print(const char* filename)
{
  std::ofstream out(filename);
  print(out);
  out.close();
}

void PIO_DATA::print(std::ostream& out)
{
  std::ios::fmtflags old_options = out.flags(std::ios::boolalpha);
  out.precision(16);
  out << "PIO DATA for PIO_DATA class " << this << '\n';
  out << "name = " << name << '\n'
      << "reverse_endian = " << reverse_endian << '\n'
      << "PIO_VERSION = " << PIO_VERSION << '\n'
      << "PIO_NAME_LENGTH = " << PIO_NAME_LENGTH << '\n'
      << "PIO_INDEX_LENGTH = " << PIO_INDEX_LENGTH << '\n'
      << "dandt (Date and Time) = " << pio_dandt << '\n'
      << "pio_num = " << pio_num << '\n'
      << "pio_position = " << pio_position << " bytes, " << pio_position / sizeof(double)
      << " doubles" << '\n'
      << "pio_signature = " << pio_signature << std::endl;
  for (int i = 0; i < pio_num; ++i)
  {
    out << "  pio_field[" << i << "].pio_name = " << pio_field[i].pio_name << '\n'
        << "  pio_field[" << i << "].index = " << pio_field[i].index << '\n'
        << "  pio_field[" << i << "].length = " << pio_field[i].length << '\n'
        << "  pio_field[" << i << "].position = " << pio_field[i].position << " bytes, "
        << pio_field[i].position / sizeof(double) << " doubles" << '\n'
        << "  pio_field[" << i << "].chksum = " << pio_field[i].chksum << std::endl;
  }
  for (int i = 0; i < pio_num; ++i)
  {
    if (pio_field[i].read_field_data && defer_read_data)
      ReadPioFieldData(pio_field[i]);
    if (pio_field[i].data != 0)
    {
      if (pio_field[i].length > 1)
      {
        out << "  Begin " << pio_field[i].pio_name << " floating point data\n";
        for (int64_t j = 0; j < pio_field[i].length; ++j)
        {
          out << "    " << pio_field[i].pio_name << "[" << j << "] = " << pio_field[i].data[j]
              << '\n';
        }
        out << "  End " << pio_field[i].pio_name << " data" << std::endl;
      }
      else
        out << "  " << pio_field[i].pio_name << " = " << pio_field[i].data[0] << std::endl;
      if (defer_read_data)
        FreePIOData(pio_field[i]);
    }
    if (pio_field[i].cdata != 0)
    {
      if (pio_field[i].length > 1)
      {
        out << "  Begin " << pio_field[i].pio_name << " character data\n";
        out << "    " << pio_field[i].pio_name << " = ";
        for (int64_t j = 0; j < pio_field[i].length; ++j)
        {
          char* c = pio_field[i].cdata + j * pio_field[i].cdata_len;
          std::string s;
          for (size_t ll = 0; ll < sizeof(double); ++ll)
          {
            if (c[ll] == 0)
              s += ' ';
            else
              s += c[ll];
          }

          out << s;

          // out<<"    "<<pio_field[i].pio_name<<"["<<j<<"] = "<<
          //     pio_field[i].cdata+j*pio_field[i].cdata_len<<'\n';
        }
        out << "\n  End " << pio_field[i].pio_name << " data" << std::endl;
      }
      else
        out << "  " << pio_field[i].pio_name << " = " << pio_field[i].cdata << std::endl;
      if (defer_read_data)
        FreePIOData(pio_field[i]);
    }
  }
  out << "END PIO DATA for PIO_DATA class " << this << std::endl;
  out.flags(old_options);
}

bool PIO_DATA::GetPIOfileTime(const char* piofile, double& time)
{
  time = -HUGE_VAL;
  delete this->Infile;
  this->Infile = new vtksys::ifstream(piofile, std::ios::binary);
  if (this->Infile->fail())
  {
    delete this->Infile;
    this->Infile = nullptr;
    return false;
  }

  double two;

  this->Infile->seekg(0, std::ios::beg);
  name = read_pio_char_string(8);
  if (strcmp(name, "pio_file") != 0)
  {
    delete this->Infile;
    this->Infile = nullptr;
    return false;
  }
  this->Infile->read((char*)&two, sizeof(two));
  reverse_endian = (two != 2.0) ? true : false;
  read_pio_word(PIO_VERSION);
  read_pio_word(PIO_NAME_LENGTH);
  read_pio_word(PIO_HEADER_LENGTH);
  read_pio_word(PIO_INDEX_LENGTH);
  pio_dandt = read_pio_char_string(16);
  read_pio_word(pio_num);
  pio_position = sizeof(double) * read_pio_word(pio_position);
  read_pio_word(pio_signature);
  if (pio_num <= 0)
  {
    pio_field = 0;
    delete this->Infile;
    this->Infile = nullptr;
    return false;
  }
  PIO_FIELD Pio_field;
  memset((void*)&Pio_field, 0, sizeof(Pio_field));
  this->Infile->seekg(pio_position, std::ios::beg);
  bool time_found = false;
  for (int i = 0; i < pio_num; ++i)
  {
    int64_t num_read = PIO_INDEX_LENGTH * sizeof(double);
    Pio_field.pio_name = read_pio_char_string(PIO_NAME_LENGTH);
    num_read -= PIO_NAME_LENGTH;
    read_pio_word(Pio_field.index);
    num_read -= sizeof(double);
    read_pio_word(Pio_field.length);
    num_read -= sizeof(double);
    Pio_field.position = sizeof(double) * read_pio_word(Pio_field.position);
    num_read -= sizeof(double);
    this->Infile->seekg(num_read, std::ios::cur);
    if (strcmp(Pio_field.pio_name, "controller_r8") == 0)
    {
      time_found = true;
      break;
    }
  }
  this->Infile->seekg(Pio_field.position, std::ios::beg);
  read_pio_word(time);
  delete this->Infile;
  this->Infile = nullptr;
  return time_found;
} // End PIO_DATA::GetPIOfileTime(const char *piofile,double &time)

bool GetPIOfileTime(const char* piofile, double& time)
{
  PIO_DATA PioData;
  return PioData.GetPIOfileTime(piofile, time);
} // End GetPIOfileTime

bool IsPIOfile(const char* piofile)
{
  char name[9];
  vtksys::ifstream file(piofile, std::ios::binary);
  if (!file)
    return false;

  file.seekg(0, std::ios::beg);
  file.read(name, 8);
  name[8] = '\0';
  file.close();
  return (strcmp(name, "pio_file") == 0) ? true : false;
} // End IsPIOfile

void PIO_DATA::GetPIOData(PIO_FIELD& _pio_field, const double*& _data, const char*& _cdata)
{
  _data = 0;
  _cdata = 0;
  if (!_pio_field.read_field_data)
    return;
  // Check if data is already read
  if (_pio_field.data != 0)
  {
    _data = _pio_field.data;
    return;
  }
  if (_pio_field.cdata != 0)
  {
    _cdata = _pio_field.cdata;
    return;
  }
  ReadPioFieldData(_pio_field);
  _data = _pio_field.data;
  _cdata = _pio_field.cdata;
}

void PIO_DATA::GetPIOData(PIO_FIELD& _pio_field, const double*& _data)
{
  _data = 0;
  if (!_pio_field.read_field_data)
    return;
  // Check if data is already read
  if (_pio_field.data != 0)
  {
    _data = _pio_field.data;
    return;
  }
  ReadPioFieldData(_pio_field);
  _data = _pio_field.data;
  if (_data == 0)
    FreePIOData(_pio_field);
}

void PIO_DATA::GetPIOData(PIO_FIELD& _pio_field, const char*& _cdata)
{
  _cdata = 0;
  if (!_pio_field.read_field_data)
    return;
  // Check if data is already read
  if (_pio_field.cdata != 0)
  {
    _cdata = _pio_field.cdata;
    return;
  }
  ReadPioFieldData(_pio_field);
  _cdata = _pio_field.cdata;
  if (_cdata == 0)
    FreePIOData(_pio_field);
}

const double* PIO_DATA::GetPIOData(PIO_FIELD& _pio_field)
{
  if (!_pio_field.read_field_data)
    return 0;
  // Check if data is already read
  if (_pio_field.data == 0)
  {
    ReadPioFieldData(_pio_field);
    if (_pio_field.data == 0)
      FreePIOData(_pio_field);
  }
  return _pio_field.data;
}

void PIO_DATA::GetPIOData(const char* _name, const double*& _data, const char*& _cdata)
{
  _data = 0;
  _cdata = 0;
  if ((_name != 0) && (VarMMap.find(_name) != VarMMap.end()))
  {
    PIO_FIELD* Pio_field = VarMMap.equal_range(_name).first->second;
    GetPIOData(*Pio_field, _data, _cdata);
  }
}

void PIO_DATA::GetPIOData(const char* _name, const double*& _data)
{
  _data = 0;
  if ((_name != 0) && (VarMMap.find(_name) != VarMMap.end()))
  {
    PIO_FIELD* Pio_field = VarMMap.equal_range(_name).first->second;
    _data = GetPIOData(*Pio_field);
  }
}

double PIO_DATA::GetPIOData(const char* _name, int index)
{
  if ((_name != 0) && (VarMMap.find(_name) != VarMMap.end()))
  {
    PIO_FIELD* Pio_field = VarMMap.equal_range(_name).first->second;
    const double* data = GetPIOData(*Pio_field);
    return data[index];
  }
  return -HUGE_VAL;
}

void PIO_DATA::GetPIOData(const char* _name, const char*& _cdata)
{
  _cdata = 0;
  if ((_name != 0) && (VarMMap.find(_name) != VarMMap.end()))
  {
    PIO_FIELD* Pio_field = VarMMap.equal_range(_name).first->second;
    GetPIOData(*Pio_field, _cdata);
  }
}

const double* PIO_DATA::GetPIOData(const char* _name)
{
  if ((_name != 0) && (VarMMap.find(_name) != VarMMap.end()))
  {
    PIO_FIELD* Pio_field = VarMMap.equal_range(_name).first->second;
    return GetPIOData(*Pio_field);
  }
  return 0;
}

int PIO_DATA::get_pio_num_with_size(int64_t n) const
{
  int num = 0;
  if (n == 0)
    num = pio_num;
  else
  {
    for (int i = 0; i < pio_num; ++i)
    {
      if (pio_field[i].data && pio_field[i].length == n)
        ++num;
    }
  }
  return num;
}

void PIO_DATA::FreePIOData(PIO_FIELD& _pio_field)
{
  if (_pio_field.data != 0)
    delete[] _pio_field.data;
  if (_pio_field.cdata != 0)
    delete[] _pio_field.cdata;
  _pio_field.cdata_len = 0;
  _pio_field.data = 0;
  _pio_field.cdata = 0;
}

void PIO_DATA::ReadPioFieldData(PIO_FIELD& _pio_field)
{
  if (_pio_field.data != 0 || _pio_field.cdata != 0)
    return; // Data already read
  this->Infile->seekg(_pio_field.position, std::ios::beg);
  size_t slen = sizeof(double);
  if (_pio_field.data != 0)
    delete[] _pio_field.data;
  _pio_field.data = new double[_pio_field.length];
  bool char_data = true;
  for (int64_t j = 0; j < _pio_field.length; ++j)
  {
    read_pio_word(_pio_field.data[j]);
    if (char_data && !is_a_string((char*)(_pio_field.data + j), slen))
      char_data = false;
  }
  if (char_data)
  {
    char_data = false;
    for (int64_t j = 0; j < _pio_field.length; ++j)
    {
      if (_pio_field.data[j] != 0.0)
      {
        char_data = true;
        break;
      }
    }
  }
  if (RealData.find(_pio_field.pio_name) != RealData.end())
    char_data = false;
  else if (CharData.find(_pio_field.pio_name) != CharData.end())
    char_data = true;
  if (char_data)
  {
    _pio_field.cdata_len = slen + 1;
    if (_pio_field.cdata != 0)
      delete[] _pio_field.cdata;
    _pio_field.cdata = new char[_pio_field.length * _pio_field.cdata_len];
    for (int64_t j = 0; j < _pio_field.length; ++j)
    {
      char* c = (char*)(_pio_field.data + j);
      char* cc = _pio_field.cdata + j * _pio_field.cdata_len;
      if (reverse_endian)
      {
        for (size_t k = 0; k < slen; ++k)
          cc[k] = c[slen - 1 - k];
      }
      else
      {
        for (size_t k = 0; k < slen; ++k)
          cc[k] = c[k];
      }
      cc[slen] = '\0';
      fstr2Cstr(cc, slen);
    }
    delete[] _pio_field.data;
    _pio_field.data = 0;
    if ((strcmp(_pio_field.pio_name, "hist_dandt") == 0) ||
      (strcmp(_pio_field.pio_name, "hist_prbnm") == 0))
    { // These are 16 char long strings
      size_t cnew_len = 2 * slen + 1;
      char* cnew = new char[(_pio_field.length / 2) * cnew_len];
      for (int64_t j = 0; j < _pio_field.length; j += 2)
      {
        char* c1 = _pio_field.cdata + j * _pio_field.cdata_len;
        char* c2 = _pio_field.cdata + (j + 1) * _pio_field.cdata_len;
        char* cc = cnew + (j / 2) * cnew_len;
        strcpy(cc, c1);
        strcat(cc, c2);
        fstr2Cstr(cc, cnew_len - 1);
      }
      _pio_field.length /= 2;
      delete[] _pio_field.cdata;
      _pio_field.cdata = cnew;
      _pio_field.cdata_len = cnew_len;
    }
    if ((strcmp(_pio_field.pio_name, "matident") == 0))
    {
      if (matident_len != sizeof(double))
      {
        this->Infile->seekg(_pio_field.position, std::ios::beg);
        if (_pio_field.cdata)
          delete[] _pio_field.cdata;
        _pio_field.cdata_len = matident_len + 1;
        _pio_field.length = _pio_field.length * sizeof(double) / matident_len;
        _pio_field.cdata = new char[_pio_field.length * _pio_field.cdata_len];
        for (int64_t j = 0; j < _pio_field.length; ++j)
        {
          this->Infile->read(_pio_field.cdata + j * _pio_field.cdata_len, matident_len);
          fstr2Cstr(_pio_field.cdata + j * _pio_field.cdata_len, matident_len);
        }
      }
    }
    if ((strcmp(_pio_field.pio_name, "timertype") == 0))
    {
      if (timertype_len != 2 * sizeof(double))
      {
        this->Infile->seekg(_pio_field.position, std::ios::beg);
        if (_pio_field.cdata)
          delete[] _pio_field.cdata;
        _pio_field.cdata_len = timertype_len + 1;
        _pio_field.length = _pio_field.length * 2 * sizeof(double) / timertype_len;
        _pio_field.cdata = new char[_pio_field.length * _pio_field.cdata_len];
        for (int64_t j = 0; j < _pio_field.length; ++j)
        {
          this->Infile->read(_pio_field.cdata + j * _pio_field.cdata_len, timertype_len);
          fstr2Cstr(_pio_field.cdata + j * _pio_field.cdata_len, timertype_len);
        }
      }
    }
  }
}
