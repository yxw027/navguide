#include "model.h"

// constructors

Model::~Model ()
{
}

// clear a model
void
Model::Clear()
{
    _faces.clear();
    _vertices.clear();
    _texels.clear();
    _coordIndex.clear();

    if ( _textureValid )
         glDeleteTextures( 1, &_texture );

    _valid = 0;
    _textureValid = 0;

    _texture_tx = _texture_ty = 0.0;
    _texture_sx = _texture_sy = 1.0;

 }

/* print out model info */
void Model::PrintInfo()
{
    dbg(DBG_INFO, "# faces: %d  # vertices: %d  Diameter: %f\n", _faces.size(), 
        _vertices.size(), Diameter() );
}

/* Detect model format and read it */
void
Model::Read ( const char *filename )
{
    char ext[5];

    Clear();

    GetFileExtension( filename, ext );

    if ( strncmp( ext, "off", 3 ) == 0 )
        ReadOFFFormat( filename );
    else if ( strncmp( ext, "wrl", 3 ) == 0 )
        ReadVRMLFormat( filename );
    else 
        dbg(DBG_INFO,"Unknown format type.\n");

}

// read a model from file (OFF format)
void
Model::ReadOFFFormat (const char *filename)
{
    dbg(DBG_INFO, "Reading model in file %s (OFF format)\n", filename);
    
    // clear the model
    Clear();

    // open the file
    FILE *f = fopen( filename, "r");

    if ( !f ) {
        dbg(DBG_ERROR, "Failed to open model file %s!\n", filename);
        return;
    }

    // read the header (should be "OFF")
    char header[20];
    fscanf(f,"%s",header);
    if ( strncmp(header,"OFF",3) != 0 ) {
        dbg(DBG_ERROR,"Invalid header for an OFF file.\n");
        fclose(f);
        return;
    }
    
    int n_vertices, n_faces;

    // read number of vertices and faces
    int tmp;
    if ( fscanf(f,"%d%d%d",&n_vertices,&n_faces,&tmp) != 3 ) {
        dbg( DBG_ERROR, "Error reading model file %s\n", filename);
        Clear();
        fclose(f);
        return;
    }

    // read the vertices
    int ii;
    for (ii=0;ii<n_vertices;ii++) {
        
        double x,y,z;
        if ( fscanf(f, "%lf%lf%lf", &x, &y, &z) != 3 ) {
            dbg(DBG_ERROR, "Error reading model file %s\n", filename);
            Clear();
            fclose(f);
            return;
        }
    
        Vec3f vv(x,y,z);
        Vertex v;
        v.v (vv);
        v.id (ii);

        _vertices.push_back ( v );
    }

    // read the faces
    for (ii=0;ii<n_faces;ii++) {
        int n;
        if ( fscanf(f,"%d",&n) != 1 ) {
            dbg(DBG_ERROR, "Error reading model file %s\n", filename);
            Clear();
            fclose(f);
            return;
        }
        
        Face fc;
        fc.n = n;
        fc.id = ii;

        for (int jj=0;jj<n;jj++) {
            int p;
            if ( fscanf(f,"%d",&p) != 1 ) {
                dbg(DBG_ERROR, "Error reading model file %s\n", filename);
                Clear();
                fclose(f);
                return;
            }
            
            fc.vid[jj] = p;

            //GetVertex(p).fid.push_back( ii );
        }

        // compute the face normal
        Vec3f e1 = norm( GetVertex(fc.vid[1]).v() - GetVertex(fc.vid[0]).v() );
        Vec3f e2 = norm( GetVertex(fc.vid[2]).v() - GetVertex(fc.vid[0]).v() );

        fc.nm = norm(cross(e1, e2));

        _faces.push_back( fc );
    }

    // close the file
    fclose( f );

    dbg(DBG_INFO, "Read %d vertices and %d faces.\n", NVertices(), NFaces());

    _valid = 1;
}

/* Read a VRML 2.0 file */

