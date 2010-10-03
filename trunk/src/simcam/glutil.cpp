#include "glutil.h"



void
Viewer3dParam::Zoom(float r)
{
    if ( _mode == 0 ) {
        rho *= r;
        eye = target-rho*unit;
    } else { 
        if ( r < 1.0 ) {
            eye += 10 * unit;
        } else {
            eye -= 10 * unit;
        }
    }

    Update();
}

Viewer3dParam::Viewer3dParam()
{
    Reset();
}

void
Viewer3dParam::Reset()
{
    _mode = 0;
    unit = norm(Vec3f( -1,-1,-1));
    lat =  norm(Vec3f(-1,1,0));

    rho = 1000.0;
    eye = -rho * unit;
    target = Vec3f(0,0,0);

    //roll = 0.0;

    zNear = 1.0;
    zFar = 10000.0;
    fov = 45.10; // typical fov for hand-held camera

    Update();
}

int
Viewer3dParam::Mode( int mode )
{
    if ( mode == 0 ) {
        target = eye + rho * unit;
    }

    if ( mode == 0 || mode == 1 ) {
       
        _mode = mode;
        return 0;
    }
    
    return -1;
}

void Viewer3dParam::Update ()
{

    unit = norm(unit);
    lat = norm(lat);
    
    // depending on the mode (inside, outside)
    //if ( _mode == 0 ) {
    //    eye = - rho * unit;
    //}

    // compute the <up> vector
    up = norm(cross( lat, unit));
   
}
void Viewer3dParam::Yaw( float r )
{
    // rotate along up axis
    Quaternion q( up, r);

    unit = q.Rotate( unit );

    lat = q.Rotate( lat );

    if ( _mode == 0 ) {
        eye = target - rho * unit;
    }

    Update();
}

void Viewer3dParam::Pitch( float r )
{
    // rotate along lateral axis
    Quaternion q( lat, r);

    unit = q.Rotate( unit );

    up = q.Rotate( up);

    if ( _mode == 0 ) {
        eye = target - rho * unit;
    }
}

void Viewer3dParam::Roll( float r )
{
    // rotate along lateral axis
    Quaternion q( unit, r);

    lat = q.Rotate( lat );

    up = q.Rotate( up);

    if ( _mode == 0 ) {
        eye = target - rho * unit;
    }
}


void Viewer3dParam::Print()
{
    dbg(DBG_INFO,"Rho: %.2f\n", rho);//, theta, phi);
    dbg(DBG_INFO,"near, far: %.2f, %.2f\n", zNear, zFar);
    dbg(DBG_INFO,"fov: %f   mode: %d\n", fov, _mode);
    dbg(DBG_INFO,"eye:  %.2f %.2f %.2f\n", eye[0], eye[1], eye[2]);
    dbg(DBG_INFO,"unit: %.2f %.2f %.2f\n", unit[0], unit[1], unit[2]);
    dbg(DBG_INFO,"up:  %.2f %.2f %.2f\n", up[0], up[1], up[2]);

}   

void Viewer3dParam::Print( FILE *fp )
{
    fprintf(fp, "%f %f %f ", eye[0], eye[1], eye[2]);
    Quaternion q = GetRotation();
    q.Print(fp);
    fprintf(fp, "\n");
}

void Viewer3dParam::Read( FILE *fp ) 
{
    float tx, ty, tz, qw, qx, qy, qz;

    if ( fscanf(fp, "%f%f%f%f%f%f%f", &tx, &ty, &tz, &qw, &qx, &qy, &qz) ==7 ) {
        SetPose( Quaternion( qw, Vec3f(qx,qy,qz)), Vec3f(tx,ty,tz));
    }
}

// get the rotation matrix that brings the coordinate frame to the camera coordinate frame
Quaternion Viewer3dParam::GetRotation()
{
    // compute the rotation that brings (unit,lat) onto (Z,X)
    Quaternion f = ComputeRotation( unit, lat, Vec3f(0,0,1), Vec3f(1,0,0) );

    //Vec3f a = f.Rotate( unit );
    //Vec3f b = f.Rotate( lat );
    //Vec3f c = f.Rotate( up );

    //log_dbg(DBG_INFO, "unit -> %f %f %f  lat -> %f %f %f  up-> %f %f %f", 
    //a[0], a[1], a[2], b[0], b[1], b[2], c[0], c[1], c[2]);

    return f;//.Conjugate();
}

