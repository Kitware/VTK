#ifndef ZFP_ARRAY2_H
#define ZFP_ARRAY2_H

#include <cstddef>
#include <iterator>
#include <cstring>
#include "zfparray.h"
#include "zfpcodec.h"
#include "zfp/cache.h"

namespace zfp {

// compressed 2D array of scalars
template < typename Scalar, class Codec = zfp::codec<Scalar> >
class array2 : public array {
public:
  // forward declarations
  class reference;
  class pointer;
  class iterator;
  class view;
  #include "zfp/reference2.h"
  #include "zfp/pointer2.h"
  #include "zfp/iterator2.h"
  #include "zfp/view2.h"

  // default constructor
  array2() : array(2, Codec::type) {}

  // constructor of nx * ny array using rate bits per value, at least
  // csize bytes of cache, and optionally initialized from flat array p
  array2(uint nx, uint ny, double rate, const Scalar* p = 0, size_t csize = 0) :
    array(2, Codec::type),
    cache(lines(csize, nx, ny))
  {
    set_rate(rate);
    resize(nx, ny, p == 0);
    if (p)
      set(p);
  }

  // constructor, from previously-serialized compressed array
  array2(const zfp::array::header& h, const uchar* buffer = 0, size_t buffer_size_bytes = 0) :
    array(2, Codec::type, h, buffer_size_bytes)
  {
    resize(nx, ny, false);
    if (buffer)
      memcpy(data, buffer, bytes);
  }

  // copy constructor--performs a deep copy
  array2(const array2& a) :
    array()
  {
    deep_copy(a);
  }

  // construction from view--perform deep copy of (sub)array
  template <class View>
  array2(const View& v) :
    array(2, Codec::type),
    cache(lines(0, v.size_x(), v.size_y()))
  {
    set_rate(v.rate());
    resize(v.size_x(), v.size_y(), true);
    // initialize array in its preferred order
    for (iterator it = begin(); it != end(); ++it)
      *it = v(it.i(), it.j());
  }

  // virtual destructor
  virtual ~array2() {}

  // assignment operator--performs a deep copy
  array2& operator=(const array2& a)
  {
    if (this != &a)
      deep_copy(a);
    return *this;
  }

  // total number of elements in array
  size_t size() const { return size_t(nx) * size_t(ny); }

  // array dimensions
  uint size_x() const { return nx; }
  uint size_y() const { return ny; }

  // resize the array (all previously stored data will be lost)
  void resize(uint nx, uint ny, bool clear = true)
  {
    if (nx == 0 || ny == 0)
      free();
    else {
      this->nx = nx;
      this->ny = ny;
      bx = (nx + 3) / 4;
      by = (ny + 3) / 4;
      blocks = bx * by;
      alloc(clear);

      // precompute block dimensions
      zfp::deallocate(shape);
      if ((nx | ny) & 3u) {
        shape = (uchar*)zfp::allocate(blocks);
        uchar* p = shape;
        for (uint j = 0; j < by; j++)
          for (uint i = 0; i < bx; i++)
            *p++ = (i == bx - 1 ? -nx & 3u : 0) + 4 * (j == by - 1 ? -ny & 3u : 0);
      }
      else
        shape = 0;
    }
  }

  // cache size in number of bytes
  size_t cache_size() const { return cache.size() * sizeof(CacheLine); }

  // set minimum cache size in bytes (array dimensions must be known)
  void set_cache_size(size_t csize)
  {
    flush_cache();
    cache.resize(lines(csize, nx, ny));
  }

  // empty cache without compressing modified cached blocks
  void clear_cache() const { cache.clear(); }

  // flush cache by compressing all modified cached blocks
  void flush_cache() const
  {
    for (typename zfp::Cache<CacheLine>::const_iterator p = cache.first(); p; p++) {
      if (p->tag.dirty()) {
        uint b = p->tag.index() - 1;
        encode(b, p->line->data());
      }
      cache.flush(p->line);
    }
  }

  // decompress array and store at p
  void get(Scalar* p) const
  {
    uint b = 0;
    for (uint j = 0; j < by; j++, p += 4 * (nx - bx))
      for (uint i = 0; i < bx; i++, p += 4, b++) {
        const CacheLine* line = cache.lookup(b + 1);
        if (line)
          line->get(p, 1, nx, shape ? shape[b] : 0);
        else
          decode(b, p, 1, nx);
      }
  }

  // initialize array by copying and compressing data stored at p
  void set(const Scalar* p)
  {
    uint b = 0;
    for (uint j = 0; j < by; j++, p += 4 * (nx - bx))
      for (uint i = 0; i < bx; i++, p += 4, b++)
        encode(b, p, 1, nx);
    cache.clear();
  }

  // (i, j) accessors
  Scalar operator()(uint i, uint j) const { return get(i, j); }
  reference operator()(uint i, uint j) { return reference(this, i, j); }

