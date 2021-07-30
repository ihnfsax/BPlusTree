#pragma once
#include <deque>  // debug
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string.h>
#include <utility>

namespace my {

#define DELETE(ptr)        \
    do {                   \
        if (ptr) {         \
            delete ptr;    \
            ptr = nullptr; \
        }                  \
    } while (0)

#define DELETE_ARRAY(arrPtr)  \
    do {                      \
        if (arrPtr) {         \
            delete[] arrPtr;  \
            arrPtr = nullptr; \
        }                     \
    } while (0)

#define MOVE_NODE(des, des_idx, src, src_idx)                  \
    do {                                                       \
        des->key[des_idx] = src->key[src_idx];                 \
        if (des->type) {                                       \
            des->entry[des_idx]      = src->entry[src_idx];    \
            des->entry[des_idx]->key = &(des->key[des_idx]);   \
        }                                                      \
        else {                                                 \
            des->child[des_idx]         = src->child[src_idx]; \
            des->child[des_idx]->parent = des;                 \
        }                                                      \
    } while (0)

template <class Key, class T> class BPlusTree {
public:
    typedef Key                     key_type;
    typedef T                       data_type;
    typedef std::pair<const Key, T> value_type;
    typedef short                   order_type;
    typedef unsigned int            size_type;

protected:
    typedef unsigned int node_size_type;  // no need bigger than size_type
    typedef struct ListNode {
        key_type*        key;
        data_type        data;
        struct ListNode* prior = nullptr;
        struct ListNode* next  = nullptr;
        ListNode();
        explicit ListNode(key_type* k) : key(k){};
        ListNode(key_type* k, data_type d) : key(k), data(d){};
        ~ListNode() {
            if (this->prior)
                this->prior->next = this->next;
            if (this->next)
                this->next->prior = this->prior;
        }
        /* flag: true: insert before, false: insert after */
        void insert(ListNode* p, bool flag) {
            if (p == nullptr)
                throw std::runtime_error("try to insert a null node");
            if (flag) {
                p->prior = this->prior;
                p->next  = this;
                if (this->prior)
                    this->prior->next = p;
                this->prior = p;
                return;
            }
            p->prior = this;
            p->next  = this->next;
            if (this->next) {
                this->next->prior = p;
            }
            this->next = p;
        }
    } ListNode, *ListPtr;

    typedef struct BTPair {
        typedef Key first_type;
        typedef T   second_type;
        const Key   first;
        T&          second;
        BTPair(const Key& t1, T& t2) : first(t1), second(t2){};
    } BTPair;

    class BTIterator {
    private:
        ListPtr node = nullptr;
        BTPair* pair = nullptr;

    public:
        BTIterator(ListPtr n) : node(n) {
            if (n) {
                pair = new BTPair(*n->key, n->data);
            }
        };

        ~BTIterator() {
            DELETE(pair);
        }

        BTIterator(const BTIterator& it) {
            node = it.node;
            DELETE(pair);
            if (it.node)
                pair = new BTPair(it.pair->first, it.pair->second);
        }

        BTIterator& operator=(const BTIterator& it) {
            if (this == &it)
                return *this;
            node = it.node;
            DELETE(pair);
            if (it.node)
                pair = new BTPair(it.pair->first, it.pair->second);
            return *this;
        }

        BTPair* operator->() {
            if (pair == nullptr) {
                throw std::runtime_error("try to get value from nullptr");
            }
            return pair;
        }

        bool operator==(const BTIterator& other) const {
            if (this->node == other.node) {
                return true;
            }
            else {
                return false;
            }
        }

        bool operator!=(const BTIterator& other) const {
            if (this->node != other.node) {
                return true;
            }
            else {
                return false;
            }
        }

        void operator++() {
            if (node == nullptr) {
                throw std::runtime_error("BPlusTree iterator not incrementable");
            }
            else {
                node = node->next;
                DELETE(pair);
                pair = node ? new BTPair(*node->key, node->data) : nullptr;
            }
        }

        void operator--() {
            if (node == nullptr) {
                throw std::runtime_error("BPlusTree iterator not decrementable");
            }
            else {
                node = node->prior;
                DELETE(pair);
                pair = node ? new BTPair(*node->key, node->data) : nullptr;
            }
        }
    };

    class BTNode {
    public:
        const order_type order;
        const bool       type;  // 0: inner node; 1: leaf node
        key_type*        key    = nullptr;
        BTNode**         child  = nullptr;
        ListPtr*         entry  = nullptr;
        BTNode*          parent = nullptr;
        order_type       count  = 0;
        BTNode(order_type m, bool type) : order(m), type(type) {
            if (m < 3) {
                throw std::runtime_error("order of BPlusTree must >= 3");
            }
            key = new key_type[m];
            if (type)
                entry = new ListPtr[m];
            else
                child = new BTNode*[m];
        }
        ~BTNode() {
            DELETE_ARRAY(key);
            if (type)
                DELETE_ARRAY(entry);
            else
                DELETE_ARRAY(child);
        }

        BTNode(const BTNode& n) {
            this->order  = n.order;
            this->type   = n.type;
            this->count  = n.count;
            this->parent = n.parent;
            DELETE_ARRAY(key);
            DELETE_ARRAY(entry);
            DELETE_ARRAY(child);
            if (n.key && order > 0) {
                key = new key_type[order];
                memcpy(key, n.key, order * sizeof(key_type));
            }
            if (type && n.entry && order > 0) {
                entry = new ListPtr[order];
                memcpy(entry, n.entry, order * sizeof(ListPtr));
            }
            else if (n.child && order > 0) {
                child = new BTNode*[order];
                memcpy(child, n.child, order * sizeof(BTNode*));
            }
        }

        BTNode& operator=(const BTNode& n) {
            if (this == &n) {
                return *this;
            }
            this->order  = n.order;
            this->type   = n.type;
            this->count  = n.count;
            this->parent = n.parent;
            DELETE_ARRAY(key);
            DELETE_ARRAY(entry);
            DELETE_ARRAY(child);
            if (n.key && order > 0) {
                key = new key_type[order];
                memcpy(key, n.key, order * sizeof(key_type));
            }
            if (type && n.entry && order > 0) {
                entry = new ListPtr[order];
                memcpy(entry, n.entry, order * sizeof(ListPtr));
            }
            else if (n.child && order > 0) {
                child = new BTNode*[order];
                memcpy(child, n.child, order * sizeof(BTNode*));
            }
            return *this;
        }

        order_type search(const key_type& k) const {
            if (count == 0)
                return -2;
            if (k < key[0])
                return -1;
            order_type i = 0;
            for (; i < count - 1; ++i) {
                if (k < key[i + 1])
                    return i;
            }
            return i;
        }

        order_type insert(const order_type& r, const key_type& k, const data_type* const d = nullptr) {
            if (type && d == nullptr)
                throw std::runtime_error("try to insert a nullptr to data field");
            for (order_type i = count - 1; i > r; --i) {
                MOVE_NODE(this, i + 1, this, i);
            }
            count++;
            key[r + 1] = k;
            if (type) { /* leaf node */
                entry[r + 1] = new ListNode(&key[r + 1], *d);
                if (count != 1) {
                    if (r + 1 == count - 1)
                        entry[r]->insert(entry[r + 1], false); /* insert after */
                    else
                        entry[r + 2]->insert(entry[r + 1], true); /* insert before */
                }
            }
            else /* inner node */
                child[r + 1] = nullptr;
            return r + 1;
        }

        void erase(const order_type& r, bool flag = true) {
            if (type && flag) {
                DELETE(entry[r]);
            }
            for (order_type i = r; i < count - 1; ++i) {
                MOVE_NODE(this, i, this, i + 1);
            }
            if (r == 0) {
                fresh();
            }
            count--;
        }

        void fresh() {
            BTNode* v = this;
            while (v) {
                order_type pr = 0;
                if (v->parent) {
                    while (v->parent->child[pr] != v)
                        pr++;
                    v->parent->key[pr] = v->key[0];
                }
                v = v->parent;
            }
        }

        BTNode* splitSelf(order_type s) {
            if (s >= count - 1 || s < -1)
                throw std::runtime_error("can't split node");
            BTNode* node = new BTNode(order, type);
            node->count  = count - s - 1;
            memcpy(node->key, &key[s + 1], node->count * sizeof(key_type));
            if (type) {
                memcpy(node->entry, &entry[s + 1], node->count * sizeof(ListPtr));
                for (order_type i = 0; i < node->count; ++i) {
                    node->entry[i]->key = &node->key[i];
                }
            }
            else {
                memcpy(node->child, &child[s + 1], node->count * sizeof(BTNode*));
                for (order_type i = 0; i < node->count; ++i) {
                    node->child[i]->parent = node;
                }
            }
            count        = s + 1;
            node->parent = parent;
            return node;
        }
    };

    size_type      _size       = 0;  // 数据数量
    node_size_type _node_count = 0;  // 节点数量
    order_type     _order;
    BTNode*        _root = nullptr;
    ListPtr        _head = nullptr;

public:
    BPlusTree(order_type order = 3) : _order(order) {
        if (order < 3) {
            throw std::runtime_error("order of BPlusTree must >= 3");
        }
    };

    size_type size() const {
        return _size;
    }

    order_type order() const {
        return _order;
    }

    node_size_type ncount() const {
        return _node_count;
    }

    BTIterator end() {
        return BTIterator(nullptr);
    }

    BTIterator begin() {
        return BTIterator(_head);
    }

    BTIterator find(const key_type& key) {
        BTNode* v = _root;
        if (v == nullptr) {
            return BTIterator(nullptr);
        }
        while (true) {
            order_type r = v->search(key);
            if (v->type && r >= 0 && v->key[r] == key) {
                return BTIterator(v->entry[r]);
            }
            else if (!v->type && r != -1)
                v = v->child[r];
            else
                return BTIterator(nullptr);
        }
    }

    bool insert(const key_type& key, const data_type& data) {
        BTNode* v = _root;
        if (v == nullptr) { /* only for _root */
            v = new BTNode(_order, true);
            v->insert(-1, key, &data);
            _root = v;
            _head = v->entry[0];
            _size++;
            _node_count++;
            return true;
        }
        while (true) {
            order_type r = v->search(key);
            if (v->type) {
                if (r >= 0 && v->key[r] == key)
                    return false; /* already exists */
                doInsert(v, r, key, data);
                if (v->entry[r]->prior == nullptr)
                    _head = v->entry[r];
                _size++;
                return true;
            }
            else {
                if (r == -1)
                    v->key[0] = key, r = 0;
                v = v->child[r];
            }
        }
    }

    /* can not bind iterator to specific BPlusTree */
    // bool insert(const BTIterator& iterator) {
    //     if (iterator != end())
    //         return insert(iterator->first, iterator->second);
    //     else
    //         return false;
    // }

    bool insert(const value_type& pair) {
        return insert(pair.first, pair.second);
    }

    bool erase(const key_type& key) {
        BTNode* v = _root;
        if (v == nullptr)
            return false;
        while (true) {
            order_type r = v->search(key);
            if (v->type && r >= 0 && v->key[r] == key) {
                doErase(v, r);
                if (v->entry[0]->prior == nullptr)
                    _head = v->entry[0];
                _size--;
                return true;
            }
            else if (!v->type && r != -1)
                v = v->child[r];
            else
                return false;
        }
    }

    /* can not bind iterator to specific BPlusTree */
    // bool erase(BTIterator& iterator) {
    //     if (iterator != end()) {
    //         key_type   k    = iterator->first;
    //         BTIterator temp = iterator;
    //         ++temp;
    //         if (erase(k)) {
    //             iterator = temp;
    //             return true;
    //         }
    //         else
    //             return false;
    //     }
    //     else
    //         return false;
    // }

    /* for debug */
    void print() {
        BTNode*             v = nullptr;
        std::deque<BTNode*> pnodes, cnodes;
        int                 l = 0;
        if (_root != nullptr) {
            pnodes.push_back(_root);
            while (!pnodes.empty()) {
                std::cout << "Level " << l++ << ": ";
                while (!pnodes.empty()) {
                    v = *(pnodes.begin());
                    pnodes.pop_front();
                    std::cout << "|";
                    for (order_type i = 0; i < v->count; ++i) {
                        if (v->type) {
                            std::cout << v->key[i] << ":" << v->entry[i]->data << "|";
                        }
                        else {
                            std::cout << v->key[i] << "|";
                            cnodes.push_back(v->child[i]);
                        }
                    }
                    if (!pnodes.empty()) {
                        std::cout << "-----";
                    }
                }
                std::cout << "\n";
                pnodes = cnodes;
                cnodes.clear();
            }
        }
        ListPtr k = _head;
        while (k) {
            std::cout << *(k->key) << ":" << k->data << "  ";
            k = k->next;
        }
        std::cout << "\n";
    }

private:
    void doInsert(BTNode* n, order_type& r, const key_type& k, const data_type& d) {
        if (n->count < _order) {
            r = n->insert(r, k, &d);
            return;
        }
        order_type s  = _order / 2;
        BTNode*    n2 = nullptr;
        if (r < s) { /* insert to left */
            n2 = n->splitSelf(s - 1);
            r  = n->insert(r, k, &d);
        }
        else { /* insert to right */
            n2 = n->splitSelf(s);
            r  = n2->insert(r - s - 1, k, &d);
        }
        if (n->parent) { /* recursion */
            order_type r2 = n->parent->search(n2->key[0]);
            doInsert(n->parent, r2, n2->key[0], d);
            n->parent->child[r2] = n2;
        }
        else { /* create new root */
            n->parent = new BTNode(_order, false);
            n->parent->insert(-1, n->key[0]);
            n->parent->insert(0, n2->key[0]);
            n->parent->child[0] = n;
            n->parent->child[1] = n2;
            n2->parent          = n->parent;
            _root               = n->parent;
            _node_count++;
        }
        n = r < s ? n : n2;
        _node_count++;
    }

    void doErase(BTNode* n, const order_type& r) {
        BTNode* p = n->parent;
        if (p == nullptr || n->count > (_order + 1) / 2) {
            n->erase(r);
        }
        else {
            order_type pr = 0;
            while (p->child[pr] != n)
                pr++;
            /* borrow from left */
            if (pr > 0 && p->child[pr - 1]->count > (_order + 1) / 2) {
                BTNode* lb = p->child[pr - 1];
                if (n->type) {
                    DELETE(n->entry[r]);
                }
                for (order_type i = r; i > 0; --i) {
                    MOVE_NODE(n, i, n, i - 1);
                }
                MOVE_NODE(n, 0, lb, lb->count - 1);
                lb->count--;
                p->key[pr] = n->key[0];
            }
            /* borrow from right */
            else if (pr < p->count - 1 && p->child[pr + 1]->count > (_order + 1) / 2) {
                BTNode* rb = p->child[pr + 1];
                n->erase(r);
                MOVE_NODE(n, n->count, rb, 0);
                rb->erase(0, false);
                n->count++;
            }
            /* merge with left */
            else if (pr > 0) {
                BTNode* lb = p->child[pr - 1];
                n->erase(r);
                for (order_type i = 0; i < n->count; ++i) {
                    MOVE_NODE(lb, lb->count + i, n, i);
                }
                lb->count += n->count;
                doErase(p, pr);
                DELETE(n);
                n = lb;
                _node_count--;
            }
            else { /* merge with right */
                BTNode* rb = p->child[pr + 1];
                n->erase(r);
                for (order_type i = 0; i < rb->count; ++i) {
                    MOVE_NODE(n, n->count + i, rb, i);
                }
                n->count += rb->count;
                doErase(p, pr + 1);
                DELETE(rb);
                _node_count--;
            }
        }
        if (p && p->count == 1) {
            n->parent = nullptr;
            DELETE(p);
            _root = n;
            _node_count--;
        }
    }
};  // namespace my
#undef MOVE_NODE
#undef DELETE_ARRAY
#undef DELETE
}  // namespace my