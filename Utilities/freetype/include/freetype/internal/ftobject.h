#ifndef __FT_OBJECT_H__
#define __FT_OBJECT_H__

#include <ft2build.h>
#include FT_FREETYPE_H

FT_BEGIN_HEADER

 /**************************************************************
  *
  * @type: FT_Object
  *
  * @description:
  *   handle to a FreeType Object. See @FT_ObjectRec
  */
  typedef struct FT_ObjectRec_*        FT_Object;


 /**************************************************************
  *
  * @type: FT_Class
  *
  * @description:
  *   handle to a constant class handle to a FreeType Object.
  *
  *   Note that a class is itself a @FT_Object and are dynamically
  *   allocated on the heap.
  *
  * @also:
  *  @FT_ClassRec, @FT_Object, @FT_ObjectRec, @FT_Type, @FT_TypeRec
  */
  typedef const struct FT_ClassRec_*   FT_Class;


 /**************************************************************
  *
  * @type: FT_Type
  *
  * @description:
  *   handle to a constant structure (of type @FT_TypeRec) used
  *   to describe a given @FT_Class type to the FreeType object
  *   sub-system.
  */
  typedef const struct FT_TypeRec_*    FT_Type;



 /**************************************************************
  *
  * @struct: FT_ObjectRec
  *
  * @description:
  *   a structure describing the root fields of all @FT_Object
  *   class instances in FreeType
  *
  * @fields:
  *   clazz     :: handle to the object's class
  *   ref_count :: object's reference count. Starts at 1
  */
  typedef struct FT_ObjectRec_
  {
    FT_Class  clazz;
    FT_Int    ref_count;

  } FT_ObjectRec;


 /**************************************************************
  *
  * @macro: FT_OBJECT (x)
  *
  * @description:
  *   a useful macro to type-cast anything to a @FT_Object
  *   handle. No check performed..
  */
#define  FT_OBJECT(x)    ((FT_Object)(x))


 /**************************************************************
  *
  * @macro: FT_OBJECT_P (x)
  *
  * @description:
  *   a useful macro to type-cast anything to a pointer to
  *   @FT_Object handle.
  */
#define  FT_OBJECT_P(x)  ((FT_Object*)(x))


 /**************************************************************
  *
  * @macro: FT_OBJECT__CLASS (obj)
  *
  * @description:
  *   a useful macro to return the class of any object
  */
#define  FT_OBJECT__CLASS(x)      FT_OBJECT(x)->clazz


 /**************************************************************
  *
  * @macro: FT_OBJECT__REF_COUNT (obj)
  *
  * @description:
  *   a useful macro to return the reference count of any object
  */
#define  FT_OBJECT__REF_COUNT(x)  FT_OBJECT(x)->ref_count


 /**************************************************************
  *
  * @macro: FT_OBJECT__MEMORY (obj)
  *
  * @description:
  *   a useful macro to return a handle to the memory manager
  *   used to allocate a given object
  */
#define  FT_OBJECT__MEMORY(x)     FT_CLASS__MEMORY(FT_OBJECT(x)->clazz)


 /**************************************************************
  *
  * @macro: FT_OBJECT__LIBRARY (obj)
  *
  * @description:
  *   a useful macro to return a handle to the library handle
  *   that owns the object
  */
#define  FT_OBJECT__LIBRARY(x)    FT_CLASS__LIBRARY(FT_OBJECT(x)->clazz)


 /**************************************************************
  *
  * @functype: FT_Object_InitFunc
  *
  * @description:
  *   a function used to initialize a new object
  *
  * @input:
  *   object    :: target object handle
  *   init_data :: optional pointer to initialization data
  *
  * @throws: any
  *
  *   the object is _assumed_ to be reachable from the cleanup
  *   stack when the constructor is called. This means that
  *   any exception can be thrown safely in it.
  */
  typedef void  (*FT_Object_InitFunc)( FT_Object   object,
                                       FT_Pointer  init_data );

 /**************************************************************
  *
  * @functype: FT_Object_DoneFunc
  *
  * @description:
  *   a function used to finalize a given object
  *
  * @input:
  *   object    :: handle to target object
  *
  * @throws: *never* !!
  */
  typedef void  (*FT_Object_DoneFunc)( FT_Object   object );


 /**************************************************************
  *
  * @struct: FT_ClassRec
  *
  * @description:
  *   a structure used to describe a given object class within
  *   FreeType
  *
  * @fields:
  *   object   :: root @FT_ObjectRec fields, since each class is
  *               itself an object. (it's an instance of the
  *               "metaclass", a special object of the FreeType
  *               object sub-system.)
  *
  *   magic    :: a 32-bit magic number used for decoding
  *   type     :: the @FT_Type descriptor of this class
  *   memory   :: the current memory manager handle
  *   library  :: the current library handle
  *   info     :: an opaque pointer to class-specific information
  *               managed by the FreeType object sub-system
  *
  *   obj_size :: size of class instances in bytes
  *   obj_init :: class instance constructor
  *   obj_done :: class instance destructor
  */
  typedef struct FT_ClassRec_
  {
    FT_ObjectRec        object;
    FT_UInt32           magic;
    FT_Type             type;
    FT_Memory           memory;
    FT_Library          library;
    FT_Pointer          info;

    FT_UInt             obj_size;
    FT_Object_InitFunc  obj_init;
    FT_Object_DoneFunc  obj_done;

  } FT_ClassRec;


 /**************************************************************
  *
  * @macro: FT_CLASS (x)
  *
  * @description:
  *   a useful macro to convert anything to a class handle
  *   without checks
  */
#define  FT_CLASS(x)    ((FT_Class)(x))


 /**************************************************************
  *
  * @macro: FT_CLASS_P (x)
  *
  * @description:
  *   a useful macro to convert anything to a pointer to a class
  *   handle without checks
  */
