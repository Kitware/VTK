#include "ftobject.c"

#define  FT_MAGIC_DEATH   0xDEADdead
#define  FT_MAGIC_CLASS   0x12345678

#define  FT_OBJECT_CHECK(o)                                  \
           ( FT_OBJECT(o)              != NULL            && \
             FT_OBJECT(o)->clazz       != NULL            && \
             FT_OBJECT(o)->ref_count   >= 1               && \
             FT_OBJECT(o)->clazz->magic == FT_MAGIC_CLASS )


 /*******************************************************************/
 /*******************************************************************/
 /*****                                                         *****/
 /*****                                                         *****/
 /*****                  M E T A - C L A S S                    *****/
 /*****                                                         *****/
 /*****                                                         *****/
 /*******************************************************************/
 /*******************************************************************/

 /* we use a dynamic hash table to map types to classes */
 /* this structure defines the layout of each node of   */
 /* this table                                          */
  typedef struct FT_ClassHNodeRec_
  {
    FT_HashNodeRec  hnode;
    FT_Type         ctype;
    FT_Class        clazz;

  } FT_ClassHNodeRec, *FT_ClassHNode;


 /* the meta class contains a type -> class mapping */
 /* and owns all class objects..                    */
 /*                                                 */
  typedef struct FT_MetaClassRec_
  {
    FT_ClassRec  clazz;
    FT_HashRec   type_to_class;

  } FT_MetaClassRec, *FT_MetaClass;

 /* forward declaration */
  static const FT_TypeRec  ft_meta_class_type;


 /* destroy a given class */
  static void
  ft_class_hnode_destroy( FT_ClassHNode  node )
  {
    FT_Clazz   clazz  = node->clazz;
    FT_Memory  memory = clazz->memory;
    FT_Type    ctype  = clazz->type;

    if ( ctype->class_done )
      ctype->class_done( clazz );

    FT_FREE( clazz );

    node->clazz = NULL;
    node->type  = NULL;

    FT_FREE( node );
  }


  static FT_Int
  ft_class_hnode_compare( const FT_ClassHNode  node1,
                          const FT_ClassHNode  node2 )
  {
    return ( node1->type == node2->type );
  }


  static void
  ft_metaclass_done( FT_MetaClass  meta )
  {
    /* clear all objects */
    ft_hash_done( &meta->type_to_class,
                  (FT_Hash_ForeachFunc) ft_class_destroy,
                   NULL );

    meta->clazz->object.clazz    = NULL;
    meta->clazz->object.ref_count = 0;
    meta->clazz->magic            = FT_MAGIC_DEATH;
  }


  static void
  ft_metaclass_init( FT_MetaClass  meta,
                     FT_Library    library )
  {
    FT_ClassRec*  clazz = meta->clazz;

    /* the meta-class is its OWN class !! */
    clazz->object.clazz     = (FT_Class) clazz;
    clazz->object.ref_count = 1;
    clazz->magic            = FT_MAGIC_CLASS;
    clazz->library          = library;
    clazz->memory           = library->memory;
    clazz->type             = &ft_metaclass_type;
    clazz->info             = NULL;

    clazz->obj_size         = sizeof( FT_ClassRec );
    clazz->obj_init         = NULL;
    clazz->obj_done         = NULL;

    ft_hash_init( &meta->type_to_class,
                  (FT_Hash_CompareFunc) ft_class_hnode_compare,
                  library->memory );
  }


 /* find or create the class corresponding to a given type */
  static FT_Class
  ft_metaclass_get_class( FT_MetaClass  meta,
                          FT_Type       ctype )
  {
    FT_ClassHNodeRec   keynode, *node, **pnode;
    FT_Memory          memory;

    keynode.hnode.hash = (FT_UInt32)( ctype >> 2 );
    keynode.type       = type;

    pnode = (FT_ClassHNode) ft_hash_lookup( &meta->type_to_class,
                                            &noderec );
    node  = *pnode;
    if ( node != NULL )
      return node->clazz;

    memory = FT_CLASS__MEMORY(meta);
    node   = FT_MEM_SAFE_ALLOC( sizeof(*node) );
    if ( node != NULL )
    {
      FT_ClassRec*  clazz;

      clazz = FT_MEM_SAFE_ALLOC( ctype->class_size );
      if ( clazz == NULL )
      {
        FT_FREE( node );
        FT_XTHROW( FT_Err_Out_Of_Memory );
      }
    }
  }


  static const FT_TypeRec  ft_meta_class_type =
  {
    "FT2.MetaClass",
    NULL,

    sizeof( FT_MetaClassRec ),
    (FT_Object_InitFunc)  ft_metaclass_init,
    (FT_Object_DoneFunc)  ft_metaclass_done,

    sizeof( FT_ClassRec ),
    (FT_Object_InitFunc)  ft_class_init,
    (FT_Object_DoneFunc)  ft_class_done
  };














  FT_BASE_DEF( FT_Int )
  ft_object_check( FT_Pointer  obj )
  {
    return FT_OBJECT_CHECK(obj);
  }


  FT_BASE_DEF( FT_Int )
  ft_object_is_a( FT_Pointer  obj,
                  FT_Class    clazz )
  {
    if ( FT_OBJECT_CHECK(obj) )
    {
      FT_Object  o = FT_OBJECT(obj);
      FT_Class   c = FT_OBJECT__CLASS(obj);

      do
      {
        if ( c == clazz )
          return 1;

        c = c->super;
      }
      while ( c == NULL );

      return (clazz == NULL);
    }
  }


 /* the cleanup routine for all objects */
  static void
  ft_object_cleanup( FT_Object  object )
  {
    FT_Memory  memory = FT_OBJECT__MEMORY(object);
    FT_Class   clazz  = FT_OBJECT__CLASS(object);

    if ( clazz->obj_done )
      clazz->obj_done( object );

    FT_FREE( object );
  }


  FT_BASE_DEF( FT_Object )
  ft_object_new( FT_Class    clazz,
                 FT_Pointer  init_data )
  {
    FT_Memory  memory;
    FT_Object  obj;


    FT_ASSERT_IS_CLASS(clazz);

    memory         = FT_CLASS__MEMORY(clazz);
    obj            = ft_mem_alloc( clazz->obj_size, memory );
    obj->clazz     = clazz;
    obj->ref_count = 1;

    if ( clazz->obj_init )
    {
      FT_CleanupStack  stack = FT_MEMORY__CLEANUP(memory);


      ft_cleanup_push( stack, obj, (FT_CleanupFunc) ft_object_cleanup, NULL );

      clazz->obj_init( obj, init_data );

      ft_cleanup_pop( stack, obj, 0 );
    }
    return obj;
  }



  FT_BASE_DEF( void )
  ft_object_create( FT_Object  *pobject,
                    FT_Class    clazz,
                    FT_Pointer  init_data )
  {
    FT_Memory  memory;
    FT_Object  obj;

    FT_ASSERT_IS_CLASS(clazz);

    memory         = FT_CLASS__MEMORY(memory);
    obj            = ft_mem_alloc( clazz->obj_size, memory );
    obj->clazz     = clazz;
    obj->ref_count = 1;
    *pobject       = obj;

    if ( clazz->obj_init )
      clazz->obj_init( obj, init_data );
  }


  FT_BASE_DEF( FT_Class )
  ft_class_find_by_type( FT_Type    type,
                         FT_Memory  memory )
  {
  }


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

