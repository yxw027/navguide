#include "flow.h"


const int MAX_COUNT = 500;
const int WIN_SIZE = 15;
const int MIN_VOTES = 10;

#define FLOW_DEBUG 0

flow_t *flow_init ()
{
    flow_t *f = (flow_t*)malloc(sizeof(flow_t));
    f->flow_grey = NULL;
    f->flow_prev_grey = NULL;
    f->flow_pyramid = NULL;
    f->flow_prev_pyramid = NULL;
    f->flow_swap_temp = NULL;
    f->flow_points[0] = NULL;
    f->flow_points[1] = NULL;
    f->flow_swap_points = NULL;
    f->flow_status = NULL;
    f->flow_flags = 0;
    f->flow_need_to_init = 1;
    f->flow_count = 0;
    f->utime0 = 0;
    f->utime1 = 0;
    f->width = 0;
    f->height = 0;

    return f;

}

void flow_t_destroy (flow_t *f)
{
    if (f->flow_grey)
        cvReleaseImage (&f->flow_grey);
    if (f->flow_prev_grey)
        cvReleaseImage (&f->flow_prev_grey);
    if (f->flow_pyramid)
        cvReleaseImage (&f->flow_pyramid);
    if (f->flow_prev_pyramid)
        cvReleaseImage (&f->flow_prev_pyramid);
    if (f->flow_swap_temp)
        cvReleaseImage (&f->flow_swap_temp);
    if (f->flow_status)
        cvFree (&f->flow_status);
    if (f->flow_points[0])
        cvFree (&f->flow_points[0]);
    if (f->flow_points[1])
        cvFree (&f->flow_points[1]);
    if (f->flow_swap_points)
        cvFree (&f->flow_swap_points);

    free (f);
}

void flow_reverse (flow_t *f)
{
    for (int i=0;i<f->flow_count;i++) {
        double dx = f->flow_points[1][i].x - f->flow_points[0][i].x;
        double dy = f->flow_points[1][i].y - f->flow_points[0][i].y;

        f->flow_points[1][i].x = f->flow_points[0][i].x - dx;
        f->flow_points[1][i].y = f->flow_points[0][i].y - dy;
    }
}

/* extract good features to track
 */
void flow_good_features_to_track (IplImage *img, CvPoint2D32f *points, int *count, double scale)
{
    IplImage *eig = cvCreateImage ( cvGetSize (img), IPL_DEPTH_32F, 1);
    IplImage *tmp = cvCreateImage ( cvGetSize (img), IPL_DEPTH_32F, 1);

    // compute good features to track 
    double quality_level = 0.01;
    double min_distance = 5.0;
    int block_size = 3;
    int use_harris = 0;
    double harris_param = 0.04;// used only if use_harris is 1

    int max_count=300;

    *count=max_count;

    cvGoodFeaturesToTrack (img, eig, tmp, points, count, quality_level, min_distance,
            NULL, block_size, use_harris, harris_param);

    int win_size = MIN (10, MAX (3, img->width/40));

    // subpixel accuracy
    cvFindCornerSubPix (img, points, *count, cvSize(win_size, win_size), cvSize(-1,-1),
            cvTermCriteria (CV_TERMCRIT_ITER|CV_TERMCRIT_EPS, 20, 0.03));

    cvReleaseImage (&eig);
    cvReleaseImage (&tmp);
}

