// 3D array views; these classes are nested within zfp::array3

// abstract view of 3D array (base class)
class preview {
public:
  // rate in bits per value
  double rate() const { return array->rate(); }

  // dimensions of (sub)array
  size_t size() const { return size_t(nx) * size_t(ny) * size_t(nz); }

  // local to global array indices
  uint global_x(uint i) const { return x + i; }
  uint global_y(uint j) const { return y + j; }
  uint global_z(uint k) const { return z + k; }

protected:
  // construction and assignment--perform shallow copy of (sub)array
  explicit preview(array3* array) : array(array), x(0), y(0), z(0), nx(array->nx), ny(array->ny), nz(array->nz) {}
  explicit preview(array3* array, uint x, uint y, uint z, uint nx, uint ny, uint nz) : array(array), x(x), y(y), z(z), nx(nx), ny(ny), nz(nz) {}
  preview& operator=(array3* a)
  {
    array = a;
    x = y = z = 0;
    nx = a->nx;
    ny = a->ny;
    nz = a->nz;
    return *this;
  }

  array3* array;   // underlying container
  uint x, y, z;    // offset into array
  uint nx, ny, nz; // dimensions of subarray
};

// generic read-only view into a rectangular subset of a 3D array
class const_view : public preview {
protected:
  using preview::array;
  using preview::x;
  using preview::y;
  using preview::z;
  using preview::nx;
  using preview::ny;
  using preview::nz;
public:
  // construction--perform shallow copy of (sub)array
  const_view(array3* array) : preview(array) {}
  const_view(array3* array, uint x, uint y, uint z, uint nx, uint ny, uint nz) : preview(array, x, y, z, nx, ny, nz) {}

  // dimensions of (sub)array
  uint size_x() const { return nx; }
  uint size_y() const { return ny; }
  uint size_z() const { return nz; }

  // (i, j, k) accessor
  Scalar operator()(uint i, uint j, uint k) const { return array->get(x + i, y + j, z + k); }
};

// generic read-write view into a rectangular subset of a 3D array
class view : public const_view {
protected:
  using preview::array;
  using preview::x;
  using preview::y;
  using preview::z;
  using preview::nx;
  using preview::ny;
  using preview::nz;
public:
  // construction--perform shallow copy of (sub)array
  view(array3* array) : const_view(array) {}
  view(array3* array, uint x, uint y, uint z, uint nx, uint ny, uint nz) : const_view(array, x, y, z, nx, ny, nz) {}

  // (i, j, k) accessor from base class
  using const_view::operator();

  // (i, j, k) mutator
  reference operator()(uint i, uint j, uint k) { return reference(array, x + i, y + j, z + k); }
};

// flat view of 3D array (operator[] returns scalar)
class flat_view : public view {
protected:
  using preview::array;
  using preview::x;
  using preview::y;
  using preview::z;
  using preview::nx;
  using preview::ny;
  using preview::nz;
public:
  // construction--perform shallow copy of (sub)array
  flat_view(array3* array) : view(array) {}
  flat_view(array3* array, uint x, uint y, uint z, uint nx, uint ny, uint nz) : view(array, x, y, z, nx, ny, nz) {}

  // convert (i, j, k) index to flat index
  uint index(uint i, uint j, uint k) const { return i + nx * (j + ny * k); }

  // convert flat index to (i, j, k) index
  void ijk(uint& i, uint& j, uint& k, uint index) const
  {
    i = index % nx; index /= nx;
    j = index % ny; index /= ny;
    k = index;
  }

  // flat index accessors
  Scalar operator[](uint index) const
  {
    uint i, j, k;
    ijk(i, j, k, index);
    return array->get(x + i, y + j, z + k);
  }
  reference operator[](uint index)
  {
    uint i, j, k;
    ijk(i, j, k, index);
    return reference(array, x + i, y + j, z + k);
  }
};

// forward declaration of friends
class nested_view1;
class nested_view2;
class nested_view3;

// nested view into a 1D rectangular subset of a 3D array
class nested_view1 : public preview {
protected:
  using preview::array;
  using preview::x;
  using preview::y;
  using preview::z;
  using preview::nx;
  using preview::ny;
  using preview::nz;
public:
  // dimensions of (sub)array
  uint size_x() const { return nx; }

  // [i] accessor and mutator
  Scalar operator[](uint index) const { return array->get(x + index, y, z); }
  reference operator[](uint index) { return reference(array, x + index, y, z); }

  // (i) accessor and mutator
  Scalar operator()(uint i) const { return array->get(x + i, y, z); }
  reference operator()(uint i) { return reference(array, x + i, y, z); }

protected:
  // construction--perform shallow copy of (sub)array
  friend class nested_view2;
  explicit nested_view1(array3* array) : preview(array) {}
  explicit nested_view1(array3* array, uint x, uint y, uint z, uint nx, uint ny, uint nz) : preview(array, x, y, z, nx, ny, nz) {}
};

