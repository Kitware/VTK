// 2D array views; these classes are nested within zfp::array2

// abstract view of 2D array (base class)
class preview {
public:
  // rate in bits per value
  double rate() const { return array->rate(); }

  // dimensions of (sub)array
  size_t size() const { return size_t(nx) * size_t(ny); }

  // local to global array indices
  uint global_x(uint i) const { return x + i; }
  uint global_y(uint j) const { return y + j; }

protected:
  // construction and assignment--perform shallow copy of (sub)array
  explicit preview(array2* array) : array(array), x(0), y(0), nx(array->nx), ny(array->ny) {}
  explicit preview(array2* array, uint x, uint y, uint nx, uint ny) : array(array), x(x), y(y), nx(nx), ny(ny) {}
  preview& operator=(array2* a)
  {
    array = a;
    x = y = 0;
    nx = a->nx;
    ny = a->ny;
    return *this;
  }

  array2* array; // underlying container
  uint x, y;     // offset into array
  uint nx, ny;   // dimensions of subarray
};

// generic read-only view into a rectangular subset of a 2D array
class const_view : public preview {
protected:
  using preview::array;
  using preview::x;
  using preview::y;
  using preview::nx;
  using preview::ny;
public:
  // construction--perform shallow copy of (sub)array
  const_view(array2* array) : preview(array) {}
  const_view(array2* array, uint x, uint y, uint nx, uint ny) : preview(array, x, y, nx, ny) {}

  // dimensions of (sub)array
  uint size_x() const { return nx; }
  uint size_y() const { return ny; }

  // (i, j) accessor
  Scalar operator()(uint i, uint j) const { return array->get(x + i, y + j); }
};

// generic read-write view into a rectangular subset of a 2D array
class view : public const_view {
protected:
  using preview::array;
  using preview::x;
  using preview::y;
  using preview::nx;
  using preview::ny;
public:
  // construction--perform shallow copy of (sub)array
  view(array2* array) : const_view(array) {}
  view(array2* array, uint x, uint y, uint nx, uint ny) : const_view(array, x, y, nx, ny) {}

  // (i, j) accessor from base class
  using const_view::operator();

  // (i, j) mutator
  reference operator()(uint i, uint j) { return reference(array, x + i, y + j); }
};

// flat view of 2D array (operator[] returns scalar)
class flat_view : public view {
protected:
  using preview::array;
  using preview::x;
  using preview::y;
  using preview::nx;
  using preview::ny;
public:
  // construction--perform shallow copy of (sub)array
  flat_view(array2* array) : view(array) {}
  flat_view(array2* array, uint x, uint y, uint nx, uint ny) : view(array, x, y, nx, ny) {}

  // convert (i, j) index to flat index
  uint index(uint i, uint j) const { return i + nx * j; }

  // convert flat index to (i, j) index
  void ij(uint& i, uint& j, uint index) const
  {
    i = index % nx; index /= nx;
    j = index;
  }

  // flat index accessors
  Scalar operator[](uint index) const
  {
    uint i, j;
    ij(i, j, index);
    return array->get(x + i, y + j);
  }
  reference operator[](uint index)
  {
    uint i, j;
    ij(i, j, index);
    return reference(array, x + i, y + j);
  }
};

// forward declaration of friends
class nested_view1;
class nested_view2;

// nested view into a 1D rectangular subset of a 2D array
class nested_view1 : public preview {
protected:
  using preview::array;
  using preview::x;
  using preview::y;
  using preview::nx;
  using preview::ny;
public:
  // dimensions of (sub)array
  uint size_x() const { return nx; }

  // [i] accessor and mutator
  Scalar operator[](uint index) const { return array->get(x + index, y); }
  reference operator[](uint index) { return reference(array, x + index, y); }

  // (i) accessor and mutator
  Scalar operator()(uint i) const { return array->get(x + i, y); }
  reference operator()(uint i) { return reference(array, x + i, y); }

protected:
  // construction--perform shallow copy of (sub)array
  friend class nested_view2;
  explicit nested_view1(array2* array) : preview(array) {}
  explicit nested_view1(array2* array, uint x, uint y, uint nx, uint ny) : preview(array, x, y, nx, ny) {}
};

// nested view into a 2D rectangular subset of a 2D array
class nested_view2 : public preview {
protected:
  using preview::array;
  using preview::x;
  using preview::y;
  using preview::nx;
  using preview::ny;
public:
  // construction--perform shallow copy of (sub)array
  nested_view2(array2* array) : preview(array) {}
  nested_view2(array2* array, uint x, uint y, uint nx, uint ny) : preview(array, x, y, nx, ny) {}

