#ifndef ZFP_ARRAY_H
#define ZFP_ARRAY_H

#include <algorithm>
#include <climits>
#include <cstring>
#include <stdexcept>
#include <string>

#include "zfp.h"
#include "zfp/memory.h"

// all undefined at end
#define DIV_ROUND_UP(x, y) (((x) + (y) - 1) / (y))
#define BITS_TO_BYTES(x) DIV_ROUND_UP(x, CHAR_BIT)

#define ZFP_HEADER_SIZE_BITS (ZFP_MAGIC_BITS + ZFP_META_BITS + ZFP_MODE_SHORT_BITS)

namespace zfp {

// abstract base class for compressed array of scalars
class array {
public:
  #include "zfp/header.h"

  static zfp::array* construct(const zfp::array::header& header, const uchar* buffer = 0, size_t buffer_size_bytes = 0);

protected:
  // default constructor
  array() :
    dims(0), type(zfp_type_none),
    nx(0), ny(0), nz(0),
    bx(0), by(0), bz(0),
    blocks(0), blkbits(0),
    bytes(0), data(0),
    zfp(0),
    shape(0)
  {}

  // generic array with 'dims' dimensions and scalar type 'type'
  array(uint dims, zfp_type type) :
    dims(dims), type(type),
    nx(0), ny(0), nz(0),
    bx(0), by(0), bz(0),
    blocks(0), blkbits(0),
    bytes(0), data(0),
    zfp(zfp_stream_open(0)),
    shape(0)
  {}

  // constructor, from previously-serialized compressed array
  array(uint dims, zfp_type type, const zfp::array::header& h, size_t expected_buffer_size_bytes) :
    dims(dims), type(type),
    nx(0), ny(0), nz(0),
    bx(0), by(0), bz(0),
    blocks(0), blkbits(0),
    bytes(0), data(0),
    zfp(zfp_stream_open(0)),
    shape(0)
  {
    // read header to populate member variables associated with zfp_stream
    try {
      read_from_header(h);
    } catch (zfp::array::header::exception const &) {
      zfp_stream_close(zfp);
      throw;
    }

    if (expected_buffer_size_bytes && !is_valid_buffer_size(zfp, nx, ny, nz, expected_buffer_size_bytes)) {
      zfp_stream_close(zfp);
      throw zfp::array::header::exception("ZFP header expects a longer buffer than what was passed in.");
    }
  }

  // copy constructor--performs a deep copy
  array(const array& a) :
    data(0),
    zfp(0),
    shape(0)
  {
    deep_copy(a);
  }

  // assignment operator--performs a deep copy
  array& operator=(const array& a)
  {
    deep_copy(a);
    return *this;
  }

public:
  // public virtual destructor (can delete array through base class pointer)
  virtual ~array()
  {
    free();
    zfp_stream_close(zfp);
  }

  // rate in bits per value
  double rate() const { return double(blkbits) / block_size(); }

  // set compression rate in bits per value
  double set_rate(double rate)
  {
    rate = zfp_stream_set_rate(zfp, rate, type, dims, 1);
    blkbits = zfp->maxbits;
    alloc();
    return rate;
  }

  // empty cache without compressing modified cached blocks
  virtual void clear_cache() const = 0;

  // flush cache by compressing all modified cached blocks
  virtual void flush_cache() const = 0;

  // number of bytes of compressed data
  size_t compressed_size() const { return bytes; }

  // pointer to compressed data for read or write access
  uchar* compressed_data() const
  {
    // first write back any modified cached data
    flush_cache();
    return data;
  }

  // dimensionality
  uint dimensionality() const { return dims; }

  // underlying scalar type
  zfp_type scalar_type() const { return type; }

  // write header with latest metadata
  zfp::array::header get_header() const
  {
    // intermediate buffer needed (bitstream accesses multiples of wordsize)
    AlignedBufferHandle abh;
    DualBitstreamHandle dbh(zfp, abh);

    ZfpFieldHandle zfh(type, nx, ny, nz);

    // avoid long header (alignment issue)
    if (zfp_stream_mode(zfp) > ZFP_MODE_SHORT_MAX)
      throw zfp::array::header::exception("ZFP compressed arrays only support short headers at this time.");

    if (!zfp_write_header(zfp, zfh.field, ZFP_HEADER_FULL))
      throw zfp::array::header::exception("ZFP could not write a header to buffer.");
    stream_flush(zfp->stream);

    zfp::array::header h;
    abh.copy_to_header(&h);

    return h;
  }

private:
  // private members used when reading/writing headers
  #include "zfp/headerHelpers.h"

protected:
  // number of values per block
  uint block_size() const { return 1u << (2 * dims); }

