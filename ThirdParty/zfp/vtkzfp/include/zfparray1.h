#ifndef ZFP_ARRAY1_H
#define ZFP_ARRAY1_H

#include <cstddef>
#include <iterator>
#include <cstring>
#include "zfparray.h"
#include "zfpcodec.h"
#include "zfp/cache.h"

namespace zfp {

// compressed 1D array of scalars
template < typename Scalar, class Codec = zfp::codec<Scalar> >
class array1 : public array {
public:
  // forward declarations
  class reference;
  class pointer;
  class iterator;
  class view;
  #include "zfp/reference1.h"
  #include "zfp/pointer1.h"
  #include "zfp/iterator1.h"
  #include "zfp/view1.h"

  // default constructor
  array1() : array(1, Codec::type) {}

  // constructor of n-sample array using rate bits per value, at least
  // csize bytes of cache, and optionally initialized from flat array p
  array1(uint n, double rate, const Scalar* p = 0, size_t csize = 0) :
    array(1, Codec::type),
    cache(lines(csize, n))
  {
    set_rate(rate);
    resize(n, p == 0);
    if (p)
      set(p);
  }

  // constructor, from previously-serialized compressed array
  array1(const zfp::array::header& h, const uchar* buffer = 0, size_t buffer_size_bytes = 0) :
    array(1, Codec::type, h, buffer_size_bytes)
  {
    resize(nx, false);
    if (buffer)
      memcpy(data, buffer, bytes);
  }

  // copy constructor--performs a deep copy
  array1(const array1& a) :
    array()
  {
    deep_copy(a);
  }

  // construction from view--perform deep copy of (sub)array
  template <class View>
  array1(const View& v) :
    array(1, Codec::type),
    cache(lines(0, v.size_x()))
  {
    set_rate(v.rate());
    resize(v.size_x(), true);
    // initialize array in its preferred order
    for (iterator it = begin(); it != end(); ++it)
      *it = v(it.i());
  }

  // virtual destructor
  virtual ~array1() {}

  // assignment operator--performs a deep copy
  array1& operator=(const array1& a)
  {
    if (this != &a)
      deep_copy(a);
    return *this;
  }

  // total number of elements in array
  size_t size() const { return size_t(nx); }

  // array dimensions
  uint size_x() const { return nx; }

  // resize the array (all previously stored data will be lost)
  void resize(uint n, bool clear = true)
  {
    if (n == 0)
      free();
    else {
      nx = n;
      bx = (nx + 3) / 4;
      blocks = bx;
      alloc(clear);

      // precompute block dimensions
      zfp::deallocate(shape);
      if (nx & 3u) {
        shape = (uchar*)zfp::allocate(blocks);
        uchar* p = shape;
        for (uint i = 0; i < bx; i++)
          *p++ = (i == bx - 1 ? -nx & 3u : 0);
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
    cache.resize(lines(csize, nx));
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
    for (uint i = 0; i < bx; i++, p += 4, b++) {
      const CacheLine* line = cache.lookup(b + 1);
      if (line)
        line->get(p, 1, shape ? shape[b] : 0);
      else
        decode(b, p, 1);
    }
  }

  // initialize array by copying and compressing data stored at p
  void set(const Scalar* p)
  {
    uint b = 0;
    for (uint i = 0; i < bx; i++, b++, p += 4)
      encode(b, p, 1);
    cache.clear();
  }

  // (i) accessors
  Scalar operator()(uint i) const { return get(i); }
  reference operator()(uint i) { return reference(this, i); }

  // flat index accessors
  Scalar operator[](uint index) const { return get(index); }
  reference operator[](uint index) { return reference(this, index); }

  // random access iterators
  iterator begin() { return iterator(this, 0); }
  iterator end() { return iterator(this, nx); }

protected:
  // cache line representing one block of decompressed values
  class CacheLine {
  public:
    Scalar operator()(uint i) const { return a[index(i)]; }
    Scalar& operator()(uint i) { return a[index(i)]; }
    const Scalar* data() const { return a; }
    Scalar* data() { return a; }
    // copy cache line
    void get(Scalar* p, int sx) const
    {
      const Scalar* q = a;
      for (uint x = 0; x < 4; x++, p += sx, q++)
        *p = *q;
    }
    void get(Scalar* p, int sx, uint shape) const
    {
      if (!shape)
        get(p, sx);
      else {
        // determine block dimensions
        uint nx = 4 - (shape & 3u); shape >>= 2;
        const Scalar* q = a;
        for (uint x = 0; x < nx; x++, p += sx, q++)
          *p = *q;
      }
    }
  protected:
    static uint index(uint i) { return i & 3u; }
    Scalar a[4];
  };

  // perform a deep copy
  void deep_copy(const array1& a)
  {
    // copy base class members
    array::deep_copy(a);
    // copy cache
    cache = a.cache;
  }

  // inspector
  Scalar get(uint i) const
  {
    const CacheLine* p = line(i, false);
    return (*p)(i);
  }

  // mutator
  void set(uint i, Scalar val)
  {
    CacheLine* p = line(i, true);
    (*p)(i) = val;
  }

  // in-place updates
  void add(uint i, Scalar val) { (*line(i, true))(i) += val; }
  void sub(uint i, Scalar val) { (*line(i, true))(i) -= val; }
  void mul(uint i, Scalar val) { (*line(i, true))(i) *= val; }
  void div(uint i, Scalar val) { (*line(i, true))(i) /= val; }

  // return cache line for i; may require write-back and fetch
  CacheLine* line(uint i, bool write) const
  {
    CacheLine* p = 0;
    uint b = block(i);
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
    Codec::encode_block_1(zfp, block, shape ? shape[index] : 0);
    stream_flush(zfp->stream);
  }

  // encode block with given index from strided array
  void encode(uint index, const Scalar* p, int sx) const
  {
    stream_wseek(zfp->stream, index * blkbits);
    Codec::encode_block_strided_1(zfp, p, shape ? shape[index] : 0, sx);
    stream_flush(zfp->stream);
  }

  // decode block with given index
  void decode(uint index, Scalar* block) const
  {
    stream_rseek(zfp->stream, index * blkbits);
    Codec::decode_block_1(zfp, block, shape ? shape[index] : 0);
  }

  // decode block with given index to strided array
  void decode(uint index, Scalar* p, int sx) const
  {
    stream_rseek(zfp->stream, index * blkbits);
    Codec::decode_block_strided_1(zfp, p, shape ? shape[index] : 0, sx);
  }

  // block index for i
  static uint block(uint i) { return i / 4; }

  // number of cache lines corresponding to size (or suggested size if zero)
  static uint lines(size_t size, uint n)
  {
    n = size ? (size + sizeof(CacheLine) - 1) / sizeof(CacheLine) : array::lines(size_t((n + 3) / 4));
    return std::max(n, 1u);
  }

  mutable zfp::Cache<CacheLine> cache; // cache of decompressed blocks
};

typedef array1<float> array1f;
typedef array1<double> array1d;

}

#endif