  // dimensions of (sub)array
  uint size_x() const { return nx; }
  uint size_y() const { return ny; }

  // 1D view
  nested_view1 operator[](uint index) const { return nested_view1(array, x, y + index, nx, 1); }

  // (i, j) accessor and mutator
  Scalar operator()(uint i, uint j) const { return array->get(x + i, y + j); }
  reference operator()(uint i, uint j) { return reference(array, x + i, y + j); }
};

typedef nested_view2 nested_view;

// thread-safe read-only view of 2D (sub)array with private cache
class private_const_view : public preview {
protected:
  using preview::array;
  using preview::x;
  using preview::y;
  using preview::nx;
  using preview::ny;
public:
  // construction--perform shallow copy of (sub)array
  private_const_view(array2* array) :
    preview(array),
    cache(array->cache.size())
  {
    init();
  }
  private_const_view(array2* array, uint x, uint y, uint nx, uint ny) :
    preview(array, x, y, nx, ny),
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

  // cache size in number of bytes
  size_t cache_size() const { return cache.size() * sizeof(CacheLine); }

  // set minimum cache size in bytes (array dimensions must be known)
  void set_cache_size(size_t csize)
  {
    cache.resize(array->lines(csize, nx, ny));
  }

  // empty cache without compressing modified cached blocks
  void clear_cache() const { cache.clear(); }

  // (i, j) accessor
  Scalar operator()(uint i, uint j) const { return get(x + i, y + j); }

protected:
  // cache line representing one block of decompressed values
  class CacheLine {
  public:
    const Scalar& operator()(uint i, uint j) const { return a[index(i, j)]; }
    Scalar& operator()(uint i, uint j) { return a[index(i, j)]; }
    const Scalar* data() const { return a; }
    Scalar* data() { return a; }
  protected:
    static uint index(uint i, uint j) { return (i & 3u) + 4 * (j & 3u); }
    Scalar a[16];
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
  const Scalar& get(uint i, uint j) const
  {
    const CacheLine* p = line(i, j);
    return (*p)(i, j);
  }

  // return cache line for (i, j); may require write-back and fetch
  CacheLine* line(uint i, uint j) const
  {
    CacheLine* p = 0;
    uint b = array->block(i, j);
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
    Codec::decode_block_2(zfp, block, array->shape ? array->shape[index] : 0);
  }

  zfp_stream* zfp;                // stream of compressed blocks
  mutable Cache<CacheLine> cache; // cache of decompressed blocks
};

// thread-safe read-write view of private 2D (sub)array
class private_view : public private_const_view {
protected:
  using preview::array;
  using preview::x;
  using preview::y;
  using preview::nx;
  using preview::ny;
  using private_const_view::zfp;
  using private_const_view::cache;
  using private_const_view::init;
  using private_const_view::decode;
  class view_reference;
  typedef typename private_const_view::CacheLine CacheLine;
public:
  // construction--perform shallow copy of (sub)array
  private_view(array2* array) : private_const_view(array) {}
  private_view(array2* array, uint x, uint y, uint nx, uint ny) : private_const_view(array, x, y, nx, ny) {}

  // partition view into count block-aligned pieces, with 0 <= index < count
  void partition(uint index, uint count)
  {
    if (nx > ny)
      partition(x, nx, index, count);
    else
      partition(y, ny, index, count);
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

  // (i, j) accessor from base class
  using private_const_view::operator();

  // (i, j) mutator
  view_reference operator()(uint i, uint j) { return view_reference(this, x + i, y + j); }

protected:
  class view_reference {
  public:
    operator Scalar() const { return view->get(i, j); }
    view_reference operator=(const view_reference& r) { view->set(i, j, r.operator Scalar()); return *this; }
    view_reference operator=(Scalar val) { view->set(i, j, val); return *this; }
    view_reference operator+=(Scalar val) { view->add(i, j, val); return *this; }
    view_reference operator-=(Scalar val) { view->sub(i, j, val); return *this; }
    view_reference operator*=(Scalar val) { view->mul(i, j, val); return *this; }
    view_reference operator/=(Scalar val) { view->div(i, j, val); return *this; }
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
    explicit view_reference(private_view* view, uint i, uint j) : view(view), i(i), j(j) {}
    private_view* view;
    uint i, j;
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
    uint b = array->block(i, j);
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
    Codec::encode_block_2(zfp, block, array->shape ? array->shape[index] : 0);
    stream_flush(zfp->stream);
  }
};