// nested view into a 2D rectangular subset of a 3D array
class nested_view2 : public preview {
protected:
  using preview::array;
  using preview::x;
  using preview::y;
  using preview::z;
  using preview::nx;
  using preview::ny;
  using preview::nz;
public:
  // dimensions of (sub)array
  uint size_x() const { return nx; }
  uint size_y() const { return ny; }

  // 1D view
  nested_view1 operator[](uint index) const { return nested_view1(array, x, y + index, z, nx, 1, 1); }

  // (i, j) accessor and mutator
  Scalar operator()(uint i, uint j) const { return array->get(x + i, y + j, z); }
  reference operator()(uint i, uint j) { return reference(array, x + i, y + j, z); }

protected:
  // construction--perform shallow copy of (sub)array
  friend class nested_view3;
  explicit nested_view2(array3* array) : preview(array) {}
  explicit nested_view2(array3* array, uint x, uint y, uint z, uint nx, uint ny, uint nz) : preview(array, x, y, z, nx, ny, nz) {}
};

// nested view into a 3D rectangular subset of a 3D array
class nested_view3 : public preview {
protected:
  using preview::array;
  using preview::x;
  using preview::y;
  using preview::z;
  using preview::nx;
  using preview::ny;
  using preview::nz;
public:
  // construction--perform shallow copy of (sub)array
  nested_view3(array3* array) : preview(array) {}
  nested_view3(array3* array, uint x, uint y, uint z, uint nx, uint ny, uint nz) : preview(array, x, y, z, nx, ny, nz) {}

  // dimensions of (sub)array
  uint size_x() const { return nx; }
  uint size_y() const { return ny; }
  uint size_z() const { return nz; }

  // 2D view
  nested_view2 operator[](uint index) const { return nested_view2(array, x, y, z + index, nx, ny, 1); }

  // (i, j, k) accessor and mutator
  Scalar operator()(uint i, uint j, uint k) const { return array->get(x + i, y + j, z + k); }
  reference operator()(uint i, uint j, uint k) { return reference(array, x + i, y + j, z + k); }
};

typedef nested_view3 nested_view;

// thread-safe read-only view of 3D (sub)array with private cache
class private_const_view : public preview {
protected:
  using preview::array;
  using preview::x;
  using preview::y;
  using preview::z;
  using preview::nx;
  using preview::ny;
  using preview::nz;
public:
  // construction--perform shallow copy of (sub)array
  private_const_view(array3* array) :
    preview(array),
    cache(array->cache.size())
  {
    init();
  }
  private_const_view(array3* array, uint x, uint y, uint z, uint nx, uint ny, uint nz) :
    preview(array, x, y, z, nx, ny, nz),
    cache(array->cache.size())
  {
    init();
  }

  // destructor
  ~private_const_view()
  {
    stream_close(zfp->stream);
    zfp_stream_close(zfp);
  }

  // dimensions of (sub)array
  uint size_x() const { return nx; }
  uint size_y() const { return ny; }
  uint size_z() const { return nz; }

  // cache size in number of bytes
  size_t cache_size() const { return cache.size() * sizeof(CacheLine); }

  // set minimum cache size in bytes (array dimensions must be known)
  void set_cache_size(size_t csize)
  {
    cache.resize(array->lines(csize, nx, ny, nz));
  }

  // empty cache without compressing modified cached blocks
  void clear_cache() const { cache.clear(); }

  // (i, j, k) accessor
  Scalar operator()(uint i, uint j, uint k) const { return get(x + i, y + j, z + k); }

protected:
  // cache line representing one block of decompressed values
  class CacheLine {
  public:
    const Scalar& operator()(uint i, uint j, uint k) const { return a[index(i, j, k)]; }
    Scalar& operator()(uint i, uint j, uint k) { return a[index(i, j, k)]; }
    const Scalar* data() const { return a; }
    Scalar* data() { return a; }
  protected:
    static uint index(uint i, uint j, uint k) { return (i & 3u) + 4 * ((j & 3u) + 4 * (k & 3u)); }
    Scalar a[64];
  };

  // copy private data
  void init()
  {
    // copy compressed stream
    zfp = zfp_stream_open(0);
    *zfp = *array->zfp;
    // copy bit stream
    zfp->stream = stream_clone(array->zfp->stream);
  }

  // inspector
  const Scalar& get(uint i, uint j, uint k) const
  {
    const CacheLine* p = line(i, j, k);
    return (*p)(i, j, k);
  }

  // return cache line for (i, j, k); may require write-back and fetch
  CacheLine* line(uint i, uint j, uint k) const
  {
    CacheLine* p = 0;
    uint b = array->block(i, j, k);
    typename Cache<CacheLine>::Tag t = cache.access(p, b + 1, false);
    uint c = t.index() - 1;
    // fetch cache line; no writeback possible since view is read-only
    if (c != b)
      decode(b, p->data());
    return p;
  }