void
Model::ReadVRMLFormat ( const char *filename )
{
    int min_r = 100000000, min_c = 100000000;
    int max_r = 0, max_c = 0;

    dbg(DBG_INFO, "Reading model in file %s (VRML format)\n", filename);

    // clear the model
    Clear();

    int nfc = -1; // faces are triangle or squares

    // open the file
    FILE *fp = fopen( filename, "r" );

    if ( !fp ) {
        dbg(DBG_ERROR, "Failed to open file %s\n", filename);
        fclose(fp);
        return;
    }
    
    dbg(DBG_INFO, "Reading model in file %s (VRML format)\n", filename);
    
    // read the file
    char str[1024];
    int mode = 0;

    while ( fgets( str, 256, fp ) ) {

        // first read the geometry

        if ( mode == 0 ) {
            
            // scan the texture transform
            if ( strstr( str, "TextureTransform" ) ) {
                double tx,ty;
                char tmp[256];

                fscanf(fp,"%s",tmp);
                
                if ( fscanf(fp,"%lf %lf",&tx,&ty) == 2 ) {
                    _texture_tx = tx;
                    _texture_ty = ty;
                   
                }
                fscanf(fp,"%s",tmp);
                
                if ( fscanf(fp,"%lg", &_texture_sx) == 1 ) {
                    if ( fscanf(fp,"%lg", &_texture_sy) == 1 ) {
                       
                    }
                }

                dbg(DBG_INFO, "Texturetransform: [%lf,%lf] [%.10f,%.10f]\n", _texture_tx, _texture_ty,\
                    _texture_sx, _texture_sy);
            }
            

            if ( strstr( str, "IndexedFaceSet" ) ) 
                mode = 1;
        }

        if ( mode == 1 ) {
            if ( strstr( str, "point" ) ) 
                mode = 2;
        }

        if ( mode == 2 ) {
            double x,y,z;
            if ( fscanf(fp,"%lf%lf%lf", &x, &y, &z) == 3 ) {
                Vertex v;
                v.v (Vec3f(x,y,z));
                v.id (_vertices.size());
                _vertices.push_back( v );
            }
        }

        if ( mode == 2 ) {
            if ( strstr ( str, "}" ) )
                mode = 3;
        }

        if ( mode == 3 ) {
            if ( strstr( str, "coordIndex" ) )
                mode = 4;
        }

        if ( mode == 4 ) {
            int v[4];
            int c = 0;
            int id;
            while ( fscanf(fp,"%d,",&id) == 1 ) {
                if ( id != -1 ) {
                    _coordIndex.push_back( id );
                    v[c] = id;
                    c++;
                }
                if ( id == -1 ) {
                    Face fc;
                    fc.n = c;

                    if ( fc.n != nfc ) {
                        if ( nfc == -1 )
                            nfc = fc.n;
                        else {
                            dbg(DBG_ERROR, "Inconsistent number of vertices for face %d.\n", _faces.size());
                            Clear();
                            return;
                        }
                    }
                            
                    fc.id = _faces.size();
                    for (int jj=0;jj<fc.n;jj++) {
                        fc.vid[jj] = v[jj];
                        //GetVertex(v[jj]).fid.push_back( fc.id );
                    }

                    // compute the face normal
                    Vec3f e1 = norm( GetVertex(fc.vid[1]).v() - GetVertex(fc.vid[0]).v() );
                    Vec3f e2 = norm( GetVertex(fc.vid[2]).v() - GetVertex(fc.vid[0]).v() );
                    
                    fc.nm = norm(cross(e1, e2));

                    _faces.push_back( fc );

                    c = 0;
                }
            }

            mode = 5;
        }

        // second read the textures
        
        if ( mode == 5 && strstr( str, "point" ) ) {

            mode = 6;

            // check that model has only triangles
            if ( nfc != 3 ) {
                dbg(DBG_ERROR, "Does not handle VRML faces that are not triangles (nfc = %d)\n", nfc);
                return;
            }
        }

        if ( mode == 6 ) {
            
            int c,r;
            int count = 0;

            TriangleTexel tx;

            while ( fscanf(fp,"%d%d,", &c, &r) == 2 || fscanf(fp,"%d%d", &c, &r) == 2) {
                min_r = MIN(min_r, r);
                min_c = MIN(min_c, c);
                max_r = MAX(max_r, r);
                max_c = MAX(max_c, c);
               
                if ( count == 0 ) {
                    tx.c0 = c;
                    tx.r0 = r;
 
                }
                if ( count == 1 ) {
                    tx.c1 = c;
                    tx.r1 = r;
                }
                if ( count == 2 ) {
                    tx.c2 = c;
                    tx.r2 = r;
                }

                count++;

                if ( count == 3 ) {
                    
                    tx.a = tx.b = tx.c = 0;
                    
                    _texels.push_back( tx );
            
                    count = 0;
                }
            }

            dbg(DBG_INFO,"Read %d texels.\n", _texels.size());

            mode = 7;
        }

        if ( mode == 7 ) {

            if ( strstr( str, "texCoordIndex" ) ) {
                mode = 8;
            }
        }

        if ( mode == 8 ) {

            int id = 0;
            int count = 0;
            int fcount = 0;

            while ( fscanf(fp,"%d,",&id) == 1 ) {
                if ( id == -1 )
                    continue;
                if ( count == 0 )
                    _texels[fcount].a = id;
                if ( count == 1 )
                    _texels[fcount].b = id;
                if ( count == 2 )
                    _texels[fcount].c = id;
                count++;
                if ( count == 3 ) {
                    count = 0;
                    fcount++;
                }
            }

            mode = 9;
        }
        
        if ( mode == 9 )
            break;
    }
            
    //_texels.resize( 25 );

     // close the file
    fclose( fp );

    dbg(DBG_INFO, "Read %d vertices and %d faces. %d elements in coordIndex\n", NVertices(), NFaces(), _coordIndex.size());

    _valid = 1;

    dbg(DBG_INFO, "min,max r: %d %d  min, max c: %d %d\n", min_r, max_r, min_c, max_c);

}

