#include <stdio.h>
#include <stdlib.h>

#include <common/config.h>
#include <common/sensor_transforms.h>

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "Error: no config input file specified.\n");
    exit(-1);
  }

  FILE* fp = fopen(argv[1], "r");
  if (NULL == fp) {
    fprintf(stderr, "Error: can't open file %s for read.\n", argv[1]);
    exit(-1);
  }

  Config* cfg = config_parse_file(fp, argv[1]);
  fclose(fp);

  camera_transform_t* cam =
    camera_transform_create(cfg, "calibration.cameras.roof_center");
  fprintf(stdout, "Read and created camera.\n");

  double x, y, z, u, v;
  strans_undistort_pixel(0,0,cam,&u,&v);
  fprintf(stdout, "Undistort: (0, 0) goes to (%f, %f)\n", u, v);

  strans_distort_pixel(u,v,cam,&x,&y);
  fprintf(stdout, "\nDistort: (%f, %f) goes to (%f, %f)\n", u, v, x, y);

  strans_pixel_to_ray(352,240,cam,&x,&y,&z);
  fprintf(stdout, "\nRay: (0, 0) goes to (%f, %f, %f)\n", x, y, z);

  return 1;
}
