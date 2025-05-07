/*
 *
 * fft.c written by S. ISHIZAKA 1999/07
 *
 */

#include<stdio.h>
#include<stdlib.h>
#include<math.h>

#define PI 3.141592653589793

#define MAXP 24
#define MAXDATA (1 << MAXP)

double fr[MAXDATA], fi[MAXDATA], gr[MAXDATA], gi[MAXDATA];
double workr[MAXDATA / 2], worki[MAXDATA / 2];

static void fft(int p,int num);
static int read_data(const char *fname1, int *p, int *n, double *min, double *max);
static int save_data(const char *fname2, int p, int n, double minx, double maxx);

int
main(int argc,char **argv)
{
  const char *fname1, *fname2;
  int n, p, num;
  double minx, maxx;

  if (argc < 3) {
    fprintf(stderr, "usage: fft input output");
    exit(1);
  }
  fname1 = argv[1];
  fname2 = argv[2];

  num = read_data(fname1, &p, &n, &minx, &maxx);
  if (num < 0) {
    fprintf(stderr, "error: open (%s)", fname1);
    exit(1);
  } else if (num < 2) {
    fprintf(stderr, "error: too small number of data");
    exit(1);
  }

  if (save_data(fname2, p, n, minx, maxx)) {
    fprintf(stderr, "error: open (%s)", fname2);
    exit(1);
  }

  return 0;
}

static int
save_data(const char *fname2, int p, int n, double minx, double maxx)
{
  FILE *fp2;
  double dx;
  int i;

  fp2 = fopen(fname2, "wt");
  if (fp2 == NULL) {
    return 1;
  }

  fft(p, n);
  dx = (n - 1) / (maxx - minx) / n;
  for (i = n / 2; i < n; i++) {
    fprintf(fp2, "%+E %+.15E %+.15E\n", dx * (i - n), gr[i], gi[i]);
  }
  for (i = 0; i < n / 2; i++) {
    fprintf(fp2, "%+E %+.15E %+.15E\n", dx * i, gr[i], gi[i]);
  }
  fclose(fp2);

  return 0;
}

static int
read_data(const char *fname1, int *fft_p, int *fft_n, double *min, double *max)
{
  FILE *fp1;
  int num, n, p;
  double x, y, minx, maxx;
  char line[1024];

  fp1 = fopen(fname1, "rt");
  if (fp1 == NULL) {
    return -1;
  }

  num = 0;
  n = 1;
  p = 0;
  maxx = 0;
  minx = 0;

  while (fgets(line, sizeof(line), fp1)) {
    if (sscanf(line, "%lf%*[ \t,]%lf", &x, &y) != 2) {
      continue;
    }
    if (num == 0) {
      minx = x;
    }
    fr[num] = y;
    fi[num] = 0;
    num++;
    if (num == (n*2)) {
      n *= 2;
      p++;
      if (p == MAXP) {
	break;
      }
      maxx=x;
    }
  }

  fclose(fp1);

  *fft_n = n;
  *fft_p = p;
  *min = minx;
  *max = maxx;

  return num;
}

static void
fft(int p,int n)
{
  int i,j,k,l,l1,m,n2;
  double ar,ai,br,bi;

  n2=n/2;
  for (i=0;i<n2;i++) {
    workr[i]=cos(2*PI/n*i);
    worki[i]=sin(2*PI/n*i);
  }
  for (j=0;j<n2;j++) {
    k=j*2;
    gr[k]=fr[j]+fr[j+n2];
    gi[k]=fi[j]+fi[j+n2];
    gr[k+1]=fr[j]-fr[j+n2];
    gi[k+1]=fi[j]-fi[j+n2];
  }
  l1=1;
  for (l=1;l<p;l++) {
    int l2;
    l1*=2;
    l2=n2/l1;
    for (i=0;i<n;i++) {
      fr[i]=gr[i];
      fi[i]=gi[i];
    }
    for (k=0;k<l1;k++) {
      for (j=0;j<l2;j++) {
        m=j*l1;
        ar=fr[m+k];
        ai=fi[m+k];
        br=fr[m+k+n2]*workr[k*l2]-fi[m+k+n2]*worki[k*l2];
        bi=fr[m+k+n2]*worki[k*l2]+fi[m+k+n2]*workr[k*l2];
        gr[m+m+k]=ar+br;
        gi[m+m+k]=ai+bi;
        gr[m+m+k+l1]=ar-br;
        gi[m+m+k+l1]=ai-bi;
      }
    }
  }
  for (i=0;i<n;i++) {
    gr[i]/=n;
    gi[i]/=n;
  }
}
