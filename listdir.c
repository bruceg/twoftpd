#include <dirent.h>
#include <stdlib.h>
#include <sys/types.h>
#include "twoftpd.h"

static char* buffer = 0;
static size_t buflen = 0;
static size_t bufsize = 0;
static const char** ptrs = 0;

static void append(const char* name)
{
  char* newbuf;
  size_t len;

  len = strlen(name) + 1;
  if (buflen + len > bufsize) {
    if (!bufsize) bufsize = 16;
    while (buflen + len > bufsize)
      bufsize *= 2;
    newbuf = malloc(bufsize);
    memcpy(newbuf, buffer, buflen);
    free(buffer);
    buffer = newbuf;
  }
  memcpy(buffer+buflen, name, len);
  buflen += len;
}

static int strpcmp(const void* a, const void* b)
{
  char** aa = (char**)a;
  char** bb = (char**)b;
  return strcmp(*aa, *bb);
}

const char** listdir(const char* path)
{
  unsigned count;
  unsigned i;
  DIR* dir;
  struct dirent* entry;
  const char* ptr;
  
  buflen = 0;
  count = 0;
  if ((dir = opendir(path)) == 0) return 0;
  while ((entry = readdir(dir)) != 0) {
    append(entry->d_name);
    ++count;
  }
  closedir(dir);

  if (ptrs) free(ptrs);
  ptrs = calloc(count+1, sizeof(char*));
  for (ptr = buffer, i = 0; i < count; i++) {
    ptrs[i] = ptr;
    ptr += strlen(ptr) + 1;
  }
  qsort(ptrs, count, sizeof(char*), strpcmp);
  ptrs[count] = 0;
  return ptrs;
}
