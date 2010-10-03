#ifndef _MODEL_H__
#define _MODEL_H__

/* From the standard C library */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <assert.h>
#include <math.h>

/* From the STL library */
#include <vector>
#include <map>
#include <algorithm>
using namespace std;

/* From the GL library */
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#include "math/geom.h"
#include "common/dbg.h"
#include "image/image.h"
#include "jpegcodec/imgload.h"
#include "common/fileio.h"

/* From the VL libary */
#include "vl/VLf.h"

#ifndef ABS
#define ABS(x)    (((x) > 0) ? (x) : (-(x)))
#define MAX(x,y)  (((x) > (y)) ? (x) : (y))
#define MIN(x,y)  (((x) < (y)) ? (x) : (y))
#endif

#define PATTERN_NX 5 //  number of squares in the X direction
#define PATTERN_NY 7 // number of squares in the Y direction
#define PATTERN_SIZE 3.8 // in cm

class Vertex { 
    Vec3f _v; int _id; /*vector<int> fid;*/
    
 public:
    
    Vertex() { _v = Vec3f(0,0,0); _id = 0;}; 
    Vertex( Vec3f v, int id ) { _v = v; _id = id; };
    inline Vec3f v() { return _v; };
    inline void v(Vec3f v) { _v = v;};
        
    inline int id() { return _id;};
    inline void id(int id) { _id = id;};
};

class Face { 
 public:
    int vid[4]; int id; int n; Vec3f nm;
    
    Face() {};

    Face( int a, int b, int c, int i, Vec3f m) 
        { vid[0]=a; vid[1]=b; vid[2]=c; id=i; n=3; nm = m;};

    Face( int a, int b, int c, int i) 
        { vid[0]=a; vid[1]=b; vid[2]=c; id=i; n=3;};
};

/* a structure for triangular texture items */
typedef struct TriangleTexel { 
    int c0,r0,c1,r1,c2,r2; // square on the image
    int a,b,c; // vertex indices
};



/* a class for a 3D model */

class Model 
{
    int _valid;
    int _textureValid;

    double _texture_tx, _texture_ty; // texture transform (VRML)
    double _texture_sx, _texture_sy;

    vector<int>_coordIndex;

    std::vector<Vertex> _vertices;
    std::vector<Face> _faces;
    std::vector<TriangleTexel> _texels; // textures

    GLuint _texture;

 public:
    
    Model() { Clear(); };
    ~Model();

    // read model from file
    void Read ( const char *filename );
    void ReadOFFFormat ( const char *filename );
    void ReadVRMLFormat ( const char *filename );
    void InitTextures( const char *filename );
    void Clear();

    // accessors
    inline int NVertices() const { return _vertices.size(); };
    inline int NFaces() const { return _faces.size(); };
    inline int NTexels() const { return _texels.size(); }

    inline int GetTextureId() const { return _texture; }
    inline int GetCoordIndex( int ii ) const { return _coordIndex[ii];}
    inline TriangleTexel GetTexel( int ii) const { return _texels[ii]; }
    inline int TextureValid() const {return _textureValid;};
    inline double GetTextureSx() const { return _texture_sx;}
    inline double GetTextureSy() const { return _texture_sy;}

    inline int Valid() const { return _valid; };
    inline Vertex GetVertex( int p ) const { return _vertices[p]; };
    inline Face   GetFace  ( int p ) const { return _faces[p]; };
    void GetVertices(vector<Vec3f> &data);

    // geometric features
    Vec3f Centroid();
    float Diameter ();
    void ComputeNormals();

    void GenerateVRMLGrid( const char *filename, int width, int height, int size, \
                           vector<Vec3f> vertices, vector<int> indices, \
                           int nx, int ny, int tile_w, int tile_h);
    void GenerateSimulatedModel (const char *input, const char *filename);

    void FindClosestNormals( Vec3f t, int n, vector<Vec3f> &data );
    Vec3f GetNormal( Vec3f p );

    // transform
    void Subtract( Vec3f t );

    void GenerateCalibrationPattern();
    void GenerateCalibrationPattern2();

    // sphere tessellation
    void MakeIcosahedron(float size);
    void Tessellate( int level );
    
    void PrintInfo();
};



#endif
