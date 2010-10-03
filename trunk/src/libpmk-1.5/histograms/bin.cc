// Copyright 2007, Massachusetts Institute of Technology.
// The use of this code is permitted for research only. There is
// absolutely no warranty for this software.
//
// Author: John Lee (jjl@mit.edu)
//

#include "histograms/bin.h"
#include "util/sparse-vector.h"

namespace libpmk {

Bin::Bin() : TreeNode(), size_(0), count_(0) { }
Bin::Bin(const LargeIndex& index) : TreeNode(index), size_(0), count_(0) { }

double Bin::GetSize() const {
   return size_;
}

double Bin::SetSize(double size) {
   double old_size = size_;
   size_ = size;
   return old_size;
}

double Bin::IncrementCount() {
   return count_ += 1;
}

double Bin::DecrementCount() {
   return count_ -= 1;
}

double Bin::SetCount(double count) {
   return (count_ = count);
}

double Bin::GetCount() const {
   return count_;
}

void Bin::ReadData(istream& input_stream) {
   input_stream.read((char *)&size_, sizeof(double));
   input_stream.read((char *)&count_, sizeof(double));
}

void Bin::WriteData(ostream& output_stream) const {
   output_stream.write((char *)&size_, sizeof(double));
   output_stream.write((char *)&count_, sizeof(double));
}

// To combine Bins, we'll keep the max of the two sizes and add the
// counts.
void Bin::Combine(const TreeNode& tree_other) {
   const Bin& other(static_cast<const Bin&>(tree_other));
   double max_size = other.size_ > size_ ? other.size_ : size_;
   size_ = max_size;
   count_ += other.count_;
}

}  // namespace libpmk