/* Generate a VRML model given a series of quads <vertices>, 
 * a series of texture indices <indices>
 * the number of cols and rows on the texture image <nx,ny>
 * the width and height of the texture image (<width,height>)
 * the number of subdivision of each texture square <size> (e.g 20)
 * and the output filename <filename>
 * this function assumes that the image is made of nx x ny texture quads
 * of size <tile_w,tile_h> pixels (e.g 440, 590)
 */
void
Model::GenerateVRMLGrid( const char *filename, int width, int height, int size, \
                         vector<Vec3f> vertices, vector<int> indices, \
                         int nx, int ny, int tile_w, int tile_h)
{
    FILE *fp = fopen( filename, "w" );
    
    if ( !fp )
        return;

    double scale_x = 1.0/width;
    double scale_y = 1.0/height;

    fprintf(fp,"#VRML V2.0 utf8\n");

    fprintf(fp,"Transform {\n");

    fprintf(fp," children [\n");

    fprintf(fp,"  Shape {\n");

    fprintf(fp,"   appearance Appearance {\n");

    fprintf(fp,"    texture ImageTexture { url \"\" }\n");

    fprintf(fp,"   textureTransform TextureTransform {\n");

    fprintf(fp,"      translation 0.500000 0.500000\n");

    fprintf(fp,"      scale %.10f %.10f\n", scale_x, scale_y);
    
    fprintf(fp,"geometry IndexedFaceSet {\n");

    fprintf(fp," coord Coordinate {\n");

    fprintf(fp,"	point [\n");

    // generate the points
    //int x,y;
    int rcount = 0;
    int count = 0;
    int dcount = 0;

    int ii,jj;
    int n_x = int(width/(size*nx));
    int n_y = int(height/(size*ny));
    
    while ( rcount < (int)vertices.size() ) {
        //for (y=0;y<ny;y++) {
        //for (x=0;x<nx;x++) {
        
        Vec3f sa = vertices[rcount];
        Vec3f sb = vertices[rcount+1];
        Vec3f sc = vertices[rcount+2];
        Vec3f sd = vertices[rcount+3];
        
        rcount += 4;
                
        float dx = len(sb-sa)/n_x; // 200.0 * (float)1/n_x;
        float dy = len(sd-sa)/n_y; //200.0 * (float)1/n_y;
        
        Vec3f su = norm(sb-sa);
        Vec3f sv = norm(sd-sa);
        
        for (ii=0;ii<n_x;ii++) {
            for (jj=0;jj<n_y;jj++) {
                
                Vec3f a = sa + ii * dx * su + jj * dy * sv;
                
                //Vec3f a = Vec3f(-100,-100,0.0) + 
                //    Vec3f( 200 * (float)ii/n_x, 200 * (float)jj/n_y, 0.0);
                
                Vec3f b = a + dx * su; //Vec3f( dx, 0, 0.0);
                Vec3f c = a + dx * su + dy * sv; //ii * Vec3f( dx, dy, 0.0);
                Vec3f d = a + dy * sv; //Vec3f( 0, dy, 0.0);
                
                fprintf( fp, "%f %f %f,\n", a[0], a[1], a[2]);
                fprintf( fp, "%f %f %f,\n", b[0], b[1], b[2]);
                fprintf( fp, "%f %f %f,\n", c[0], c[1], c[2]);
                fprintf( fp, "%f %f %f", d[0], d[1], d[2]);
                
                dcount++;
                
                if ( ii != n_x-1 || jj != n_y-1 )
                    fprintf(fp, ",\n");
            }
        }
                
        count++;
        
        if ( count == (int)vertices.size()/4 ) {
            fprintf(fp, "]\n");
        } else {
            fprintf( fp, ",\n");
        }
    } 
    
    dbg(DBG_INFO,"generated %d faces.\n", dcount);

    fprintf(fp, "}\n");
    
    fprintf(fp, "ccw TRUE\n");
    
    fprintf(fp, "colorPerVertex FALSE\n");
    fprintf(fp, "normalPerVertex TRUE\n");
    
    fprintf(fp, "	coordIndex [\n");
    
    // dcount = number of faces

    for (ii=0;ii<dcount;ii++) {
            
        fprintf(fp, "%d %d %d %d,\n", 4*ii, 4*ii+1, 4*ii+2, -1);
        fprintf(fp, "%d %d %d %d", 4*ii, 4*ii+2, 4*ii+3, -1);
        
        if ( ii == dcount-1 ) {
            fprintf(fp,"]\n");
        } else {
            fprintf(fp,",\n");
        }
    }
        
    fprintf(fp, "texCoord TextureCoordinate {\n");

    fprintf(fp, "	point [\n");

    count = 0;

    float size_x = (float)tile_w / n_x;
    float size_y = (float)tile_h / n_y;
    
    while ( count < (int)indices.size() ) {
        
        float off_y = (int)(indices[count]/nx) * tile_h;
        
        //for (x=0;x<nx;x++) {
        
        float off_x = (int)(indices[count]%nx) * tile_w;
        
        for (ii=0;ii<n_x;ii++) {
            for (jj=0;jj<n_y;jj++) {
                
                Vec2f a ( off_x + ii * size_x, off_y + jj * size_y );
                Vec2f b = a + Vec2f( size_x, 0);
                Vec2f c = a + Vec2f(size_x,size_y);
                Vec2f d = a + Vec2f(0,size_y);
                
                fprintf(fp, "%d %d,\n",int(a[0]),int(a[1]));
                fprintf(fp, "%d %d,\n",int(b[0]),int(b[1]));
                fprintf(fp, "%d %d,\n",int(c[0]),int(c[1]));
                
                fprintf(fp, "%d %d,\n",int(a[0]),int(a[1]));
                fprintf(fp, "%d %d,\n",int(c[0]),int(c[1]));
                fprintf(fp, "%d %d",int(d[0]),int(d[1]));
                
                if ( ii != n_x-1 || jj != n_y-1) 
                    fprintf(fp, ",\n");
            }
        }
        
        count++;
        
        if ( count == (int)indices.size() ) { 
            fprintf(fp, "]\n");
            break;
        } else {
            fprintf(fp,",\n");
        }
    }

    fprintf(fp,"}\n");
    fprintf(fp,"texCoordIndex [\n");

    for (ii=0;ii<dcount;ii++) {
        fprintf(fp,"%d %d %d %d,\n", 6*ii, 6*ii+1, 6*ii+2, -1 );
        fprintf(fp,"%d %d %d %d", 6*ii+3, 6*ii+4, 6*ii+5, -1);
        
        if ( ii == dcount-1 ) {
            fprintf(fp, "]\n");
        } else {
            fprintf(fp, ",\n");
        }
    }

    fprintf(fp,"}\n}\n]\n}\n");

    fclose(fp);
}

