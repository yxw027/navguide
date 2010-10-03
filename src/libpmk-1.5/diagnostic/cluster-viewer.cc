// Copyright 2007, Massachusetts Institute of Technology.
// The use of this code is permitted for research only. There is
// absolutely no warranty for this software.
//
// Author: John Lee (jjl@mit.edu)
//

#include <stdio.h>
#include <string>
#include <fstream>
#include <vector>
#include "util/sparse-vector.h"
#include "util/distance-computer.h"
#include "point_set/point-set-list.h"
#include "clustering/k-means-clusterer.h"

using namespace std;
using namespace libpmk;

int main(int argc, char** argv) {
   if (argc != 2) {
      printf("Usage: %s <input_cl_file>\n", argv[0]);
      exit(0);
   }

   L2DistanceComputer dist;
   KMeansClusterer clusterer(1, 1, dist);
   string input_file(argv[1]);
   
   clusterer.ReadFromFile(input_file.c_str());

   PointSet centers(clusterer.GetClusterCenters());

   for (int ii = 0; ii < clusterer.GetNumCenters(); ++ii) {
      printf("CENTER %d of %d\n", ii+1, clusterer.GetNumCenters());
      for (int kk = 0; kk < centers.GetFeatureDim(); ++kk) {
         printf("%f ", centers[ii][kk]);
      }
      printf("\n");
   }

   return 0;
}
