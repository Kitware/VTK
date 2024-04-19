// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2021, Triad National Security, LLC
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-LANL-Triad-USGov
/**
 *
 * @class PIO_DATA_PIO
 * @brief   class for reading PIO (Parallel Input Output) data files
 *
 * This class reads in dump files generated from xRage, a LANL physics code.
 * The PIO (Parallel Input Output) library is used to create the dump files.
 *
 * @par Thanks:
 * Developed by Patricia Fasel at Los Alamos National Laboratory
 */

#if !defined(_PIODATAPIO_H)
#define _PIODATAPIO_H

#include <PIOData.h>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string.h>
#include <string>
#include <valarray>

VTK_ABI_NAMESPACE_BEGIN
// Class Declarations
class PIO_DATA;
class PIO_DATA_PIO;
class PIO_FIELD;

class PIO_DATA_PIO : public PIO_DATA
{
public:
  PIO_DATA_PIO(const char* piofile = nullptr,
    const std::list<std::string>* fields_to_read = nullptr, bool _defer_read_data = true,
    const std::set<const char*, Cstring_less>* rdata = nullptr,
    const std::set<const char*, Cstring_less>* cdata = nullptr);
  ~PIO_DATA_PIO() override;
  bool GetPIOfileTime(const char*, double&);
  void print(std::ostream&);
  void print(const char*);
  bool set_scalar_field(std::valarray<int>&, const char*) override;
  bool set_scalar_field(std::valarray<int64_t>&, const char*) override;
  bool set_scalar_field(std::valarray<double>&, const char*) override;
  bool set_vector_field(std::valarray<std::valarray<double>>&, const char*) override;
  inline bool good_read() override { return (pio_field != nullptr) ? true : false; }
  const char* get_name() const { return name; }
  bool get_reverse_endian() const { return reverse_endian; }
  int get_PIO_VERSION() const { return PIO_VERSION; }
  int get_PIO_NAME_LENGTH() const { return PIO_NAME_LENGTH; }
  int get_PIO_HEADER_LENGTH() const { return PIO_HEADER_LENGTH; }
  int get_PIO_INDEX_LENGTH() const { return PIO_INDEX_LENGTH; }
  const char* get_pio_dandt() const { return pio_dandt; }
  int get_pio_num() const override;
  int get_pio_num_with_size(int64_t n) const;
  int get_pio_signature() const { return pio_signature; }
  int get_cycle() override;
  double get_simtime() override;
  int get_dimension() override;
  bool get_gridsize(std::valarray<int>&) override;
  bool get_gridscale(std::valarray<double>&) override;
  bool get_gridorigin(std::valarray<double>&) override;
  std::string get_eap_version() override;
  std::string get_username() override;
  std::string get_problemname() override;
  bool get_material_names(std::valarray<std::string>&) override;
  bool get_tracer_variable_names(std::valarray<std::string>&) override;

  PIO_FIELD* get_pio_field() const override;
  void GetPIOData(PIO_FIELD&, const double*&, const char*&);
  void GetPIOData(PIO_FIELD&, const double*&);
  void GetPIOData(PIO_FIELD&, const char*&);
  const double* GetPIOData(PIO_FIELD&);
  void GetPIOData(const char*, const double*&, const char*&);
  void GetPIOData(const char*, const double*&);
  void GetPIOData(const char*, const char*&);
  const double* GetPIOData(const char*);
  double GetPIOData(const char*, int);
  bool reconstruct_chunk_field(int64_t numcell, std::valarray<double>& va, const char* prefix,
    const char* var, int materialId) override;
  int get_num_components(const char*) const override;
  int get_num_materials() const override;
  int64_t get_num_cells() override;
  bool has_field(const char*) override; // true if field exists

