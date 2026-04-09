#define RBT_COLOR_RED 0
#define RBT_COLOR_BLACK 1

#include <assert.h>

typedef struct minimorf_cached_word *node;

auto void insert_case1(node);
auto void insert_case2(node);
auto void insert_case3(node);
auto void insert_case4(node);
auto void insert_case5(node);
#if 0
auto void delete_case1(node);
auto void delete_case2(node);
auto void delete_case3(node);
auto void delete_case4(node);
auto void delete_case5(node);
auto void delete_case6(node);
auto node maximum_node(node);
#endif

node grandparent(node n) {
    if (!n || !n->parent || !n->parent->parent) return NULL;
    return n->parent->parent;
}
node sibling(node n) {
    if (!n || !n->parent) return NULL;
    if (n == n->parent->left)
        return n->parent->right;
    else
        return n->parent->left;
}
node uncle(node n) {
    return sibling(n->parent);
}

node lookup_node( char* key) {
    node n = md->root;
    while (n != NULL) {
        int comp_result = strcmp(key, md->memo+n->data);
        if (comp_result == 0) {
            return n;
        } else if (comp_result < 0) {
            n = n->left;
        } else {
            n = n->right;
        }
    }
    return n;
}

void replace_node( node oldn, node newn) {
    if (oldn->parent == NULL) {
        md->root = newn;
    } else {
        if (oldn == oldn->parent->left)
            oldn->parent->left = newn;
        else
            oldn->parent->right = newn;
    }
    if (newn != NULL) {
        newn->parent = oldn->parent;
    }
}


void rotate_left(node n) {
    node r = n->right;
    replace_node(n, r);
    n->right = r->left;
    if (r->left != NULL) {
        r->left->parent = n;
    }
    r->left = n;
    n->parent = r;
}

void rotate_right(node n) {
    node L = n->left;
    replace_node(n, L);
    n->left = L->right;
    if (L->right != NULL) {
        L->right->parent = n;
    }
    L->right = n;
    n->parent = L;
}

int node_color(node n) {
    return n == NULL ? RBT_COLOR_BLACK : n->color;
}

auto void insert_case5(node n) {
    n->parent->color = RBT_COLOR_BLACK;
    grandparent(n)->color = RBT_COLOR_RED;
    if (n == n->parent->left && n->parent == grandparent(n)->left) {
        rotate_right(grandparent(n));
    } else {
        rotate_left(grandparent(n));
    }
}
auto void insert_case4(node n) {
    if (n == n->parent->right && n->parent == grandparent(n)->left) {
        rotate_left(n->parent);
        n = n->left;
    } else if (n == n->parent->left && n->parent == grandparent(n)->right) {
        rotate_right(n->parent);
        n = n->right;
    }
    insert_case5(n);
}
auto void insert_case3(node n) {
    if (node_color(uncle(n)) == RBT_COLOR_RED) {
        n->parent->color = RBT_COLOR_BLACK;
        uncle(n)->color = RBT_COLOR_BLACK;
        grandparent(n)->color = RBT_COLOR_RED;
        insert_case1(grandparent(n));
    } else {
        insert_case4(n);
    }
}
auto void insert_case2(node n) {
    if (node_color(n->parent) == RBT_COLOR_BLACK)
        return; /* Tree is still valid */
    else
        insert_case3(n);
}

auto void insert_case1(node n) {
    if (n->parent == NULL)
        n->color = RBT_COLOR_BLACK;
    else
        insert_case2(n);
}

void insert_node(node inserted_node) {
    inserted_node->parent=NULL;
    inserted_node->left=NULL;
    inserted_node->right=NULL;
    inserted_node->color=RBT_COLOR_RED;
    if (md->root == NULL) {
        md->root = inserted_node;
    } else {
        node n = md->root;
        while (1) {
            int comp_result = strcmp(md->memo+inserted_node->data, md->memo+n->data);
            if (comp_result < 0) {
                if (n->left == NULL) {
                    n->left = inserted_node;
                    break;
                } else {
                    n = n->left;
                }
            } else {
                if (n->right == NULL) {
                    n->right = inserted_node;
                    break;
                } else {
                    n = n->right;
                }
            }
        }
        inserted_node->parent = n;
    }
    insert_case1(inserted_node);
}