flow_t *flow_compute (unsigned char *data1, unsigned char *data2, int data_width, int data_height, int64_t utime1, int64_t utime2, double scale, double *success_rate)
{
    GTimer *timer = g_timer_new ();

    flow_t *f = flow_init ();

    int width = data_width;
    int height = data_height;

    if (scale < .99) {
        width = (int)(data_width * scale);
        height = (int)(data_height * scale);
    }
    
    IplImage *grey = cvCreateImage ( cvSize (data_width, data_height), 8, 1);
    IplImage *prev_grey = cvCreateImage ( cvSize (data_width, data_height), 8, 1);

    memcpy (prev_grey->imageData, data1, data_width * data_height);
    memcpy (grey->imageData, data2, data_width * data_height);

    f->flow_grey = cvCreateImage( cvSize(width, height), 8, 1 );
    f->flow_prev_grey = cvCreateImage( cvSize(width, height), 8, 1 );
    f->flow_pyramid = cvCreateImage( cvSize(width, height), 8, 1 );
    f->flow_prev_pyramid = cvCreateImage( cvSize(width, height), 8, 1 );
    f->flow_points[0] = (CvPoint2D32f*)cvAlloc(MAX_COUNT*sizeof(CvPoint2D32f));
    f->flow_points[1] = (CvPoint2D32f*)cvAlloc(MAX_COUNT*sizeof(CvPoint2D32f));
    f->flow_status = (char*)cvAlloc(MAX_COUNT);
    f->flow_flags = 0;
    f->width = width;
    f->height = height;
    f->utime0 = utime1;
    f->utime1 = utime2;

    cvResize (grey, f->flow_grey);
    cvResize (prev_grey, f->flow_prev_grey);

    cvReleaseImage (&grey);
    cvReleaseImage (&prev_grey);

    flow_good_features_to_track (f->flow_grey, f->flow_points[0], &f->flow_count, scale);

    int win_size = (int)(WIN_SIZE * scale)+5;

    if( f->flow_count > 0 )
    {
        cvCalcOpticalFlowPyrLK( f->flow_prev_grey, f->flow_grey, f->flow_prev_pyramid, f->flow_pyramid,
                f->flow_points[0], f->flow_points[1], f->flow_count, cvSize(win_size, win_size), 3, f->flow_status, 0,
                cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS,20,0.03), f->flow_flags );
    }


    double secs = g_timer_elapsed (timer, NULL);
    dbg (DBG_CLASS, "flow timer: %d x %d image. (%.1f Hz)", width, height, 1.0/secs);
    g_timer_destroy (timer);

    return f;
}

IplImage* flow_to_image (flow_t *f)
{
    IplImage *imf = cvCreateImage (cvSize (f->width, f->height), 8, 3); // flow image

    // white background
    cvRectangle (imf, cvPoint(0,0), cvPoint(f->width-1, f->height-1), CV_RGB(255, 255, 255), CV_FILLED);

    for (int i=0 ; i<f->flow_count; i++) {
        if (!f->flow_status[i])
            continue;

        double x = f->flow_points[0][i].x;
        double y = f->flow_points[0][i].y;

        double fx = ( f->flow_points[1][i].x - f->flow_points[0][i].x );
        double fy = ( f->flow_points[1][i].y - f->flow_points[0][i].y );

        CvPoint p1 = cvPoint (x, y);
        CvPoint p2 = cvPoint (x+fx, y+fy);

        cvLine (imf, p1, p2, CV_RGB (255, 0, 0));

        double a1 = atan2(fy,fx) + to_radians (120.0);
        double a2 = atan2(fy,fx) - to_radians (120.0);
        double length = .03;
        cvLine (imf, p2, cvPoint (p2.x + f->width * length * cos(a1), p2.y + f->height * length * sin(a1)), CV_RGB (255, 0, 0)); 
        cvLine (imf, p2, cvPoint (p2.x + f->width * length * cos(a2), p2.y + f->height * length * sin(a2)), CV_RGB (255, 0, 0)); 
    }

    return imf;
}

void flow_draw (flow_t *f, char *fname)
{
    if (! (f && f->width > 0 && f->height > 0))
        return;

    IplImage *imf =  flow_to_image (f);

    cvSaveImage (fname, imf);

    cvReleaseImage (&imf);
}

void flow_set_draw (flow_t **f, int num, char *fname)
{
    GQueue *imf_queue = g_queue_new ();
    int width, height;

    for (int i=0;i<num;i++) {
        g_queue_push_tail (imf_queue, flow_to_image (f[i]));
        width = f[i]->width;
        height = f[i]->height;
    }

    IplImage *img = cvCreateImage (cvSize (num * width, height), 8, 3);

    for (int i=0;i<num;i++) {
        IplImage *im = (IplImage*)g_queue_peek_nth (imf_queue, i);
        cvSetImageROI (img, cvRect (i*width, 0, width, height));
        cvCopy (im, img, NULL);
        cvReleaseImage (&im);
    }

    cvSaveImage (fname, img);

    cvReleaseImage (&img);
    
    g_queue_free (imf_queue);
                
}

