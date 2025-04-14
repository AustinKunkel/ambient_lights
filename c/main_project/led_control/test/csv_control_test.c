#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
  int res_x;
  int res_y;
  int boolean;
  long test;
} Config;

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

char* next_token(char **line) {
  char *token = strtok(*line, ",");
  *line = NULL;  // After first call, use NULL to get next token
  return token;
}
int read_data(char *filename, Config* data) {
  FILE *fp = fopen(filename, "r");
  if(fp == NULL) {
    perror("Unable to open file\n");
    return 1;
  }

  char line[1024];

  // Read a line from the file
  if (fgets(line, sizeof(line), fp) == NULL) {
    perror("Failed to read from file\n");
    fclose(fp);
    return 1;
  }

  char *line_ptr = line;

  // Parsing values from the line
  data->res_x = atoi(next_token(&line_ptr));
  data->res_y = atoi(next_token(&line_ptr));
  data->boolean = atoi(next_token(&line_ptr));

  // Parsing the hex color code
  char *hex = next_token(&line_ptr);
  if (hex) {
    // Skip the '#' character if it's present
    char *hex_ptr = (hex[0] == '#') ? hex + 1 : hex;
    data->test = strtol(hex_ptr, NULL, 16);
  }

  fclose(fp);
  return 0;
}

int parse_config_data_to_string(Config config, char *str) {
  // Use sprintf to format and store the result into str
  // Returns the number of characters written
  return sprintf(str, "%d,%d,%d,#%06lX",
    config.res_x,
    config.res_y,
    config.boolean,
    config.test);
}

int main() {
  char *filename = "led_control/data/test_data.csv";
  char *header = "Res x, Res y, boolean, test\n";
  char *data = "640, 480, 1, #FF00AA";

  printf("Writing data to %s: %s\n", filename, data);

  if(write_data(filename, header, data)) {
    perror("Issue with write data\n");
    return 1;
  }

  Config config;
  if(read_data(filename, &config)) {
    perror("Could not read the data\n");
    return 1;
  }
  printf("Current config values: res_x: %d, res_y: %d, boolean: %d, color: #%06lX\n", config.res_x, config.res_y, config.boolean, config.test);

  config.res_x = 1920;
  config.res_y = 1080;
  config.boolean = 0;
  config.test = 0x00FFAA;
  printf("Setting config values: res_x: %d, res_y: %d, boolean: %d, color: #%06lX\n", config.res_x, config.res_y, config.boolean, config.test);

  char buffer[100];
  int len = parse_config_data_to_string(config, buffer);
  printf("CSV output of current config: %s\n", buffer);
  
  printf("Writing data to %s: %s\n", filename, buffer);
  if(write_data(filename, header, buffer)) {
    perror("Failed to write data\n");
    return 1;
  }

  printf("Setting all config to 0 and reading from file again...\n");
  config.boolean = 0;
  config.res_x = 0;
  config.res_y = 0;
  config.test = 0;

  if(read_data(filename, &config)) {
    perror("Could not read the data\n");
    return 1;
  }

  printf("Current config values: res_x: %d, res_y: %d, boolean: %d, color: #%06lX\n", config.res_x, config.res_y, config.boolean, config.test);

  return 0;
}