/* Generate a simulated model in VRML 
 */
void
Model::GenerateSimulatedModel (const char *input, const char *filename)
{
    vector<Vec3f> vertices;
    vector<int>   indices;

    FILE *fp = fopen( input, "r" );

    assert( fp );

    int count = 0;

    while (!feof(fp) ) {
        int a;
        float fa, fb, fc;
        if ( fscanf(fp, "%f%f%f%d", &fa, &fb, &fc, &a) == 4 ) {
            vertices.push_back( Vec3f(fa,fb,fc));
            if ( count %4 == 0 ) 
                indices.push_back( a );
            count++;
        }
    }

    fclose( fp );

    int size = 40;
    int nx = 6;
    int ny = 8;
    int tile_w = 440;
    int tile_h = 590;
    int width = nx * tile_w;
    int height = ny * tile_h;

    GenerateVRMLGrid( filename, width, height, size, \
                      vertices, indices, nx, ny, tile_w, tile_h);
}


void
Model::InitTextures( const char *filename )
{
    // exit if no texture was read
    //if ( _n_texels == 0 )
    //    return;

    char ext[5];
    GetFileExtension( filename, ext) ;
    image_format_t type = EXT_NOK;
    if ( strncmp( ext, "jpg",3 ) == 0 )
        type = EXT_JPG;
    if ( strncmp( ext, "png",3 ) == 0 )
        type = EXT_PNG;

    // read the texture image
    int width, height, channels;
    ImgColor *img = LoadImage( filename, type, &width, &height, &channels );

    if ( !img )
        return;

    // allocate texture names
    //_textures = (GLuint*)malloc(sizeof(GLuint));

    glGenTextures( 1, &_texture );

    glBindTexture( GL_TEXTURE_2D, _texture );

    // select modulate to mix texture with color for shading
    glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

    glPixelStorei(GL_UNPACK_ALIGNMENT,1);

    // when texture area is small, bilinear filter the closest mipmap
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    
    GLubyte * data = (GLubyte*)malloc(img->Width()*img->Height()*3);
    
    int cnt=0;

    for (int jj=0;jj<img->Height();jj++) {
        for (int ii=0;ii<img->Width();ii++) {
            data[cnt] = img->GetPixel(ii,img->Height()-1-jj,2);
            data[cnt+1] = img->GetPixel(ii,img->Height()-1-jj,1);
            data[cnt+2] = img->GetPixel(ii,img->Height()-1-jj,0);
            cnt+=3;
        }
    }

    // build our texture mipmaps
    gluBuild2DMipmaps( GL_TEXTURE_2D, 3, img->Width(), img->Height(),
                       GL_RGB, GL_UNSIGNED_BYTE, data );
    
    // free data
    free( data );

    _textureValid = 1;
}