/* code for flow fields **************************************************************************
*/
void flow_field_init (flow_field_t *fc, int nbins)
{
    fc->nbins = nbins;
    fc->flowx = (double*)calloc(nbins*nbins, sizeof(double));
    fc->flowy = (double*)calloc(nbins*nbins, sizeof(double));
    fc->nflow = (int*)calloc(nbins*nbins, sizeof(int));

    flow_field_reset (fc);
}

/* assume (x,y) is in unit coordinates
*/
void FLOW_FIELD_SET (flow_field_t *fc, double x, double y, double fx, double fy, int n)
{
    int nx = MIN (int (x * fc->nbins), fc->nbins-1); 
    int ny = MIN (int (y * fc->nbins), fc->nbins-1); 
    fc->flowx[nx*fc->nbins+ny] = fx;
    fc->flowy[nx*fc->nbins+ny] = fy;
    fc->nflow[nx*fc->nbins+ny] = n;
}

void FLOW_FIELD_GET_WITH_INDEX (flow_field_t *fc, int idx, int idy, double *fx, double *fy, int *n)
{
    assert (0 <= idx && idx < fc->nbins);
    assert (0 <= idy && idy < fc->nbins);

    if (fx) *fx = fc->flowx[idx*fc->nbins+idy];
    if (fy) *fy = fc->flowy[idx*fc->nbins+idy];
    if (n)  *n = fc->nflow[idx*fc->nbins+idy];
}

void FLOW_FIELD_GET (flow_field_t *fc, double x, double y, double *fx, double *fy, int *n)
{
    assert (-1E-6 < x && x < 1.0 + 1E-6);
    assert (-1E-6 < y && y < 1.0 + 1E-6);

    int idx = MIN (int (x * fc->nbins), fc->nbins-1); 
    int idy = MIN (int (y * fc->nbins), fc->nbins-1); 

    FLOW_FIELD_GET_WITH_INDEX (fc, idx, idy, fx, fy, n);
}

void FLOW_FIELD_UPDATE (flow_field_t *fc, double x, double y, double fx, double fy)
{
    double cfx, cfy;
    int n;

    FLOW_FIELD_GET (fc, x, y, &cfx, &cfy, &n);

    double nfx = 1.0 / (n+1) * fx + 1.0 * n / (n+1) * cfx;
    double nfy = 1.0 / (n+1) * fy + 1.0 * n / (n+1) * cfy;

    FLOW_FIELD_SET (fc, x, y, nfx, nfy, n+1);
}

void FLOW_FIELD_MAX_N (flow_field_t *fc, int *maxn)
{
    *maxn = 0;

    for (int i=0;i<fc->nbins*fc->nbins;i++) {
        *maxn = MAX (*maxn, fc->nflow[i]);
    }
}

void flow_field_reset (flow_field_t *fc)
{
    for (int i=0;i<fc->nbins*fc->nbins;i++) {
        fc->flowx[i] = .0;
        fc->flowy[i] = .0;
        fc->nflow[i] = 0;
    }
}