// get the translation to the camera center
Vec3f Viewer3dParam::GetTranslation()
{
    return eye;
}

// set the rotation of the 3D viewer
void  Viewer3dParam::SetPose( Quaternion q, Vec3f t )
{

    Rotation( q.Conjugate() );
    
    Translation( t );

}
    
void Viewer3dParam::Rotation( Quaternion q )
{
    unit = q.Rotate(Vec3f(0,0,1));
    up   = -q.Rotate(Vec3f(0,1,0));
    lat  = q.Rotate(Vec3f(1,0,0));
    
    Update();
}

void  Viewer3dParam::Translation( Vec3f t) 
{
    eye = t;

    Update();

}

/****************************************************/


void
GlScreenshot( int w, int h, const char *filename )
{
    // grab pixels from the screen
    float *buffer = (float*)malloc( 3*w*h*sizeof(float));
    
    ImgColor *img = new ImgColor( w, h );
    
    glReadPixels( 0, 0, w, h, GL_RGB, GL_FLOAT, buffer);

    // multiply by 255 and flip vertically (OpenGL origin is lower left)
    int i,j;
    for (i=0;i<h;i++) {
        for (j=0;j<w;j++) {
            img->SetPixel( j, i, 
                           (unsigned char)(255*buffer[3*(w*(h-1-i)+j)]), 
                           (unsigned char)(255*buffer[3*(w*(h-1-i)+j)+1]),
                           (unsigned char)(255*buffer[3*(w*(h-1-i)+j)+2])
                           );
        }
    }
    
    // save image
    char ext[5];
    GetFileExtension( filename, ext );
    
    if ( strncmp(ext,"jpg",3) == 0 ) 
        SaveImage( img, filename, EXT_JPG );
    if ( strncmp(ext,"png",3) == 0 ) 
        SaveImage( img, filename, EXT_PNG );

    free( buffer );
}

void
DrawModel( Model *model, int textureEnabled )
{
    glColor(WHITE);
    glPolygonMode( GL_FRONT, GL_FILL );
    glPolygonMode( GL_BACK, GL_POINT);

    if ( textureEnabled && model->TextureValid() ) {
        //dbg(DBG_INFO,"TEXTURE ENABLED\n");
        glEnable( GL_TEXTURE_2D );
        glBindTexture( GL_TEXTURE_2D, model->GetTextureId());
    }
    else {
        glDisable( GL_TEXTURE_2D );
        // dbg(DBG_INFO,"TEXTURE DISABLED\n");
    }

    double scale_x = model->GetTextureSx();//0.0009765625;
    double scale_y = model->GetTextureSy();//0.0001408848;
    
    int ii=0,jj;
    if ( textureEnabled && model->TextureValid()) {

        // with texture
        for (ii=0;ii<model->NTexels();ii++) {
            TriangleTexel tx = model->GetTexel(ii);
            
            glBegin(GL_TRIANGLES);
            Vec3f nl = norm(model->GetFace(ii).nm);
            Vec3f n1 = model->GetVertex(model->GetFace(ii).vid[0]).v();
            Vec3f n2 = model->GetVertex(model->GetFace(ii).vid[1]).v();
            Vec3f n3 = model->GetVertex(model->GetFace(ii).vid[2]).v();

            glNormal3f( nl[0], nl[1], nl[2] );  

            glTexCoord2f( (double)(tx.c0)*scale_x, (double)scale_y * tx.r0);
            int id = model->GetCoordIndex(tx.a);
            //Vec3f vx = model->GetVertex(tx.a).v;
            glVertex3f( n1[0], n1[1], n1[2] );
            
            glTexCoord2f( (double)(tx.c1)*scale_x, (double)scale_y * tx.r1);
            id = model->GetCoordIndex(tx.b);
            //vx = model->GetVertex(tx.b).v;
            glVertex3f( n2[0], n2[1], n2[2] );
            
            glTexCoord2f( (double)(tx.c2)*scale_x, (double)scale_y * tx.r2);
            id = model->GetCoordIndex(tx.c);
            //vx = model->GetVertex(tx.x).v;
            glVertex3f( n3[0], n3[1], n3[2] );
            
            glEnd();

        } 
    } else {

        // no texture
        for (ii=0;ii<model->NFaces();ii++) {
            Face fc = model->GetFace(ii);
            
            glNormal3f( fc.nm[0], fc.nm[1], fc.nm[2] );    
            
            if ( fc.n == 3 ) {
                glBegin(GL_TRIANGLES);
                
            } else if ( fc.n == 4 ) {
                glBegin(GL_QUADS);
            } else {
                return;
            }
            
            for (jj=0;jj<fc.n;jj++) {
                
                Vec3f vx = model->GetVertex(fc.vid[jj]).v();
                glVertex3f( vx[0], vx[1], vx[2] );
            }

            glEnd();
        }
    }
    
}