#if 0
auto node maximum_node(node n) {
    while (n->right != NULL) {
        n = n->right;
    }
    return n;
}
auto void delete_case1(node n) {
    if (n->parent == NULL)
        return;
    else
        delete_case2(n);
}
auto void delete_case2(node n) {
    if (node_color(sibling(n)) == RBT_COLOR_RED) {
        n->parent->color = RBT_COLOR_RED;
        sibling(n)->color = RBT_COLOR_BLACK;
        if (n == n->parent->left)
            rotate_left(n->parent);
        else
            rotate_right(n->parent);
    }
    delete_case3(n);
}
auto void delete_case3(node n) {
    if (node_color(n->parent) == RBT_COLOR_BLACK &&
        node_color(sibling(n)) == RBT_COLOR_BLACK &&
        node_color(sibling(n)->left) == RBT_COLOR_BLACK &&
        node_color(sibling(n)->right) == RBT_COLOR_BLACK)
    {
        sibling(n)->color = RBT_COLOR_RED;
        delete_case1(n->parent);
    }
    else
        delete_case4(n);
}
auto void delete_case4(node n) {
    if (node_color(n->parent) == RBT_COLOR_RED &&
        node_color(sibling(n)) == RBT_COLOR_BLACK &&
        node_color(sibling(n)->left) == RBT_COLOR_BLACK &&
        node_color(sibling(n)->right) == RBT_COLOR_BLACK)
    {
        sibling(n)->color = RBT_COLOR_RED;
        n->parent->color = RBT_COLOR_BLACK;
    }
    else
        delete_case5(n);
}
auto void delete_case5(node n) {
    if (n == n->parent->left &&
        node_color(sibling(n)) == RBT_COLOR_BLACK &&
        node_color(sibling(n)->left) == RBT_COLOR_RED &&
        node_color(sibling(n)->right) == RBT_COLOR_BLACK)
    {
        sibling(n)->color = RBT_COLOR_RED;
        sibling(n)->left->color = RBT_COLOR_BLACK;
        rotate_right(sibling(n));
    }
    else if (n == n->parent->right &&
             node_color(sibling(n)) == RBT_COLOR_BLACK &&
             node_color(sibling(n)->right) == RBT_COLOR_RED &&
             node_color(sibling(n)->left) == RBT_COLOR_BLACK)
    {
        sibling(n)->color = RBT_COLOR_RED;
        sibling(n)->right->color = RBT_COLOR_BLACK;
        rotate_left(sibling(n));
    }
    delete_case6(n);
}
auto void delete_case6(node n) {
    sibling(n)->color = node_color(n->parent);
    n->parent->color = RBT_COLOR_BLACK;
    if (n == n->parent->left) {
        sibling(n)->right->color = RBT_COLOR_BLACK;
        rotate_left(n->parent);
    }
    else
    {
        sibling(n)->left->color = RBT_COLOR_BLACK;
        rotate_right(n->parent);
    }
}


node delete_node(node n) {
    node child;
    if (n == NULL) return;  /* Key not found, do nothing */
/*
    if (n->left != NULL && n->right != NULL) {
        node pred = maximum_node(n->left);
        strcpy(n->name,pred->name);
        n->gcount=pred->gcount;
        memcpy(n->gramas,pred->gramas,sizeof(pred->gramas));
        n->succ->pred=n->pred;
        n->pred->succ=n->succ;
        n->succ=pred->succ;
        n->pred=pred->pred;
        n->succ->pred=n;
        n->pred->succ=n;
        n=pred;
    }
    else {
        n->succ->pred=n;
        n->pred->succ=n;
    }
    child = n->right == NULL ? n->left  : n->right;
    if (node_color(n) == RBT_COLOR_BLACK) {
        n->color = node_color(child);
        delete_case1(n);
    }
    replace_node(n, child);
    if (n->parent == NULL && child != NULL) // root should be black
        child->color = RBT_COLOR_BLACK;
*/
    return n;
}

#endif

int cache_GetWord(char *word,u_int64_t *gramas)
{
    struct minimorf_cached_word *w;
    w=lookup_node(word);
    if (w) {
        md->hit_count+=1;
        w->count += 1;
	//fprintf(stderr,"W->gcount=%d\n",w->gcount);
        if (gramas && w->gcount) memcpy(gramas,md->memo+w->data+strlen(md->memo+w->data)+1,8*w->gcount);
        return w->gcount;
    }
    return -1;
}

struct minimorf_cached_word *alloc_node_r(char *word,int ngram,u_int64_t *gramas)
{
    u_int16_t datasize=2+strlen(word)+1+8*ngram;
    struct minimorf_cached_word *w;
    int needed=datasize+sizeof(struct minimorf_cached_word)+4;
    if (md->data_end + needed > md->struct_start) {
        return NULL;
    }
    md->struct_start -= sizeof(*w);
    w=(void *)(md->memo+md->struct_start);
    memcpy(md->memo+md->data_end,&md->struct_start,4);
    memcpy(md->memo+md->data_end+4,&datasize,2);
    strcpy(md->memo+md->data_end+6,word);
    if (ngram) {
        memcpy(md->memo+md->data_end+7+strlen(word),gramas,8*ngram);
    }
    w->data=md->data_end+6;
    md->list_size += 1;
    md->data_end += 4+datasize;
    w->count=1;
    w->gcount=ngram;
    return w;
}