void flow_field_to_image (flow_field_t *fc, IplImage *imf, IplImage *imn)
{
    assert (imf);

    int width = imf->width;
    int height = imf->height;

    int step_x = int (1.0*width/fc->nbins);
    int step_y = int (1.0*height/fc->nbins);

    int maxn;
    FLOW_FIELD_MAX_N (fc, &maxn);

    // white background
    cvRectangle (imf, cvPoint(0,0), cvPoint(width-1, height-1), CV_RGB(255, 255, 255), CV_FILLED);
    if (imn)
        cvRectangle (imn, cvPoint(0,0), cvPoint(width-1, height-1), CV_RGB(255, 255, 255), CV_FILLED);

    // draw grid
    for (int i=0;i<fc->nbins;i++) {
        int x = MIN (width-1, MAX(0, int(1.0*i/fc->nbins*width)));
        for (int j=0;j<fc->nbins;j++) {
            int y = MIN (height-1, MAX(0, int(1.0*j/fc->nbins*height)));
            cvRectangle (imf, cvPoint (x, y), cvPoint (x+step_x, y+step_y), CV_RGB (50,50,50));
            if (imn)
                cvRectangle (imn, cvPoint (x, y), cvPoint (x+step_x, y+step_y), CV_RGB (50,50,50));
        }
    }

    // draw flow arrows
    for (int i=0;i<fc->nbins;i++) {
        int x = MIN (width-1, MAX(0, int(1.0*i/fc->nbins*width)));
        for (int j=0;j<fc->nbins;j++) {
            int y = MIN (height-1, MAX(0, int(1.0*j/fc->nbins*height)));
            double fx,fy;
            int nn;
            FLOW_FIELD_GET_WITH_INDEX (fc, i, j, &fx, &fy, &nn);
            CvPoint p1 = cvPoint (x+step_x/2, y+step_y/2);
            CvPoint p2 = cvPoint (p1.x + fx * width, p1.y + fy * height);
            CvScalar color = nn < MIN_VOTES ? CV_RGB (0,0,255) : CV_RGB (255, 0, 0);
            cvRectangle (imf, cvPoint (p1.x-3, p1.y-3), cvPoint (p1.x+3, p1.y+3), CV_RGB (255, 255, 255), CV_FILLED);
            cvLine (imf, p1, p2, color);
            double a1 = atan2(fy,fx) + to_radians (120.0);
            double a2 = atan2(fy,fx) - to_radians (120.0);
            double length = .03;
            cvLine (imf, p2, cvPoint (p2.x + width * length * cos(a1), p2.y + height * length * sin(a1)), color); 
            cvLine (imf, p2, cvPoint (p2.x + width * length * cos(a2), p2.y + height * length * sin(a2)), color);
        }
    }

    // draw flow count
    if (imn) {
        for (int i=0;i<fc->nbins;i++) {
            int x = MIN (width-1, MAX(0, int(1.0*i/fc->nbins*width)));
            for (int j=0;j<fc->nbins;j++) {
                int y = MIN (height-1, MAX(0, int(1.0*j/fc->nbins*height)));
                int n;
                FLOW_FIELD_GET_WITH_INDEX (fc, i, j, NULL, NULL, &n);
                cvRectangle (imn, cvPoint (x+1, y+1), cvPoint (x+step_x-2, y+step_y-2), CV_RGB (int (1.0*n/maxn*255.0),0,0), CV_FILLED);
            }
        }
    }
}


void flow_field_draw (flow_field_t *fc, int width, int height, char *fname1, char *fname2)
{
    assert (fname1);

    IplImage *imf = cvCreateImage (cvSize (width, height), 8, 3); // flow image
    IplImage *imn = cvCreateImage (cvSize (width, height), 8, 3); // flow count image

    flow_field_to_image (fc, imf, fname2 ? imn : NULL);

    cvSaveImage (fname1, imf);
    if (fname2)
        cvSaveImage (fname2, imn);

    cvReleaseImage (&imf);
    cvReleaseImage (&imn);
}

void flow_field_set_draw (flow_field_set_t *fc, int width, int height, char *fname)
{
    GQueue *imf_queue = g_queue_new ();

    for (int i=0;i<fc->nfields;i++) {
        IplImage *im = cvCreateImage (cvSize (width, height), 8, 3);
        flow_field_to_image (&fc->fields[i], im, NULL);
        g_queue_push_tail (imf_queue, im);
    }

    IplImage *img = cvCreateImage (cvSize (fc->nfields * width, height), 8, 3);

    for (int i=0;i<fc->nfields;i++) {
        IplImage *im = (IplImage*)g_queue_peek_nth (imf_queue, i);
        cvSetImageROI (img, cvRect (i * width, 0, width, height));
        cvCopy (im, img, NULL);
        cvReleaseImage (&im);
    }

    printf ("saved flow field to image. image size: %d x %d\n", img->width, img->height);

    cvSetImageROI (img, cvRect (0, 0, img->width, img->height));

    cvSaveImage (fname, img);

    cvReleaseImage (&img);
    
    g_queue_free (imf_queue);
                
}

int flow_field_update_from_flow (flow_field_t *fc, flow_t *f)
{
    int64_t delta_usecs = f->utime1 - f->utime0;

    if (!f->flow_grey || !f->flow_points[0] || !f->flow_points[1] || delta_usecs == 0) return 0;

    for (int i=0 ; i<f->flow_count; i++) {
        if (!f->flow_status[i])
            continue;

        double x = f->flow_points[0][i].x/f->width;
        double y = f->flow_points[0][i].y/f->height;

        double fx = ( f->flow_points[1][i].x - f->flow_points[0][i].x ) * 1000000.0 / delta_usecs;
        double fy = ( f->flow_points[1][i].y - f->flow_points[0][i].y ) * 1000000.0 / delta_usecs;

        if (!(-1E-6 < x && x < 1.0 + 1E-6) || !(-1E-6 < y && y < 1.0 + 1E-6)) 
            continue;

        FLOW_FIELD_UPDATE (fc, x, y, fx/f->width, fy/f->height);
    }
}

