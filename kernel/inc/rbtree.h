#pragma once

#include <stdint.h>
#include <kheap.h>
#include <kprintf>

enum rb_color { RED, BLACK };

struct rb_node {
    int key;
    enum rb_color color;
    struct rb_node *left, *right, *parent;
};

struct rb_tree {
    struct rb_node *root;
};

struct rb_node *rb_new_node(int key);
void rb_rotate_left(struct rb_tree *tree, struct rb_node *x);
void rb_rotate_right(struct rb_tree *tree, struct rb_node *y);
void rb_insert_fixup(struct rb_tree *tree, struct rb_node *z);
void rb_insert(struct rb_tree *tree, int key);
void rb_inorder(struct rb_node *root);
struct rb_node *rb_search(struct rb_tree *tree, int key);

struct rb_tree *create_tree();

void rb_delete(struct rb_tree *tree, int key);
void rb_delete_fixup(struct rb_tree *tree, struct rb_node *x);
struct rb_node *rb_minimum(struct rb_node *node);
