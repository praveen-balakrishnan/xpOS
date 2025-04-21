/**
    Copyright 2023-2025 Praveen Balakrishnan

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

    xpOS v1.0
*/

#ifndef XPOS_COMMON_BINARYSEARCHTREE_H
#define XPOS_COMMON_BINARYSEARCHTREE_H

#include <concepts>
#include <cstdint>
#include <utility>

namespace Common {

/**
 * A binary search tree that does not rely on dynamic memory allocation as it is
 * used to manage the kernel heap. It is implemented as a contiguous array starting 
 * at a specified address.
*/
template <std::totally_ordered T>
class BinarySearchTree
{
//private:
public:
    struct TreeNode;
public:

    /**
     * Constructs a binary search tree with the backing array at the specified
     * address.
     * 
     * @param address the starting address that nodes are placed at.
    */
    BinarySearchTree(void* address)
    {
        m_startingAddress = static_cast<TreeNode*>(address);
        m_root = m_startingAddress;
        m_elementCount = 0;
    }

    BinarySearchTree(BinarySearchTree const&) = delete;

    BinarySearchTree& operator=(BinarySearchTree const&) = delete;

    /**
     * Inserts an element into the binary search tree, ordered by the less than
     * predicate.
    */
    void insert_element(T element)
    {   
        // New elements are inserted as leaf nodes. We use the typical trailing
        // and leading pointer approach.
        // The trailing pointer keeps a reference to the parent.
        TreeNode* trailingPointer = nullptr;
        TreeNode* leadingPointer = get_root_node();
        while (leadingPointer) {
            trailingPointer = leadingPointer;
            leadingPointer = element < leadingPointer->value ? leadingPointer->left : leadingPointer->right;
        }
        /// TODO: allow for move semantics here
        auto node = new (m_startingAddress + m_elementCount) TreeNode(element);
        m_elementCount++;

        node->parent = trailingPointer;
        if (!trailingPointer) {
            m_root = node;
            return;
        }
        if (element < trailingPointer->value) {
            trailingPointer->left = node;
            return;
        }
        trailingPointer->right = node;
    }

    /**
     * Searches for and removes an element.
    */
    void remove_element(T element)
    {
        TreeNode* node = find_node_from_element(element, get_root_node());
        remove_node(node);

        // As nodes are not dynamically allocated, but are instead contiguous in memory, we must maintain this property.
        // If the removed node was not at the end, we move the last node in to "fill the gap".
        if (node != m_startingAddress + m_elementCount) {
            TreeNode* last = m_startingAddress + m_elementCount;
            if (last->parent) {
                if (last == last->parent->left) { last->parent->left = node; }
                if (last == last->parent->right) { last->parent->right = node; }
            } else {
                m_root = node;
            }
            if (last->left)
                last->left->parent = node;
            if (last->right)
                last->right->parent = node;
            *node = m_startingAddress[m_elementCount];
        }
    }

    /**
     * Returns the smallest element greater than or equal to the specified element
     * (the successor).
    */
    T* successor(T element)
    {
        insert_element(element);
        TreeNode* succ = successor_node(find_node_from_element(element, get_root_node()));
        remove_element(element);
        if (!succ)
            return nullptr;
        return &succ->value;
    }
    
    std::size_t size()
    {
        return m_elementCount;
    }

    /**
     * Returns the size of the backing array of the list.
     * This is used so that memory can be managed to store the tree.
     * 
     * @return the size of the backing array in bytes.
    */
    std::size_t byte_size()
    {
        return m_elementCount * sizeof(TreeNode);
    }

    /**
     * Get the size of a single node in the tree. This is used so that sufficient
     * memory can be allocated for the tree.
     * 
     * @return the size of a single node.
    */
    std::size_t node_size()
    {
        return sizeof(TreeNode);
    }

//private:

    struct TreeNode
    {
        TreeNode* left = nullptr;
        TreeNode* right = nullptr;
        TreeNode* parent = nullptr;
        T value;
        TreeNode(T value)
            : value(std::move(value))
        {}
    };

    TreeNode* m_startingAddress;

    TreeNode* m_root;

    uint64_t m_elementCount;

    void remove_node(TreeNode* node)
    {
        m_elementCount--;
        if (m_elementCount==0) { return; }
        // If we do not have a left or right child, we can simply move the other subtree in to replace the removed node
        if (!node->left) {
            substitute_nodes(node, node->right);
            return;
        }
        if (!node->right) {
            substitute_nodes(node, node->left);
            return;
        }

        // We have two remaining cases where the successor (S) displaces the removed node (N):
        // (i) if S is N's right child, S displaces N.
        // (ii) if S instead is only in N's right subtree, we first replace S with its right child, then it displaces N.

        TreeNode* succ = successor_node(node);
        if (succ->parent != node) {
            substitute_nodes(succ, succ->right);
            succ->right = node->right;
            succ->right->parent = succ;
        }
        substitute_nodes(node, succ);
        succ->left = node->left;
        succ->left->parent = succ;
    }

    void substitute_nodes(TreeNode* before, TreeNode* after)
    {
        if (!before->parent)
            m_root = after;
        else if (before == before->parent->left)
            before->parent->left = after;
        else
            before->parent->right = after; 
        
        if (after)
            after->parent = before->parent;
    }

    TreeNode* successor_node(TreeNode* node)
    {
        // If the node has a right branch, the minimum node of the subtree will be the smallest larger element.
        if (node->right) {
            return minimum_node(node->right);
        }

        // Otherwise, keep traversing up until the node is a left child.
        TreeNode* parent = node->parent;
        while (parent && node == parent->right) {
            node = parent;
            parent = node->parent;
        }
        return parent;
    }

    TreeNode* find_node_from_element(T item, TreeNode* node)
    {
        // Recursively find the node using the ordered subtree property of the binary search tree.
        if (!node || item == node->value)
            return node;
        
        if (item < node->value)
            return find_node_from_element(item, node->left);
        else
            return find_node_from_element(item, node->right);
        return node;
    }

    TreeNode* minimum_node(TreeNode* node)
    {
        // Recursively find the minimum node in the tree.
        if (node->left)
            return minimum_node(node->left);
        
        return node;
    }

    TreeNode* get_root_node()
    {
        if (m_elementCount == 0)
            return nullptr;
        TreeNode* root = m_root;

        // Verify that this is the root node.
        while (root->parent) {
            root = root->parent;
        }
        return root;
    }

    template <typename F>
    void iterate(F&& f, TreeNode* node)
    {
        f(node->value);
        if (node->left)
            iterate(f, node->left);
        if (node-> right)
            iterate(f, node->right);
    }
};

} // Namespace.

#endif // Include guard.