void drop_node(node w)
{
    w->color |= 32;
}

int node_dropped(node w)
{
    return w->color & 32;
}

void invalidate_node_data(node w)
{
    u_int32_t hdx;
    hdx=0x7fffffff;
    memcpy(md->memo+w->data-6,&hdx,4);
    w->data=0;
}

void collect_strings(void)
{
    int posin,posout;
    u_int32_t hdx;
    u_int16_t len;
    node w;
    for (posin=posout=0;posin<md->data_end;) {
        memcpy(&hdx,md->memo+posin,4);
        memcpy(&len,md->memo+posin+4,2);
        if (hdx != 0x7fffffff) {
            if (posin != posout) {
                w=(void *)(md->memo+hdx);
                w->data=posout+6;
                memmove(md->memo+posout,md->memo+posin,4+len);
            }
            posout += len+4;
        }
        posin += len+4;
    }
    md->data_end=posout;
}

void collect_nodes(void)
{
    u_int32_t posin,posout;
    posin=posout=md->memo_size;
    node w,wnew;
    while (posin > md->struct_start) {
        posin -= sizeof(struct minimorf_cached_word);
        w=(node)(md->memo+posin);
        if (node_dropped(w)) {
            continue;
        }
        posout -= sizeof(struct minimorf_cached_word);
        if (posout != posin) {
            memcpy(md->memo+posout,md->memo+posin,sizeof(struct minimorf_cached_word));
            memcpy(md->memo+w->data-6,&posout,4);
            wnew=(node)(md->memo+posout);
            wnew->succ->pred=wnew;
            wnew->pred->succ=wnew;
            if (wnew->left) wnew->left->parent=wnew;
            if (wnew->right) wnew->right->parent=wnew;
            if (wnew->parent) {
                if (wnew->parent->left == w) {
                    wnew->parent->left=wnew;
                }
                else {
                    wnew->parent->right=wnew;
                }
            }
            else {
                md->root=wnew;
            }
        }
    }
    md->struct_start=posout;
}


u_int32_t *arr;
int array_size;

void create_node_array(void)
{
    
    void append_node(node w)
    {
        while (w) {
            append_node(w->right);
            if (!node_dropped(w)) {
                arr -= 1;
                array_size += 1;
                *arr=((char *)w)-md->memo;
            }
            w=w->left;
        }
    }
    
    arr=(void *)(md->memo+md->struct_start);
    array_size=0;
    append_node(md->root);
}

node arr2bst(u_int32_t *arr,int start,int end,node parent)
{
    int mid;node w;
    if (start > end) {
        return NULL;
    }
    mid=(start+end)/2;
    w=(node)(md->memo+arr[mid]);
    w->parent=parent;
    w->left=arr2bst(arr,start,mid-1,w);
    w->right=arr2bst(arr,mid+1,end,w);
    return w;
}

int tree_height(node w)
{
    int n1,n2;
    int min(a,b) {
        return (a>b)?b:a;
    }
    int max(a,b) {
        return (a>b)?a:b;
    }
    if (!w->left) n1=1;else n1=1+tree_height(w->left);
    if (!w->right) n2=1;else n2=1+tree_height(w->right);
    if (!w->left || !w->right) w->min_height=0;
    else w->min_height=1+min(w->left->min_height,w->right->min_height);
    return max(n1,n2);
}

int colorize(node w)
{
    if (!w) return 0;
    if (!w->parent) {
        int h=tree_height(w);
        w->color=RBT_COLOR_BLACK;
        w->black_quota=(h+1)/2;
    }
    else if (node_color(w->parent) == RBT_COLOR_RED) {
        w->color=RBT_COLOR_BLACK;
        w->black_quota=w->parent->black_quota;
    }
    else {
        if (w->min_height < w->parent->black_quota) {
            return -1;
        }
        else if (w->min_height == w->parent->black_quota) {
            w->color = RBT_COLOR_BLACK;
        }
        else {
            w->color = RBT_COLOR_RED;
        }
        w->black_quota = w->parent->black_quota - 1;
    }
    return colorize(w->left) || colorize(w->right);
}


auto void verify_property_1(node n);
auto void verify_property_2(node n);
auto void verify_property_4(node n);
auto void verify_property_5(node n);
auto void verify_property_5_helper(node n, int black_count, int* path_black_count);

