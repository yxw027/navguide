// Copyright 2007, Massachusetts Institute of Technology.
// The use of this code is permitted for research only. There is
// absolutely no warranty for this software.
//
// Author: John Lee (jjl@mit.edu)
//

#include <iostream>
#include "util/tree-node.h"

using namespace std;
using namespace libpmk;

TreeNode::TreeNode() : index_(LargeIndex()), parent_(NULL),
                       next_sibling_(NULL), previous_sibling_(NULL),
                       first_child_(NULL), last_child_(NULL) { }

TreeNode::TreeNode(const TreeNode& other) :
   index_(other.index_), parent_(NULL),
   next_sibling_(NULL), previous_sibling_(NULL),
   first_child_(NULL), last_child_(NULL) { }

TreeNode::TreeNode(const LargeIndex& index) :
   index_(index), parent_(NULL), next_sibling_(NULL), previous_sibling_(NULL),
   first_child_(NULL), last_child_(NULL) { }

TreeNode::~TreeNode() {
   index_.clear();
}

const LargeIndex& TreeNode::GetIndex() const {
   return index_;
}

TreeNode* TreeNode::GetParent() {
   return parent_;
}

TreeNode* TreeNode::GetNextSibling() {
   return next_sibling_;
}

TreeNode* TreeNode::GetPreviousSibling() {
   return previous_sibling_;
}

TreeNode* TreeNode::GetFirstChild() {
   return first_child_;
}

TreeNode* TreeNode::GetLastChild() {
   return last_child_;
}

void TreeNode::SetParent(TreeNode* parent) {
   parent_ = parent;
}

void TreeNode::SetIndex(const LargeIndex& index) {
   index_ = index;
}

void TreeNode::SetNextSibling(TreeNode* sibling) {
   next_sibling_ = sibling;
}

void TreeNode::SetPreviousSibling(TreeNode* sibling) {
   previous_sibling_ = sibling;
}

void TreeNode::SetFirstChild(TreeNode* child) {
   first_child_ = child;
}

void TreeNode::SetLastChild(TreeNode* child) {
   last_child_ = child;
}

bool TreeNode::HasParent() const {
   return parent_ != NULL;
}

bool TreeNode::HasNextSibling() const {
   return next_sibling_ != NULL;
}

bool TreeNode::HasPreviousSibling() const {
   return previous_sibling_ != NULL;
}

bool TreeNode::HasChild() const {
   return first_child_ != NULL;
}

bool TreeNode::operator<(const TreeNode& other) const {
   return index_ < other.index_;
}

bool TreeNode::CompareNodes(const TreeNode* a, const TreeNode* b) {
   return a->GetIndex() < b->GetIndex();
}

void TreeNode::ReadFromStream(istream& input_stream) {
   index_.clear();
   int32_t index_size;
   input_stream.read((char *)&index_size, sizeof(int32_t));
   for (int ii = 0; ii < index_size; ++ii) {
      int32_t index_element;
      input_stream.read((char *)&index_element, sizeof(int32_t));
      index_.push_back(index_element);
   }
   ReadData(input_stream);
}

void TreeNode::WriteToStream(ostream& output_stream) const {
   int32_t index_size = index_.size();   
   output_stream.write((char *)&index_size, sizeof(int32_t));
   for (int ii = 0; ii < index_size; ++ii) {
      int32_t element = index_[ii];
      output_stream.write((char *)&element, sizeof(int32_t));
   }
   WriteData(output_stream);
}

