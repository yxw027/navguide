// Copyright 2007, Massachusetts Institute of Technology.
// The use of this code is permitted for research only. There is
// absolutely no warranty for this software.
//
// Author: John Lee (jjl@mit.edu)
//

#ifndef UTIL_TREE_NODE_H
#define UTIL_TREE_NODE_H

#include <iostream>
#include "util/sparse-vector.h"

using namespace std;

namespace libpmk {

/// An indexed node, used by Tree.
/**
 * A TreeNode has an identifier of type LargeIndex (a vector of
 * ints). TreeNode also contains pointers to parents, children, and
 * siblings, but makes no effort to maintain about them, and as such,
 * we make no guarantees on it. For example, the "sibling"
 * pointer can point to an arbitrary bin which may or may not be the
 * real sibling. Tree handles the getting/setting of these pointers.
 *
 * To augment a TreeNode with your own data, there are three functions
 * you need to provide: (1) reading the data from a stream, (2)
 * writing the data to a stream, and (3) how to combine two nodes. The
 * definition of "combine" will vary depending on what you're doing,
 * but the general idea is that sometimes you will have two TreeNodes
 * with the same LargeIndex that you want to insert into a tree.  It
 * is advisable that you make the output of Combine() is symmetric,
 * since there are no guarantees on what Tree does with
 * duplicate-index bins other than calling Combine().
 *
 * Remember that if you implement a copy constructor for your TreeNode,
 * you should have your copy constructor call the base class's copy
 * constructor: <br>
 * <tt>
 * MyNode::MyNode(const MyNode& other) <b>: TreeNode(other)</b> {<br>
 * // your copy code<br>
 * }
 * </tt>
 */
class TreeNode {
 public:
   /// Create a new node with an empty index.
   TreeNode();

   /// Create a new node with the given index.
   TreeNode(const LargeIndex& index);

   /// A 'copy' constructor, which only copies over the index.
   /**
    * The pointers to data will NOT be copied over; they will be NULL.
    */
   TreeNode(const TreeNode& other);

   virtual ~TreeNode();

   const LargeIndex& GetIndex() const;
   TreeNode* GetParent();
   TreeNode* GetPreviousSibling();
   TreeNode* GetNextSibling();
   TreeNode* GetFirstChild();
   TreeNode* GetLastChild();

   void SetParent(TreeNode* parent);
   void SetPreviousSibling(TreeNode* sibling);
   void SetNextSibling(TreeNode* sibling);
   void SetFirstChild(TreeNode* child);
   void SetLastChild(TreeNode* child);
   void SetIndex(const LargeIndex& index);

   bool HasParent() const;
   bool HasPreviousSibling() const;
   bool HasNextSibling() const;
   bool HasChild() const;
   
   /// Read the data, including the index, from a stream.
   /**
    * Calling this will override the node's current index.
    * The format is:
    * <ul>
    * <li>(int32_t) index_size</li>
    * <li>(index_size * int32_t) the index</li>
    * </ul>
    * It will then call ReadData().
    * \sa ReadData()
    */
   void ReadFromStream(istream& input_stream);

   /// Write the data, including the index, to a stream.
   /**
    * This will simply write the index to the stream, followed by
    * WriteData().
    * \sa WriteData().
    */
   void WriteToStream(ostream& output_stream) const;

   /// Add the data from <other> to self.
   virtual void Combine(const TreeNode& other) = 0;

   /// Returns true if this node's index is smaller than that of <other>.
   bool operator<(const TreeNode& other) const;

   /// Returns true if <one>'s index is smaller than that of <two>.
   static bool CompareNodes(const TreeNode* one, const TreeNode* two);

 protected:
   /// Read the data, excluding the index, from a stream.
   virtual void ReadData(istream& input_stream) = 0;

   /// Write the data, excluding the index, to a stream.
   virtual void WriteData(ostream& output_stream) const = 0;
 private:
   LargeIndex index_;
   TreeNode* parent_;
   TreeNode* next_sibling_;
   TreeNode* previous_sibling_;
   TreeNode* first_child_;
   TreeNode* last_child_;
};
}  // namespace libpmk
#endif  // UTIL_TREE_NODE_H