#define  FT_CLASS_P(x)  ((FT_Class*)(x))


 /**************************************************************
  *
  * @macro: FT_CLASS__MEMORY (clazz)
  *
  * @description:
  *   a useful macro to return the memory manager handle of a
  *   given class
  */
#define  FT_CLASS__MEMORY(x)   FT_CLASS(x)->memory


 /**************************************************************
  *
  * @macro: FT_CLASS__LIBRARY (clazz)
  *
  * @description:
  *   a useful macro to return the library handle of a
  *   given class
  */
#define  FT_CLASS__LIBRARY(x)  FT_CLASS(x)->library



 /**************************************************************
  *
  * @macro: FT_CLASS__TYPE (clazz)
  *
  * @description:
  *   a useful macro to return the type of a given class
  *   given class
  */
#define  FT_CLASS__TYPE(x)     FT_CLASS(x)->type

 /* */
#define  FT_CLASS__INFO(x)     FT_CLASS(x)->info
#define  FT_CLASS__MAGIC(x)    FT_CLASS(x)->magic


 /**************************************************************
  *
  * @struct: FT_TypeRec
  *
  * @description:
  *   a structure used to describe a given class to the FreeType
  *   object sub-system.
  *
  * @fields:
  *   name       :: class name. only used for debugging
  *   super      :: type of super-class. NULL if none
  *
  *   class_size :: size of class structure in bytes
  *   class_init :: class constructor
  *   class_done :: class finalizer
  *
  *   obj_size   :: instance size in bytes
  *   obj_init   :: instance constructor. can be NULL
  *   obj_done   :: instance destructor. can be NULL
  *
  * @note:
  *   if 'obj_init' is NULL, the class will use it's parent
  *   constructor.
  *
  *   if 'obj_done' is NULL, the class will use it's parent
  *   finalizer.
  *
  *   the object sub-system allocates a new class, copies
  *   the content of its super-class into the new structure,
  *   _then_ calls 'clazz_init'.
  */
  typedef struct FT_TypeRec_
  {
    const char*         name;
    FT_Type             super;

    FT_UInt             class_size;
    FT_Object_InitFunc  class_init;
    FT_Object_DoneFunc  class_done;

    FT_UInt             obj_size;
    FT_Object_InitFunc  obj_init;
    FT_Object_DoneFunc  obj_done;

  } FT_TypeRec;


 /**************************************************************
  *
  * @macro: FT_TYPE (x)
  *
  * @description:
  *   a useful macro to convert anything to a class type handle
  *   without checks
  */
#define  FT_TYPE(x)  ((FT_Type)(x))


 /**************************************************************
  *
  * @function: ft_object_check
  *
  * @description:
  *   checks that a handle points to a valid @FT_Object
  *
  * @input:
  *   obj :: handle/pointer
  *
  * @return:
  *   1 iff the handle points to a valid object. 0 otherwise
  */
  FT_BASE_DEF( FT_Int )
  ft_object_check( FT_Pointer  obj );


 /**************************************************************
  *
  * @function: ft_object_is_a
  *
  * @description:
  *   checks that a handle points to a valid @FT_Object that
  *   is an instance of a given class (or of any of its sub-classes)
  *
  * @input:
  *   obj   :: handle/pointer
  *   clazz :: class handle to check
  *
  * @return:
  *   1 iff the handle points to a valid 'clazz' instance. 0
  *   otherwise.
  */
  FT_BASE_DEF( FT_Int )
  ft_object_is_a( FT_Pointer  obj,
                  FT_Class    clazz );


 /**************************************************************
  *
  * @function: ft_object_new
  *
  * @description:
  *   create a new object (class instance)
  *
  * @input:
  *   clazz     :: object's class pointer
  *   init_data :: optional pointer to initialization data
  *
  * @return:
  *   handle to new object. Cannot be NULL !
  */
  FT_BASE_DEF( FT_Object )
  ft_object_new( FT_Class    clazz,
                 FT_Pointer  init_data );


 /**************************************************************
  *
  * @function: ft_object_create
  *
  * @description:
  *   a variation of @ft_object_new that should be used when
  *   creating a new object that is owned by another object
  *   which is reachable from the cleanup stack.
  *
  *   this function is a bit more akward to use but completely
  *   avoids push/pop pairs during object construction and is
  *   therefore faster.
  *
  * @output:
  *   pobject   :: new object handle
  *
  * @input:
  *   clazz     :: object's class pointer
  *   init_data :: optional pointer to initialization data
  *   push      :: boolean. If true, the new object is pushed
  *                on top of the cleanup stack.
  */
  FT_BASE_DEF( void )
  ft_object_create( FT_Object  *pobject,
                    FT_Class    clazz,
                    FT_Pointer  init_data );

 /* */

  FT_BASE_DEF( FT_Class )
  ft_class_find_by_type( FT_Type    type,
                         FT_Memory  memory );

  FT_BASE_DEF( FT_Class )
  ft_class_find_by_name( FT_CString  class_name,
                         FT_Memory   memory );

  FT_BASE_DEF( FT_Object )
  ft_object_new_from_type( FT_Type     type,
                           FT_Pointer  data,
                           FT_Memory   memory );

  FT_BASE_DEF( void )
  ft_object_create_from_type( FT_Object  *pobject,
                              FT_Type     type,
                              FT_Pointer  init_data,
                              FT_Memory   memory );

  FT_BASE_DEF( void )
  ft_object_push( FT_Object  object );

  FT_BASE_DEF( void )
  ft_object_pop( FT_Object  object );

  FT_BASE_DEF( void )
  ft_object_pop_destroy( FT_Object  object );


FT_END_HEADER

#endif /* __FT_OBJECT_H__ */