/* compute the centroid of a model */

Vec3f
Model::Centroid ()
{

    Vec3f c(0,0,0);

    for (int ii=0;ii<NVertices();ii++) {
        Vec3f p = GetVertex(ii).v();

        c += p / NVertices();
    }

    return c;
}

/* compute the model diameter */
float
Model::Diameter ()
{
    Vec3f c = Centroid();

    float d = 0.0;

    for (int i=0;i<NVertices();i++) {
        Vec3f p = GetVertex(i).v();
        double dd = len(p-c);
        if ( dd > d )
            d = dd;
    }

    return d;
}
    

/* Translate a model */

void
Model::Subtract ( Vec3f v )
{
    for (int ii=0;ii<NVertices();ii++) {
        _vertices[ii].v ( _vertices[ii].v() - v );
    }
}

void
Model::GetVertices( vector<Vec3f> &data )
{
    for (vector<Vertex>::iterator iter = _vertices.begin(); iter != _vertices.end(); iter++) {
        data.push_back( iter->v() );
    }
}

/* Generate a 3D model for the XYZ axis
   this is for validation only */
void
Model::GenerateCalibrationPattern2()
{
    Clear();
    
    Vec3f a(0,0,0);
    Vec3f b(50,0,0);
    Vec3f c(0,50,0);
    Vec3f d(0,0,50);
    
    Vec3f dp(-4,-4,0);
    Vec3f bp(0,0,-4);
    Vec3f cp(-4,0,0);

    _vertices.push_back(Vertex(a,0));  // 0
    _vertices.push_back(Vertex(b,1));  // 0
    _vertices.push_back(Vertex(c,2));  // 0
    _vertices.push_back(Vertex(d,3));  // 0
    _vertices.push_back(Vertex(bp,4));  // 0
    _vertices.push_back(Vertex(cp,5));  // 0
    _vertices.push_back(Vertex(dp,6));  // 0

    Face f1;
    f1.id = 0;
    f1.n = 3;
    f1.vid[0] = 0; // a,d, dp
    f1.vid[1] = 3;
    f1.vid[2] = 6;

    Face f2;
    f2.id = 0;
    f2.n = 3;
    f2.vid[0] = 0; // a,b, bp
    f2.vid[1] = 1;
    f2.vid[2] = 4;

    Face f3;
    f3.id = 0;
    f3.n = 3;
    f3.vid[0] = 0; // a,c, cp
    f3.vid[1] = 2;
    f3.vid[2] = 5;
    
    _faces.push_back( f1 );
    _faces.push_back( f2 );
    _faces.push_back( f3 );
    
    _valid = 1;

}

