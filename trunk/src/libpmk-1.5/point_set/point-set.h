// Copyright 2007, Massachusetts Institute of Technology.
// The use of this code is permitted for research only. There is
// absolutely no warranty for this software.
//
// Author: John Lee (jjl@mit.edu)
//
#ifndef POINT_SET_POINT_SET_H
#define POINT_SET_POINT_SET_H

#include <vector>
#include "point_set/feature.h"

using namespace std;

namespace libpmk {
/// A data structure representing a list of Feature vectors.
/**
 * All features in the set must have the same dimension, and this is
 * determined when the PointSet is constructed. Features may be added,
 * removed, and replaced by index. Bounds checking is done
 * automatically, as is checking that any new Features added to the
 * PointSet have the same dimension.
 */
class PointSet
{
 public:
   /// Creates an initially empty set (such that GetNumFeatures() == 0).
   PointSet(int feature_dim);
   
   int GetNumFeatures() const;
   int GetFeatureDim() const;

   /// Returns a copy of the feature at <index>.
   Feature GetFeature(int index) const;

   /// Returns a reference to the feature at <index>.
   Feature& operator[](int index);

   /// Returns a const reference to the feature at <index>.
   const Feature& operator[](int index) const;

   /// Adds a copy of the feature to the end of the list.
   void AddFeature(const Feature& feature);

   /// Removes the feature at <index>.
   void RemoveFeature(int index);

   /// Replaces the Feature at <index> with a copy of <feature>.
   void SetFeature(int index, const Feature& feature);

   /// Writes the point set to a stream.
   /**
    * The format is as follows:
    * <ul>
    * <li>(int32_t) num_features
    * <li>(int32_t) feature_dim
    * <li>for each feature:
    *    <ul><li>(float * feature_dim) the feature vector</ul>
    * <li>then, again, for each feature:
    *    <ul><li>(float) the feature vector's weight</li></ul>
    */
   void WriteToStream(ostream& output_stream) const;

   /// Reads the point set from a stream.
   /**
    * This function will clear any Features present currently. It will
    * also assume that the incoming PointSet has the proper feature
    * dim (observe that a serialized PointSet doesn't actually record
    * feature dim-- that is up to PointSetList to remember).
    */
   void ReadFromStream(istream& input_stream);

 private:
   int feature_dim_;
   vector<Feature> features_;
};
}  // namespace libpmk
#endif  // POINT_SET_POINT_SET_H