int flow_field_write_to_file (flow_field_t *fc, FILE *fp)
{
    if (!fp) return 0;

    fwrite (&fc->nbins, sizeof(int), 1, fp);
    fwrite (fc->flowx, sizeof(double), fc->nbins*fc->nbins, fp);
    fwrite (fc->flowy, sizeof(double), fc->nbins*fc->nbins, fp);
    fwrite (fc->nflow, sizeof(int), fc->nbins*fc->nbins, fp);

    return 1;
}

int flow_field_read_from_file (flow_field_t *fc, FILE *fp)
{
    if (!fp) return 0;

    fread (&fc->nbins, sizeof(int), 1, fp);

    fc->flowx = (double*)malloc(fc->nbins*fc->nbins*sizeof(double));
    fc->flowy = (double*)malloc(fc->nbins*fc->nbins*sizeof(double));
    fc->nflow = (int*)malloc(fc->nbins*fc->nbins*sizeof(int));

    fread (fc->flowx, sizeof(double), fc->nbins*fc->nbins, fp);
    fread (fc->flowy, sizeof(double), fc->nbins*fc->nbins, fp);
    fread (fc->nflow, sizeof(int), fc->nbins*fc->nbins, fp);

    return 1;

}

flow_field_set_t* flow_field_set_init ()
{
    flow_field_set_t *f = (flow_field_set_t*)calloc (1, sizeof(flow_field_set_t));

    f->fields = NULL;
    f->nfields = 0;
    f->name = NULL;

    return f;
}

flow_field_set_t* flow_field_set_init_with_data (int nfields, int nbins, char *name)
{
    flow_field_set_t *f = (flow_field_set_t*)calloc (1, sizeof(flow_field_set_t));
    
    f->fields = (flow_field_t*)calloc (nfields, sizeof(flow_field_t));
  
    for (int i=0;i<nfields;i++)
        flow_field_init (&f->fields[i], nbins);

    f->nfields = nfields;

    f->name = name ? strdup (name) : strdup ("");

    return f;
}

int flow_field_set_write_to_file (flow_field_set_t *fc, FILE *fp)
{
    int name_length = fc->name ? strlen (fc->name) : 0;

    fwrite (&name_length, sizeof(int), 1, fp);
    
    if (fc->name) 
        fwrite (fc->name, sizeof(char), name_length, fp);

    fwrite (&fc->nfields, sizeof(int), 1, fp);

    for (int i=0;i<fc->nfields;i++)
        flow_field_write_to_file (&fc->fields[i], fp);
}

int flow_field_set_read_from_file (flow_field_set_t *f, FILE *fp)
{
    int name_length;

    fread (&name_length, sizeof(int), 1, fp);

    f->name = name_length > 0 ? (char*)calloc (name_length, sizeof(char)) : NULL;

    if (name_length > 0)
        fread (f->name, sizeof(char), name_length, fp);

    fread (&f->nfields, sizeof(int), 1, fp);

    f->fields = (flow_field_t*)calloc (f->nfields, sizeof(flow_field_t));

    for (int i=0;i<f->nfields;i++) {
        flow_field_read_from_file (&f->fields[i], fp);
    }
}

int flow_field_set_series_read_from_file (GQueue *flow_fields, char **filenames, char **names, int nnames, int nsensors, const char *dirname)
{
    for (int i=0;i<nnames;i++) {

        char fname[256];
        sprintf (fname, "%s/%s", dirname, filenames[i]);

        FILE *fp = fopen (fname, "rb");
        if (fp) {
            dbg (DBG_CLASS, "reading motion configuration from %s", fname);
            flow_field_set_t *f = (flow_field_set_t*)calloc (1, sizeof(flow_field_set_t));
            flow_field_set_read_from_file (f, fp);
            fclose (fp);


            f->name = strdup (names[i]);

            g_queue_push_tail (flow_fields, f);
        }
    }

    return 0;
}

