#if !defined(_PIODATA_H)
#define _PIODATA_H

#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string.h>
#include <string>
#include <valarray>

struct Cstring_less
{
  bool operator()(const char* p, const char* q) const { return strcmp(p, q) < 0; };
};

// Class Declarations

class PIO_DATA;
class PIO_FIELD;

class PIO_FIELD
{
public:
  char* pio_name;
  int index;
  int64_t length;
  int64_t position;
  int64_t chksum;
  size_t cdata_len;
  bool read_field_data;
  friend class PIO_DATA;

protected:
  double* data;
  char* cdata;
}; // End class PIO_FIELD

// Typedefs for the mapping between the names of the PIO blocks in the PIO file and
// the PIO_FIELD's used to store the data values in the PIO blocks.
typedef std::multimap<const char*, PIO_FIELD*, Cstring_less> VAR_MAP;
typedef VAR_MAP::iterator VMI;
typedef VAR_MAP::const_iterator CVMI;
typedef std::pair<VMI, VMI> VMP;
typedef std::pair<CVMI, CVMI> CVMP;

class PIO_DATA
{
public:
  PIO_DATA(const char* piofile = 0, const std::list<std::string>* fields_to_read = 0,
    bool _defer_read_data = false, const std::set<const char*, Cstring_less>* rdata = 0,
    const std::set<const char*, Cstring_less>* cdata = 0);
  ~PIO_DATA();
  bool GetPIOfileTime(const char*, double&);
  void print(std::ostream&);
  void print(const char*);
  bool set_scalar_field(std::valarray<int>&, const char*);
  bool set_scalar_field(std::valarray<int64_t>&, const char*);
  bool set_scalar_field(std::valarray<uint64_t>&, const char*);
  bool set_scalar_field(std::valarray<double>&, const char*);
  bool set_vector_field(std::valarray<std::valarray<double> >&, const char*);
  inline bool good_read() { return (pio_field != 0) ? true : false; }
  VAR_MAP VarMMap; // Multimap from pio_name to a PIO_FIELD class
  const char* get_name() const { return name; }
  bool get_reverse_endian() const { return reverse_endian; }
  int get_PIO_VERSION() const { return PIO_VERSION; }
  int get_PIO_NAME_LENGTH() const { return PIO_NAME_LENGTH; }
  int get_PIO_HEADER_LENGTH() const { return PIO_HEADER_LENGTH; }
  int get_PIO_INDEX_LENGTH() const { return PIO_INDEX_LENGTH; }
  const char* get_pio_dandt() const { return pio_dandt; }
  int get_pio_num() const { return pio_num; }
  int get_pio_num_with_size(int64_t n) const;
  int get_pio_signature() const { return pio_signature; }
  PIO_FIELD* get_pio_field() const { return pio_field; }
  void GetPIOData(PIO_FIELD&, const double*&, const char*&);
  void GetPIOData(PIO_FIELD&, const double*&);
  void GetPIOData(PIO_FIELD&, const char*&);
  const double* GetPIOData(PIO_FIELD&);
  void GetPIOData(const char*, const double*&, const char*&);
  void GetPIOData(const char*, const double*&);
  void GetPIOData(const char*, const char*&);
  const double* GetPIOData(const char*);
  double GetPIOData(const char*, int);
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
  bool read(const char*, const std::list<std::string>* fields_to_read = 0);
  bool read(const std::list<std::string>* fields_to_read = 0);
  inline void byte_flip(char* word, int64_t size)
  {
    if (size_buf < (size_t)size)
    {
      if (buf)
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
      if (buf)
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
    if (fields_to_read == 0)
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
}; // End class PIO_DATA

// Locations of various data items from the input arrays, amhc_i, amhc_r8,
// amch_l, and controller_r8
enum
{
  Ntime = 0,    // time = controller_r8[Ntime];
  Nnumdim = 42, // numdim = amhc_i[Nnumdim]
  Nmesh0 = 16,  // N[0] = amhc_i[Nmesh0]
  Nmesh1 = 17,  // N[1] = amhc_i[Nmesh1]
  Nmesh2 = 29,  // N[2] = amhc_i[Nmesh2]
  Nd0 = 21,     // d[0] = amhc_r8[Nd0]
  Nd1 = 22,     // d[1] = amhc_r8[Nd1]
  Nd2 = 38,     // d[2] = amhc_r8[Nd2]
  NZero0 = 19,  // Zero[0] = amhc_r8[NZero0]
  NZero1 = 20,  // Zero[1] = amhc_r8[NZero1]
  NZero2 = 35,  // Zero[2] = amhc_r8[NZero2]
  Ncylin = 1,   // cylindrically (axisymmetric) symmetric
                // geometry if amhc_l[Ncylin]!=0
  Nsphere = 8   // spherically symmetirc geometry if
                // amhc_l[Nsphere]!=0
};

// Prototypes
bool GetPIOfileTime(const char*, double&);
bool IsPIOfile(const char*);
#endif //! defined(_PIODATA_H)
