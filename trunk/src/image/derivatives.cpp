//#include <malloc.h>
//#include <stdio.h>
//#include "ary.h"
/*#include <vision/image_format.h>
#include <vision/libary.h>*/
//#include "matop.h"
//#include "jet.h"
//#include <math.h>
#include "image.h"

static float** table_exp;
static int table_x, table_y;
static float total;

#define gauxx(i,j) (table_exp[i][j])

static float** init_gauss(float scale)
{
  int i,j;
  static int inited = 0;
  static float scale_intern = -1;
  int size;

  if (scale != scale_intern) {
    scale_intern = scale;
    if (inited) {
      for (i=0;i<table_x;i++)
        free( table_exp[i] );
      free( table_exp );
    }

    table_exp = (float**)malloc((int)(GAUSS_CUTOFF*scale+1)*sizeof(float*));
    for (i=0;i<(int)(GAUSS_CUTOFF*scale+1);i++) 
      table_exp[i] = (float*)malloc((int)(GAUSS_CUTOFF*scale+1)*sizeof(float));
    table_x = (int)(GAUSS_CUTOFF*scale+1);
    table_y = (int)(GAUSS_CUTOFF*scale+1);

    dbg(DBG_INFO, "Init gauss: [%d,%d] scale = %f\n", table_x, table_y, scale);

    //matrix_alloc((int)(GAUSS_CUTOFF*scale+1),(int)(GAUSS_CUTOFF*scale+1));
    inited = 1;
    for (i=0;i<=(GAUSS_CUTOFF*scale);i++)
      for (j=0;j<=(GAUSS_CUTOFF*scale);j++)
	table_exp[i][j] = exp(-0.5*((i*i+j*j)/(scale*scale)));


    total = 0;
    
    size = (int)(GAUSS_CUTOFF*scale);
    for (i=-size;i<0;i++)
      for (j=-size;j<0;j++)
	total = total + table_exp[-i][-j];
    for (i=-size;i<0;i++)
      for (j=0;j<=size;j++)
	total = total + table_exp[-i][j];

    for (i=0;i<=size;i++)
      for (j=-size;j<0;j++)
	total = total + table_exp[i][-j];
    for (i=0;i<=size;i++)
      for (j=0;j<=size;j++)
	total = total + table_exp[i][j];

    
  }

  return table_exp;
}


float ImgGray::zero(int x, int y, float scale)
{
  float value;
  int size;
  int i,j;
  
  /* calculate the mask for the gaussian */
  /* convolution with the mask in x,y */

  init_gauss(scale);

  size = (int)(GAUSS_CUTOFF*scale);

  value = 0;

  for (i=-size;i<0;i++)
    for (j=-size;j<0;j++)
      value = value +  GetPixel(x+i,y+j) * gauxx(-i,-j);
  for (i=-size;i<0;i++)
    for (j=0;j<=size;j++)
      value = value + GetPixel(x+i,y+j) * gauxx(-i,j);

  for (i=0;i<=size;i++)
    for (j=-size;j<0;j++)
      value = value + GetPixel(x+i,y+j) * gauxx(i,-j);
  for (i=0;i<=size;i++)
    for (j=0;j<=size;j++)
      value = value + GetPixel(x+i,y+j) * gauxx(i,j);

  return(value/total);
}

float ImgGray::First_x(int x, int y, float scale)
{
  float value;
  float square_scale;
  int i,j;
  int size;
  
  init_gauss(scale);
  square_scale = scale * scale;
  
  /* calculate the mask for the first derivative in x direction */
  /* convolution with the mask in x,y */
  
  size = (int)(GAUSS_CUTOFF*scale);
  
  value = 0;
  
  /* calcul des valeurs negatives de i */
  for (i=-size;i<0;i++)
    { /* valeurs negatives de j */
      for (j=-size;j<0;j++)
        {
          value = value + (GetPixel(x+i,y+j) * //image->el[y+j][x+i] * 
                           i/square_scale * gauxx(-i,-j));
        }
      /* valeurs positives de j */
      for (j=0;j<=size;j++)
        {
          value = value + (GetPixel(x+i,y+j) * //image->el[y+j][x+i] * 
                           i/square_scale * gauxx(-i,j));
        }
    }
  /* calcul des valeurs positives de i */
  for (i=0;i<=size;i++)
    {
      for (j=-size;j<0;j++)
        {
          /* valeurs positives de j */
          value = value + (GetPixel(x+i,y+j) * //image->el[y+j][x+i] * 
                           i/square_scale * gauxx(i,-j));
        }
      /* valeurs positives de j */
      for (j=0;j<=size;j++)
        {
          value = value + (GetPixel(x+i,y+j) * //image->el[y+j][x+i] * 
                           i/square_scale * gauxx(i,j));
        }
    }
  
  
  return(value/total);
}

