#include<stdio.h>
#include<stdlib.h>

#define BUF_SIZE 65536

int main(int argc, char *argv[])
{
  FILE *src, *dst;
  size_t in, out;
  char buf_in[BUF_SIZE];
  char buf_out[BUF_SIZE];
  int bufsize;

  if (argc != 3) {
    fprintf(stderr, "Usage: %s <buffer size> <src>\n", argv[0]);
    exit(1);
  }

  bufsize = atoi(argv[1]);
  if (bufsize > BUF_SIZE) {
    fprintf(stderr,"Error: max. buffer size is %d\n", BUF_SIZE);
    exit(1);
  }

  src = fopen(argv[2], "r");
  if (NULL == src) {
    fprintf(stderr,"Error: Opening %s file\n",argv[2]);
    exit(1);
  }

  while (1) {
    in = fread(buf_in, 1, bufsize, src);
    if (0 == in) break;
    for (int i=0; i<bufsize; i++) {
      buf_out[i*8+0] = (buf_in[i] & 1)?255:0;
      buf_out[i*8+1] = (buf_in[i] & 2)?255:0;
      buf_out[i*8+2] = (buf_in[i] & 4)?255:0;
      buf_out[i*8+3] = (buf_in[i] & 8)?255:0;
      buf_out[i*8+4] = (buf_in[i] & 16)?255:0;
      buf_out[i*8+5] = (buf_in[i] & 32)?255:0;
      buf_out[i*8+6] = (buf_in[i] & 64)?255:0;
      buf_out[i*8+7] = (buf_in[i] & 128)?255:0;
    }
    out = fwrite(buf_out, 1, in*8, stdout);
    if (0 == out) break;
  }
  fclose(src);
  exit(0);
}