/* draw a model in feedback mode and return the list of visible vertices */
void
DrawModelFeedbackMode( Model *model, int width, int height, 
                       Viewer3dParam *par, vector<int> &vertexIndices )
{

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glRenderMode (GL_RENDER);
    GlClearWindow(1);
    
    Setup3d( par );
    DrawModel( model, 0);

    GLfloat *pixels = (GLfloat*)malloc(3*width*height*sizeof(GLfloat));
    glReadPixels(0,0,width, height, GL_DEPTH_COMPONENT,GL_FLOAT,(void*)pixels);

    // no texture
    int ii;
    
    // init feedback mode
    int BUFSIZE_FEEDBACK = 6 * ( model->NVertices() + 1 );
    GLfloat *feedbackBuffer = (GLfloat *)malloc(BUFSIZE_FEEDBACK*sizeof(GLfloat));
    if ( feedbackBuffer == NULL ) {
        dbg(DBG_INFO, "Failed to init feedback data!\n");
    }
    glFeedbackBuffer(BUFSIZE_FEEDBACK,GL_3D,feedbackBuffer);
    
    // set mode to feedback
    glRenderMode(GL_FEEDBACK);

    Setup3d( par );
    
    for (ii=0;ii<model->NVertices();ii++) {
        Vec3f vx = model->GetVertex(ii).v();
        
        // pass index in feedback mode
        glPassThrough(ii);

        glBegin(GL_POINTS);
        glVertex3f( vx[0], vx[1], vx[2] );
        glEnd();
    }

    // retrieve feedback
    GLuint hits = glRenderMode( GL_RENDER );

    // parse hits
    if ( (int)hits != -1 ) {
        GLfloat *ptr = (GLfloat*)feedbackBuffer;
        GLfloat x,y,z;
        GLint code,id;
        int counter =0;

        for (ii=0;ii<(int)hits;ii++) {

            code = (int)*ptr;
            
            if ( code == GL_PASS_THROUGH_TOKEN ) {
                id = (int)*(ptr+1);
                ptr+=2;
                ii+=1;
                continue;
            }

            if ( code == GL_POINT_TOKEN ) {
                x = *(ptr+1);
                y = *(ptr+2);
                z = *(ptr+3);
                int xx = x - floor(x) > 0.5 ? int(floor(x))+1 : int(floor(x));
                int yy = y - floor(y) > 0.5 ? int(floor(y))+1 : int(floor(y));
                
                if ( xx < width - 1 && xx > 1 && \
                     yy < height - 1 && yy > 1 ) {
         
                    float thresh = MAX( MAX( MAX( pixels[yy*width+xx], 
                                                  pixels[(yy-1)*width+xx]), 
                                             pixels[yy*width+xx-1] ), 
                                        pixels[(yy-1)*width+xx-1] );

                    if ( z <= thresh + 1E-5 ) {
                        vertexIndices.push_back( id );
                        counter++;
                    }
                }

                ptr += 4;
                ii += 3;
                continue;
            }
        }

        dbg(DBG_INFO,"%d visible points (%d model points)\n", counter, model->NVertices());
    }    

    // free memory

    delete ( feedbackBuffer );
    delete ( pixels );

}