float ImgGray::First_y(int x, int y, float scale)
{
  float value;
  float square_scale;
  int i,j;
  int size;

  init_gauss(scale);
  square_scale = scale * scale;

  size = (int)(GAUSS_CUTOFF*scale);

  value = 0;


  /* calcul des valeurs negatives de j */
  for (i=-size;i<0;i++)
    { /* valeurs negatives de j */
    for (j=-size;j<0;j++)
      {
	value = value + (GetPixel(x+i,y+j) * //image->el[y+j][x+i] * 
			 j/square_scale * gauxx(-i,-j));
      }
    /* valeurs positives de j */
    for (j=0;j<=size;j++)
      {
	value = value + (GetPixel(x+i,y+j) * //image->el[y+j][x+i] * 
			 j/square_scale * gauxx(-i,j));
      }
  }
  /* calcul des valeurs positives de i */
  for (i=0;i<=size;i++)
    {
      for (j=-size;j<0;j++)
	{
	  /* valeurs positives de j */
	  value = value + (GetPixel(x+i,y+j) * //image->el[y+j][x+i] * 
			   j/square_scale * gauxx(i,-j));
	}
      /* valeurs positives de j */
      for (j=0;j<=size;j++)
	{
	  value = value + (GetPixel(x+i,y+j) * //image->el[y+j][x+i] * 
			   j/square_scale * gauxx(i,j));
	}
    }

  return(value/total);
}

float ImgGray::Second_xx(int x, int y,float scale)
{
  float value;
  float square_scale, square_square_scale;
  int i,j;
  int size;
  
  init_gauss(scale);
  size = (int)(GAUSS_CUTOFF*scale);

  square_scale = scale * scale;
  square_square_scale = square_scale * square_scale;
  value = 0;

  /* calcul des valeurs negatives de i */
  for (i=-size;i<0;i++)
    { /* valeurs negatives de j */
    for (j=-size;j<0;j++)
      {
	value = value + (GetPixel(x+i,y+j) * //image->el[y+j][x+i] * 
		((-(1/(square_scale)))+(i*i/square_square_scale))
		  * gauxx(-i,-j) );
      }
    /* valeurs positives de j */
    for (j=0;j<=size;j++)
      {
	value = value + (GetPixel(x+i,y+j) * //image->el[y+j][x+i] * 
		((-(1/(square_scale)))+(i*i/square_square_scale))
		  * gauxx(-i,j) );
      }
  }
  /* calcul des valeurs positives de i */
  for (i=0;i<=size;i++)
    {
      for (j=-size;j<0;j++)
	{
	  /* valeurs positives de j */
	  value = value + (GetPixel(x+i,y+j) * //image->el[y+j][x+i] * 
			   ((-(1/(square_scale)))+
			    (i*i/square_square_scale))
			   * gauxx(i,-j) );
	}
      /* valeurs positives de j */
      for (j=0;j<=size;j++)
	{
	  value = value + (GetPixel(x+i,y+j) * //image->el[y+j][x+i] * 
			   ((-(1/(square_scale)))+
			    (i*i/(square_square_scale)))
			   * gauxx(i,j) );
	}
    }

  return(value/total);
}