  // flat index accessors
  Scalar operator[](uint index) const
  {
    uint i, j;
    ij(i, j, index);
    return get(i, j);
  }
  reference operator[](uint index)
  {
    uint i, j;
    ij(i, j, index);
    return reference(this, i, j);
  }

  // sequential iterators
  iterator begin() { return iterator(this, 0, 0); }
  iterator end() { return iterator(this, 0, ny); }

protected:
  // cache line representing one block of decompressed values
  class CacheLine {
  public:
    Scalar operator()(uint i, uint j) const { return a[index(i, j)]; }
    Scalar& operator()(uint i, uint j) { return a[index(i, j)]; }
    const Scalar* data() const { return a; }
    Scalar* data() { return a; }
    // copy cache line
    void get(Scalar* p, int sx, int sy) const
    {
      const Scalar* q = a;
      for (uint y = 0; y < 4; y++, p += sy - 4 * sx)
        for (uint x = 0; x < 4; x++, p += sx, q++)
          *p = *q;
    }
    void get(Scalar* p, int sx, int sy, uint shape) const
    {
      if (!shape)
        get(p, sx, sy);
      else {
        // determine block dimensions
        uint nx = 4 - (shape & 3u); shape >>= 2;
        uint ny = 4 - (shape & 3u); shape >>= 2;
        const Scalar* q = a;
        for (uint y = 0; y < ny; y++, p += sy - (ptrdiff_t)nx * sx, q += 4 - nx)
          for (uint x = 0; x < nx; x++, p += sx, q++)
            *p = *q;
      }
    }
  protected:
    static uint index(uint i, uint j) { return (i & 3u) + 4 * (j & 3u); }
    Scalar a[16];
  };

  // perform a deep copy
  void deep_copy(const array2& a)
  {
    // copy base class members
    array::deep_copy(a);
    // copy cache
    cache = a.cache;
  }

  // inspector
  Scalar get(uint i, uint j) const
  {
    const CacheLine* p = line(i, j, false);
    return (*p)(i, j);
  }

  // mutator
  void set(uint i, uint j, Scalar val)
  {
    CacheLine* p = line(i, j, true);
    (*p)(i, j) = val;
  }

  // in-place updates
  void add(uint i, uint j, Scalar val) { (*line(i, j, true))(i, j) += val; }
  void sub(uint i, uint j, Scalar val) { (*line(i, j, true))(i, j) -= val; }
  void mul(uint i, uint j, Scalar val) { (*line(i, j, true))(i, j) *= val; }
  void div(uint i, uint j, Scalar val) { (*line(i, j, true))(i, j) /= val; }

  // return cache line for (i, j); may require write-back and fetch
  CacheLine* line(uint i, uint j, bool write) const
  {
    CacheLine* p = 0;
    uint b = block(i, j);
    typename zfp::Cache<CacheLine>::Tag t = cache.access(p, b + 1, write);
    uint c = t.index() - 1;
    if (c != b) {
      // write back occupied cache line if it is dirty
      if (t.dirty())
        encode(c, p->data());
      // fetch cache line
      decode(b, p->data());
    }
    return p;
  }

  // encode block with given index
  void encode(uint index, const Scalar* block) const
  {
    stream_wseek(zfp->stream, index * blkbits);
    Codec::encode_block_2(zfp, block, shape ? shape[index] : 0);
    stream_flush(zfp->stream);
  }

  // encode block with given index from strided array
  void encode(uint index, const Scalar* p, int sx, int sy) const
  {
    stream_wseek(zfp->stream, index * blkbits);
    Codec::encode_block_strided_2(zfp, p, shape ? shape[index] : 0, sx, sy);
    stream_flush(zfp->stream);
  }

  // decode block with given index
  void decode(uint index, Scalar* block) const
  {
    stream_rseek(zfp->stream, index * blkbits);
    Codec::decode_block_2(zfp, block, shape ? shape[index] : 0);
  }

  // decode block with given index to strided array
  void decode(uint index, Scalar* p, int sx, int sy) const
  {
    stream_rseek(zfp->stream, index * blkbits);
    Codec::decode_block_strided_2(zfp, p, shape ? shape[index] : 0, sx, sy);
  }

  // block index for (i, j)
  uint block(uint i, uint j) const { return (i / 4) + bx * (j / 4); }

  // convert flat index to (i, j)
  void ij(uint& i, uint& j, uint index) const
  {
    i = index % nx;
    index /= nx;
    j = index;
  }

  // number of cache lines corresponding to size (or suggested size if zero)
  static uint lines(size_t size, uint nx, uint ny)
  {
    uint n = size ? uint((size + sizeof(CacheLine) - 1) / sizeof(CacheLine)) : array::lines(size_t((nx + 3) / 4) * size_t((ny + 3) / 4));
    return std::max(n, 1u);
  }

  mutable zfp::Cache<CacheLine> cache; // cache of decompressed blocks
};

typedef array2<float> array2f;
typedef array2<double> array2d;

}

#endif
