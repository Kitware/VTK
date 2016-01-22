#ifndef DIY_COLLECTION_HPP
#define DIY_COLLECTION_HPP

#include <vector>

#include "serialization.hpp"
#include "storage.hpp"
#include "thread.hpp"


namespace diy
{
  class Collection
  {
    public:
      typedef       void*                                       Element;
      typedef       std::vector<Element>                        Elements;
      typedef       critical_resource<int, recursive_mutex>     CInt;

      typedef       void* (*Create)();
      typedef       void  (*Destroy)(void*);
      typedef       detail::Save                                Save;
      typedef       detail::Load                                Load;

    public:
                    Collection(Create               create,
                               Destroy              destroy,
                               ExternalStorage*     storage,
                               Save                 save,
                               Load                 load):
                        create_(create),
                        destroy_(destroy),
                        storage_(storage),
                        save_(save),
                        load_(load),
                        in_memory_(0)               {}

      size_t        size() const                    { return elements_.size(); }
      const CInt&   in_memory() const               { return in_memory_; }
      inline void   clear();

      int           add(Element e)                  { elements_.push_back(e); external_.push_back(-1); ++(*in_memory_.access()); return elements_.size() - 1; }
      void*         release(int i)                  { void* e = get(i); elements_[i] = 0; return e; }

      void*         find(int i) const               { return elements_[i]; }                        // possibly returns 0, if the element is unloaded
      void*         get(int i)                      { if (!find(i)) load(i); return find(i); }      // loads the element first, and then returns its address

      int           available() const               { int i = 0; for (; i < (int)size(); ++i) if (find(i) != 0) break; return i; }

      inline void   load(int i);
      inline void   unload(int i);

      Create        creator() const                 { return create_; }
      Destroy       destroyer() const               { return destroy_; }
      Load          loader() const                  { return load_; }
      Save          saver() const                   { return save_; }

      void*         create() const                  { return create_(); }
      void          destroy(int i)                  { if (find(i)) { destroy_(find(i)); elements_[i] = 0; } else if (external_[i] != -1) storage_->destroy(external_[i]); }

      bool          own() const                     { return destroy_ != 0; }

      ExternalStorage*      storage() const         { return storage_; }

    private:
      Create                create_;
      Destroy               destroy_;
      ExternalStorage*      storage_;
      Save                  save_;
      Load                  load_;

      Elements              elements_;
      std::vector<int>      external_;
      CInt                  in_memory_;
  };
}

void
diy::Collection::
clear()
{
  if (own())
    for (size_t i = 0; i < size(); ++i)
      destroy(i);
  elements_.clear();
  external_.clear();
  *in_memory_.access() = 0;
}

void
diy::Collection::
unload(int i)
{
  //BinaryBuffer bb;
  void* e = find(i);
  //save_(e, bb);
  //external_[i] = storage_->put(bb);
  external_[i] = storage_->put(e, save_);

  destroy_(e);
  elements_[i] = 0;

  --(*in_memory_.access());
}

void
diy::Collection::
load(int i)
{
  //BinaryBuffer bb;
  //storage_->get(external_[i], bb);
  void* e = create_();
  //load_(e, bb);
  storage_->get(external_[i], e, load_);
  elements_[i] = e;
  external_[i] = -1;

  ++(*in_memory_.access());
}

#endif