float ImgGray::Second_xy(int x, int y,float scale)
{
  float value;
  float square_scale, square_square_scale;
  int i,j;
  int size;
  
  /* calculate the mask for the second derivative in x direction */
  /* convolution with the mask in x,y */

  init_gauss(scale);
  size = (int)(GAUSS_CUTOFF*scale);

  square_scale = scale * scale;
  square_square_scale = square_scale * square_scale;
  value = 0;

  /* calcul des valeurs negatives de i */
  for (i=-size;i<0;i++)
    { 
      /* valeurs negatives de j */
      for (j=-size;j<0;j++)
	{
	  value = value + (GetPixel(x+i,y+j) * //image->el[y+j][x+i] * 
			   (i*j/square_square_scale)
			   * gauxx(-i,-j) );
	}
      /* valeurs positives de j */
      for (j=0;j<=size;j++)
	{
	  value = value + (GetPixel(x+i,y+j) * //image->el[y+j][x+i] * 
			   (i*j/square_square_scale)
			   * gauxx(-i,j));
	}
    }
  /* calcul des valeurs positives de i */
  for (i=0;i<=size;i++)
    {
      for (j=-size;j<0;j++)
	{
	  value = value + (GetPixel(x+i,y+j) * //image->el[y+j][x+i] * 
			   (i*j/square_square_scale)
			   * gauxx(i,-j) );
	}
      /* valeurs positives de j */
      for (j=0;j<=size;j++)
	{
	  value = value + (GetPixel(x+i,y+j) * //image->el[y+j][x+i] * 
			   (i*j/square_square_scale)
			   * gauxx(i,j) );
	}
    }

  return(value/total);
}


float ImgGray::Second_yy(int x, int y,float scale)
{
  float value;
  float square_scale, square_square_scale;
  int i,j;
  int size;
  
  /* calculate the mask for the second derivative in x direction */
  /* convolution with the mask in x,y */

  init_gauss(scale);
  size = (int)(GAUSS_CUTOFF*scale);

  square_scale = scale * scale;
  square_square_scale = square_scale * square_scale;
  value = 0;

  /* calcul des valeurs negatives de i */
  for (i=-size;i<0;i++)
    { /* valeurs negatives de j */
    for (j=-size;j<0;j++)
      {
	value = value + (GetPixel(x+i,y+j) * //image->el[y+j][x+i] * 
		((-(1/(square_scale)))+(j*j/square_square_scale))
		  * gauxx(-i,-j) );
      }
    /* valeurs positives de j */
    for (j=0;j<=size;j++)
      {
	value = value + (GetPixel(x+i,y+j) * //image->el[y+j][x+i] * 
		((-(1/(square_scale)))+(j*j/square_square_scale))
		  * gauxx(-i,j) );
      }
  }
  /* calcul des valeurs positives de i */
  for (i=0;i<=size;i++)
    {
      for (j=-size;j<0;j++)
	{
	  /* valeurs positives de j */
	  value = value + (GetPixel(x+i,y+j) * //image->el[y+j][x+i] * 
			   ((-(1/(square_scale)))+
			    (j*j/square_square_scale))
			   * gauxx(i,-j) );
	}
      /* valeurs positives de j */
      for (j=0;j<=size;j++)
	{
	  value = value + (GetPixel(x+i,y+j) * //image->el[y+j][x+i] * 
			   ((-(1/(square_scale)))+
			    (j*j/(square_square_scale)))
			   * gauxx(i,j) );
	}
    }

  return(value/total);
}


