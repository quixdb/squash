/* Copyright (c) 2005 Ian Piumarta
 * Copyright (c) 2013 Evan Nemerson
 * 
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the 'Software'), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, and/or sell copies of the
 * Software, and to permit persons to whom the Software is furnished
 * to do so, provided that the above copyright notice(s) and this
 * permission notice appear in all copies of the Software and that
 * both the above copyright notice(s) and this permission notice
 * appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS'.  USE ENTIRELY AT YOUR OWN RISK.
 *
 * Authors:
 *   Ian Piumarta
 *   Evan Nemerson <evan@coeus-group.com>
 */

/* Copied from http://piumarta.com/software/tree/ and modified a bit,
 * mainly:
 *
 *  * Moved everything into SQUASH_TREE_*
 *  * Added the prototype stuff (for -Wmissing-prototypes)
 *  * Made the functions static inline
 *  * Added SQUASH_TREE_ENTRY_INIT
 *
 *********************************************************************
 *
 * This file defines an AVL balanced binary tree [Georgii
 * M. Adelson-Velskii and Evgenii M. Landis, 'An algorithm for the
 * organization of information', Doklady Akademii Nauk SSSR,
 * 146:263-266, 1962 (Russian).  Also in Myron J. Ricci (trans.),
 * Soviet Math, 3:1259-1263, 1962 (English)].
 * 
 * An AVL tree is headed by pointers to the root node and to a
 * function defining the ordering relation between nodes.  Each node
 * contains an arbitrary payload plus three fields per tree entry: the
 * depth of the subtree for which it forms the root and two pointers
 * to child nodes (singly-linked for minimum space, at the expense of
 * direct access to the parent node given a pointer to one of the
 * children).  The tree is rebalanced after every insertion or
 * removal.  The tree may be traversed in two directions: forward
 * (in-order left-to-right) and reverse (in-order, right-to-left).
 * 
 * Because of the recursive nature of many of the operations on trees
 * it is necessary to define a number of helper functions for each
 * type of tree node.  The macro TREE_DEFINE(node_tag, entry_name)
 * defines these functions with unique names according to the
 * node_tag.  This macro should be invoked, thereby defining the
 * necessary functions, once per node tag in the program.
 * 
 * For details on the use of these macros, see the tree(3) manual page
 * (which is not distributed with SQUASH--you can get a copy from
 * http://piumarta.com/software/tree/).
 */

#ifndef __SQUASH_TREE_H__
#define __SQUASH_TREE_H__

#if !defined (SQUASH_COMPILATION)
#error "This is internal API; you cannot use it."
#endif

#define SQUASH_TREE_DELTA_MAX 1

#define SQUASH_TREE_ENTRY(type)                 \
  struct {                                      \
    struct type *avl_left;                      \
    struct type *avl_right;                     \
    int          avl_height;                    \
  }

#define SQUASH_TREE_HEAD(name, type)                    \
  struct name {                                         \
    struct type *th_root;                               \
    int  (*th_cmp)(struct type *lhs, struct type *rhs); \
  }

#define SQUASH_TREE_INITIALIZER(cmp) { 0, cmp }

#define SQUASH_TREE_DELTA(self, field)                                     \
  (( (((self)->field.avl_left)  ? (self)->field.avl_left->field.avl_height  : 0)) \
   - (((self)->field.avl_right) ? (self)->field.avl_right->field.avl_height : 0))

#define SQUASH_TREE_API static inline

/* Recursion prevents the following from being defined as macros. */

#define SQUASH_TREE_PROTOTYPES(node, field)                                \
  SQUASH_TREE_API struct node *SQUASH_TREE_BALANCE_##node##_##field(struct node *); \
  SQUASH_TREE_API struct node *SQUASH_TREE_ROTL_##node##_##field(struct node *self); \
  SQUASH_TREE_API struct node *SQUASH_TREE_ROTR_##node##_##field(struct node *self); \
  SQUASH_TREE_API struct node *SQUASH_TREE_INSERT_##node##_##field            \
  (struct node *self, struct node *elm, int (*compare)(struct node *lhs, struct node *rhs)); \
  SQUASH_TREE_API struct node *SQUASH_TREE_FIND_##node##_##field              \
  (struct node *self, struct node *elm, int (*compare)(struct node *lhs, struct node *rhs)); \
  SQUASH_TREE_API struct node *SQUASH_TREE_MOVE_RIGHT_##node##_##field(struct node *self, struct node *rhs); \
  SQUASH_TREE_API struct node *SQUASH_TREE_REMOVE_##node##_##field            \
  (struct node *self, struct node *elm, int (*compare)(struct node *lhs, struct node *rhs)); \
  SQUASH_TREE_API void SQUASH_TREE_FORWARD_APPLY_ALL_##node##_##field         \
  (struct node *self, void (*function)(struct node *node, void *data), void *data); \
  SQUASH_TREE_API void SQUASH_TREE_REVERSE_APPLY_ALL_##node##_##field         \
  (struct node *self, void (*function)(struct node *node, void *data), void *data);
  

