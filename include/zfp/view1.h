// 1D array views; these classes are nested within zfp::array1

// abstract view of 1D array (base class)
class preview {
public:
  // rate in bits per value
  double rate() const { return array->rate(); }

  // dimensions of (sub)array
  size_t size() const { return size_t(nx); }

  // local to global array index
  uint global_x(uint i) const { return x + i; }

protected:
  // construction and assignment--perform shallow copy of (sub)array
  explicit preview(array1* array) : array(array), x(0), nx(array->nx) {}
  explicit preview(array1* array, uint x, uint nx) : array(array), x(x), nx(nx) {}
  preview& operator=(array1* a)
  {
    array = a;
    x = 0;
    nx = a->nx;
    return *this;
  }

  array1* array; // underlying container
  uint x;        // offset into array
  uint nx;       // dimensions of subarray
};

// generic read-only view into a rectangular subset of a 1D array
class const_view : public preview {
protected:
  using preview::array;
  using preview::x;
  using preview::nx;
public:
  // construction--perform shallow copy of (sub)array
  const_view(array1* array) : preview(array) {}
  const_view(array1* array, uint x, uint nx) : preview(array, x, nx) {}

  // dimensions of (sub)array
  uint size_x() const { return nx; }

  // [i] accessor
  Scalar operator[](uint index) const { return array->get(x + index); }

  // (i) accessor
  Scalar operator()(uint i) const { return array->get(x + i); }
};

// generic read-write view into a rectangular subset of a 1D array
class view : public const_view {
protected:
  using preview::array;
  using preview::x;
  using preview::nx;
public:
  // construction--perform shallow copy of (sub)array
  view(array1* array) : const_view(array) {}
  view(array1* array, uint x, uint nx) : const_view(array, x, nx) {}

  // [i] accessor from base class
  using const_view::operator[];

  // (i) accessor from base class
  using const_view::operator();

  // [i] mutator
  reference operator[](uint index) { return reference(array, x + index); }

  // (i) mutator
  reference operator()(uint i) { return reference(array, x + i); }
};

// thread-safe read-only view of 1D (sub)array with private cache
class private_const_view : public preview {
protected:
  using preview::array;
  using preview::x;
  using preview::nx;
public:
  // construction--perform shallow copy of (sub)array
  private_const_view(array1* array) :
    preview(array),
    cache(array->cache.size())
  {
    init();
  }
  private_const_view(array1* array, uint x, uint nx) :
    preview(array, x, nx),
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

  // cache size in number of bytes
  size_t cache_size() const { return cache.size() * sizeof(CacheLine); }

  // set minimum cache size in bytes (array dimensions must be known)
  void set_cache_size(size_t csize)
  {
    cache.resize(array->lines(csize, nx));
  }

  // empty cache without compressing modified cached blocks
  void clear_cache() const { cache.clear(); }

  // (i) accessor
  Scalar operator()(uint i) const { return get(x + i); }

protected:
  // cache line representing one block of decompressed values
  class CacheLine {
  public:
    const Scalar& operator()(uint i) const { return a[index(i)]; }
    Scalar& operator()(uint i) { return a[index(i)]; }
    const Scalar* data() const { return a; }
    Scalar* data() { return a; }
  protected:
    static uint index(uint i) { return i & 3u; }
    Scalar a[4];
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
  const Scalar& get(uint i) const
  {
    const CacheLine* p = line(i);
    return (*p)(i);
  }

  // return cache line for i; may require write-back and fetch
  CacheLine* line(uint i) const
  {
    CacheLine* p = 0;
    uint b = array->block(i);
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
    Codec::decode_block_1(zfp, block, array->shape ? array->shape[index] : 0);
  }

  zfp_stream* zfp;                // stream of compressed blocks
  mutable Cache<CacheLine> cache; // cache of decompressed blocks
};

// thread-safe read-write view of private 1D (sub)array
class private_view : public private_const_view {
protected:
  using preview::array;
  using preview::x;
  using preview::nx;
  using private_const_view::zfp;
  using private_const_view::cache;
  using private_const_view::init;
  using private_const_view::decode;
  class view_reference;
  typedef typename private_const_view::CacheLine CacheLine;
public:
  // construction--perform shallow copy of (sub)array
  private_view(array1* array) : private_const_view(array) {}
  private_view(array1* array, uint x, uint nx) : private_const_view(array, x, nx) {}

  // partition view into count block-aligned pieces, with 0 <= index < count
  void partition(uint index, uint count)
  {
    partition(x, nx, index, count);
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

  // (i) accessor from base class
  using private_const_view::operator();

  // (i) mutator
  view_reference operator()(uint i) { return view_reference(this, x + i); }

protected:
  class view_reference {
  public:
    operator Scalar() const { return view->get(i); }
    view_reference operator=(const view_reference& r) { view->set(i, r.operator Scalar()); return *this; }
    view_reference operator=(Scalar val) { view->set(i, val); return *this; }
    view_reference operator+=(Scalar val) { view->add(i, val); return *this; }
    view_reference operator-=(Scalar val) { view->sub(i, val); return *this; }
    view_reference operator*=(Scalar val) { view->mul(i, val); return *this; }
    view_reference operator/=(Scalar val) { view->div(i, val); return *this; }
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
    explicit view_reference(private_view* view, uint i) : view(view), i(i) {}
    private_view* view;
    uint i;
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
    uint b = array->block(i);
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
    Codec::encode_block_1(zfp, block, array->shape ? array->shape[index] : 0);
    stream_flush(zfp->stream);
  }
};
