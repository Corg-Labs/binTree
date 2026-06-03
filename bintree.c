/*
 * bintree.c — Interactive Binary Search Tree Visualiser
 * Corg-Labs | gcc bintree.c -o bintree
 *
 * Commands: insert N | delete N | search N | inorder | clear | quit
 *
 * The tree is drawn sideways:
 *   right subtree appears at the top of the terminal,
 *   root in the middle, left subtree at the bottom.
 * Branches use Unicode box-drawing characters.
 * ANSI colours: root = cyan, found/highlighted = green, others = white.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ── ANSI colour macros ─────────────────────────────────────────────────── */
#define RST    "\033[0m"
#define BOLD   "\033[1m"
#define DIM    "\033[2m"
#define CYAN   "\033[1;36m"
#define GREEN  "\033[1;32m"
#define YELLOW "\033[1;33m"
#define RED    "\033[1;31m"

/* ── Node ───────────────────────────────────────────────────────────────── */
typedef struct Node {
    int val;
    struct Node *left, *right;
} Node;

static Node *root       = NULL;
static int   n_nodes    = 0;
static int   highlight  = -999999;   /* value lit green after search */

/* ── Memory ─────────────────────────────────────────────────────────────── */
static Node *mknode(int v) {
    Node *n = calloc(1, sizeof *n);
    if (!n) { perror("calloc"); exit(1); }
    n->val = v;
    return n;
}

static void free_tree(Node *n) {
    if (!n) return;
    free_tree(n->left);
    free_tree(n->right);
    free(n);
}

/* ── BST insert ─────────────────────────────────────────────────────────── */
static Node *bst_insert(Node *n, int v, int *inserted) {
    if (!n) { *inserted = 1; n_nodes++; return mknode(v); }
    if      (v < n->val) n->left  = bst_insert(n->left,  v, inserted);
    else if (v > n->val) n->right = bst_insert(n->right, v, inserted);
    return n;
}

/* ── BST delete ─────────────────────────────────────────────────────────── */
static Node *min_right(Node *n) { while (n->left) n = n->left; return n; }

static Node *bst_delete(Node *n, int v, int *deleted) {
    if (!n) return NULL;
    if (v < n->val) {
        n->left  = bst_delete(n->left,  v, deleted);
    } else if (v > n->val) {
        n->right = bst_delete(n->right, v, deleted);
    } else {
        *deleted = 1;
        n_nodes--;
        if (!n->left)  { Node *r = n->right; free(n); return r; }
        if (!n->right) { Node *l = n->left;  free(n); return l; }
        Node *s = min_right(n->right);
        n->val   = s->val;
        int dummy = 0;
        n->right = bst_delete(n->right, s->val, &dummy);
        n_nodes++;    /* compensate: delete above decremented again */
    }
    return n;
}

/* ── BST search ─────────────────────────────────────────────────────────── */
static int bst_search(Node *n, int v) {
    if (!n) return 0;
    if (v == n->val) return 1;
    return v < n->val ? bst_search(n->left, v) : bst_search(n->right, v);
}

/* ── Sideways tree printer ──────────────────────────────────────────────── *
 *  Right child printed first (= top of screen).
 *  prefix        : already-printed column guides for ancestor lines
 *  is_right_child: whether this node is a right child (affects connector)
 *  depth         : 0 for root
 * ─────────────────────────────────────────────────────────────────────── */
static void print_tree(const Node *n,
                        char *prefix, size_t pfx_cap,
                        int is_right_child, int depth)
{
    if (!n) return;

    /* ── right subtree (upper half) ──────────────────────────────────── */
    {
        size_t plen = strlen(prefix);
        char  *p2   = malloc(pfx_cap);
        memcpy(p2, prefix, plen + 1);
        if (depth == 0)
            strncat(p2, "      ", pfx_cap - plen - 1);
        else
            strncat(p2, is_right_child ? "│     " : "      ",
                    pfx_cap - plen - 1);
        print_tree(n->right, p2, pfx_cap, 1, depth + 1);
        free(p2);
    }

    /* ── this node ───────────────────────────────────────────────────── */
    {
        const char *branch;
        if      (depth == 0)        branch = "";
        else if (is_right_child)    branch = "┌──── ";
        else                        branch = "└──── ";

        const char *col = (n->val == highlight) ? GREEN :
                          (depth == 0)          ? CYAN  : "";
        const char *bld = (depth == 0)          ? BOLD  : "";

        printf("%s" DIM "%s" RST "%s%s%d" RST "\n",
               prefix, branch, col, bld, n->val);
    }

    /* ── left subtree (lower half) ───────────────────────────────────── */
    {
        size_t plen = strlen(prefix);
        char  *p2   = malloc(pfx_cap);
        memcpy(p2, prefix, plen + 1);
        if (depth == 0)
            strncat(p2, "      ", pfx_cap - plen - 1);
        else
            strncat(p2, is_right_child ? "      " : "│     ",
                    pfx_cap - plen - 1);
        print_tree(n->left, p2, pfx_cap, 0, depth + 1);
        free(p2);
    }
}