#define SQUASH_TREE_DEFINE(node, field)                                    \
                                                                        \
  SQUASH_TREE_API struct node *SQUASH_TREE_ROTL_##node##_##field(struct node *self) \
  {                                                                     \
    struct node *r= self->field.avl_right;                              \
    self->field.avl_right= r->field.avl_left;                           \
    r->field.avl_left= SQUASH_TREE_BALANCE_##node##_##field(self);         \
      return SQUASH_TREE_BALANCE_##node##_##field(r);                      \
  }                                                                     \
                                                                        \
  SQUASH_TREE_API struct node *SQUASH_TREE_ROTR_##node##_##field(struct node *self) \
  {                                                                     \
    struct node *l= self->field.avl_left;                               \
    self->field.avl_left= l->field.avl_right;                           \
    l->field.avl_right= SQUASH_TREE_BALANCE_##node##_##field(self);        \
    return SQUASH_TREE_BALANCE_##node##_##field(l);                        \
  }                                                                     \
                                                                        \
  SQUASH_TREE_API struct node *SQUASH_TREE_BALANCE_##node##_##field(struct node *self) \
  {                                                                     \
    int delta= SQUASH_TREE_DELTA(self, field);                             \
                                                                        \
    if (delta < -SQUASH_TREE_DELTA_MAX)                                    \
      {                                                                 \
        if (SQUASH_TREE_DELTA(self->field.avl_right, field) > 0)           \
          self->field.avl_right= SQUASH_TREE_ROTR_##node##_##field(self->field.avl_right); \
        return SQUASH_TREE_ROTL_##node##_##field(self);                    \
      }                                                                 \
    else if (delta > SQUASH_TREE_DELTA_MAX)                                \
      {                                                                 \
        if (SQUASH_TREE_DELTA(self->field.avl_left, field) < 0)            \
          self->field.avl_left= SQUASH_TREE_ROTL_##node##_##field(self->field.avl_left); \
        return SQUASH_TREE_ROTR_##node##_##field(self);                    \
      }                                                                 \
    self->field.avl_height= 0;                                          \
    if (self->field.avl_left && (self->field.avl_left->field.avl_height > self->field.avl_height)) \
      self->field.avl_height= self->field.avl_left->field.avl_height;   \
    if (self->field.avl_right && (self->field.avl_right->field.avl_height > self->field.avl_height)) \
      self->field.avl_height= self->field.avl_right->field.avl_height;  \
    self->field.avl_height += 1;                                        \
    return self;                                                        \
  }                                                                     \
                                                                        \
  SQUASH_TREE_API struct node *SQUASH_TREE_INSERT_##node##_##field            \
  (struct node *self, struct node *elm, int (*compare)(struct node *lhs, struct node *rhs)) \
  {                                                                     \
    if (!self)                                                          \
      return elm;                                                       \
    if (compare(elm, self) < 0)                                         \
      self->field.avl_left= SQUASH_TREE_INSERT_##node##_##field(self->field.avl_left, elm, compare); \
    else                                                                \
      self->field.avl_right= SQUASH_TREE_INSERT_##node##_##field(self->field.avl_right, elm, compare); \
    return SQUASH_TREE_BALANCE_##node##_##field(self);                     \
  }                                                                     \
                                                                        \
  SQUASH_TREE_API struct node *SQUASH_TREE_FIND_##node##_##field              \
  (struct node *self, struct node *elm, int (*compare)(struct node *lhs, struct node *rhs)) \
  {                                                                     \
    if (!self)                                                          \
      return 0;                                                         \
    if (compare(elm, self) == 0)                                        \
      return self;                                                      \
    if (compare(elm, self) < 0)                                         \
      return SQUASH_TREE_FIND_##node##_##field(self->field.avl_left, elm, compare); \
    else                                                                \
      return SQUASH_TREE_FIND_##node##_##field(self->field.avl_right, elm, compare); \
  }                                                                     \
                                                                        \
  SQUASH_TREE_API struct node *SQUASH_TREE_MOVE_RIGHT_##node##_##field(struct node *self, struct node *rhs) \
  {                                                                     \
    if (!self)                                                          \
      return rhs;                                                       \
    self->field.avl_right= SQUASH_TREE_MOVE_RIGHT_##node##_##field(self->field.avl_right, rhs); \
      return SQUASH_TREE_BALANCE_##node##_##field(self);                   \
  }                                                                     \
                                                                        \
  SQUASH_TREE_API struct node *SQUASH_TREE_REMOVE_##node##_##field            \
  (struct node *self, struct node *elm, int (*compare)(struct node *lhs, struct node *rhs)) \
  {                                                                     \
    if (!self) return 0;                                                \
                                                                        \
    if (compare(elm, self) == 0)                                        \
      {                                                                 \
        struct node *tmp= SQUASH_TREE_MOVE_RIGHT_##node##_##field(self->field.avl_left, self->field.avl_right); \
          self->field.avl_left= 0;                                      \
          self->field.avl_right= 0;                                     \
          return tmp;                                                   \
      }                                                                 \
    if (compare(elm, self) < 0)                                         \
      self->field.avl_left= SQUASH_TREE_REMOVE_##node##_##field(self->field.avl_left, elm, compare); \
    else                                                                \
      self->field.avl_right= SQUASH_TREE_REMOVE_##node##_##field(self->field.avl_right, elm, compare); \
    return SQUASH_TREE_BALANCE_##node##_##field(self);                     \
  }                                                                     \
                                                                        \
  SQUASH_TREE_API void SQUASH_TREE_FORWARD_APPLY_ALL_##node##_##field         \
  (struct node *self, void (*function)(struct node *node, void *data), void *data) \
  {                                                                     \
    if (self)                                                           \
      {                                                                 \
        SQUASH_TREE_FORWARD_APPLY_ALL_##node##_##field(self->field.avl_left, function, data); \
        function(self, data);                                           \
        SQUASH_TREE_FORWARD_APPLY_ALL_##node##_##field(self->field.avl_right, function, data); \
      }                                                                 \
  }                                                                     \
                                                                        \
  SQUASH_TREE_API void SQUASH_TREE_REVERSE_APPLY_ALL_##node##_##field        \
  (struct node *self, void (*function)(struct node *node, void *data), void *data) \
  {                                                                     \
    if (self)                                                           \
      {                                                                 \
        SQUASH_TREE_REVERSE_APPLY_ALL_##node##_##field(self->field.avl_right, function, data); \
        function(self, data);                                           \
        SQUASH_TREE_REVERSE_APPLY_ALL_##node##_##field(self->field.avl_left, function, data); \
      }                                                                 \
  }

