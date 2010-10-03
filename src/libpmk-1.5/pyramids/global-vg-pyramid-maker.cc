// Copyright 2007, Massachusetts Institute of Technology.
// The use of this code is permitted for research only. There is
// absolutely no warranty for this software.
//
// Author: John Lee (jjl@mit.edu)
//

#include <values.h>
#include <vector>
#include <fstream>
#include "pyramids/global-vg-pyramid-maker.h"
#include "point_set/point-set-list.h"
#include "util/sparse-vector.h"
#include "histograms/multi-resolution-histogram.h"
#include "util/distance-computer.h"
#include "clustering/hierarchical-clusterer.h"

namespace libpmk {

GlobalVGPyramidMaker::GlobalVGPyramidMaker(const HierarchicalClusterer& c,
                                           const DistanceComputer& dist) :
   VGPyramidMaker(c, dist), preprocessed_(false) {
   const PointSetList& centers(c.GetClusterCenters());
   global_used_bins_.resize(centers.GetNumPointSets());
   for (int ii = 0; ii < centers.GetNumPointSets(); ++ii) {
      global_used_bins_[ii].resize(centers[ii].GetNumFeatures());
      for (int jj = 0; jj < centers[ii].GetNumFeatures(); ++jj) {
         global_used_bins_[ii][jj] = false;
      }
   }
}

GlobalVGPyramidMaker::~GlobalVGPyramidMaker() {
   global_pyramid_.reset(NULL);
}

void GlobalVGPyramidMaker::Preprocess(const PointSetList& point_sets) {
   global_pyramid_.reset(new MultiResolutionHistogram());
   Tree<IndexNode> inverted_tree;

   vector<PointRef> all_points;
   point_sets.GetPointRefs(&all_points);

   for (int ii = 0; ii < (int)all_points.size(); ++ii) {
      // We explicitly use VGPyramidMaker::GetMembershipPath because
      // it does not mess with global_pyramid_. We don't care about
      // global_pyramid_, because Preprocess() is the function that
      // generates it.
      pair<LargeIndex, vector<double> > membership(
         VGPyramidMaker::GetMembershipPath(all_points[ii].GetFeature()));

      const LargeIndex& membership_path(membership.first);

      LargeIndex path;
      Bin* finger;
      IndexNode* finger_index;

      Bin root_bin(path);
      IndexNode root_index_node(path);

      root_bin.SetCount(all_points[ii].GetFeature().GetWeight());
      root_index_node.AddIndex(ii);

      finger = global_pyramid_->AddBin(root_bin);
      finger_index = inverted_tree.AddNode(root_index_node);

      global_used_bins_[0][0] = true;

      for (int jj = 1; jj < (int)membership_path.size(); ++jj) {
         int level = jj;
         int index = membership_path[jj];
         global_used_bins_.at(level).at(index) = true;
         path.push_back(index);

         Bin new_bin(path);
         IndexNode new_index_node(path);

         new_bin.SetCount(all_points[ii].GetFeature().GetWeight());
         new_index_node.AddIndex(ii);

         finger = global_pyramid_->AddBin(new_bin, finger);
         finger_index = inverted_tree.AddNode(new_index_node, finger_index);
      }
   }

   // Now that the structure of the tree is computed, traverse the tree
   // once. For each bin, take the PointRefs that belong in that bin,
   // find the max pairwise distance, and set it as the size.

   int count = 0;
   vector<Bin*> todo;
   todo.push_back(global_pyramid_->GetRootBin());

   Tree<IndexNode>::PreorderIterator iter = inverted_tree.BeginPreorder();

   while (!todo.empty()) {
      assert(iter != inverted_tree.EndPreorder());
      // Process the bin at the top of the stack.
      Bin* current = todo.back();
      todo.pop_back();
      IndexNode* current_indices = iter.get();
      assert(current_indices->GetIndex() == current->GetIndex());

      double max_size = 0;
      const set<int>& indices(current_indices->GetIndices());
      for (set<int>::const_iterator iter1 = indices.begin();
           iter1 != indices.end(); ++iter1) {
         for (set<int>::const_iterator iter2 = indices.begin();
              iter2 != iter1; ++iter2) {
            double distance = distance_computer_.ComputeDistance(
               all_points[*iter1].GetFeature(),
               all_points[*iter2].GetFeature());
            if (distance > max_size) {
               max_size = distance;
            }
         }
      }
      current->SetSize(max_size);

      if (current->HasChild()) {
         vector<Bin*> to_push;
         Bin* child_iter = (Bin*)(current->GetFirstChild());
         while (child_iter != NULL) {
            to_push.push_back(child_iter);
            child_iter = (Bin*)(child_iter->GetNextSibling());
         }

         while (!to_push.empty()) {
            todo.push_back(to_push.back());
            to_push.pop_back();
         }
      }
      ++iter;
      ++count;
   }

   preprocessed_ = true;
}


MultiResolutionHistogram* GlobalVGPyramidMaker::
MakePyramid(const PointSet& point_set) {
   assert(preprocessed_);
   
   MultiResolutionHistogram* pyramid = new MultiResolutionHistogram();
   
   for (int ii = 0; ii < point_set.GetNumFeatures(); ++ii) {
      // Place the point into the appropriate set of bins:
      pair<LargeIndex, vector<double> > membership(
         GlobalVGPyramidMaker::GetMembershipPath(point_set[ii]));
      const LargeIndex& membership_path(membership.first);

      LargeIndex path;
      Bin* finger;         // Current position in the local pyramid
      Bin* global_finger;  // Current position in the global pyramid
      Bin root_bin(path);
      root_bin.SetCount(point_set[ii].GetWeight());
      root_bin.SetSize(global_pyramid_->GetBin(path)->GetSize());
      finger = pyramid->AddBin(root_bin);
      global_finger = global_pyramid_->GetBin(path);

      for (int jj = 1; jj < (int)membership_path.size(); ++jj) {
         path.push_back(membership_path[jj]);
         Bin new_bin(path);
         Bin* global_bin(global_pyramid_->GetBin(path, global_finger));
         new_bin.SetCount(point_set[ii].GetWeight());
         new_bin.SetSize(global_bin->GetSize());
         finger = pyramid->AddBin(new_bin, finger);
         global_finger = global_bin;
      }
   }
   return pyramid;
}


void GlobalVGPyramidMaker::ReadFromStream(istream& input_stream) {
   global_pyramid_.reset(
      MultiResolutionHistogram::ReadSingleHistogramFromStream(input_stream));
   preprocessed_ = true;
}

void GlobalVGPyramidMaker::ReadFromFile(const char* filename) {
   ifstream input_stream(filename, ios::binary);
   ReadFromStream(input_stream);
   input_stream.close();
}

void GlobalVGPyramidMaker::WriteToStream(ostream& output_stream) const {
   assert(preprocessed_);
   MultiResolutionHistogram::
      WriteSingleHistogramToStream(output_stream, global_pyramid_.get());
}

void GlobalVGPyramidMaker::WriteToFile(const char* filename) const {
   ofstream output_stream(filename, ios::binary | ios::trunc);
   WriteToStream(output_stream);
   output_stream.close();
}

pair<LargeIndex, vector<double> > GlobalVGPyramidMaker::
GetMembershipPath(const Feature& f) {
   LargeIndex answer;
   vector<double> distances;

   // For each level, find the bin center closest to f, and add it
   // to <answer>. We can skip the first computation since it's always 0.
   answer.push_back(0);
   distances.push_back(distance_computer_.ComputeDistance(f, centers_[0][0]));

   for (int ii = 1; ii < centers_.GetNumPointSets(); ++ii) {
      double min_distance = DBL_MAX;
      int best_index = -1;

      pair<int, int> range = clusterer_.GetChildRange(ii-1, answer.back());
      assert(range.first >= 0);
      assert(range.second >= 0);
      for (int jj = range.first; jj <= range.second; ++jj) {
         if (global_used_bins_.at(ii).at(jj) == true) {
            double distance =
               distance_computer_.ComputeDistance(f, centers_[ii][jj],
                                                  min_distance);
            if (distance < min_distance) {
               min_distance = distance;
               best_index = jj;
            }
         }
      }

      assert(best_index != -1);
      answer.push_back(best_index);
      distances.push_back(min_distance);
   }

   assert(answer.size() == distances.size());
   return pair<LargeIndex, vector<double> >(answer, distances);
}

}  // namespace libpmk
