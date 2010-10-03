// Copyright 2007, Massachusetts Institute of Technology.
// The use of this code is permitted for research only. There is
// absolutely no warranty for this software.
//
// Author: John Lee (jjl@mit.edu)
//

#include <iostream>
#include <vector>
#include <fstream>
#include "clustering/hierarchical-clusterer.h"
#include "clustering/k-means-clusterer.h"
#include "util/sparse-vector.h"
#include "util/distance-computer.h"

using namespace std;

namespace libpmk {

const int HierarchicalClusterer::MAX_ITERATIONS(100);

HierarchicalClusterer::HierarchicalClusterer() : done_(false) { }

const PointSetList& HierarchicalClusterer::GetClusterCenters() const {
   assert(done_);
   return centers_;
}

const vector<LargeIndex>& HierarchicalClusterer::GetMemberships() const {
   assert(done_);
   return memberships_;
}

pair<int, int> HierarchicalClusterer::
GetChildRange(int level, int index) const {
   return pair<int, int>((int)tree_indices_[level][index][1],
                         (int)tree_indices_[level][index][2]);
}

int HierarchicalClusterer::GetParentIndex(int level, int index) const {
   return (int)tree_indices_[level][index][0];
}

void HierarchicalClusterer::
Cluster(int num_levels, int branch_factor, const vector<PointRef>& all_points,
        const DistanceComputer& distance_computer) {
   // Do the top level, which is just the centroid (k-means with 1 cluster).
   KMeansClusterer clusterer(1, MAX_ITERATIONS, distance_computer);
   clusterer.Cluster(all_points);
   centers_.AddPointSet(clusterer.GetClusterCenters());

   PointSet indices(3);
   Feature span(3);
   span[0] = -1; span[1] = -1; span[2] = -1;
   indices.AddFeature(span);
   tree_indices_.AddPointSet(indices);

   memberships_.resize(all_points.size());

   // Finish initializing centers_ with empty point sets. Start at ii=1
   // since ii=0 was already handled in the previous line.
   for (int ii = 0; ii < (int)all_points.size(); ++ii) {
      memberships_[ii].push_back(0);
   }

   // Build a list of indices into all_points of things we want to continue
   // to cluster. In this case, we want to continue clustering all of the
   // points.
   vector<int> to_cluster;
   for (int ii = 0; ii < (int)all_points.size(); ++ii) {
      to_cluster.push_back(ii);
   }

   RecursiveCluster(1, to_cluster, num_levels, branch_factor, all_points,
                    distance_computer, 0);
   done_ = true;   
}



// RecursiveCluster will cluster the data, then break the input indices
// into smaller chunks and recursively cluster those. These smaller chunks,
// of course, are determined by k-means clustering.
// parent_index is the index of the parent center into centers_[level].
void HierarchicalClusterer::
RecursiveCluster(int level, const vector<int>& indices, int num_levels,
                 int branch_factor, const vector<PointRef>& all_points,
                 const DistanceComputer& distance_computer,
                 int parent_index) {
   auto_ptr<KMeansClusterer> clusterer(
      new KMeansClusterer(branch_factor, MAX_ITERATIONS, distance_computer));
   
   // Convert the indices into a list of PointRefs which we can throw
   // into the clusterer.
   vector<PointRef> points;
   for (int ii = 0; ii < (int)indices.size(); ++ii) {
      points.push_back(all_points[indices[ii]]);
   }


   clusterer->Cluster(points);

   // Add these centers to end of centers_[level].
   if (level >= centers_.GetNumPointSets()) {
      PointSet empty(centers_[0].GetFeatureDim());
      centers_.AddPointSet(empty);

      PointSet indices(3);
      tree_indices_.AddPointSet(indices);
   }

   // cluster_offset holds the index of the first new element that we
   // will add to centers_[level].
   int cluster_offset = centers_[level].GetNumFeatures();

   PointSet centers(clusterer->GetClusterCenters());

   for (int ii = 0; ii < centers.GetNumFeatures(); ++ii) {
      centers_[level].AddFeature(centers[ii]);

      Feature span(3);
      span[0] = parent_index;
      span[1] = -1; span[2] = -1;
      tree_indices_[level].AddFeature(span);
   }

   tree_indices_[level-1][parent_index][1] = cluster_offset;
   tree_indices_[level-1][parent_index][2] =
      cluster_offset + centers.GetNumFeatures() - 1;


   // Update memberships_ to reflect the newly computed clusters.
   vector<int> membership = clusterer->GetMembership();
   for (int ii = 0; ii < (int)indices.size(); ++ii) {
      assert((int)memberships_[indices[ii]].size() == level);
      int index = membership[ii] + cluster_offset;
      memberships_[indices[ii]].push_back(index);
   }

   // Before we start recursing, free up as much memory as we can.
   clusterer.reset(NULL);

   
   // Only bother continuing to cluster lower levels if we need to.
   if (level < num_levels - 1) {
      // For each cluster returned:
      for (int ii = 0; ii < branch_factor; ++ii) {
         // Create a new list of indices, which contain only points
         // that belong to this cluster.
         // The variable names can get confusing, so here is a summary:
         // * ii indexes a "local" cluster; "local" means it is something
         //   that was returned by the k-means clusterer that just ran.
         // * jj indexes a point within local cluster.
         // * indices[jj] gives you an index into all_points corresponding
         //   to the point that is referred to by the local point jj.
         vector<int> next_level_indices;
         bool found = false;
         for (int jj = 0; jj < (int)membership.size(); ++jj) {
            if (membership[jj] == ii) {
               next_level_indices.push_back(indices[jj]);
               found = true;
            }
         }

         // Only recurse if there are more points to cluster.
         if (found) {
            RecursiveCluster(level + 1, next_level_indices, num_levels,
                             branch_factor, all_points,
                             distance_computer, cluster_offset + ii);
         }
      }
   }
}


// Output format:
// (int32_t) num levels
// (int32_t) num points
// (int32_t) feature dim f
// foreach level:
//   (int32_t) num centers at this level
//   foreach center:
//     (f * float) the center
//     (int32_t) the index of the parent in the previous level
//     (2 * int32_t) the indices of this center's children in the next level
// foreach point
//    (int32_t) size of point's largeindex n
//    (n * int32_t) the point's largeindex
//   
void HierarchicalClusterer::WriteToStream(ostream& output_stream) const {
   assert(done_);
   assert(output_stream.good());
   int32_t num_points = (int32_t)memberships_.size();
   int32_t num_levels = (int32_t)centers_.GetNumPointSets();
   int32_t feature_dim = (int32_t)centers_[0].GetFeatureDim();
   output_stream.write((char*)&num_levels, sizeof(int32_t));
   output_stream.write((char*)&num_points, sizeof(int32_t));
   output_stream.write((char*)&feature_dim, sizeof(int32_t));

   for (int ii = 0; ii < num_levels; ++ii) {
      int32_t num_centers = centers_[ii].GetNumFeatures();
      output_stream.write((char*)&num_centers, sizeof(int32_t));
      for (int jj = 0; jj < num_centers; ++jj) {
         for (int kk = 0; kk < feature_dim; ++kk) {
            float value = centers_[ii][jj][kk];
            output_stream.write((char*)&value, sizeof(float));
         }

         int32_t parent = (int32_t)tree_indices_[ii][jj][0];
         int32_t start = (int32_t)tree_indices_[ii][jj][1];
         int32_t end = (int32_t)tree_indices_[ii][jj][2];
         output_stream.write((char*)&parent, sizeof(int32_t));
         output_stream.write((char*)&start, sizeof(int32_t));
         output_stream.write((char*)&end, sizeof(int32_t));
         
      }
   }
   
   for (int ii = 0; ii < num_points; ++ii) {
      int32_t index_size = (int32_t)memberships_[ii].size();
      output_stream.write((char*)&index_size, sizeof(int32_t));
      for (int jj = 0; jj < index_size; ++jj) {
         int32_t value = memberships_[ii][jj];
         output_stream.write((char*)&value, sizeof(int32_t));
      }
   }
}

void HierarchicalClusterer::WriteToFile(const char* filename) const {
   ofstream output_stream(filename, ios::binary | ios::trunc);
   WriteToStream(output_stream);
   output_stream.close();
}

void HierarchicalClusterer::ReadFromFile(const char* filename) {
   ifstream input_stream(filename, ios::binary);
   ReadFromStream(input_stream);
   input_stream.close();
}

void HierarchicalClusterer::ReadFromStream(istream& input_stream) {
   assert(input_stream.good());
   assert(centers_.GetNumPointSets() == 0);
   int32_t num_levels, num_points, feature_dim;
   input_stream.read((char*)&num_levels, sizeof(int32_t));
   input_stream.read((char*)&num_points, sizeof(int32_t));
   input_stream.read((char*)&feature_dim, sizeof(int32_t));
   for (int ii = 0; ii < num_levels; ++ii) {
      PointSet centers(feature_dim);
      PointSet indices(3);

      vector<LargeIndex> centers_info;
      
      int32_t num_centers;
      input_stream.read((char*)&num_centers, sizeof(int32_t));

      for (int jj = 0; jj < num_centers; ++jj) {
         Feature feature(feature_dim);
         for (int kk = 0; kk < feature_dim; ++kk) {
            float value;
            input_stream.read((char*)&value, sizeof(float));
            feature[kk] = value;
         }
         centers.AddFeature(feature);

         int32_t parent, start, end;
         input_stream.read((char*)&parent, sizeof(int32_t));
         input_stream.read((char*)&start, sizeof(int32_t));
         input_stream.read((char*)&end, sizeof(int32_t));
         Feature f(3);
         f[0] = parent; f[1] = start; f[2] = end;
         indices.AddFeature(f);
      }
      centers_.AddPointSet(centers);
      tree_indices_.AddPointSet(indices);
   }

   memberships_.resize(num_points);
   for (int ii = 0; ii < num_points; ++ii) {
      LargeIndex new_index;
      int32_t index_size;
      input_stream.read((char*)&index_size, sizeof(int32_t));
      for (int jj = 0 ; jj < index_size; ++jj) {
         int32_t value;
         input_stream.read((char*)&value, sizeof(int32_t));
         new_index.push_back(value);
      }
      memberships_[ii] = new_index;
   }
   
   done_ = true;
}

}  // namespace libpmk
