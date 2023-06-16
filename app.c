#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define IOC_MAGIC 'k'

#define SUM_LENGTH _IOWR(IOC_MAGIC, 1, char *)
#define SUM_CONTENT _IOWR(IOC_MAGIC, 2, char *)

int main() {
  FILE *fp;
  int off;
  long res;
  char content[100];

  printf("Opening Driver\n");
  fp = fopen("/dev/solution_node", "rw+");
  if (fp < 0) {
    printf("Cannot open device file...\n");
    return 0;
  }
  printf("Device opened\n");

  // printf("Enter the IOCTL to send (0 length, 1 content)\n");
  // scanf("%d", &a);

  printf("Reading\n");
  fread(content, sizeof(char), 1, fp);
  printf("Read %s.\n", content);

  printf("Writing\n");
  char *word = "test";
  fwrite(word, sizeof(char), strlen(word), fp);
  printf("Wrote %s.\n", word);

  printf("Reading\n");
  fread(content, sizeof(char), 1, fp);
  printf("Read %s.\n", content);

  // printf("Seeking\n");
  // fseek(fp, 2, SEEK_SET);
  // printf("Seeked\n");

  printf("Closing Driver\n");
  fclose(fp);
  printf("Device closed\n");
}