/* generate a 3D model for a calibration pattern */
void
Model::GenerateCalibrationPattern()
{
    // clear the current model
    Clear();

    int ii,jj;
    int count = 0;
    int count2 = 0;

    dbg(DBG_INFO, "Generating pattern (%d,%d,%f)\n", 
        PATTERN_NX, PATTERN_NY, PATTERN_SIZE);

    for (ii=0;ii<PATTERN_NX;ii++) {
        for (jj=0;jj<PATTERN_NY;jj++) {
            
            Vec3f a = Vec3f(ii*PATTERN_SIZE, jj*PATTERN_SIZE, 0);
            Vec3f b = a + Vec3f(PATTERN_SIZE,0,0);
            Vec3f c = b + Vec3f(0,PATTERN_SIZE,0);
            Vec3f d = a + Vec3f(0,PATTERN_SIZE,0);
            
            if ( (ii+jj)%2 == 1 )
                continue;

            _vertices.push_back( Vertex( a, count2) );
            _vertices.push_back( Vertex( b, count2+1) );
            _vertices.push_back( Vertex( c, count2+2) );
            _vertices.push_back( Vertex( d, count2+3) );

            Face f1;
            f1.n = 3;
            f1.nm = Vec3f(0,0,1);
            f1.id = count;
            f1.vid[0] = count2;
            f1.vid[1] = count2+1;
            f1.vid[2] = count2+2;

            count++;

            Face f2;
            f2.n = 3;
            f2.nm = Vec3f(0,0,1);
            f2.id = count;
            f2.vid[0] = count2;
            f2.vid[1] = count2+2;
            f2.vid[2] = count2+3;
        
            _faces.push_back( f1 );        
            _faces.push_back( f2 );

            count2+=4;
            count++;
        }
    }
    
    _valid = 1;

    dbg(DBG_INFO, "Generated %d vertices and %d faces.\n", \
        NVertices(), NFaces());
            
}