void verify_properties(void) {
    verify_property_1(md->root);
    verify_property_2(md->root);
    /* Property 3 is implicit */
    verify_property_4(md->root);
    verify_property_5(md->root);
}
void verify_property_1(node n) {
    assert(node_color(n) == RBT_COLOR_RED || node_color(n) == RBT_COLOR_BLACK);
    if (n == NULL) return;
    verify_property_1(n->left);
    verify_property_1(n->right);
}
void verify_property_2(node root) {
    assert(node_color(root) == RBT_COLOR_BLACK);
}
void verify_property_4(node n) {
    if (node_color(n) == RBT_COLOR_RED) {
        assert (node_color(n->left)   == RBT_COLOR_BLACK);
        assert (node_color(n->right)  == RBT_COLOR_BLACK);
        assert (node_color(n->parent) == RBT_COLOR_BLACK);
    }
    if (n == NULL) return;
    verify_property_4(n->left);
    verify_property_4(n->right);
}
void verify_property_5(node root) {
    int black_count_path = -1;
    verify_property_5_helper(root, 0, &black_count_path);
}

void verify_property_5_helper(node n, int black_count, int* path_black_count) {
    if (node_color(n) == RBT_COLOR_BLACK) {
        black_count++;
    }
    if (n == NULL) {
        if (*path_black_count == -1) {
            *path_black_count = black_count;
        } else {
            assert (black_count == *path_black_count);
        }
        return;
    }
    verify_property_5_helper(n->left,  black_count, path_black_count);
    verify_property_5_helper(n->right, black_count, path_black_count);
}

void clean_list(void)
{
    int i,n;
    node w;
    int maxl=md->list_size/2;
    int minl=md->list_size/5;
    int retmemo=0;
    int datalen(node w)
    {
        u_int16_t len;
        memcpy(&len,md->memo+w->data-2,2);
        return len+4;
    }
    
    
    for (i=n=0,w=md->tail;i<maxl;i++,w=w->pred) {
        if (w->count == 1) {
            drop_node(w);
            retmemo+=datalen(w);
            n += 1;
        }
    }
    for (i=0,w=md->tail;retmemo < (md->list_size-n)*4;i++,w=w->pred) {
        if (!node_dropped(w)) {
            drop_node(w);
            retmemo+=datalen(w);
            n += 1;
        }
    }
    
    for (w=md->head;w && w->succ;w=w->succ) {
        if (node_dropped(w)) {
            w->pred->succ=w->succ;
            w->succ->pred=w->pred;
            invalidate_node_data(w);
            md->list_size -= 1;
        }
    }
    collect_strings();
    create_node_array();
    md->root=arr2bst(arr,0,array_size-1,NULL);
    collect_nodes();
    /*
     * Jeśli colorizer zwróci błąd, trzeba wyczyścić wszystko
     */
    
    if (colorize(md->root)) {
        md->root=NULL;
        md->data_end=0;
        md->struct_start=md->memo_size;
        md->head=(void *)(&md->tailpred);
        md->tail=(void *)(&md->head);
        md->list_size=0;
    }
    
    md->cache_roll_count++;
    //printf("Now free %d/%d\n",md->struct_start-md->data_end,md->memo_size);
}

void cache_SetWord(char *word,int n,u_int64_t *gramas)
{
    struct minimorf_cached_word *w;
    w=alloc_node_r(word,n,gramas);
    if (!w) {
        clean_list();
        w=alloc_node_r(word,n,gramas);
    }
    w->succ=md->head;
    w->pred=(void *)(&md->head);
    md->head->pred=w;
    md->head=w;
    insert_node(w);
    md->miss_count += 1;
}


/*


int compute_tree_height(node w)
{
    int n1,n2;
    if (!w->left) n1=0;else n1=1+compute_tree_height(w->left);
    if (!w->right) n2=0;else n2=1+compute_tree_height(w->right);
    if (!w->left || !w->right) w->min_height=0;
    else w->min_height=1+min(w->left->min_height,w->right->min_height);
    return max(n1,n2);
}

int colorize(node w)
{
    if (!w) return 0;
    if (!w->parent) {
        int h=compute_tree_height(w);
        w->color=RBT_COLOR_BLACK;
        w->black_quota=(h+1)/2;
    }
    else if (node_color(w->parent) == RBT_COLOR_RED) {
        w->color=RBT_COLOR_BLACK;
        w->black_quote=w->parent->black_quota;
    }
    else {
        if (w->min_height < w->parent->black_quota) {
            return -1;
        }
        else if (w->min_height == w->parent->black_quota) {
            w->color = RBT_COLOR_RED;
        }
        else {
            w->color = RBT_COLOR_BLACK;
        }
        w->black_quota = w->parent->black_quota - 1;
    }
    return colorize(w->left) || colorize(w->right);
}


if n is root,
    n.color = black
    n.black-quota = height n / 2, rounded up.

else if n.parent is red,
    n.color = black
    n.black-quota = n.parent.black-quota.

else (n.parent is black)
    if n.min-height < n.parent.black-quota, then
        error "shortest path was too short"
    else if n.min-height = n.parent.black-quota then
        n.color = black
    else (n.min-height > n.parent.black-quota)
        n.color = red
    either way,
        n.black-quota = n.parent.black-quota - 1
*/
