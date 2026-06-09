# Binary Search Tree Visualiser in C

An interactive BST that lets you insert, delete, and search nodes while rendering the tree sideways in your terminal with Unicode branch connectors and ANSI colour highlights.

Written in pure C with no external dependencies.

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

# How It Works

The program maintains a binary search tree in memory. After every command, it redraws the entire tree sideways using a recursive printer that visits right subtree first (top of screen), then the node itself, then the left subtree (bottom of screen). Unicode box-drawing characters connect parent-child lines, and ANSI escape codes apply colour to the root and any highlighted search result.

---

# Tutorial / Data Structures & Algorithms

## 1. The Node Structure

Every element in the tree is a `Node` holding an integer value and two child pointers:

```c
typedef struct Node {
    int val;
    struct Node *left, *right;
} Node;
```

A global `root` pointer starts as `NULL`. A global `highlight` integer tracks the most recently searched value so the printer can colour it green:

```c
static Node *root      = NULL;
static int   n_nodes   = 0;
static int   highlight = -999999;
```

---

## 2. Node Allocation

`mknode` wraps `calloc` — zero-initialised memory means both child pointers begin as `NULL` automatically:

```c
static Node *mknode(int v) {
    Node *n = calloc(1, sizeof *n);
    if (!n) { perror("calloc"); exit(1); }
    n->val = v;
    return n;
}
```

`free_tree` does a post-order traversal to deallocate every node:

```c
static void free_tree(Node *n) {
    if (!n) return;
    free_tree(n->left);
    free_tree(n->right);
    free(n);
}
```

---

## 3. BST Insert

Classic recursive descent. If the slot is empty, place a new node; otherwise go left for smaller values and right for larger ones. Duplicates are ignored — the `inserted` flag tells the caller whether anything changed:

```c
static Node *bst_insert(Node *n, int v, int *inserted) {
    if (!n) { *inserted = 1; n_nodes++; return mknode(v); }
    if      (v < n->val) n->left  = bst_insert(n->left,  v, inserted);
    else if (v > n->val) n->right = bst_insert(n->right, v, inserted);
    return n;
}
```

Usage: `root = bst_insert(root, 42, &flag);`

---

## 4. BST Delete — Three Cases

Deletion handles three structural cases:

1. **Leaf (no children):** free the node, return NULL
2. **One child:** free the node, return the surviving child
3. **Two children:** replace the node's value with its in-order successor (leftmost node in the right subtree), then recursively delete the successor from the right subtree

```c
static Node *min_right(Node *n) { while (n->left) n = n->left; return n; }

static Node *bst_delete(Node *n, int v, int *deleted) {
    if (!n) return NULL;
    if      (v < n->val) n->left  = bst_delete(n->left,  v, deleted);
    else if (v > n->val) n->right = bst_delete(n->right, v, deleted);
    else {
        *deleted = 1; n_nodes--;
        if (!n->left)  { Node *r = n->right; free(n); return r; }
        if (!n->right) { Node *l = n->left;  free(n); return l; }
        Node *s = min_right(n->right);
        n->val   = s->val;
        int dummy = 0;
        n->right = bst_delete(n->right, s->val, &dummy);
        n_nodes++;   /* compensate: inner delete decremented again */
    }
    return n;
}
```

---

## 5. BST Search

Recursive boolean descent — returns 1 if the value exists in the tree, 0 otherwise:

```c
static int bst_search(Node *n, int v) {
    if (!n) return 0;
    if (v == n->val) return 1;
    return v < n->val ? bst_search(n->left, v) : bst_search(n->right, v);
}
```

On a successful search, the global `highlight` is set so the next redraw colours that node green.

---

## 6. Sideways Tree Printer

The printer uses a reverse pre-order traversal (right → root → left) to render the tree sideways. Since the terminal scrolls top-to-bottom, visiting the right subtree first places it at the top, the root in the middle, and the left subtree at the bottom:

```
       ┌──── 90       ← right child (printed first)
       │
  ┌──── 50            ← root
  │
  │ ┌──── 30
  │ │
  └──── 20
        │
        └──── 10      ← left child (printed last)
```

The `prefix` string accumulates vertical guide lines (`│     `) for each ancestor level:

```c
static void print_tree(const Node *n,
                        char *prefix, size_t pfx_cap,
                        int is_right_child, int depth)
{
    if (!n) return;

    /* recurse right (upper half of screen) */
    {
        char *p2 = malloc(pfx_cap);
        memcpy(p2, prefix, strlen(prefix) + 1);
        strncat(p2, is_right_child ? "│     " : "      ", pfx_cap - strlen(p2) - 1);
        print_tree(n->right, p2, pfx_cap, 1, depth + 1);
        free(p2);
    }

    /* print this node */
    const char *branch = (depth == 0)       ? "" :
                         (is_right_child)   ? "┌──── " : "└──── ";
    const char *col    = (n->val == highlight) ? GREEN :
                         (depth == 0)          ? CYAN  : "";
    printf("%s" DIM "%s" RST "%s%d" RST "\n", prefix, branch, col, n->val);

    /* recurse left (lower half of screen) */
    {
        char *p2 = malloc(pfx_cap);
        memcpy(p2, prefix, strlen(prefix) + 1);
        strncat(p2, is_right_child ? "      " : "│     ", pfx_cap - strlen(p2) - 1);
        print_tree(n->left, p2, pfx_cap, 0, depth + 1);
        free(p2);
    }
}
```

