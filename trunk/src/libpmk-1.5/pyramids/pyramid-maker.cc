// Copyright 2007, Massachusetts Institute of Technology.
// The use of this code is permitted for research only. There is
// absolutely no warranty for this software.
//
// Author: John Lee (jjl@mit.edu)
//

#include <stdlib.h>
#include "pyramids/pyramid-maker.h"

namespace libpmk {

vector<MultiResolutionHistogram*> PyramidMaker::
MakePyramids(const PointSetList& psl) {
   vector<MultiResolutionHistogram*> answer;
   for (int ii = 0; ii < psl.GetNumPointSets(); ++ii) {
      MultiResolutionHistogram* mrh(MakePyramid(psl[ii]));
      answer.push_back(mrh);
   }
   return answer;
}

}  // namespace libpmk