  // decode block with given index
  void decode(uint index, Scalar* block) const
  {
    stream_rseek(zfp->stream, index * array->blkbits);
    Codec::decode_block_3(zfp, block, array->shape ? array->shape[index] : 0);
  }

  zfp_stream* zfp;                // stream of compressed blocks
  mutable Cache<CacheLine> cache; // cache of decompressed blocks
};

// thread-safe read-write view of private 3D (sub)array
class private_view : public private_const_view {
protected:
  using preview::array;
  using preview::x;
  using preview::y;
  using preview::z;
  using preview::nx;
  using preview::ny;
  using preview::nz;
  using private_const_view::zfp;
  using private_const_view::cache;
  using private_const_view::init;
  using private_const_view::decode;
  class view_reference;
  typedef typename private_const_view::CacheLine CacheLine;
public:
  // construction--perform shallow copy of (sub)array
  private_view(array3* array) : private_const_view(array) {}
  private_view(array3* array, uint x, uint y, uint z, uint nx, uint ny, uint nz) : private_const_view(array, x, y, z, nx, ny, nz) {}

  // partition view into count block-aligned pieces, with 0 <= index < count
  void partition(uint index, uint count)
  {
    if (nx > std::max(ny, nz))
      partition(x, nx, index, count);
    else if (ny > std::max(nx, nz))
      partition(y, ny, index, count);
    else
      partition(z, nz, index, count);
  }

  // flush cache by compressing all modified cached blocks
  void flush_cache() const
  {
    for (typename Cache<CacheLine>::const_iterator p = cache.first(); p; p++) {
      if (p->tag.dirty()) {
        uint b = p->tag.index() - 1;
        encode(b, p->line->data());
      }
      cache.flush(p->line);
    }
  }

  // (i, j, k) accessor from base class
  using private_const_view::operator();

  // (i, j, k) mutator
  view_reference operator()(uint i, uint j, uint k) { return view_reference(this, x + i, y + j, z + k); }

protected:
  class view_reference {
  public:
    operator Scalar() const { return view->get(i, j, k); }
    view_reference operator=(const view_reference& r) { view->set(i, j, k, r.operator Scalar()); return *this; }
    view_reference operator=(Scalar val) { view->set(i, j, k, val); return *this; }
    view_reference operator+=(Scalar val) { view->add(i, j, k, val); return *this; }
    view_reference operator-=(Scalar val) { view->sub(i, j, k, val); return *this; }
    view_reference operator*=(Scalar val) { view->mul(i, j, k, val); return *this; }
    view_reference operator/=(Scalar val) { view->div(i, j, k, val); return *this; }
    // swap two array elements via proxy references
    friend void swap(view_reference a, view_reference b)
    {
      Scalar x = a.operator Scalar();
      Scalar y = b.operator Scalar();
      b.operator=(x);
      a.operator=(y);
    }

  protected:
    friend class private_view;
    explicit view_reference(private_view* view, uint i, uint j, uint k) : view(view), i(i), j(j), k(k) {}
    private_view* view;
    uint i, j, k;
  };

  // block-aligned partition of [offset, offset + size): index out of count
  static void partition(uint& offset, uint& size, uint index, uint count)
  {
    uint bmin = offset / 4;
    uint bmax = (offset + size + 3) / 4;
    uint xmin = std::max(offset +    0, 4 * (bmin + (bmax - bmin) * (index + 0) / count));
    uint xmax = std::min(offset + size, 4 * (bmin + (bmax - bmin) * (index + 1) / count));
    offset = xmin;
    size = xmax - xmin;
  }

  // mutator
  void set(uint i, uint j, uint k, Scalar val)
  {
    CacheLine* p = line(i, j, k, true);
    (*p)(i, j, k) = val;
  }

  // in-place updates
  void add(uint i, uint j, uint k, Scalar val) { (*line(i, j, k, true))(i, j, k) += val; }
  void sub(uint i, uint j, uint k, Scalar val) { (*line(i, j, k, true))(i, j, k) -= val; }
  void mul(uint i, uint j, uint k, Scalar val) { (*line(i, j, k, true))(i, j, k) *= val; }
  void div(uint i, uint j, uint k, Scalar val) { (*line(i, j, k, true))(i, j, k) /= val; }

  // return cache line for (i, j, k); may require write-back and fetch
  CacheLine* line(uint i, uint j, uint k, bool write) const
  {
    CacheLine* p = 0;
    uint b = array->block(i, j, k);
    typename Cache<CacheLine>::Tag t = cache.access(p, b + 1, write);
    uint c = t.index() - 1;
    if (c != b) {
      // write back occupied cache line if it is dirty
      if (t.dirty())
        encode(c, p->data());
      decode(b, p->data());
    }
    return p;
  }

  // encode block with given index
  void encode(uint index, const Scalar* block) const
  {
    stream_wseek(zfp->stream, index * array->blkbits);
    Codec::encode_block_3(zfp, block, array->shape ? array->shape[index] : 0);
    stream_flush(zfp->stream);
  }
};