double flow_field_similarity (flow_field_t *fc1, flow_field_t *fc2)
{
    double s = .0;
    int count = 0;

    assert (fc1->nbins == fc2->nbins);

    for (int i=0;i<fc1->nbins*fc1->nbins;i++) {
        double fx1 = fc1->flowx[i];
        double fy1 = fc1->flowy[i];

        double fx2 = fc2->flowx[i];
        double fy2 = fc2->flowy[i];

        double n1 = sqrt (fx1*fx1+fy1*fy1);
        double n2 = sqrt (fx2*fx2+fy2*fy2);

        double dot = fx1*fx2 + fy1*fy2;

        if (fc1->nflow[i] < MIN_VOTES || fc2->nflow[i] < MIN_VOTES)
            continue;

        if (n1 > 1E-4 && n2 > 1E-4)
            dot /= n1 * n2;
        else
            dot = .0;

        s += dot;
        count++;
    }

    if (count == 0) return .0;

    assert (-1.0001 < s  /count && s/count < 1.0001);

    return s / count;

}

double flow_field_vec_length (flow_field_t *f)
{
    double vec_length = .0;
    int count=0;

    for (int i=0;i<f->nbins;i++) {
        vec_length += sqrt (powf (f->flowx[i],2)+powf (f->flowy[i],2));
        count++;
    }

    if (count > 0)
        vec_length /= count;

    return vec_length;
}

double flow_field_set_similarity (flow_field_set_t *f1, flow_field_set_t *f2)
{
    double s = .0;

    for (int i=0;i<f1->nfields;i++) {
        s += flow_field_similarity (&f1->fields[i], &f2->fields[i]);
    }

    return (s / f1->nfields + 1.0) / 2.0;
}

double *flow_field_set_score_motions (GQueue *ref_fields, flow_field_set_t *fc)
{
    int nref = g_queue_get_length (ref_fields);
    int idx=0;
    int maxscoreindex=-1;
    double maxscore = .0;

    double *scores = (double*)calloc (nref, sizeof(double));

    // compute score using similarity measure
    for (GList *iter=g_queue_peek_head_link (ref_fields);iter;iter=iter->next) {

        flow_field_set_t *ref_set = (flow_field_set_t*)iter->data;

        scores[idx] = flow_field_set_similarity (ref_set, fc);

        idx++;
    }

    // compute pair-wise template motion scores
    double *ref_sim = (double*)calloc(nref*nref, sizeof(double));
    int id1 = 0, id2=0;
    GList *iter1, *iter2;

    // compute the reference distances
    for (iter1=g_queue_peek_head_link (ref_fields);iter1;iter1=iter1->next) {
        flow_field_set_t *f1 = (flow_field_set_t*)iter1->data;
        id2 = id1+1;
        for (iter2=iter1->next;iter2;iter2=iter2->next) {
            flow_field_set_t *f2 = (flow_field_set_t*)iter2->data;
            double s12 = flow_field_set_similarity (f1, f2);
            ref_sim[id1*nref+id2] = s12;
            ref_sim[id2*nref+id1] = s12;
            id2++;

        }
        id1++;
    }

    // compute differential score
    for (int j=0;j<nref;j++) {
        double mx = .0;
        for (int i=0;i<nref;i++) 
            mx = fmax (mx, ref_sim[j*nref+i]);
        scores[j] -= .0;//mx;
    }

    // normalize scores
    double norm = .0;
    for (int i=0;i<nref;i++) {
        scores[i] = fmax (.04, scores[i]);
        norm += scores[i];
    }

    if (norm > 1E-5) {
        for (int i=0;i<nref;i++) {
            scores[i] /= norm;
        }
    }

    free (ref_sim);

    return scores;
}

gint flow_field_set_compare_func (gconstpointer el, gconstpointer data)
{
    const char *name = (const char*)data;

    const flow_field_set_t *f = (const flow_field_set_t*)el;

    if (!f->name) return -1;

    return strcmp (f->name, name);
}

/* extract the two dominant motion types from a set of scores
 */
