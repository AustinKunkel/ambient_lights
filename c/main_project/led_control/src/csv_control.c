#include "csv_control.h"

int write_data(char *filename, char *header, char *data) {
  FILE *fp = fopen(filename, "w");

  if(fp == NULL) {
    perror("Unable to open file\n");
    return 1;
  }

  // write header
  fprintf(fp, header);

  // write data
  fprintf(fp, data);

  fclose(fp);
  return 0;
}

int append_data(char *filename, char *data) {
  FILE *fp = fopen(filename, "a");

  if(fp == NULL) {
    perror("Unable to open file\n");
    return 1;
  }

  // append
  fprintf(fp, data);

  fclose(fp);
  return 0;
}

/**
 * Reads single line CSV data
 */
int read_one_line(char *filename, char *output_line, size_t size) {
  FILE *fp = fopen(filename, "r");

  if(fp == NULL) {
    perror("Unable to open file!\n");
    return 1;
  }

  char temp[1024];

  // skip header
  if(fgets(temp, sizeof(temp), fp) == NULL) {
    perror("Failed to skip header line");
    fclose(fp);
    return 1;
  }

  if(fgets(temp, sizeof(temp), fp) == NULL) {
    perror("Failed to read data line\n");
    fclose(fp);
    return 1;
  }

  // copy line to output buffer
  strncpy(output_line, temp, size - 1);
  output_line[size - 1] = '\0';

  fclose(fp);
  return 0;
}