  // allocate memory for compressed data
  void alloc(bool clear = true)
  {
    bytes = blocks * blkbits / CHAR_BIT;
    zfp::reallocate_aligned(data, bytes, 0x100u);
    if (clear)
      std::fill(data, data + bytes, 0);
    stream_close(zfp->stream);
    zfp_stream_set_bit_stream(zfp, stream_open(data, bytes));
    clear_cache();
  }

  // free memory associated with compressed data
  void free()
  {
    nx = ny = nz = 0;
    bx = by = bz = 0;
    blocks = 0;
    stream_close(zfp->stream);
    zfp_stream_set_bit_stream(zfp, 0);
    bytes = 0;
    zfp::deallocate_aligned(data);
    data = 0;
    zfp::deallocate(shape);
    shape = 0;
  }

  // perform a deep copy
  void deep_copy(const array& a)
  {
    // copy metadata
    dims = a.dims;
    type = a.type;
    nx = a.nx;
    ny = a.ny;
    nz = a.nz;
    bx = a.bx;
    by = a.by;
    bz = a.bz;
    blocks = a.blocks;
    blkbits = a.blkbits;
    bytes = a.bytes;

    // copy dynamically allocated data
    zfp::clone_aligned(data, a.data, bytes, 0x100u);
    if (zfp) {
      if (zfp->stream)
        stream_close(zfp->stream);
      zfp_stream_close(zfp);
    }
    zfp = zfp_stream_open(0);
    *zfp = *a.zfp;
    zfp_stream_set_bit_stream(zfp, stream_open(data, bytes));
    zfp::clone(shape, a.shape, blocks);
  }

  // attempt reading header from zfp::array::header
  // and verify header contents (throws exceptions upon failure)
  void read_from_header(const zfp::array::header& h)
  {
    // copy header into aligned buffer
    AlignedBufferHandle abh(&h);
    DualBitstreamHandle dbh(zfp, abh);
    ZfpFieldHandle zfh;

    // read header to populate member variables associated with zfp_stream
    size_t readbits = zfp_read_header(zfp, zfh.field, ZFP_HEADER_FULL);
    if (!readbits)
      throw zfp::array::header::exception("Invalid ZFP header.");
    else if (readbits != ZFP_HEADER_SIZE_BITS)
      throw zfp::array::header::exception("ZFP compressed arrays only support short headers at this time.");

    // verify metadata on zfp_field match that for this object
    std::string err_msg = "";
    if (type != zfp_field_type(zfh.field))
      zfp::array::header::concat_sentence(err_msg, "ZFP header specified an underlying scalar type different than that for this object.");

    if (dims != zfp_field_dimensionality(zfh.field))
      zfp::array::header::concat_sentence(err_msg, "ZFP header specified a dimensionality different than that for this object.");

    verify_header_contents(zfp, zfh.field, err_msg);

    if (!err_msg.empty())
      throw zfp::array::header::exception(err_msg);

    // set class variables
    nx = zfh.field->nx;
    ny = zfh.field->ny;
    nz = zfh.field->nz;
    type = zfh.field->type;
    blkbits = zfp->maxbits;
  }

  // default number of cache lines for array with n blocks
  static uint lines(size_t n)
  {
    // compute m = O(sqrt(n))
    size_t m;
    for (m = 1; m * m < n; m *= 2);
    return static_cast<uint>(m);
  }

  uint dims;           // array dimensionality (1, 2, or 3)
  zfp_type type;       // scalar type
  uint nx, ny, nz;     // array dimensions
  uint bx, by, bz;     // array dimensions in number of blocks
  uint blocks;         // number of blocks
  size_t blkbits;      // number of bits per compressed block
  size_t bytes;        // total bytes of compressed data
  mutable uchar* data; // pointer to compressed data
  zfp_stream* zfp;     // compressed stream of blocks
  uchar* shape;        // precomputed block dimensions (or null if uniform)
};

#undef DIV_ROUND_UP
#undef BITS_TO_BYTES

#undef ZFP_HEADER_SIZE_BITS

}

#endif
