// Copyright 2007, Massachusetts Institute of Technology.
// The use of this code is permitted for research only. There is
// absolutely no warranty for this software.
//
// Author: John Lee (jjl@mit.edu)
//
#ifndef HISTOGRAMS_BIN_H
#define HISTOGRAMS_BIN_H

#include <iostream>
#include "util/sparse-vector.h"
#include "util/tree-node.h"

namespace libpmk {
/// Encapsulates a histogram bin.
/**
 * A Bin is implemented as a TreeNode with two data members: a size
 * and a count.
 */
class Bin : public TreeNode {
 public:
   Bin();
   Bin(const LargeIndex& index);
   virtual ~Bin() { }

   /// Sets the size of the bin; returns the old size.
   double SetSize(double size);

   /// Get the size of the bin.
   double GetSize() const;

   /// Increase the count by 1. Returns the final count.
   double IncrementCount();

   /// Decrease the count by 1. Returns the final count.
   double DecrementCount();

   /// Set the count. Returns the new count.
   double SetCount(double count);

   /// Get the current count.
   double GetCount() const;

   virtual void ReadData(istream& input_stream);
   virtual void WriteData(ostream& output_stream) const;

   /// Combine data from two Bins.
   /**
    * To combine bins, we take the max of the sizes, and add the
    * counts.
    */
   virtual void Combine(const TreeNode& other);

 private:
   double size_;
   double count_;
};
}  // namespace libpmk
#endif  // HISTOGRAMS_BIN_H
