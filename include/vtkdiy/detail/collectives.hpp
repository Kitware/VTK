#ifndef DIY_COLLECTIVES_HPP
#define DIY_COLLECTIVES_HPP

namespace diy
{
namespace detail
{
  struct CollectiveOp
  {
    virtual void    init()                                  =0;
    virtual void    update(const CollectiveOp& other)       =0;
    virtual void    global(const mpi::communicator& comm)   =0;
    virtual void    copy_from(const CollectiveOp& other)    =0;
    virtual void    result_out(void* dest) const            =0;
    virtual         ~CollectiveOp()                         {}
  };

  template<class T, class Op>
  struct AllReduceOp: public CollectiveOp
  {
          AllReduceOp(const T& x, Op op):
            in_(x), op_(op)         {}

    void  init()                                    { out_ = in_; }
    void  update(const CollectiveOp& other)         { out_ = op_(out_, static_cast<const AllReduceOp&>(other).in_); }
    void  global(const mpi::communicator& comm)     { T res; mpi::all_reduce(comm, out_, res, op_); out_ = res; }
    void  copy_from(const CollectiveOp& other)      { out_ = static_cast<const AllReduceOp&>(other).out_; }
    void  result_out(void* dest) const              { *reinterpret_cast<T*>(dest) = out_; }

    private:
      T     in_, out_;
      Op    op_;
  };

  template<class T>
  struct Scratch: public CollectiveOp
  {
          Scratch(const T& x):
            x_(x)                                   {}

    void  init()                                    {}
    void  update(const CollectiveOp& other)         {}
    void  global(const mpi::communicator& comm)     {}
    void  copy_from(const CollectiveOp& other)      {}
    void  result_out(void* dest) const              { *reinterpret_cast<T*>(dest) = x_; }

    private:
      T     x_;
  };

}
}

#endif
