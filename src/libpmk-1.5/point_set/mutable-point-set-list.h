// Copyright 2007, Massachusetts Institute of Technology.
// The use of this code is permitted for research only. There is
// absolutely no warranty for this software.
//
// Author: John Lee (jjl@mit.edu)
//
#ifndef POINT_SET_MUTABLE_POINT_SET_LIST_H
#define POINT_SET_MUTABLE_POINT_SET_LIST_H

#include <assert.h>
#include <vector>
#include <string>
#include <iostream>
#include "point_set/point-set-list.h"
#include "point_set/point-set.h"
#include "point_set/point-ref.h"
#include "point_set/feature.h"

using namespace std;

namespace libpmk {

/// A mutable, in-memory PointSetList.
/**
 * This class adds AddPointSet to the interface, allowing one to (as
 * the name suggests) add new PointSets. You may also modify PointSets
 * or the Features they contain, and can replace PointSets in the list
 * with other PointSets.
 */
class MutablePointSetList : public PointSetList {
 public:
   virtual ~MutablePointSetList();

   /// Reads an entire data set from a stream.
   /**
    * Aborts if the input_stream cannot be read.
    */
   void ReadFromStream(istream& input_stream);

   /// Reads an entire data set from a file.
   /**
    * Aborts if the file cannot be read.
    */
   void ReadFromFile(const char* filename);

   /// Reads a segment of the data from a stream.
   /**
    * <start> specifies the index of the first PointSet to read in the
    * stream of data. If <selection_size> is large enough so that this
    * would want to read past the end of the file,
    * ReadSelectionFromStream will just stop reading. Observe that you
    * will need to adjust indices (i.e., if you choose to read Y
    * PointSets starting from index X, then this[0] will be
    * the 6th PointSet, as opposed to this[5]).
    *
    * Aborts if the input_stream cannot be read.
    */
   void ReadSelectionFromStream(istream& input_stream,
                                int start, int selection_size);

   virtual int GetNumPointSets() const;
   virtual int GetNumPoints() const;
   virtual int GetFeatureDim() const;
   virtual vector<int> GetSetCardinalities() const;

   /// Returns a ref to the <index>th PointSet.
   PointSet& operator[](int index);
   virtual const PointSet& operator[](int index) const;

   /// Adds a copy of <point_set> to the end of the list.
   void AddPointSet(const PointSet& point_set);


 private:
   vector<PointSet> list_;
   
};
}  // namespace libpmk
#endif  // POINT_SET_POINT_SET_LIST_H
