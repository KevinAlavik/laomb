#include <rbtree.h>
#include <string.h>

struct rb_node *rb_new_node(int key) {
    struct rb_node *node = (struct rb_node *)kmalloc(sizeof(struct rb_node));
    node->key = key;
    node->color = RED;
    node->left = node->right = node->parent = nullptr;
    return node;
}

void rb_rotate_left(struct rb_tree *tree, struct rb_node *x) {
    struct rb_node *y = x->right;
    x->right = y->left;
    if (y->left != nullptr)
        y->left->parent = x;
    y->parent = x->parent;
    if (x->parent == nullptr)
        tree->root = y;
    else if (x == x->parent->left)
        x->parent->left = y;
    else
        x->parent->right = y;
    y->left = x;
    x->parent = y;
}

void rb_rotate_right(struct rb_tree *tree, struct rb_node *y) {
    struct rb_node *x = y->left;
    y->left = x->right;
    if (x->right != nullptr)
        x->right->parent = y;
    x->parent = y->parent;
    if (y->parent == nullptr)
        tree->root = x;
    else if (y == y->parent->right)
        y->parent->right = x;
    else
        y->parent->left = x;
    x->right = y;
    y->parent = x;
}

void rb_insert_fixup(struct rb_tree *tree, struct rb_node *z) {
    while (z->parent != nullptr && z->parent->color == RED) {
        if (z->parent == z->parent->parent->left) {
            struct rb_node *y = z->parent->parent->right;
            if (y != nullptr && y->color == RED) {
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->right) {
                    z = z->parent;
                    rb_rotate_left(tree, z);
                }
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                rb_rotate_right(tree, z->parent->parent);
            }
        } else {
            struct rb_node *y = z->parent->parent->left;
            if (y != nullptr && y->color == RED) {
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->left) {
                    z = z->parent;
                    rb_rotate_right(tree, z);
                }
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                rb_rotate_left(tree, z->parent->parent);
            }
        }
    }
    tree->root->color = BLACK;
}

void rb_insert(struct rb_tree *tree, int key) {
    struct rb_node *z = rb_new_node(key);
    struct rb_node *y = nullptr;
    struct rb_node *x = tree->root;
    
    while (x != nullptr) {
        y = x;
        if (z->key < x->key)
            x = x->left;
        else
            x = x->right;
    }
    z->parent = y;
    
    if (y == nullptr)
        tree->root = z;
    else if (z->key < y->key)
        y->left = z;
    else
        y->right = z;

    rb_insert_fixup(tree, z);
}

void rb_inorder(struct rb_node *root) {
    if (root != nullptr) {
        rb_inorder(root->left);
        kprintf("%d ", root->key);
        rb_inorder(root->right);
    }
}

struct rb_tree *create_tree() {
    struct rb_tree *tree = (struct rb_tree *)kmalloc(sizeof(struct rb_tree));
    tree->root = nullptr;
    return tree;
}

struct rb_node *rb_search(struct rb_tree *tree, int key) {
    struct rb_node *node = tree->root;
    while (node != nullptr && node->key != key) {
        if (key < node->key)
            node = node->left;
        else
            node = node->right;
    }
    return node;
}

struct rb_node *rb_minimum(struct rb_node *node) {
    while (node->left != nullptr)
        node = node->left;
    return node;
}

void rb_delete_fixup(struct rb_tree *tree, struct rb_node *x) {
    while (x != tree->root && (x == nullptr || x->color == BLACK)) {
        if (x == x->parent->left) {
            struct rb_node *w = x->parent->right;
            if (w == nullptr) {
                break;
            }
            if (w->color == RED) {
                w->color = BLACK;
                x->parent->color = RED;
                rb_rotate_left(tree, x->parent);
                w = x->parent->right;
            }
            if ((w->left == nullptr || w->left->color == BLACK) &&
                (w->right == nullptr || w->right->color == BLACK)) {
                w->color = RED;
                x = x->parent;
            } else {
                if (w->right == nullptr || w->right->color == BLACK) {
                    if (w->left != nullptr)
                        w->left->color = BLACK;
                    w->color = RED;
                    rb_rotate_right(tree, w);
                    w = x->parent->right;
                }
                w->color = x->parent->color;
                x->parent->color = BLACK;
                if (w->right != nullptr)
                    w->right->color = BLACK;
                rb_rotate_left(tree, x->parent);
                x = tree->root;
            }
        } else {
            struct rb_node *w = x->parent->left;
            if (w->color == RED) {
                w->color = BLACK;
                x->parent->color = RED;
                rb_rotate_right(tree, x->parent);
                w = x->parent->left;
            }
            if ((w->right == nullptr || w->right->color == BLACK) &&
                (w->left == nullptr || w->left->color == BLACK)) {
                w->color = RED;
                x = x->parent;
            } else {
                if (w->left == nullptr || w->left->color == BLACK) {
                    if (w->right != nullptr)
                        w->right->color = BLACK;
                    w->color = RED;
                    rb_rotate_left(tree, w);
                    w = x->parent->left;
                }
                w->color = x->parent->color;
                x->parent->color = BLACK;
                if (w->left != nullptr)
                    w->left->color = BLACK;
                rb_rotate_right(tree, x->parent);
                x = tree->root;
            }
        }
    }
    if (x != nullptr)
        x->color = BLACK;
}


void rb_delete(struct rb_tree *tree, int key) {
    struct rb_node *z = rb_search(tree, key);
    if (z == nullptr)
        return;

    struct rb_node *y = z;
    struct rb_node *x = nullptr;
    enum rb_color y_original_color = y->color;

    if (z->left == nullptr) {
        x = z->right;
        if (x != nullptr)
            x->parent = z->parent;
        if (z->parent == nullptr) {
            tree->root = x;
        } else if (z == z->parent->left) {
            z->parent->left = x;
        } else {
            z->parent->right = x;
        }
        kfree(z);
    } else if (z->right == nullptr) {
        x = z->left;
        if (x != nullptr)
            x->parent = z->parent;
        if (z->parent == nullptr) {
            tree->root = x;
        } else if (z == z->parent->left) {
            z->parent->left = x;
        } else {
            z->parent->right = x;
        }
        kfree(z);
    } else {
        y = rb_minimum(z->right);
        y_original_color = y->color;
        x = y->right;

        if (y->parent != z) {
            if (x != nullptr)
                x->parent = y->parent;
            y->parent->left = x;
            y->right = z->right;
            z->right->parent = y;
        }
        
        if (z->parent == nullptr) {
            tree->root = y;
        } else if (z == z->parent->left) {
            z->parent->left = y;
        } else {
            z->parent->right = y;
        }

        y->parent = z->parent;
        y->left = z->left;
        z->left->parent = y;
        y->color = z->color;
        kfree(z);
    }

    if (y_original_color == BLACK && x != nullptr) {
        rb_delete_fixup(tree, x);
    }
}

