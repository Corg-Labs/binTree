# Binary Search Tree Visualiser in C

An **interactive BST** that lets you insert, delete, and search nodes while rendering the tree **sideways in your terminal** with Unicode branch connectors and ANSI colour highlights.

Written in pure C with no external dependencies. Part of the Corg-Labs collection.

---

# Features
- Insert, delete, and search with live visual feedback
- Sideways tree rendering: right subtree at top, root in middle, left subtree at bottom
- Unicode box-drawing branch connectors (`┌────`, `└────`, `│`)
- ANSI colour coding: root in cyan, found/highlighted node in green
- Inorder traversal printed below the tree on every redraw
- Duplicate insertion silently ignored
- Two-child delete uses the in-order successor strategy
- Node counter displayed after every operation

---

# Tutorial

## 1. The Node Structure

Every element in the tree is a `Node` holding an integer value and two child pointers.

```c
typedef struct Node {
    int val;
    struct Node *left, *right;
} Node;
```

A global `root` pointer starts as `NULL`. A global `highlight` integer tracks the most recently searched value so the printer can colour it green.

```c
static Node *root      = NULL;
static int   n_nodes   = 0;
static int   highlight = -999999;
```

## 2. Allocating a Node

`mknode` wraps `calloc` so both child pointers begin as `NULL` automatically.

```c
static Node *mknode(int v) {
    Node *n = calloc(1, sizeof *n);
    if (!n) { perror("calloc"); exit(1); }
    n->val = v;
    return n;
}
```

## 3. BST Insert Algorithm

Insertion is a classic recursive descent. If the slot is empty a new node is placed there; otherwise we go left for smaller values and right for larger ones. Duplicates are ignored — the `inserted` flag tells the caller whether anything actually changed.

```c
static Node *bst_insert(Node *n, int v, int *inserted) {
    if (!n) { *inserted = 1; n_nodes++; return mknode(v); }
    if      (v < n->val) n->left  = bst_insert(n->left,  v, inserted);
    else if (v > n->val) n->right = bst_insert(n->right, v, inserted);
    return n;   /* duplicate: return unchanged */
}
```

Usage: `root = bst_insert(root, value, &inserted_flag);`

## 4. BST Delete — the Three Cases

Deletion handles three situations: the node has no children (just free it), one child (replace with that child), or two children (replace value with in-order successor, then delete the successor).

```c
static Node *bst_delete(Node *n, int v, int *deleted) {
    if (!n) return NULL;
    if      (v < n->val) n->left  = bst_delete(n->left,  v, deleted);
    else if (v > n->val) n->right = bst_delete(n->right, v, deleted);
    else {
        *deleted = 1; n_nodes--;
        if (!n->left)  { Node *r = n->right; free(n); return r; }
        if (!n->right) { Node *l = n->left;  free(n); return l; }
        /* Two children: find in-order successor (leftmost in right subtree) */
        Node *s = min_right(n->right);
        n->val   = s->val;
        int dummy = 0;
        n->right = bst_delete(n->right, s->val, &dummy);
        n_nodes++;   /* compensate: inner delete decremented again */
    }
    return n;
}
```

## 5. Sideways Tree Printing with Recursion

The printer visits the right subtree first (which prints at the top of the screen), then the current node, then the left subtree (bottom). The `prefix` string accumulates the vertical guide lines for ancestors.

```c
static void print_tree(const Node *n,
                        char *prefix, size_t pfx_cap,
                        int is_right_child, int depth)
{
    if (!n) return;

    /* recurse right (upper half of screen) */
    /* ... extend prefix with "│     " if is_right_child, else "      " */
    print_tree(n->right, p2, pfx_cap, 1, depth + 1);

    /* print this node */
    const char *branch = (depth == 0)       ? "" :
                         (is_right_child)   ? "┌──── " : "└──── ";
    const char *col = (n->val == highlight) ? GREEN :
                      (depth == 0)          ? CYAN  : "";
    printf("%s%s%s%d\n", prefix, branch, col, n->val);

    /* recurse left (lower half of screen) */
    print_tree(n->left, p2, pfx_cap, 0, depth + 1);
}
```

The key insight: `┌────` marks right children (they appear *above* the parent in the terminal) and `└────` marks left children (they appear *below*).

## 6. ANSI Colour Codes

ANSI escape codes are plain string literals prepended to output. `RST` resets all attributes.

```c
#define RST    "\033[0m"
#define BOLD   "\033[1m"
#define DIM    "\033[2m"
#define CYAN   "\033[1;36m"
#define GREEN  "\033[1;32m"
#define YELLOW "\033[1;33m"
#define RED    "\033[1;31m"
```

To colour a number green: `printf("%s%d%s", GREEN, val, RST);`

## 7. Inorder Traversal

Inorder (left → root → right) visits nodes in sorted order. This is printed as a flat list below the tree diagram.

```c
static void inorder(const Node *n, int *first) {
    if (!n) return;
    inorder(n->left, first);
    if (!*first) printf(DIM " → " RST);
    printf("%s%d" RST,
           (n->val == highlight) ? GREEN : YELLOW, n->val);
    *first = 0;
    inorder(n->right, first);
}
```

## 8. The Main REPL Loop

The main loop reads a line, parses the command word, dispatches, then calls `redraw()` to refresh the tree display.

```c
while (1) {
    printf(CYAN "bst> " RST);
    fflush(stdout);
    if (!fgets(line, sizeof line, stdin)) break;
    trim(line);

    if (strncmp(line, "insert", 6) == 0) {
        int v, ins = 0;
        sscanf(line + 6, "%d", &v);
        root = bst_insert(root, v, &ins);
    } else if (strncmp(line, "delete", 6) == 0) { /* ... */ }
      else if (strncmp(line, "search", 6) == 0) { /* ... */ }

    redraw();
}
```

---

# Build
```
gcc bintree.c -o bintree
```

# Run
```
./bintree
```

# Controls

| Command | Action |
|---|---|
| `insert N` | Insert integer N |
| `delete N` | Delete integer N |
| `search N` | Highlight N in green |
| `inorder` | Print sorted traversal |
| `clear` | Remove all nodes |
| `quit` / `q` | Exit and free memory |

---

# Concepts Practiced
- Binary search tree insert, delete (3 cases), search
- Recursive tree traversal (inorder, preorder implicitly)
- In-order successor strategy for two-child deletion
- Recursive sideways terminal rendering with dynamic prefix strings
- ANSI escape codes for terminal colour and formatting
- Manual heap memory management with `calloc` / `free`

---

# Dependencies
Standard C libraries only: `stdio.h`, `stdlib.h`, `string.h`, `ctype.h`
