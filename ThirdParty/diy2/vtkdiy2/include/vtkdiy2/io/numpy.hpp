#ifndef DIY_IO_NMPY_HPP
#define DIY_IO_NMPY_HPP

#include <sstream>
#include <complex>
#include <stdexcept>

#include "../serialization.hpp"
#include "bov.hpp"

namespace diy
{
namespace io
{
  class NumPy: public BOV
  {
    public:
                        NumPy(mpi::io::file& f):
                          BOV(f)                                {}

      unsigned          word_size() const                       { return word_size_; }

      unsigned          read_header()
      {
        BOV::Shape  shape;
        bool        fortran;
        size_t      offset = parse_npy_header(shape, fortran);
        if (fortran)
            throw std::runtime_error("diy::io::NumPy cannot read data in fortran order");
        BOV::set_offset(offset);
        BOV::set_shape(shape);
        return word_size_;
      }

      template<class T>
      void              write_header(int dim, const DiscreteBounds& bounds);

      template<class T, class S>
      void              write_header(const S& shape);

    private:
      inline size_t     parse_npy_header(BOV::Shape& shape, bool& fortran_order);
      void              save(diy::BinaryBuffer& bb, const std::string& s)               { bb.save_binary(s.c_str(), s.size()); }
      template<class T>
      inline void       convert_and_save(diy::BinaryBuffer& bb, const T& x)
      {
          std::ostringstream oss;
          oss << x;
          save(bb, oss.str());
      }

    private:
      unsigned          word_size_;
  };

  namespace detail
  {
    inline char big_endian();
    template<class T>
    char map_numpy_type();
  }
}
}

// Modified from: https://github.com/rogersce/cnpy
// Copyright (C) 2011  Carl Rogers
// Released under MIT License
// license available at http://www.opensource.org/licenses/mit-license.php
size_t
diy::io::NumPy::
parse_npy_header(BOV::Shape& shape, bool& fortran_order)
{
    char buffer[256];
    file().read_at_all(0, buffer, 256);
    std::string header(buffer, buffer + 256);
    size_t nl = header.find('\n');
    if (nl == std::string::npos)
        throw std::runtime_error("parse_npy_header: failed to read the header");
    header = header.substr(11, nl - 11 + 1);
    size_t header_size = nl + 1;

    int loc1, loc2;

    //fortran order
    loc1 = header.find("fortran_order")+16;
    fortran_order = (header.substr(loc1,4) == "True" ? true : false);

    //shape
    unsigned ndims;
    loc1 = header.find("(");
    loc2 = header.find(")");
    std::string str_shape = header.substr(loc1+1,loc2-loc1-1);
    if(str_shape[str_shape.size()-1] == ',') ndims = 1;
    else ndims = std::count(str_shape.begin(),str_shape.end(),',')+1;
    shape.resize(ndims);
    for(unsigned int i = 0;i < ndims;i++) {
        loc1 = str_shape.find(",");
        shape[i] = atoi(str_shape.substr(0,loc1).c_str());
        str_shape = str_shape.substr(loc1+1);
    }

    //endian, word size, data type
    //byte order code | stands for not applicable.
    //not sure when this applies except for byte array
    loc1 = header.find("descr")+9;
    //bool littleEndian = (header[loc1] == '<' || header[loc1] == '|' ? true : false);
    //assert(littleEndian);

    //char type = header[loc1+1];
    //assert(type == map_type(T));

    std::string str_ws = header.substr(loc1+2);
    loc2 = str_ws.find("'");
    word_size_ = atoi(str_ws.substr(0,loc2).c_str());

    return header_size;
}

template<class T>
void
diy::io::NumPy::
write_header(int dim, const DiscreteBounds& bounds)
{
    std::vector<int> shape;
    for (int i = 0; i < dim; ++i)
        shape.push_back(bounds.max[i] - bounds.min[i] + 1);

    write_header< T, std::vector<int> >(shape);
}


template<class T, class S>
void
diy::io::NumPy::
write_header(const S& shape)
{
    BOV::set_shape(shape);

    diy::MemoryBuffer dict;
    save(dict, "{'descr': '");
    diy::save(dict, detail::big_endian());
    diy::save(dict, detail::map_numpy_type<T>());
    convert_and_save(dict, sizeof(T));
    save(dict, "', 'fortran_order': False, 'shape': (");
    convert_and_save(dict, shape[0]);
    for (int i = 1; i < (int) shape.size(); i++)
    {
        save(dict, ", ");
        convert_and_save(dict, shape[i]);
    }
    if(shape.size() == 1) save(dict, ",");
    save(dict, "), }");
    //pad with spaces so that preamble+dict is modulo 16 bytes. preamble is 10 bytes. dict needs to end with \n
    int remainder = 16 - (10 + dict.position) % 16;
    for (int i = 0; i < remainder - 1; ++i)
        diy::save(dict, ' ');
    diy::save(dict, '\n');

    diy::MemoryBuffer header;
    diy::save(header, (char) 0x93);
    save(header, "NUMPY");
    diy::save(header, (char) 0x01);  // major version of numpy format
    diy::save(header, (char) 0x00);  // minor version of numpy format
    diy::save(header, (unsigned short) dict.position);
    header.save_binary(&dict.buffer[0], dict.buffer.size());

    BOV::set_offset(header.position);

    if (file().comm().rank() == 0)
        file().write_at(0, &header.buffer[0], header.buffer.size());
}

char
diy::io::detail::big_endian()
{
  unsigned char x[] = {1,0};
  void* x_void = x;
  short y = *static_cast<short*>(x_void);
  return y == 1 ? '<' : '>';
}

namespace diy
{
namespace io
{
namespace detail
{
template<> inline char map_numpy_type<float>()                         { return 'f'; }
template<> inline char map_numpy_type<double>()                        { return 'f'; }
template<> inline char map_numpy_type<long double>()                   { return 'f'; }

template<> inline char map_numpy_type<int>()                           { return 'i'; }
template<> inline char map_numpy_type<char>()                          { return 'i'; }
template<> inline char map_numpy_type<short>()                         { return 'i'; }
template<> inline char map_numpy_type<long>()                          { return 'i'; }
template<> inline char map_numpy_type<long long>()                     { return 'i'; }

template<> inline char map_numpy_type<unsigned int>()                  { return 'u'; }
template<> inline char map_numpy_type<unsigned char>()                 { return 'u'; }
template<> inline char map_numpy_type<unsigned short>()                { return 'u'; }
template<> inline char map_numpy_type<unsigned long>()                 { return 'u'; }
template<> inline char map_numpy_type<unsigned long long>()            { return 'u'; }

template<> inline char map_numpy_type<bool>()                          { return 'b'; }

template<> inline char map_numpy_type< std::complex<float> >()         { return 'c'; }
template<> inline char map_numpy_type< std::complex<double> >()        { return 'c'; }
template<> inline char map_numpy_type< std::complex<long double> >()   { return 'c'; }
}
}
}

#endif