  void AddRealData(const char* _name)
  {
    if (RealData.find(_name) == RealData.end())
      RealData.insert(strdup(_name));
  }
  void AddCharData(const char* _name)
  {
    if (CharData.find(_name) == CharData.end())
      CharData.insert(strdup(_name));
  }
  void FreePIOData(PIO_FIELD& pio_field);
  bool verbose;

private:
  std::set<const char*, Cstring_less> RealData;
  std::set<const char*, Cstring_less> CharData;
  const char* name;
  std::istream* Infile;
  bool reverse_endian;
  int PIO_VERSION;
  int PIO_NAME_LENGTH;
  int PIO_HEADER_LENGTH;
  int PIO_INDEX_LENGTH;
  const char* pio_dandt; // Date and Time
  int pio_num;
  int64_t pio_position;
  int pio_signature;
  PIO_FIELD* pio_field;
  bool defer_read_data;
  size_t matident_len;
  size_t timertype_len;

  char* buf;
  size_t size_buf;
  void ReadPioFieldData(PIO_FIELD& pio_field);
  bool read(const char*, const std::list<std::string>* fields_to_read = nullptr);
  bool read(const std::list<std::string>* fields_to_read = nullptr);
  inline void byte_flip(char* word, int64_t size)
  {
    if (size_buf < (size_t)size)
    {
      delete[] buf;
      size_buf = size;
      buf = new char[size_buf];
    }
    memcpy((void*)buf, (const void*)word, size);
    for (int64_t i = 0; i < size; ++i)
      word[i] = buf[size - 1 - i];
  } // End byte_flip

  template <class T>
  inline T read_pio_word(T& val)
  {
    double word;
    this->Infile->read((char*)&word, sizeof(word));
    if (reverse_endian)
      byte_flip((char*)&word, sizeof(word));
    val = T(word);
    return val;
  } // End read_pio_word

  inline bool read_pio_bool()
  {
    double word;
    this->Infile->read((char*)&word, sizeof(word));
    if (reverse_endian)
      byte_flip((char*)&word, sizeof(word));
    return (word != 0) ? true : false;
  } // End read_pio_bool

  inline void fstr2Cstr(char* s, size_t len) const
  {
    s[len] = '\0';
    size_t i = len - 1;
    do
    {
      if (s[i--] == ' ')
        s[i + 1] = '\0';
    } while (i != 0);
  } // End fstr2Cstr

  inline char* read_pio_char_string(size_t len)
  {
    if (size_buf <= len)
    {
      delete[] buf;
      size_buf = len + 1;
      buf = new char[size_buf];
    }
    this->Infile->read(buf, len);
    buf[len] = '\0';
    fstr2Cstr(buf, len);
    char* val = new char[strlen(buf) + 1];
    strcpy(val, buf);
    return val;
  } // End read_pio_char_string

  inline void insert_VAR_MAP_pairs()
  {
    for (int i = 0; i < pio_num; ++i)
    {
      if (pio_field[i].read_field_data)
      {
#if !defined __SUNPRO_CC
        VarMMap.insert(std::make_pair(pio_field[i].pio_name, pio_field + i));
#else  //! defined __SUNPRO_CC
        VAR_MAP::value_type type(pio_field[i].pio_name, pio_field + i);
        VarMMap.insert(type);
#endif //! defined __SUNPRO_CC
      }
    }
  } // End insert_VAR_MAP_pairs

  inline bool read_field(const char* pio_name, const std::list<std::string>* fields_to_read)
  {
    std::string spio_name = std::string(pio_name);
    if (fields_to_read == nullptr)
      return true;
    else
    {
      for (std::list<std::string>::const_iterator pos = fields_to_read->begin();
           pos != fields_to_read->end(); ++pos)
      {
        if (spio_name == *pos)
          return true;
      }
    }
    return false;
  }
}; // End class PIO_DATA_PIO

// Prototypes
bool GetPIOfileTime(const char*, double&);
bool IsPIOfile(const char*);
VTK_ABI_NAMESPACE_END
#endif //! defined(_PIODATAPIO_H)
