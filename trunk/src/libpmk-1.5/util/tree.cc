// Copyright 2007, Massachusetts Institute of Technology.
// The use of this code is permitted for research only. There is
// absolutely no warranty for this software.
//
// Author: John Lee (jjl@mit.edu)
//

#ifndef UTIL_TREE_CC
#define UTIL_TREE_CC

#include <assert.h>
#include <list>
#include <stack>
#include "util/tree.h"
#include "util/tree-node.h"
#include "util/sparse-vector.h"

namespace libpmk {

template <class T>
typename Tree<T>::PreorderIterator Tree<T>::end_preorder_ =
typename Tree<T>::PreorderIterator(NULL);

template <class T>
typename Tree<T>::PostorderIterator Tree<T>::end_postorder_ =
typename Tree<T>::PostorderIterator(NULL);

template <class T>
typename Tree<T>::BreadthFirstIterator Tree<T>::end_breadth_first_ =
typename Tree<T>::BreadthFirstIterator(NULL);

template <class T>
Tree<T>::Tree() : num_nodes_(1) {
   root_node_ = new T(LargeIndex());
}

template <class T>
Tree<T>::~Tree() {
   FreeAllNodes();
   delete root_node_;
}

template <class T>
Tree<T>::Tree(const Tree<T>& other) {
   root_node_ = new T(*(other.root_node_));
   num_nodes_ = 1;
   CopySubtree(&root_node_, other.root_node_);
}

template <class T>
void Tree<T>::CopySubtree(T** my_node, T* other_node) {
   stack<T*> todo;
   stack<T*> todo_copy;

   // Variable naming: the '_copy' suffix indicates that it
   // it is a copy of a node in <other> (i.e., it belongs to
   // this tree).
   todo.push(other_node);
   todo_copy.push(*my_node);

   while (!todo.empty()) {
      T* current = todo.top();
      T* current_copy = todo_copy.top();
      todo.pop();
      todo_copy.pop();

      if (current->HasChild()) {
         T* child = (T*)(current->GetFirstChild());
         T* first_child_copy = new T(*child);
         ++num_nodes_;

         todo.push(child);
         todo_copy.push(first_child_copy);
         current_copy->SetFirstChild(first_child_copy);
         first_child_copy->SetParent(current_copy);

         T* previous_child_copy = first_child_copy;
         child = (T*)(child->GetNextSibling());
         while (child != NULL) {
            T* child_copy = new T(*child);
            ++num_nodes_;

            todo.push(child);
            todo_copy.push(child_copy);
            child_copy->SetParent(current_copy);
            previous_child_copy->SetNextSibling(child_copy);
            child_copy->SetPreviousSibling(previous_child_copy);
            previous_child_copy = child_copy;
            child = (T*)(child->GetNextSibling());
         }
         current_copy->SetLastChild(previous_child_copy);
      }
   }   
}

template <class T>
Tree<T>::Tree(const Tree<T>& one, const Tree<T>& two) {
   root_node_ = new T(*(one.root_node_));
   num_nodes_ = 1;
   CopySubtree(&root_node_, one.root_node_);

   // <todo> keeps a list of things in <two> that need to be copied.
   // <parents> is a parallel stack which keeps track of the parent
   //   *in the current copy*, of the corresponding <todo> node.
   stack<T*> todo;
   stack<T*> parents;
   root_node_->Combine(*(two.root_node_));
   T* child = (T*)(two.root_node_->GetFirstChild());
   while (child != NULL) {
      todo.push(child);
      parents.push(root_node_);
      child = (T*)(child->GetNextSibling());
   }

   while (!todo.empty()) {
      T* two_current = todo.top();
      T* copy_parent = parents.top();
      todo.pop();
      parents.pop();
      T* copy_current = GetNode(two_current->GetIndex(), copy_parent);

      // Try to find if a counterpart to two_current exists.
      // If it doesn't exist, add the node, and deep copy the subtree.
      // If it does exist, combine the nodes and add the children to
      //  <todo>.
      if (copy_current == NULL) {
         T* new_child = AddNode(*two_current, copy_parent);
         CopySubtree(&new_child, two_current);
      } else {
         copy_current->Combine(*two_current);
         T* two_child = (T*)(two_current->GetFirstChild());
         while (two_child != NULL) {
            todo.push(two_child);
            parents.push(copy_current);
            two_child = (T*)(two_child->GetNextSibling());
         }
      }
   }
}

template <class T>
int Tree<T>::GetNumNodes() const {
   return num_nodes_;
}

template <class T>
T* Tree<T>::GetRootNode() {
   return root_node_;
}

template <class T>
T* const Tree<T>::GetRootNode() const {
   return root_node_;
}

template <class T>
T* Tree<T>::GetNode(const LargeIndex& index) {
   return GetNode(index, GetRootNode());
}

template <class T>
T* Tree<T>::GetNode(const LargeIndex& index, TreeNode* finger) {
   LargeIndex desired_index;
   TreeNode* current_node = finger;
   LargeIndex finger_index = finger->GetIndex();

   // finger's index should be a prefix of index.
   if (index.size() < finger_index.size()) {
      return NULL;
   }
   for (int ii = 0; ii < (int)finger_index.size(); ++ii) {
      if (index[ii] != finger_index[ii]) {
         return NULL;
      }
      desired_index.push_back(finger_index[ii]);
   }

   for (int current_level = finger_index.size(); ; current_level++) {
      // This means we're at the node we want
      if (current_level == (int)index.size()) {
         return (T*)current_node;
      }

      // Otherwise, search the next level for the appropriate node,
      // and set current_node to that.
      desired_index.push_back(index[current_level]);

      bool found = false;
      for (TreeNode* next_node = current_node->GetFirstChild();
           next_node != NULL; next_node = next_node->GetNextSibling()) {
         if (next_node->GetIndex().back() == desired_index.back()) {
            current_node = next_node;
            found = true;
            break;
         }
      }

      // Quit early if we didn't find that a parent for this index
      // does not exist (and therefore that node does not exist)
      if (!found) {
         return NULL;
      }
   }

   // This should never happen.
   assert(false);
   return NULL;
}

template <class T>
void Tree<T>::FreeAllNodes() {
   vector<TreeNode*> todo;
   todo.push_back(GetRootNode());

   while (!todo.empty()) {
      TreeNode* current = todo.back();
      if (current->HasChild()) {
         todo.push_back(current->GetFirstChild());
      } else {
         todo.pop_back();
         DeleteLeafNode(current);
      }
   }
   root_node_ = new T(LargeIndex());
   num_nodes_ = 1;
}

template <class T>
void Tree<T>::DeleteNode(TreeNode* node) {
   if (node == GetRootNode()) {
      FreeAllNodes();
   } else {
      vector<TreeNode*> todo;
      todo.push_back(node);
      
      while (!todo.empty()) {
         TreeNode* current = todo.back();
         if (current->HasChild()) {
            todo.push_back(current->GetFirstChild());
         } else {
            todo.pop_back();
            DeleteLeafNode(current);
         }
      }
   }
}

template <class T>
void Tree<T>::DeleteLeafNode(TreeNode* node) {
   assert(!node->HasChild());
   
   // If it's the root, just delete and quit.
   if (!node->HasParent()) {
      assert(node == root_node_);
      delete node;
      --num_nodes_;
      assert(num_nodes_ == 0);
      return;
   }

   if (node->GetParent()->GetFirstChild() == node) {
      // If I was the first child, tell my parent to reassign the first
      // child to my next sibling.
      node->GetParent()->SetFirstChild(node->GetNextSibling());
      if (node->HasNextSibling()) {
         node->GetNextSibling()->SetPreviousSibling(NULL);
      }
   } 

   if(node->GetParent()->GetLastChild() == node) {
      // If I was the last child, tell my parent to reassign the last
      // child to my previous sibling.
      node->GetParent()->SetLastChild(node->GetPreviousSibling());
      if (node->HasPreviousSibling()) {
         node->GetPreviousSibling()->SetNextSibling(NULL);
      }
   } 

   // If I was not the first child or last child, I have to tell my
   // previous sibling that its next sibling is my next sibling,
   // not me. Clearly, since I'm not the first or last child, this
   // means that I have a PreviousSibling and a NextSibling, so I
   // just notify those two nodes that they should be joined.
   if (node->HasPreviousSibling()){  
      node->GetPreviousSibling()->SetNextSibling(node->GetNextSibling());
   }
   if (node->HasNextSibling()) {
      node->GetNextSibling()->SetPreviousSibling(node->GetPreviousSibling());
   }
   

   // I am now a dangling node, ready to be deleted.
   delete node;
   --num_nodes_;
}


// Automatically increments num_bins_.
template <class T>
T* Tree<T>::AddNode(const T& new_node) {
   // Special case: if new_node is the root node.
   if (new_node.GetIndex().size() == 0) {
      root_node_->Combine(new_node);
      return root_node_;
   }
   
   // Try to find the parent node.
   LargeIndex parent_index(new_node.GetIndex());
   parent_index.pop_back();
   T* parent = GetNode(parent_index);
   assert(parent != NULL);
   return AddNode(new_node, parent);
}

template<class T>
T* Tree<T>::AddNode(const T& new_node, TreeNode* parent) {
   LargeIndex new_index = new_node.GetIndex();
   new_index.pop_back();
   if (new_index != parent->GetIndex()) {
      return NULL;
   }

   // Try to see if a node with this index already exists.
   bool found = false;
   TreeNode* child = parent->GetFirstChild();
   while (child != NULL) {
      // For equality testing of indices, we only need to check the last
      // element, because we are guaranteed that the parent node is
      // a prefix of the new bin.
      if (new_node.GetIndex().back() == child->GetIndex().back()) {
         found = true;
         break;
      }
      child = child->GetNextSibling();
   }

   if (found) {
      // If such a node exists, Combine it with the new one.
      child->Combine(new_node);
      return (T*)child;
   } else {
      // Otherwise, create a copy of new_node and update the link structure.
      T* added = new T(new_node);
      added->SetParent(parent);

      if (parent->GetFirstChild() == NULL) {
         // Case 1: the parent was a leaf
         parent->SetFirstChild(added);
         parent->SetLastChild(added);
         ++num_nodes_;
         return added;
      } else {
         TreeNode* iter = parent->GetFirstChild();
         if (iter->GetIndex().back() > added->GetIndex().back()) {
            // Case 2: added bin is the new first child
            parent->SetFirstChild(added);
            added->SetNextSibling(iter);
            iter->SetPreviousSibling(added);
            ++num_nodes_;
            return added;
         }
         while (iter->HasNextSibling()) {
            // Case 3: added bin is somewhere in the middle
            TreeNode* next = iter->GetNextSibling();
            if (next->GetIndex().back() > added->GetIndex().back()) {
               iter->SetNextSibling(added);
               added->SetNextSibling(next);
               added->SetPreviousSibling(iter);
               next->SetPreviousSibling(added);
               ++num_nodes_;
               return added;
            }
            iter = iter->GetNextSibling();
         }
         // Case 4: added bin is at the end
         iter->SetNextSibling(added);
         added->SetPreviousSibling(iter);
         parent->SetLastChild(added);
         ++num_nodes_;
         return added;
      }
   }
}

template <class T>
void Tree<T>::WriteToStream(ostream& output_stream) const {
   assert(output_stream.good());
   int32_t num_nodes = GetNumNodes();
   output_stream.write((char *)&num_nodes, sizeof(int32_t));
   
   PreorderIterator iter = BeginPreorder();
   while (iter != EndPreorder()) {
      iter->WriteToStream(output_stream);
      ++iter;
   }
}


template <class T>
void Tree<T>::ReadFromStream(istream& input_stream) {
   assert(input_stream.good());
   FreeAllNodes();

   int32_t num_bins = 0;
   input_stream.read((char *)&num_bins, sizeof(int32_t));
   root_node_->ReadFromStream(input_stream);

   // The first (root) bin always has an empty index.
   assert((int)root_node_->GetIndex().size() == 0);

   // (num_bins - 1) since we already read the root bin.
   for (int ii = 0; ii < num_bins - 1; ++ii) {
      // TODO(jjl): The reading and the AddBin() will double copy the
      // data.  We can make this quicker by only copying data once and
      // implementing a function allowing us to directly add a bin
      // rather than add a copy, since we know that the bin won't
      // already exist.
      T temp;
      temp.ReadFromStream(input_stream);
      AddNode(temp);
   }
}



template <class T>
typename Tree<T>::Iterator& Tree<T>::PreorderIterator::operator++() {
   // If we're not already at the end, try to increment:
   if (Iterator::node_ != NULL) {

      // First, see if there are children to expand.
      // We need to add children in reverse order, so that the
      // first child is at the end of <todo_>.
      if (Iterator::node_->HasChild()) {
         TreeNode* child = Iterator::node_->GetLastChild();
         while (child != NULL) {
            todo_.push_back(child);
            child = child->GetPreviousSibling();
         }
      }
      
      // This is where the actual incrmenting happens; we set <node_> to
      // the next position in the DFS traversal.
      if (!todo_.empty()) {
         Iterator::node_ = todo_.back();
         todo_.pop_back();
      } else {
         Iterator::node_ = NULL;
      }
   }
   return *this;
}

template <class T>
Tree<T>::PostorderIterator::PostorderIterator(T* node) : Iterator(node) {
   if (node == NULL) {
      Iterator::node_ = NULL;
   } else {
      todo_.push(node);
      visited_.push(false);
      while (visited_.top() == false) {
         visited_.top() = true;
         
         TreeNode* to_expand = todo_.top();
         if (to_expand->HasChild()) {
            TreeNode* child = to_expand->GetLastChild();
            while (child != NULL) {
               todo_.push(child);
               visited_.push(false);
               child = child->GetPreviousSibling();
            }
         }
         
      }
      Iterator::node_ = todo_.top();
   }
}

template <class T>
typename Tree<T>::Iterator& Tree<T>::PostorderIterator::operator++() {
   // If we're not already at the end, try to increment:
   if (Iterator::node_ != NULL) {
      visited_.pop();
      todo_.pop();
   
      if (todo_.empty()) {
         Iterator::node_ = NULL;
         return *this;
      }

      // If new top is not visited, visit and expand, repeat.
      while (visited_.top() == false) {
         visited_.top() = true;
       
         if (todo_.top()->HasChild()) {
            TreeNode* child = todo_.top()->GetLastChild();
            while (child != NULL) {
               todo_.push(child);
               visited_.push(false);
               child = child->GetPreviousSibling();
            }
         }
      }

      // if it is visited, do nothing (it's the new node_)
      Iterator::node_ = todo_.top();
   }
   return *this;
}

template <class T>
typename Tree<T>::Iterator& Tree<T>::BreadthFirstIterator::operator++() {
   if (Iterator::node_ != NULL) {

      if (Iterator::node_->HasChild()) {
         TreeNode* child = Iterator::node_->GetFirstChild();
         while (child != NULL) {
            todo_.push_back(child);
            child = child->GetNextSibling();
         }
      }
      
      if (!todo_.empty()) {
         Iterator::node_ = todo_.front();
         
         // We need to add children in reverse order, so that the first
         // child is at the end of todo_.
         todo_.pop_front();
      } else {
         Iterator::node_ = NULL;
      }
   }
   return *this;
}

template <class T>
typename Tree<T>::PreorderIterator Tree<T>::BeginPreorder() const {
   return PreorderIterator(root_node_);
}

template <class T>
const typename Tree<T>::PreorderIterator& Tree<T>::EndPreorder() {
   return end_preorder_;
}

template <class T>
typename Tree<T>::PostorderIterator Tree<T>::BeginPostorder() const {
   return PostorderIterator(root_node_);
}

template <class T>
const typename Tree<T>::PostorderIterator& Tree<T>::EndPostorder() {
   return end_postorder_;
}

template <class T>
typename Tree<T>::BreadthFirstIterator Tree<T>::BeginBreadthFirst() const {
   return BreadthFirstIterator(root_node_);
}

template <class T>
const typename Tree<T>::BreadthFirstIterator&
Tree<T>::EndBreadthFirst() {
   return end_breadth_first_;
}

}  // namespace libpmk

#endif  // UTIL_TREE_CC