float ImgGray::Third_xxx(int x, int y,float scale)
{
  float value;
  float square_scale;    /* scale * scale */
  float exp4_scale;      /* scale * scale * scale * scale */
  int i,j;
  int size;
  
  init_gauss(scale);
  size = (int)(GAUSS_CUTOFF*scale);

  square_scale = scale * scale;
  exp4_scale = square_scale * square_scale;
  value = 0;

  /* calcul des valeurs negatives de i */
  for (i=-size;i<0;i++)
    { /* valeurs negatives de j */
    for (j=-size;j<0;j++)
      {
	value = value + (GetPixel(x+i,y+j) //image->el[y+j][x+i]
			 *((-i/exp4_scale)*(3-((i*i)/(square_scale)))
			   * gauxx(-i,-j) ));
      }
    /* valeurs positives de j */
    for (j=0;j<=size;j++)
      {
	value = value + (GetPixel(x+i,y+j) //image->el[y+j][x+i]  
			 *((-i/exp4_scale)*(3-((i*i)/(square_scale)))
			   * gauxx(-i,j) ));
      }
  }
  /* calcul des valeurs positives de i */
  for (i=0;i<=size;i++)
    {
      for (j=-size;j<0;j++)
	{
	  /* valeurs positives de j */
	  value = value + (GetPixel(x+i,y+j) //image->el[y+j][x+i] 
			   *((-i/exp4_scale)*(3-((i*i)/(square_scale)))
			     * gauxx(i,-j) ));
			 }
      /* valeurs positives de j */
      for (j=0;j<=size;j++)
	{
	  value = value + (GetPixel(x+i,y+j) //image->el[y+j][x+i]  
			   *((-i/exp4_scale)*(3-((i*i)/(square_scale)))
			     * gauxx(i,j) ));
	}
    }

  return(value/total);
}

float ImgGray::Third_xxy(int x, int y,float scale)
{
  float value;
  float square_scale;    /* scale * scale */
  float exp4_scale;      /* scale * scale * scale * scale */
  int i,j;
  int size;
  
  init_gauss(scale);
  size = (int)(GAUSS_CUTOFF*scale);

  square_scale = scale * scale;
  exp4_scale = square_scale * square_scale;

  value = 0;


  /* calcul des valeurs negatives de i */
  for (i=-size;i<0;i++)
    { /* valeurs negatives de j */
    for (j=-size;j<0;j++)
      {
	value = value + (GetPixel(x+i,y+j) //image->el[y+j][x+i]
			 *((-j/exp4_scale)*(1-((i*i)/(square_scale)))
			   * gauxx(-i,-j) ));
      }
    /* valeurs positives de j */
    for (j=0;j<=size;j++)
      {
	value = value + (GetPixel(x+i,y+j) //image->el[y+j][x+i]  
			 *((-j/exp4_scale)*(1-((i*i)/(square_scale)))
			   * gauxx(-i,j) ));
      }
  }
  /* calcul des valeurs positives de i */
  for (i=0;i<=size;i++)
    {
      for (j=-size;j<0;j++)
	{
	  /* valeurs positives de j */
	  value = value + (GetPixel(x+i,y+j) //image->el[y+j][x+i]
			   * (-j/exp4_scale) * (1-((i*i)/(square_scale)))
			     * gauxx(i,-j) );
			 }
      /* valeurs positives de j */
      for (j=0;j<=size;j++)
	{
	  value = value + (GetPixel(x+i,y+j) //image->el[y+j][x+i]  
			   *((-j/exp4_scale)*(1-((i*i)/(square_scale)))
			     * gauxx(i,j) ));
	}
    }

  return(value/total);
}

float ImgGray::Third_xyy(int x, int y,float scale)
{
  float value;
  float square_scale;    /* scale * scale */
  float exp4_scale;      /* scale * scale * scale * scale */
  int i,j;
  int size;
  
  init_gauss(scale);
  size = (int)(GAUSS_CUTOFF*scale);

  square_scale = scale * scale;
  exp4_scale = square_scale * square_scale;

  value = 0;

  /* calcul des valeurs negatives de i */
  for (i=-size;i<0;i++)
    { /* valeurs negatives de j */
    for (j=-size;j<0;j++)
      {
	value = value + (GetPixel(x+i,y+j) //image->el[y+j][x+i]
			 *((-i/exp4_scale)*(1-((j*j)/(square_scale)))
			   * gauxx(-i,-j) ));
      }
    /* valeurs positives de j */
    for (j=0;j<=size;j++)
      {
	value = value + (GetPixel(x+i,y+j) //image->el[y+j][x+i]  
			 *((-i/exp4_scale)*(1-((j*j)/(square_scale)))
			   * gauxx(-i,j) ));
      }
  }
  /* calcul des valeurs positives de i */
  for (i=0;i<=size;i++)
    {
      for (j=-size;j<0;j++)
	{
	  /* valeurs positives de j */
	  value = value + (GetPixel(x+i,y+j) //image->el[y+j][x+i] 
			   *((-i/exp4_scale)*(1-((j*j)/(square_scale)))
			     * gauxx(i,-j) ));
			 }
      /* valeurs positives de j */
      for (j=0;j<=size;j++)
	{
	  value = value + (GetPixel(x+i,y+j) //image->el[y+j][x+i]  
			   *((-i/exp4_scale)*(1-((j*j)/(square_scale)))
			     * gauxx(i,j) ));
	}
    }

  return(value/total);
}