/****************************************************************
 *     SPHERE TESSELLATION
 *
 ****************************************************************/

/* Generate a unit icosahedron
 */
void Model::MakeIcosahedron (float size)
{
    // clear the model
    Clear();
    
    int i;

    // poles
    Vertex v1( Vec3f(0,0,size), 0 );
    Vertex v2( Vec3f(0,0,-size),1 );

    _vertices.push_back( v1 );
    _vertices.push_back( v2 );
    
    // the other vertices are on two pentagons
    // of latitude theta = 26.565 degrees
    double sint = sin( toRadians( 26.565 ) );
    double cost = cos( toRadians( 26.565 ) );
    
    double psi = 0.0;
    
    for (i=0;i<5;i++) {
        
        _vertices.push_back( Vertex( size*Vec3f(cost*cos(psi), cost*sin(psi), sint), i+2 ) );
        psi += 2 * M_PI / 5.0;
    }
    
    psi = 2*M_PI/10.0;
    
    for (i=0;i<5;i++) {

        _vertices.push_back( Vertex( size*Vec3f(cost*cos(psi), cost*sin(psi), -sint), i+7) );
        psi += 2 * M_PI / 5.0;
    }

    // make faces
    _faces.push_back( Face( 2, 0, 6, 0 ));
    _faces.push_back( Face( 6, 0, 5, 1 ));
    _faces.push_back( Face( 5, 0, 4, 2 ));
    _faces.push_back( Face( 4, 0, 3, 3 ));
    _faces.push_back( Face( 3, 0, 2, 4 ));
    
    _faces.push_back( Face( 11, 2, 6, 5 ));
    _faces.push_back( Face( 10, 6, 5, 6 ));
    _faces.push_back( Face( 9, 5, 4, 7 ));
    _faces.push_back( Face( 8, 4, 3, 8 ));
    _faces.push_back( Face( 7, 3, 2, 9 ));
    
    _faces.push_back( Face( 7, 2, 11, 10 ));
    _faces.push_back( Face( 11, 6, 10, 11 ));
    _faces.push_back( Face( 10, 5, 9, 12 ));
    _faces.push_back( Face( 9, 4, 8, 13 ));
    _faces.push_back( Face( 8, 3, 7, 14 ));
    
    _faces.push_back( Face( 7, 11, 1, 15 ));
    _faces.push_back( Face( 11, 10, 1, 16 ));
    _faces.push_back( Face( 10, 9, 1, 17 ));
    _faces.push_back( Face( 9, 8, 1, 18 ));
    _faces.push_back( Face( 8, 7, 1, 19 ));
    
    // compute normals
    ComputeNormals();

    _valid = 1;
}

void Model::ComputeNormals()
{
    for (int i=0;i<(int)_faces.size();i++) {

        Face f = GetFace(i);

        Vec3f a = GetVertex( f.vid[0] ).v();
        Vec3f b = GetVertex( f.vid[1] ).v();
        Vec3f c = GetVertex( f.vid[2] ).v();

        f.nm = cross(norm(b-a), norm(c-a));

        if ( len(f.nm) < 1E-4 ) {
            dbg(DBG_INFO, "Error: can compute normal vector for following face:\n");
            dbg(DBG_INFO, "a: %f %f %f\n", a[0], a[1], a[2]);
            dbg(DBG_INFO, "b: %f %f %f\n", b[0], b[1], b[2]);
            dbg(DBG_INFO, "c: %f %f %f\n", c[0], c[1], c[2]);
        }

        f.nm = norm(f.nm );

        _faces[i].nm = f.nm ;

        assert( fabs(len(f.nm)-1.0) < 1E-4 );
    }
}

