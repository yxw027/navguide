// Copyright 2007, Massachusetts Institute of Technology.
// The use of this code is permitted for research only. There is
// absolutely no warranty for this software.
//
// Author: John Lee (jjl@mit.edu)
//

#ifndef POINT_SET_POINT_REF_H
#define POINT_SET_POINT_REF_H

#include "point_set/point-set-list.h"
#include "point_set/feature.h"

namespace libpmk {

class PointSetList;

/// A way to reference Features in a particular PointSetList.
/**
 * PointRef is basically a pair of things: (1) a pointer to a PointSetList,
 * and (2) an index (or set of indices) into that PointSetList that
 * references one particular Feature. Since there are two ways to
 * reference a particular Feature in a PointSetList (either by
 * specifying which PointSet it's in followed by the index of that
 * point within that PointSet, or just by specifying a single index as
 * if you were ignoring PointSet boundaries), PointRef will
 * transparently convert between these two methods of describing a
 * Feature, making it easy to use PointSetLists for different
 * applications.
 *
 * Note that PointRefs become invalid if the PointSetList is modified.
 * PointRef by itself does not perform any consistency checking.
 */
class PointRef {
 public:
   /// Refer to a point initially by specifying a single index. 
   PointRef(const PointSetList* data, int feature_index);

   /// Refer to a point initially by specifying which PointSet and index.
   PointRef(const PointSetList* data, int which_point_set, int which_point);

   /// Refer to a point by providing both methods of accessing a Feature.
   /**
    * This constructor does not do any consistency checking
    * i.e., to check whether feature_index refers to the same point as 
    * (which_point_set, which_point). You should only use this
    * constructor if you know for sure; the reason for this constructor
    * is to quickly build a PointRef if you already know all the indices.
    */
   PointRef(const PointSetList* data, int feature_index,
            int which_point_set, int which_point);

   /// Get the Feature referred to by this PointRef.
   const Feature& GetFeature() const;

   /// Find the index of the PointSet that this point is in.
   /**
    * This will usually be used in conjunction with GetWhichPoint().
    */
   int GetWhichPointSet() const;
   
   /// Find the index of the point within the PointSet that it's in.
   /**
    * This will usually be used in conjunction with GetWhichPointSet().
    */
   int GetWhichPoint() const;
   
   /// \brief Get the index of the point in the list of all Features
   /// in this PointSetList.
   int GetFeatureIndex() const;

 private:
   const PointSetList* data_;

   // References a particular point.
   unsigned int feature_index_;

   // These two variables combined also reference a particular point,
   // but we store both representations for ease of use.
   unsigned short int which_point_set_;
   unsigned short int which_point_;
};
}  // namespace libpmk

#endif  // POINT_SET_POINT_REF_H
