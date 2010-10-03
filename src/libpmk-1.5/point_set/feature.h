// Copyright 2007, Massachusetts Institute of Technology.
// The use of this code is permitted for research only. There is
// absolutely no warranty for this software.
//
// Author: John Lee (jjl@mit.edu)
//
#ifndef POINT_SET_FEATURE_H
#define POINT_SET_FEATURE_H

#include <vector>

using namespace std;

namespace libpmk {

/// A data structure representing a weighted vector of floats.
/**
 * The size of the feature vector is determined at construction and
 * cannot be changed. The elements in the vector and weight are
 * mutable.
 */
class Feature {
 public:
   /// Initially sets all elements in the vector to 0 and weight to 1.
   Feature(int dim);
   ~Feature();

   /// Returns the size of the vector.
   int GetDim() const;

   /// Equivalent to operator[](int index) .
   float GetValue(int index) const;

   /// Returns the current weight.
   float GetWeight() const;

   /// Returns a reference to the specified index. Does bounds checking.
   float& operator[](int index);

   /// Returns a reference to the specified index. Does bounds checking.
   float operator[](int index) const;

   /// Sets a new weight, and returns the old weight.
   float SetWeight(float weight);
   
 private:
   float weight_;
   vector<float> feature_;
};
}  // namespace libpmk

#endif  // POINT_SET_FEATURE_H
