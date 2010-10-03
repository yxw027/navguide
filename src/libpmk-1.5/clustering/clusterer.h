// Copyright 2007, Massachusetts Institute of Technology.
// The use of this code is permitted for research only. There is
// absolutely no warranty for this software.
//
// Author: John Lee (jjl@mit.edu)
//
#ifndef CLUSTERING_CLUSTERER_H
#define CLUSTERING_CLUSTERER_H

#include <vector>
#include <iostream>
#include "point_set/point-set.h"
#include "point_set/point-ref.h"

using namespace std;

namespace libpmk {
/// Abstract interface for a flat clusterer.
/**
 * There are two ways for a Clusterer to get its data: either by (1)
 * performing an actual clustering computation, or (2) by reading
 * pre-clustered data from a stream. This setup allows one to cluster
 * some data, and then save the results to a file so it can be later
 * read and further processed.
 *
 * To create your own clusterer, all you need to do is implement the
 * DoClustering() method. The I/O is handled automatically.
 */
class Clusterer {
 public:
   Clusterer();
   virtual ~Clusterer() { }

   /// Performs the clustering and stores the result internally.
   /**
    * To avoid potential memory problems, Clusterers do not operate on
    * PointSetLists or PointSets directly. Rather, they simply shuffle
    * PointRefs around.  \sa PointSetList::GetPointRefs()
    */
   void Cluster(const vector<PointRef>& data);

   /// Return the cluster centers.
   /**
    * This requires Cluster() or ReadFromStream() to have been called
    * first. It returns a PointSet where each Feature in it is one
    * of the cluster centers.
    */ 
   PointSet GetClusterCenters() const;

   /// Return the number of cluster centers.
   /**
    * this requires Cluster() or ReadFromStream() to have been called
    * first. It reutnrs the number of cluster centers.
    */
   int GetNumCenters() const;

   /// Return the membership table.
   /**
    * Let n be the number of points that were clustered. The returned
    * vector is of size n as well, where each element tells you which
    * cluster that point was placed in.
    */
   vector<int> GetMembership() const;

   /// Write the clustering data to a stream.
   /**
    * Requires Cluster() or ReadFromStream() to have been called
    * first. Output format:<ul>
    * <li> (int32) C, the number of cluster centers
    * <li> (int32) P, the total number of clustered points
    * <li> (int32) D, the dimension of the Features
    * <li> (D * float) center 1
    * <li> (D * float) center 2
    * <li> (D * float) ...
    * <li> (D * float) center C
    * <li> (int32) the cluster to which point 1 belongs
    * <li> (int32) the cluster to which point 2 belongs
    * <li> (int32) ...
    * <li> (int32) the cluster to which point P belongs
    *
    * The clustered points themselves are not written to the stream,
    * only the centers and membership data. It is assumed that the
    * caller of Cluster() already has access to those points
    * anyway. This function aborts if the stream is bad.
    */
   void WriteToStream(ostream& output_stream) const;

   /// Write the clustering data to a file.
   void WriteToFile(const char* output_filename) const;

   /// Read clustering data from a stream.
   /**
    * Can be called in lieu of Cluster(). If this is called after
    * Cluster(), all of the previous data is cleared before reading
    * the new data. For the file format, see WriteToStream. This
    * function aborts if the stream is bad. \sa WriteToStream.
    */
   void ReadFromStream(istream& input_stream);

   /// Read clustering data from a file.
   void ReadFromFile(const char* input_filename);

 protected:
   auto_ptr<PointSet> cluster_centers_;
   vector<int> membership_;
   bool done_;

   /// Performs the actual clustering.
   /**
    * DoClustering is responsible for three things:<ol>
    * <li> Filling up cluster_centers_
    * <li> Filling up membership_ with a number of elements equal
    * to data.size()
    * <li> Setting done_ to true.
    * </ol>
    */
   virtual void DoClustering(const vector<PointRef>& data) = 0;
};
}  // namespace libpmk

#endif  // CLUSTERING_CLUSTERER_H
