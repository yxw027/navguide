// Copyright 2007, Massachusetts Institute of Technology.
// The use of this code is permitted for research only. There is
// absolutely no warranty for this software.
//
// Author: John Lee (jjl@mit.edu)
//
#ifndef POINT_SET_POINT_SET_LIST_H
#define POINT_SET_POINT_SET_LIST_H

#include <assert.h>
#include <vector>
#include <string>
#include <iostream>
#include "point_set/point-set.h"
#include "point_set/point-ref.h"
#include "point_set/feature.h"

using namespace std;

namespace libpmk {

class PointRef;

/// Abstract interface for a list of PointSet. 
/**
 * All Features contained in any PointSetList must have the same
 * dimension. When serialized, a PointSetList takes on the following format:
 * <ul>
 * <li>(int32) N, the number of PointSets
 * <li>(int32) f, the dim of every feature in this PointSetList
 * <li>For each PointSet:
 *  <ul>
 *     <li>(int32) The number of points (Features) in this PointSet</li>
 *     <li>For each feature in this PointSet: <ul>
 *        <li>(float * f) The feature values
 *     </ul>
 *     <li>Then, again for each Feature in this PointSet:
 *        <ul><li>(float) The weight of the Feature</ul>
 *  </ul>
 * </ul>
 *
 * Equivalently, at a higher level:
 * <ul>
 * <li>(int32) N, the number of PointSets
 * <li>(int32) f, the dim of every feature in this PointSetList
 * <li>(N * PointSet) the N PointSets.
 * </ul>
 * \sa PointSet
 */

class PointSetList {
 public:
   virtual ~PointSetList() { }

   /// Get the total number of PointSets in this PointSetList.
   virtual int GetNumPointSets() const = 0;
   
   /// Get the total number of Features in this PointSetList.
   /**
    * This is the sum of GetNumFeatures() over all PointSets in in
    * this PointSetList.
    */
   virtual int GetNumPoints() const = 0;

   /// Get the dim of every Feature in this PointSetList.
   virtual int GetFeatureDim() const = 0;

   /// Get the number of Features in each PointSet.
   /**
    * Returns a vector of size this.GetNumPointSets(), where the nth
    * element is the number of Features in the nth PointSet in this
    * PointSetList.
    */
   virtual vector<int> GetSetCardinalities() const = 0;

   /// Returns a const ref to the <index>th PointSet.
   virtual const PointSet& operator[](int index) const = 0;

   /// Returns the <index>th Feature in the PointSetList.
   /**
    * We can also think of a PointSetList as a long vector of Features if
    * we ignore the PointSet boundaries, so you can reference individual
    * features of a PointSetList through this.
    */
   const Feature& GetFeature(int index) const;

   /// Locate a Feature in a PointSet.
   /**
    * Given an index specifying which feature we are referring to,
    * return which PointSet it belongs to, as well as the index into
    * that PointSet that the Feature belongs to.
    */
   pair<int, int> GetFeatureIndices(int index) const;
   
   /// Get PointRefs to every Feature in this PointSetList.
   /**
    * Requires out_refs != NULL. Makes out_refs a vector of size
    * GetNumPoints(), where the nth element is a PointRef pointing to
    * the nth Feature (i.e., points to GetFeature(n)). If out_refs has
    * something in it beforehand, it will be cleared prior to filling
    * it.
    * \sa PointRef
    */
   void GetPointRefs(vector<PointRef>* out_refs) const;   
   
   /// Writes the entire PointSetList to a stream.
   /** See the detailed description of PointSetList for the format. */
   void WriteToStream(ostream& output_stream) const;

   /// Writes just the serialized header to a stream.
   void WriteHeaderToStream(ostream& output_stream) const;

   /// Writes the point sets (without a header) sequentially to a stream.
   void WritePointSetsToStream(ostream& output_stream) const;

   /// Writes the entire PointSetList to a file.
   /** See detailed description of PointSetList for the format. */
   void WriteToFile(const char* output_file) const;
};
}  // namespace libpmk

#endif  // POINT_SET_POINT_SET_LIST_H