Key insight: `┌────` marks right children (they appear *above* the parent) and `└────` marks left children (they appear *below*). The dimmed `│` characters form continuous vertical lines connecting ancestors.

---

## 7. ANSI Colour Macros

ANSI escape codes are plain string literals prepended to output. `RST` resets all attributes back to terminal defaults:

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

---

## 8. Inorder Traversal

Inorder (left → root → right) visits nodes in sorted order. This is printed as a flat list below the tree diagram after every redraw:

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

The `first` flag suppresses the arrow before the initial element. Highlighted values appear in green; all others in yellow.

---

## 9. The Main REPL Loop

The main loop reads a line, parses the command word, dispatches to the appropriate handler, then calls `redraw()` to refresh the tree display:

```c
while (1) {
    printf(CYAN "bst> " RST);
    fflush(stdout);
    if (!fgets(line, sizeof line, stdin)) break;
    trim(line);

    if (strncmp(line, "insert", 6) == 0) {
        /* parse value, call bst_insert, print result */
    } else if (strncmp(line, "delete", 6) == 0) {
        /* parse value, call bst_delete, print result */
    } else if (strncmp(line, "search", 6) == 0) {
        /* parse value, call bst_search, set highlight */
    } else if (strcmp(line, "inorder") == 0) {
        /* print sorted traversal without redrawing tree */
    } else if (strcmp(line, "clear") == 0) {
        /* free all nodes, reset root to NULL */
    } else if (strcmp(line, "quit") == 0 || strcmp(line, "q") == 0) {
        /* free tree, exit */
    }

    highlight = -999999;  /* clear search highlight */
    redraw();
}
```

Every command path (except `inorder` and `quit`) falls through to `redraw()`, which prints the tree diagram, the inorder list, and the node count.

---

## 10. File I / Redraw Cycle

`redraw()` ties everything together — it prints the tree via `print_tree`, then the inorder list via `inorder`, then the node count:

```c
static void redraw(void) {
    printf("\n");
    if (!root) {
        printf(DIM "  (tree is empty — try: insert 50)\n" RST);
    } else {
        char prefix[1024] = "  ";
        print_tree(root, prefix, sizeof prefix, 0, 0);
    }
    printf("\n" BOLD "  Inorder:  " RST);
    if (root) {
        int first = 1;
        inorder(root, &first);
    } else {
        printf(DIM "(none)" RST);
    }
    printf("  " DIM "[%d node%s]" RST "\n\n",
           n_nodes, n_nodes == 1 ? "" : "s");
}
```

---

# Build

```
gcc bintree.c -o bintree
```

No flags or external libraries needed.

---

# Run

```
./bintree
```

| Command | Action |
|---------|--------|
| `insert N` | Insert integer N into the tree |
| `delete N` | Delete integer N from the tree |
| `search N` | Highlight N in green on the next redraw |
| `inorder` | Print the sorted traversal (no tree redraw) |
| `clear` | Remove every node from the tree |
| `quit` / `q` | Free all memory and exit |

Press `Ctrl+D` to quit (EOF).

---

# Customizing

Edit the colour macros at the top of `bintree.c` to change the theme:

- `CYAN` — root node colour
- `GREEN` — highlighted / searched node colour
- `YELLOW` — inorder traversal text
- `RED` — error messages
- `DIM` — branch lines and metadata

The highlight sentinel `-999999` (reset value for `highlight`) can be changed if your tree contains values near that range.

---

# Concepts Practiced

- Binary search tree property: left < root < right
- Recursive BST insertion with duplicate rejection
- Recursive BST deletion with three structural cases
- In-order successor selection for two-child deletion
- Recursive tree traversal (inorder, preorder, postorder)
- Sideways terminal rendering with dynamic prefix strings
- Unicode box-drawing characters for tree branches
- ANSI escape-code colouring and text formatting
- Manual heap memory management with `calloc` / `free`
- REPL loop with string parsing and command dispatch

---

# Dependencies

Standard C libraries only:

- `stdio.h` — `printf`, `fgets`, `sscanf`, `fflush`, `perror`
- `stdlib.h` — `calloc`, `free`, `exit`
- `string.h` — `strlen`, `strcmp`, `strncmp`, `memcpy`, `memmove`, `strncat`
- `ctype.h` — `isspace`

No external libraries or graphics engines required.