float ImgGray::Third_yyy(int x, int y,float scale)
{
  float value;
  float square_scale;    /* scale * scale */
  float exp4_scale;      /* scale * scale * scale * scale */
  int i,j;
  int size;

  init_gauss(scale);
  size = (int)(GAUSS_CUTOFF*scale);

  square_scale = scale * scale;
  exp4_scale = square_scale * square_scale;

  value = 0;

  /* calcul des valeurs negatives de i */
  for (i=-size;i<0;i++)
    { /* valeurs negatives de j */
    for (j=-size;j<0;j++)
      {
	value = value + (GetPixel(x+i,y+j) //image->el[y+j][x+i]
			 *((-j/exp4_scale)*(3-((j*j)/(square_scale)))
			   * gauxx(-i,-j) ));
      }
    /* valeurs positives de j */
    for (j=0;j<=size;j++)
      {
	value = value + (GetPixel(x+i,y+j) //image->el[y+j][x+i]  
			 *((-j/exp4_scale)*(3-((j*j)/(square_scale)))
			   * gauxx(-i,j) ));
      }
  }
  /* calcul des valeurs positives de i */
  for (i=0;i<=size;i++)
    {
      for (j=-size;j<0;j++)
	{
	  /* valeurs positives de j */
	  value = value + (GetPixel(x+i,y+j) //image->el[y+j][x+i] 
			   *((-j/exp4_scale)*(3-((j*j)/(square_scale)))
			     * gauxx(i,-j) ));
			 }
      /* valeurs positives de j */
      for (j=0;j<=size;j++)
	{
	  value = value + (GetPixel(x+i,y+j) //image->el[y+j][x+i]  
			   *((-j/exp4_scale)*(3-((j*j)/(square_scale)))
			     * gauxx(i,j) ));
	}
    }

  return(value/total);
}

/* Compute the jet for an image */
int ImgGray::Jet_calc(int x, int y, int n, float scale, float *jet)
{
  int icol,irow;
  int size; /* size of the support window of calculating the gaussian */
  
  assert( n >= 0 && n <= 3 );

  /* testing if the point lies not too close to the border of the image */
  size = (int)(GAUSS_CUTOFF*scale);

  irow = Width(); //image->ub1 - image->lb1 +1; /*number of rows */
  icol = Height(); //image->ub2 - image->lb2 + 1; /* number of columns */

  if ( (x<size) || (x>=(icol-size)) || (y<size) || (y>=(irow-size)) ){
    dbg(DBG_INFO, "Error: x = %d does not fit in [%d,%d] or "
        " y = %d does not fit in [%d,%d]\n", x, size, icol-size, y, size, irow-size);
    assert(false);
    return -1;
  }

  else{
    //vect_size = ((n+1)*(n+2))/2;

    //vect = (vector)vector_alloc(vect_size);
  
    /* initialising the vector with the greyvalue */
    jet[0] = zero(x,y,scale);
  
    /* calculate the first derivatives */
    if (n >= 1){
      jet[1] = First_x(x,y,scale);
      jet[2] = First_y(x,y,scale);
    }
    
    /* caculate the second derivatives */
    if (n>=2){
      jet[3] = Second_xx(x,y,scale);
      jet[4] = Second_xy(x,y,scale);
      jet[5] = Second_yy(x,y,scale);
    }
    
    if (n>=3){
      jet[6] = Third_xxx(x,y,scale);
      jet[7] = Third_xxy(x,y,scale);
      jet[8] = Third_xyy(x,y,scale);
      jet[9] = Third_yyy(x,y,scale);
    }
    
    if (n>=4)
      printf("the order of derivation is too high \n");
      
      return 0;
  }
}