void flow_process_scores (GQueue *ref_fields, double *scores, int *motion1, int *motion2)
{
    GList *iter1, *iter2, *iter3;
    int id1, id2, id3;
    int nref = g_queue_get_length (ref_fields);

    double *ref_sim = (double*)calloc(nref*nref, sizeof(double));
    id1 = 0;

    // compute the reference distances
    for (iter1=g_queue_peek_head_link (ref_fields);iter1;iter1=iter1->next) {
        flow_field_set_t *f1 = (flow_field_set_t*)iter1->data;
        id2 = id1+1;
        for (iter2=iter1->next;iter2;iter2=iter2->next) {
            flow_field_set_t *f2 = (flow_field_set_t*)iter2->data;
            double s12 = flow_field_set_similarity (f1, f2);
            ref_sim[id1*nref+id2] = s12;
            ref_sim[id2*nref+id1] = s12;
            id2++;

        }
        id1++;
    }

    // find the top two highest similarity
    double maxscore[2] = {.0, .0};
    int maxscoreindex[2] = {-1, -1};
    for (int i=0;i<nref;i++) {
        if (maxscoreindex[0] == -1 || maxscore[0] < scores[i]) {
            maxscore[1] = maxscore[0];
            maxscoreindex[1] = maxscoreindex[0];
            maxscore[0] = scores[i];
            maxscoreindex[0] = i;
            continue;
        }
        if (maxscoreindex[1] == -1 || maxscore[1] < scores[i]) {
            maxscore[1] = scores[i];
            maxscoreindex[1] = i;
        }
    }

    gboolean ok[2] = {1,1};

    // check that it is closer than any other reference field
    for (int i=0;i<nref;i++) {
        if (maxscore[0] < ref_sim[maxscoreindex[0]*nref+i]) {
            ok[0] = 0;
        }
        if (maxscore[1] < ref_sim[maxscoreindex[1]*nref+i]) {
            ok[1] = 0;
        }
    }

    if (!ok[0])
        maxscoreindex[0] = -1;
    if (!ok[1])
        maxscoreindex[1] = -1;

    *motion1 = maxscoreindex[0];
    *motion2 = maxscoreindex[1];

    free (ref_sim);
}

navlcm_flow_t* flow_field_set_to_navlcm_flow (flow_field_set_t *f)
{
    navlcm_flow_t *nvf = (navlcm_flow_t*)calloc(1, sizeof(navlcm_flow_t));
    nvf->utime1 = 0;
    nvf->utime2 = 0;
    nvf->num = 0;
    nvf->x = NULL;
    nvf->y = NULL;
    nvf->flowx = NULL;
    nvf->flowy = NULL;
    nvf->sensor = NULL;
    nvf->status = NULL;
    nvf->width = 1;
    nvf->height = 1;

    for (int k=0;k<f->nfields;k++) {
        flow_field_t *fc = (flow_field_t*)&f->fields[k];

        for (int i=0;i<fc->nbins;i++) {
            double x = (1.0 * i + .5) / fc->nbins;
            for (int j=0;j<fc->nbins;j++) {
                double y = (1.0 * j + .5) / fc->nbins;
                double fx,fy;
                int nn;
                FLOW_FIELD_GET_WITH_INDEX (fc, i, j, &fx, &fy, &nn);

                nvf->x = (double*)realloc (nvf->x, (nvf->num+1)*sizeof(double));
                nvf->y = (double*)realloc (nvf->y, (nvf->num+1)*sizeof(double));
                nvf->flowx = (double*)realloc (nvf->flowx, (nvf->num+1)*sizeof(double));
                nvf->flowy = (double*)realloc (nvf->flowy, (nvf->num+1)*sizeof(double));
                nvf->sensor = (unsigned char*)realloc (nvf->sensor, (nvf->num+1)*sizeof(unsigned char));
                nvf->status = (unsigned char*)realloc (nvf->status, (nvf->num+1)*sizeof(unsigned char));

                nvf->x[nvf->num] = x;
                nvf->y[nvf->num] = y;
                nvf->flowx[nvf->num] = nn < MIN_VOTES ? .0 : fx;
                nvf->flowy[nvf->num] = nn < MIN_VOTES ? .0 : fy;
                nvf->sensor[nvf->num] = k;
                nvf->status[nvf->num] = nn >= MIN_VOTES;
                nvf->num++;
            }
        }
    }

    return nvf;
}