/* generate a triangle subdivision of initial points with triangle of size smaller than <resolution>
 */
void Model::Tessellate ( int level )
{
    int i;

    float size = Diameter();

    // create the new points
    for (int k=0;k<level;k++) {
       
        int nf = _faces.size();

        int nv = _vertices.size();

        int count = nf;

        for (i=0;i<nf;i++) {
            
            Face fc = GetFace(i);

            // create new vertices
            Vec3f a = GetVertex( fc.vid[0] ).v();
            Vec3f b = GetVertex( fc.vid[1] ).v();
            Vec3f c = GetVertex( fc.vid[2] ).v();
        
            Vec3f d = size*norm((a+b)/2);
            Vec3f e = size*norm((b+c)/2);
            Vec3f f = size*norm((c+a)/2);

            _vertices.push_back( Vertex( d, nv ));
            _vertices.push_back( Vertex( e, nv+1 ));
            _vertices.push_back( Vertex( f, nv+2 ));
            
            // create new faces
            Face f1 = Face( fc.vid[0], nv, nv+2, count );
            Face f2 = Face( fc.vid[1], nv+1, nv, count+1 );
            Face f3 = Face( fc.vid[2], nv+2, nv+1, count+2 );
            Face f4 = Face( nv, nv+1, nv+2, count+3 );

            _faces.push_back( f1 );
            _faces.push_back( f2 );
            _faces.push_back( f3 );
            _faces.push_back( f4 );

            nv += 3;

            count += 4;
        }

        // remove the original vertices
        for (i=0;i<nf;i++) {
            _faces.erase( _faces.begin() );
        }
    }
        
    // compute normals
    ComputeNormals();

    // measure the angular resolution of the tessellation
    Vec3f v = _vertices[0].v();

    vector<Vec3f> data;

    FindClosestNormals( v, 5, data );

    for (i=0;i<(int)data.size();i++) {
        
        //printf("neighbor[%d]: %f %f %f\n", i, data[i][0], data[i][1], data[i][2]);
        //printf("dot: %f\n", dot(v,data[i]));
        dbg(DBG_INFO, "angle error: %f\n", fabs(acos(dot(norm(v),norm(data[i]))))/MPIOVER180);
    }
}

/* Find the n-closest normal vectors to a given vector
 */

void Model::FindClosestNormals( Vec3f t, int n, vector<Vec3f> &data )
{
    data.clear();

    vector< std::pair< float, Vec3f > > tab;

    for (vector<Face>::iterator iter = _faces.begin(); iter != _faces.end(); iter++) {
        
        Vec3f tmp_n = iter->nm;
        
        float tmp_s = dot( tmp_n, t );

        tab.push_back( std::pair< float, Vec3f > ( tmp_s, tmp_n ) );
    }

    std::sort( tab.begin(), tab.end() );

    std::reverse( tab.begin(), tab.end() );

    tab.resize( n );

    int i;

    for (i=0;i<(int)tab.size();i++)
        data.push_back( tab[i].second );
}


/* Get model normal at a given 3D position */
        
Vec3f Model::GetNormal( Vec3f p )
{
    vector< std::pair< float, Vec3f > > book;

    if ( _faces.empty() ) {
        dbg(DBG_INFO, "Warning: Model is empty!!\n");
        return Vec3f(0,0,1);
    }

    // find the closest face in the model
    int n;
    
    for (n=0;n<(int)_faces.size();n++) {
        
        Vec3f v = _vertices[_faces[n].vid[0]].v();

        book.push_back( std::pair< float, Vec3f > ( len(v-p), _faces[n].nm ) );
    }

    dbg(DBG_INFO, "Book unsorted (%d items)\n", (int)book.size());

    // sort the book

    std::sort( book.begin(), book.end() );

    dbg(DBG_INFO, "Book sorted (%d items)\n", (int)book.size());

    // take the first items and return the normal vector

    return book[0].second;
    
}
