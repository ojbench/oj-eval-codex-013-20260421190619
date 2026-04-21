/**
* implement a container like std::map
*/
#ifndef SJTU_MAP_HPP
#define SJTU_MAP_HPP

// only for std::less<T>
#include <functional>
#include <cstddef>
#include "utility.hpp"
#include "exceptions.hpp"

namespace sjtu {

template<
   class Key,
   class T,
   class Compare = std::less <Key>
   > class map {
  public:
   typedef pair<const Key, T> value_type;

   class const_iterator;

   // forward declaration of node
  private:
   struct Node;
  public:
   class iterator {
      public:
       const map *owner;
       Node *cur;
       iterator() : owner(nullptr), cur(nullptr) {}
       iterator(const map *m, Node *p) : owner(m), cur(p) {}
       iterator(const iterator &other) : owner(other.owner), cur(other.cur) {}

       iterator operator++(int) { iterator tmp(*this); ++(*this); return tmp; }
       iterator &operator++() {
         if (owner == nullptr) throw invalid_iterator();
         if (cur == nullptr) throw invalid_iterator();
         cur = owner->next_node(cur);
         return *this;
       }
       iterator operator--(int) { iterator tmp(*this); --(*this); return tmp; }
       iterator &operator--() {
         if (owner == nullptr) throw invalid_iterator();
         if (cur == nullptr) {
           if (owner->root == nullptr) throw invalid_iterator();
           cur = owner->max_node(owner->root);
           return *this;
         }
         Node *p = owner->prev_node(cur);
         if (p == nullptr) throw invalid_iterator();
         cur = p;
         return *this;
       }
       value_type &operator*() const {
         if (cur == nullptr) throw invalid_iterator();
         return cur->val;
       }
       bool operator==(const iterator &rhs) const { return owner == rhs.owner && cur == rhs.cur; }
       bool operator==(const const_iterator &rhs) const { return owner == rhs.owner && cur == rhs.cur; }
       bool operator!=(const iterator &rhs) const { return !(*this == rhs); }
       bool operator!=(const const_iterator &rhs) const { return owner != rhs.owner || cur != rhs.cur; }
       value_type *operator->() const noexcept { return &cur->val; }
   };

   class const_iterator {
      public:
       const map *owner;
       const Node *cur;
       const_iterator() : owner(nullptr), cur(nullptr) {}
       const_iterator(const const_iterator &other) : owner(other.owner), cur(other.cur) {}
       const_iterator(const iterator &other) : owner(other.owner), cur(other.cur) {}
       const_iterator operator++(int) { const_iterator tmp(*this); ++(*this); return tmp; }
       const_iterator &operator++() {
         if (owner == nullptr) throw invalid_iterator();
         if (cur == nullptr) throw invalid_iterator();
         cur = owner->next_node(const_cast<Node*>(cur));
         return *this;
       }
       const_iterator operator--(int) { const_iterator tmp(*this); --(*this); return tmp; }
       const_iterator &operator--() {
         if (owner == nullptr) throw invalid_iterator();
         if (cur == nullptr) {
           if (owner->root == nullptr) throw invalid_iterator();
           cur = owner->max_node(owner->root);
           return *this;
         }
         Node *p = owner->prev_node(const_cast<Node*>(cur));
         if (p == nullptr) throw invalid_iterator();
         cur = p;
         return *this;
       }
       const value_type &operator*() const {
         if (cur == nullptr) throw invalid_iterator();
         return cur->val;
       }
       bool operator==(const const_iterator &rhs) const { return owner == rhs.owner && cur == rhs.cur; }
       bool operator==(const iterator &rhs) const { return owner == rhs.owner && cur == rhs.cur; }
       bool operator!=(const const_iterator &rhs) const { return !(*this == rhs); }
       bool operator!=(const iterator &rhs) const { return !(*this == rhs); }
       const value_type *operator->() const noexcept { return &cur->val; }
   };


   map() : root(nullptr), node_count(0), comp(Compare()), seed(146527ULL) {}

   map(const map &other) : root(nullptr), node_count(0), comp(other.comp), seed(other.seed ^ 0x9e3779b97f4a7c15ULL) {
     root = clone(other.root, nullptr);
     node_count = other.node_count;
   }

   map &operator=(const map &other) {
     if (this == &other) return *this;
     clear();
     comp = other.comp;
     seed = other.seed ^ 0x517cc1b727220a95ULL;
     root = clone(other.root, nullptr);
     node_count = other.node_count;
     return *this;
   }

   ~map() { clear(); }

   T &at(const Key &key) {
     Node *p = find_node(key);
     if (!p) throw index_out_of_bound();
     return p->val.second;
   }

   const T &at(const Key &key) const {
     Node *p = find_node(key);
     if (!p) throw index_out_of_bound();
     return p->val.second;
   }

   T &operator[](const Key &key) {
     Node *p = find_node(key);
     if (p) return p->val.second;
     value_type v(key, T());
     bool inserted = false;
     Node *out = nullptr;
     root = insert_node(root, nullptr, v, inserted, &out);
     if (root) root->parent = nullptr;
     return out->val.second;
   }

   const T &operator[](const Key &key) const {
     Node *p = find_node(key);
     if (!p) throw index_out_of_bound();
     return p->val.second;
   }

   iterator begin() { return iterator(this, min_node(root)); }
   const_iterator cbegin() const { return const_iterator(iterator(this, min_node(root))); }

   iterator end() { return iterator(this, nullptr); }
   const_iterator cend() const { return const_iterator(iterator(this, nullptr)); }

   bool empty() const { return node_count == 0; }
   size_t size() const { return node_count; }

   void clear() {
     destroy(root);
     root = nullptr;
     node_count = 0;
   }

   pair<iterator, bool> insert(const value_type &value) {
     bool inserted = false;
     Node *out = nullptr;
     root = insert_node(root, nullptr, value, inserted, &out);
     if (root) root->parent = nullptr;
     return pair<iterator, bool>(iterator(this, out), inserted);
   }

   void erase(iterator pos) {
     if (pos.owner != this) throw invalid_iterator();
     Node *target = pos.cur;
     if (target == nullptr) throw invalid_iterator();
     bool existed = false;
     root = erase_node(root, target->val.first, existed);
     if (root) root->parent = nullptr;
     if (!existed) throw invalid_iterator();
   }

   size_t count(const Key &key) const { return find_node(key) ? 1 : 0; }

   iterator find(const Key &key) { return iterator(this, find_node(key)); }
   const_iterator find(const Key &key) const { return const_iterator(iterator(this, find_node(key))); }

  private:
   struct Node {
     value_type val;
     Node *left;
     Node *right;
     Node *parent;
     unsigned prio;
     Node(const value_type &v, unsigned pr) : val(v), left(nullptr), right(nullptr), parent(nullptr), prio(pr) {}
   };

   Node *root;
   size_t node_count;
   Compare comp;
   unsigned long long seed;

   unsigned next_rand() const {
     // simple xorshift64* variant; but keep const-ness by casting away for update
     map *self = const_cast<map*>(this);
     self->seed ^= self->seed << 7;
     self->seed ^= self->seed >> 9;
     return (unsigned)(self->seed & 0xffffffffu);
   }

   Node *min_node(Node *n) const { if (!n) return nullptr; while (n->left) n = n->left; return n; }
   Node *max_node(Node *n) const { if (!n) return nullptr; while (n->right) n = n->right; return n; }

   Node *next_node(Node *n) const {
     if (!n) return nullptr;
     if (n->right) return min_node(n->right);
     Node *p = n->parent;
     while (p && n == p->right) { n = p; p = p->parent; }
     return p;
   }
   Node *prev_node(Node *n) const {
     if (!n) return nullptr;
     if (n->left) return max_node(n->left);
     Node *p = n->parent;
     while (p && n == p->left) { n = p; p = p->parent; }
     return p;
   }

   Node *rotate_right(Node *y) {
     Node *x = y->left;
     Node *B = x->right;
     x->right = y; y->parent = x;
     y->left = B; if (B) B->parent = y;
     return x;
   }
   Node *rotate_left(Node *x) {
     Node *y = x->right;
     Node *B = y->left;
     y->left = x; x->parent = y;
     x->right = B; if (B) B->parent = x;
     return y;
   }

   Node *insert_node(Node *n, Node *parent, const value_type &v, bool &inserted, Node **out) {
     if (!n) {
       inserted = true;
       node_count++;
       Node *nn = new Node(v, next_rand());
       nn->parent = parent;
       if (out) *out = nn;
       return nn;
     }
     if (comp(v.first, n->val.first)) {
       n->left = insert_node(n->left, n, v, inserted, out);
       if (n->left && n->left->prio < n->prio) {
         Node *res = rotate_right(n);
         res->parent = parent;
         n->parent = res;
         return res;
       }
     } else if (comp(n->val.first, v.first)) {
       n->right = insert_node(n->right, n, v, inserted, out);
       if (n->right && n->right->prio < n->prio) {
         Node *res = rotate_left(n);
         res->parent = parent;
         n->parent = res;
         return res;
       }
     } else {
       inserted = false;
       if (out) *out = n;
       return n;
     }
     n->parent = parent;
     return n;
   }

   Node *erase_node(Node *n, const Key &key, bool &erased) {
     if (!n) return nullptr;
     if (comp(key, n->val.first)) {
       n->left = erase_node(n->left, key, erased);
       if (n->left) n->left->parent = n;
       return n;
     } else if (comp(n->val.first, key)) {
       n->right = erase_node(n->right, key, erased);
       if (n->right) n->right->parent = n;
       return n;
     } else {
       erased = true;
       // delete this node by rotating down until it has <=1 child
       if (!n->left && !n->right) {
         delete n;
         if (node_count) --node_count;
         return nullptr;
       } else if (!n->left) {
         Node *res = n->right;
         res->parent = n->parent;
         delete n;
         if (node_count) --node_count;
         return res;
       } else if (!n->right) {
         Node *res = n->left;
         res->parent = n->parent;
         delete n;
         if (node_count) --node_count;
         return res;
       } else {
         if (n->left->prio < n->right->prio) {
           Node *res = rotate_right(n);
           res->right = erase_node(res->right, key, erased);
           if (res->right) res->right->parent = res;
           return res;
         } else {
           Node *res = rotate_left(n);
           res->left = erase_node(res->left, key, erased);
           if (res->left) res->left->parent = res;
           return res;
         }
       }
     }
   }

   Node *find_node(const Key &key) const {
     Node *n = root;
     while (n) {
       if (comp(key, n->val.first)) n = n->left;
       else if (comp(n->val.first, key)) n = n->right;
       else return n;
     }
     return nullptr;
   }

   void destroy(Node *n) {
     if (!n) return;
     destroy(n->left);
     destroy(n->right);
     delete n;
   }

   Node *clone(Node *n, Node *parent) {
     if (!n) return nullptr;
     Node *nn = new Node(n->val, n->prio);
     nn->parent = parent;
     nn->left = clone(n->left, nn);
     nn->right = clone(n->right, nn);
     return nn;
   }

   friend class iterator;
   friend class const_iterator;
 };

}

#endif