/* ── Inorder print ──────────────────────────────────────────────────────── */
static void inorder(const Node *n, int *first) {
    if (!n) return;
    inorder(n->left, first);
    if (!*first) printf(DIM " → " RST);
    printf("%s%d" RST,
           (n->val == highlight) ? GREEN : YELLOW,
           n->val);
    *first = 0;
    inorder(n->right, first);
}

/* ── Full redraw ────────────────────────────────────────────────────────── */
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

/* ── Banner ─────────────────────────────────────────────────────────────── */
static void banner(void) {
    printf(CYAN BOLD
        "╔══════════════════════════════════════════════════╗\n"
        "║        Binary Search Tree Visualiser            ║\n"
        "╚══════════════════════════════════════════════════╝\n"
        RST);
    printf(DIM
        "  Commands:\n"
        "    insert N   — insert value N\n"
        "    delete N   — delete value N\n"
        "    search N   — highlight value N\n"
        "    inorder    — print sorted order\n"
        "    clear      — remove all nodes\n"
        "    quit       — exit\n"
        RST "\n");
}

/* ── String trim ────────────────────────────────────────────────────────── */
static void trim(char *s) {
    int i = 0;
    while (s[i] && isspace((unsigned char)s[i])) i++;
    if (i) memmove(s, s + i, strlen(s) - i + 1);
    int n = (int)strlen(s);
    while (n > 0 && isspace((unsigned char)s[n-1])) s[--n] = '\0';
}

/* ── Main ───────────────────────────────────────────────────────────────── */
int main(void) {
    banner();
    redraw();

    char line[256];
    while (1) {
        printf(CYAN "bst> " RST);
        fflush(stdout);

        if (!fgets(line, sizeof line, stdin)) break;
        trim(line);
        if (!*line) { redraw(); continue; }

        highlight = -999999;

        /* ── insert ── */
        if (strncmp(line, "insert", 6) == 0 && (line[6] == ' ' || !line[6])) {
            int v, ins = 0;
            if (sscanf(line + 6, "%d", &v) == 1) {
                root = bst_insert(root, v, &ins);
                if (ins) printf(GREEN "  ✓ Inserted %d\n" RST, v);
                else     printf(YELLOW "  (duplicate %d ignored)\n" RST, v);
            } else {
                printf(RED "  Usage: insert <number>\n" RST);
            }

        /* ── delete ── */
        } else if (strncmp(line, "delete", 6) == 0 && (line[6] == ' ' || !line[6])) {
            int v, del = 0;
            if (sscanf(line + 6, "%d", &v) == 1) {
                root = bst_delete(root, v, &del);
                if (del) printf(YELLOW "  ✓ Deleted %d\n" RST, v);
                else     printf(RED "  %d not in tree\n" RST, v);
            } else {
                printf(RED "  Usage: delete <number>\n" RST);
            }

        /* ── search ── */
        } else if (strncmp(line, "search", 6) == 0 && (line[6] == ' ' || !line[6])) {
            int v;
            if (sscanf(line + 6, "%d", &v) == 1) {
                if (bst_search(root, v)) {
                    highlight = v;
                    printf(GREEN "  ✓ Found %d (shown in green)\n" RST, v);
                } else {
                    printf(RED "  %d not found\n" RST, v);
                }
            } else {
                printf(RED "  Usage: search <number>\n" RST);
            }

        /* ── inorder ── */
        } else if (strcmp(line, "inorder") == 0) {
            printf(BOLD "  Sorted: " RST);
            int first = 1;
            inorder(root, &first);
            printf("\n\n");
            continue;

        /* ── clear ── */
        } else if (strcmp(line, "clear") == 0) {
            free_tree(root);
            root = NULL; n_nodes = 0;
            printf(YELLOW "  Tree cleared.\n" RST);

        /* ── quit ── */
        } else if (strcmp(line, "quit") == 0 || strcmp(line, "q") == 0) {
            printf(DIM "  Goodbye!\n" RST);
            free_tree(root);
            return 0;

        } else {
            printf(RED "  Unknown command.\n" RST);
        }

        redraw();
    }

    free_tree(root);
    return 0;
}
