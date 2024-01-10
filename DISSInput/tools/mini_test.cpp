#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"


int main(int argc, char **argv) {
  if (argc < 2)
    return 0;
	

  FILE *fp;
  char buf[255];
  memset(buf, 0x00, 255);

  char answer[] = "hello";
  size_t ret = 5;

  fp = fopen(argv[1], "rb");

  setvbuf(fp, NULL, _IONBF, 0);
  setvbuf(stdout, NULL, _IONBF, 0);

  if (!fp) {
    printf("st err\n");
    return 0;
  }

  int len = 5;
  int i = 0, j = 1;
  // dfsan_read_label(&(len), sizeof *buf);
  while(fgets(buf, 100, fp)){
    i ++;
    if (!strncmp(buf, answer, 5)){
      i = 0;
      j ++;
      fprintf(stdout, "right!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    } else {
      fprintf(stdout, "wrong!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    }
    memset(buf, 0x00, 255);
  }
  fclose(fp);
  return 0;
}