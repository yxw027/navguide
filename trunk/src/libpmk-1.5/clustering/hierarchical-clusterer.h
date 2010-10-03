// Copyright 2007, Massachusetts Institute of Technology.
// The use of this code is permitted for research only. There is
// absolutely no warranty for this software.
//
// Author: John Lee (jjl@mit.edu)
//
// TODO(jjl): Allow users to pass in arbitrary Clusterer objects, not just
// hard-coded KMeansClusterer.
//
#ifndef CLUSTERING_HIERARCHICAL_CLUSTERER_H
#define CLUSTERING_HIERARCHICAL_CLUSTERER_H

#include <vector>
#include <iostream>
#include "clustering/clusterer.h"
#include "point_set/point-ref.h"
#include "point_set/point-set-list.h"
#include "point_set/mutable-point-set-list.h"
#include "util/sparse-vector.h"
#include "util/distance-computer.h"

using namespace std;

namespace libpmk {
/// Hierarchical K-Means clusterer.
/**
 * This clusterer does not use the Clusterer interface because its
 * output involves multiple levels. However, similar to Clusterer,
 * HierarchicalClusterer has the ability to get its data either by
 * actually doing hierarchical K-means, or by reading
 * already-processed data. The difference is that the returned cluster
 * centers are a PointSetList, where each element in the list gives
 * the centers for a particular level.
 */ 
class HierarchicalClusterer {
 public:
   HierarchicalClusterer();


   /// Get the cluster centers.
   /** 
    * Must be called after Cluster() or ReadFromStream(). Returns an
    * ordered list of point sets. The ith element in this list
    * corresponds to the centers at the ith level, where i=0 is the
    * root (which is one giant cluster).
    *
    * Note that this function does NOT tell you the hierarchical
    * relationships. You will not know which clusters are subclusters
    * of which ones. GetChildRange() and GetParentIndex() provide this
    * functionality.
    */
   const PointSetList& GetClusterCenters() const;

   /// Get the membership data.
   /**
    * Must be called after Cluster() or ReadFromStream(). The size of
    * this returned vector, of course, is the total number of points
    * that were clustered. Each element in this vector is a LargeIndex
    * describing a path down a tree. Membership of a single point is a
    * LargeIndex, as opposed to an int which is used by flat
    * (non-hierarchical) clustering methods. Naturally, the first
    * element of all of the LargeIndices here will be 0, since the top
    * level is one giant cluster. Following that, the cluster center
    * can be retrieved (see GetClusterCenters()) by looking at each
    * element in the LargeIndex.
    *
    * For example, let C be the PointSetList returned by
    * GetClusterCenters(). Suppose we have a LargeIndex [0 3
    * 9]. C[0][0] is the root cluster. Then C[1][3] is the center that
    * this point belonged to at the first level, and C[2][9] is the
    * center that this point belonged to at the bottom level.
    *
    * Note that this function does NOT tell you the hierarchical
    * relationships. You will not know which clusters are subclusters
    * of which ones. GetChildRange() and GetParentIndex() provide this
    * functionality.
    */ 
   const vector<LargeIndex>& GetMemberships() const;

   /// Performs the actual clustering and stores the result internally.
   void Cluster(int num_levels, int branch_factor,
                const vector<PointRef>& points,
                const DistanceComputer& distance_computer);
   
   /// Get the start and end indices of children at the next level.
   /**
    * Requires Preprocess() or ReadFromStream() to be called first.
    * The <level> and <index> parameters specify a cluster center.
    * The two returned integers are the start and end indices into
    * the cluster centers (returned by GetClusterCenters()) of that
    * cluster center's children at the (<level>+1)st level. The
    * returned pair is (-1, -1) if it has no children.
    */
   pair<int, int> GetChildRange(int level, int index) const;

   /// Get the index of the parent of a node.
   /**
    * Requires Preprocess() or ReadFromStream() to be called first.
    * The <level> and <index> parameters specify a cluster center. The
    * returned integer is the index into the cluster centers (returned
    * by GetClusterCenters()) of that cluster center's parent at the
    * (<level>-1)st level. Returns -1 for the root node (the only one
    * without a parent).
    */
   int GetParentIndex(int level, int index) const;

   
   /// Write the clustered data to a stream.
   /**
    * Must be called after Cluster() or ReadFromStream(). File format:
    * <ul>
    * <li> (int32) N, the number of levels
    * <li> (int32) P, the total number of clustered points
    * <li> (int32) D, the feature dim
    * <li> For each level: <ul>
    *      <li> (int32) n, the number of centers at this level
    *      <li> For each center at this level: <ul>
    *           <li> (D * float) the center
    *           <li> (int32) the parent index (see GetParentIndex())
    *           <li> (int32) child range start index (see GetChildRange())
    *           <li> (int32) child range end index (see GetChildRange())
    * </ul></ul>
    * <li> For each point: <ul>
    *      <li> (int32) the size of the point's LargeIndex (see
    *            GetMemberships())
    *      <li> (index_size * int32) the point's LargeIndex
    * </ul>
    * </ul>
    *
    * This function will abort if the stream is bad.
    */
   void WriteToStream(ostream& output_stream) const;

   /// Write the clustered data to a file.
   void WriteToFile(const char* output_filename) const;

   /// Read clustered data from a stream.
   /**
    * Can be called in lieu of Cluster(). See WriteToStream() for the
    * format.  This function will abort if the stream is bad.
    * \sa WriteToStream()
    */
   void ReadFromStream(istream& input_stream);

   /// Read the clustered data from a file.
   void ReadFromFile(const char* input_filename);
   
   /// Default 100.
   static const int MAX_ITERATIONS;

 private:
   // RecursiveCluster() takes as input which level it's going to get
   // clusters for, as well as a list of ints. This list of ints gives
   // *indices into all_points_* to cluster (note that all_points_ itself
   // is a list of indices into the actual data).
   void RecursiveCluster(int level, const vector<int>& indices,
                         int num_levels, int branch_factor,
                         const vector<PointRef>& points,
                         const DistanceComputer& distance_computer,
                         int parent_index);


   MutablePointSetList centers_;

   // Has the same structure as centers_, except the features are
   // 3-dim.  They tell you the index of the parent, and the start and
   // end indices of its children in the next level. -1 if N/A, like
   // if it has no children or if it has no parent.
   MutablePointSetList tree_indices_;

   // A vector of size num_levels. For each level, this tells you
   // how many centers are in that group. We need this beecause centers_
   // is sparsely done, meaning if there are clusters with 0 points,
   // they simply don't show up in centers_.
   vector<LargeIndex> memberships_;


   bool done_;
};
}  // namespace libpmk

#endif  // CLUSTERING_HIERARCHICAL_CLUSTERER_H