#define SQUASH_TREE_INSERT(head, node, field, elm)                         \
  ((head)->th_root= SQUASH_TREE_INSERT_##node##_##field((head)->th_root, (elm), (head)->th_cmp))

#define SQUASH_TREE_FIND(head, node, field, elm)                           \
  (SQUASH_TREE_FIND_##node##_##field((head)->th_root, (elm), (head)->th_cmp))

#define SQUASH_TREE_REMOVE(head, node, field, elm)                         \
  ((head)->th_root= SQUASH_TREE_REMOVE_##node##_##field((head)->th_root, (elm), (head)->th_cmp))

#define SQUASH_TREE_DEPTH(head, field)             \
  ((head)->th_root->field.avl_height)

#define SQUASH_TREE_FORWARD_APPLY(head, node, field, function, data)       \
  SQUASH_TREE_FORWARD_APPLY_ALL_##node##_##field((head)->th_root, function, data)

#define SQUASH_TREE_REVERSE_APPLY(head, node, field, function, data)       \
  SQUASH_TREE_REVERSE_APPLY_ALL_##node##_##field((head)->th_root, function, data)

#define SQUASH_TREE_INIT(head, cmp) do {           \
    (head)->th_root= 0;                         \
    (head)->th_cmp= (cmp);                      \
  } while (0)

#define SQUASH_TREE_ENTRY_INIT(field) do {         \
    field.avl_left = NULL;                      \
    field.avl_right = NULL;                     \
    field.avl_height = 0;                       \
  } while (0)

#endif /* __SQUASH_TREE_H__ */