void
Setup3dSelect ( Viewer3dParam *par, GLuint *buffer, int cursorX, int cursorY )
{
        GLint viewport[4];
        glSelectBuffer(SELECT_BUFSIZE, buffer);
        glRenderMode(GL_SELECT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
    
        glGetIntegerv(GL_VIEWPORT,viewport);
        gluPickMatrix(cursorX,viewport[3]-cursorY,
			5,5,viewport);
        
        float ratio = 1.386;

         gluPerspective(par->fov, ratio, par->zNear, par->zFar);

         glMatrixMode(GL_MODELVIEW);
         glLoadIdentity();
         gluLookAt(par->eye[0],par->eye[1],par->eye[2],
                   par->unit[0],par->unit[1],par->unit[2],
                   par->up[0],par->up[1],par->up[2]);

         glInitNames();
}



void
Setup3d ( Viewer3dParam *par )

{
    //dbg(DBG_INFO,"Setting up 3D viewer...\n");
 
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float ratio = 640.0 / 480.0;
    gluPerspective(par->fov, ratio, par->zNear, par->zFar);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(par->eye[0],par->eye[1],par->eye[2],
              par->eye[0]+par->unit[0],par->eye[1]+par->unit[1],\
              par->eye[2]+par->unit[2],
              par->up[0],par->up[1],par->up[2]);
}



void TestCameraProjection( Vec3f p, CamExtr *extr, CamIntr *intr )
{
    printlog( "Testing projection of p = %f %f %f", p[0], p[1], p[2]);

    // transform from world frame to camera frame
    Vec3f cp = extr->Transform( p  );      

    printlog( "In camera frame: %f %f %f", cp[0], cp[1], cp[2]);

    // project onto the camera plane
    Vec2f pp = intr->ProjectToPixels( cp );

    printlog( "On camera plane: %f %f", pp[0], pp[1]);
}
 


/* project a set of 3D points onto the camera and display them 
 * in select mode, points can be selected by clicking on them */

void Draw3DPoints2D( vector<Vec3f> &points, Color color, int size, int select,
                   CamExtr *extr, CamIntr *intr )
{ 
    //TestCameraProjection( Vec3f(0,0,0), extr, intr );

    // in select mode, draw a box around the points
    if ( select ) {

        int counter = 0;
        
        for (vector<Vec3f>::iterator iter = points.begin(); iter != points.end(); iter++) {
            
            Vec3f v = *iter;
            
            glPushName( counter );
            
            // transform from world frame to camera frame
            Vec3f cp = extr->Transform( v );      

            // project onto the camera plane
            Vec2f pp = intr->ProjectToPixels( cp );

            // draw the point on the image
            glBegin(GL_QUADS);
            DrawBox( pp, size );
            glEnd();

            glPopName();
            
            counter++;
            
        }

        // in render mode, simply draw the points
    } else {

        glColor(color);
        glPointSize(size);
        glBegin(GL_QUADS);
        
        for (vector<Vec3f>::iterator iter = points.begin(); iter != points.end(); iter++) {
            
            Vec3f v = *iter;
            
            // transform from world frame to camera frame
            Vec3f cp = extr->Transform( v );      

            // project onto the camera plane
            Vec2f pp = intr->ProjectToPixels( cp );
            
            // draw the point on the image
            DrawBox( pp, size );
            
        }
        
        glEnd();
        
        glPointSize(1);
    }
}

/* project a 3D model onto the camera plane and draw it in 2D */
void DrawModel2D( Model *model, CamExtr *extr, CamIntr *intr, 
                  int textureEnabled )
{
    if ( !intr->Valid() )
        return;
    
    glColor(WHITE);
    
    if ( textureEnabled && model->TextureValid() ) {
        //dbg(DBG_INFO,"TEXTURE ENABLED\n");
        glEnable( GL_TEXTURE_2D );
        glBindTexture( GL_TEXTURE_2D, model->GetTextureId());
    }
    else {
        glDisable( GL_TEXTURE_2D );
        // dbg(DBG_INFO,"TEXTURE DISABLED\n");
    }

    double scale_x = model->GetTextureSx();//0.0009765625;
    double scale_y = model->GetTextureSy();//0.0001408848;
    
    // sort faces by depth
    std::vector< std::pair< float, int > > book;

    int ii;

    Vec3f t0 = extr->Translation();

    for (ii=0;ii<model->NFaces();ii++) {

        Face fc = model->GetFace(ii);

        Vec3f wp1 = model->GetVertex(fc.vid[0]).v();
        Vec3f wp2 = model->GetVertex(fc.vid[1]).v();
        Vec3f wp3 = model->GetVertex(fc.vid[2]).v();

        double d = ( len(wp1-t0) + len(wp2-t0) + len(wp3-t0) ) / 3.0;

        book.push_back( std::pair<float, int> (d,ii) );

    }

    std::sort( book.begin(), book.end() );

    std::reverse( book.begin(), book.end() );

    // draw each face
    
    int jj=0;

    for (jj=0;jj<model->NFaces();jj++) {
        
        ii = book[jj].second;

        Face fc = model->GetFace(ii);
        
        
        
        Vec3f wp1 = model->GetVertex(fc.vid[0]).v();
        Vec3f wp2 = model->GetVertex(fc.vid[1]).v();
        Vec3f wp3 = model->GetVertex(fc.vid[2]).v();

        // transform from world frame to camera frame
        Vec3f cp1 = extr->Transform( wp1 );      
        // project onto the camera plane
        Vec2f pp1 = intr->ProjectToPixels( cp1 );
        
        // transform from world frame to camera frame
        Vec3f cp2 = extr->Transform( wp2 );      
        // project onto the camera plane
        Vec2f pp2 = intr->ProjectToPixels( cp2 );
        
        // transform from world frame to camera frame
        Vec3f cp3 = extr->Transform( wp3 );      
        // project onto the camera plane
        Vec2f pp3 = intr->ProjectToPixels( cp3 );
        
        if ( cp1[2] < 1E-6 || cp2[2] < 1E-6 || cp3[2] < 1E-6 )
            continue;

        glBegin(GL_TRIANGLES);
        Vec3f nl = norm(fc.nm);
        
        
        glNormal3f( nl[0], nl[1], nl[2] );  
        TriangleTexel tx;

         if ( textureEnabled && model->TextureValid() ) {
             tx = model->GetTexel(ii);
             glTexCoord2f( (double)(tx.c0)*scale_x, (double)scale_y * tx.r0);
         }

         // draw the point on the image
         glVertex2f(pp1[0], pp1[1]);
         
         
         if ( textureEnabled && model->TextureValid() )
             glTexCoord2f( (double)(tx.c1)*scale_x, (double)scale_y * tx.r1);

         // draw the point on the image
         glVertex2f(pp2[0], pp2[1]);
         
         if ( textureEnabled && model->TextureValid() )
             glTexCoord2f( (double)(tx.c2)*scale_x, (double)scale_y * tx.r2);

         // draw the point on the image
         glVertex2f(pp3[0], pp3[1]);
         
         glEnd();
    }

    // draw the center point in green
    /*glColor(GREEN);
    glPointSize(4);
    glBegin(GL_POINTS);
    glVertex2f(intr->Cx(), intr->Cy());
    glEnd();
    glPointSize(1);
    */
}

void DrawCamera( CamExtr *extr, float size, Color color )
{
    Vec3f o = extr->Translation();
    
    Vec3f a = extr->X();
    Vec3f b = extr->Y();
    Vec3f c = extr->Z();

    glColor(YELLOW);
    glLineWidth(2);

    glPointSize(10);
    glBegin(GL_POINTS);
    glVertex(o);
    glEnd();

    glBegin(GL_LINES);
    glVertex(o);
    glVertex(o+size*c);
    glEnd();

    glLineWidth(1);
    
    glBegin(GL_LINES);
    glVertex(o);
    glVertex(o+size*a);
    glVertex(o);
    glVertex(o+size*b);
    glEnd();
    
    glPointSize(1);
}
