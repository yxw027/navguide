// Copyright 2007, Massachusetts Institute of Technology.
// The use of this code is permitted for research only. There is
// absolutely no warranty for this software.
//
// Author: John Lee (jjl@mit.edu)
//
// TODO(jjl): global_used_bins is no longer needed.

#ifndef PYRAMIDS_GLOBAL_VG_PYRAMID_MAKER_H
#define PYRAMIDS_GLOBAL_VG_PYRAMID_MAKER_H

#include <vector>
#include <iostream>
#include <set>
#include "pyramids/vg-pyramid-maker.h"
#include "point_set/point-set-list.h"
#include "util/sparse-vector.h"
#include "histograms/multi-resolution-histogram.h"
#include "util/distance-computer.h"
#include "clustering/hierarchical-clusterer.h"
#include "util/tree-node.h"
#include "util/tree.cc"

namespace libpmk {

/// \brief Makes pyramids with bin sizes determined by a particular
/// set of points.
class GlobalVGPyramidMaker : public VGPyramidMaker {
 public:
   GlobalVGPyramidMaker(const HierarchicalClusterer& clusterer,
                        const DistanceComputer& distance_computer);
   ~GlobalVGPyramidMaker();

   /// Initialize the global bin sizes.
   /**
    * This takes O(n^2) time, where n is the total number of points in
    * <point_sets>. This is because at the top level bin, it computes
    * the furthest pair between any of the two points to set the bin
    * size.
    *
    * Rather than doing this every single time, you may also call
    * ReadFromStream() which will read already-preprocessed data.
    */
   void Preprocess(const PointSetList& point_sets);

   virtual MultiResolutionHistogram* MakePyramid(const PointSet& point_set);

   /// Initialize the global bin sizes.
   /**
    * Can be called in lieu of Preprocess().  Aborts if the stream is
    * bad.
    */ 
   void ReadFromStream(istream& input_stream);

   /// Initialize the global bin sizes.
   /**
    * \sa ReadFromStream
    */ 
   void ReadFromFile(const char* filename);

   /// Write the global bin sizes to a stream.
   /**
    * Requires Preprocess() or ReadFromStream() to be called first.
    * Aborts if the stream is bad.
    */ 
   void WriteToStream(ostream& output_stream) const;

   /// Write the global bin sizes to a file.
   /**
    * \sa WriteToStream
    */ 
   void WriteToFile(const char* filename) const;

 protected:
   /// Get the membership data relative to the global pyramid data.
   /**
    * This function is overridden so that only paths that were
    * present in the point sets that this was constructed with
    * can appear. Otherwise, this function does the same thing
    * as its parent VGPyramidMaker::GetMembershipPath().
    * \sa VGPyramidMaker::GetMembershipPath()
    */
   virtual pair<LargeIndex, vector<double> >
      GetMembershipPath(const Feature& f);

 private:
   class IndexNode : public TreeNode {
   public:
      IndexNode() : TreeNode() {}
      IndexNode(const LargeIndex& index) : TreeNode(index) {}
      virtual ~IndexNode() { }

      virtual void Combine(const TreeNode& other) {
         const set<int>& other_indices = 
            static_cast<const IndexNode&>(other).indices_;
         for (set<int>::iterator iter = other_indices.begin();
              iter != other_indices.end(); ++iter) {
            indices_.insert(*iter);
         }
      }

      const set<int>& GetIndices() const {
         return indices_;
      }

      void AddIndex(int index) {
         indices_.insert(index);
      }
      
   protected:
      virtual void ReadData(istream& input_stream) {
         int32_t size;
         input_stream.read((char *)&size, sizeof(int32_t));
         for (int ii = 0; ii < size; ++ii) {
            int32_t index;
            input_stream.read((char *)&index, sizeof(int32_t));
            indices_.insert((int)index);
         }
      }
      
      virtual void WriteData(ostream& output_stream) const {
         int32_t size = (int32_t)indices_.size();
         output_stream.write((char *)&size, sizeof(int32_t));
         for (set<int>::iterator iter = indices_.begin();
              iter != indices_.end(); ++iter) {
            int32_t index = *iter;
            output_stream.write((char *)&index, sizeof(int32_t));
         }
      }
      
   private:
      set<int> indices_;
   };


   auto_ptr<MultiResolutionHistogram> global_pyramid_;
   vector<vector<bool> > global_used_bins_;
   bool preprocessed_;
};
}  // namespace libpmk

#endif  // PYRAMIDS_GLOBAL_VG_PYRAMID_MAKER_H
