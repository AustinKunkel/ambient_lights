CC = gcc

# Directories
INC_DIR = /usr/local/include
LIB_DIR = /use/local/lib

# Compiler Flags
CFLAGS = -I$(INC_DIR) -I./cJSON -I./libwebsockets/include -I./libwebsockets/build/include
LDFLAGS = -I$(LIB_DIR) ./libwebsockets/build/lib/libwebsockets.a -lcjson -lws2811 -pthread -lrt -lgnutls -lm -lssl -lcrypto

# Source Files
SRV_TST_SRC = led_control/test/server_test.c \
							led_control/test/led_test.c \
							led_control/test/led_capture_test.c \
							led_control/test/screen_capture_functions.c

CSV_CTRL_TEST_SRC = led_control/test/csv_control_test.c

MAIN_SRC = led_control/src/server.c \
					 led_control/src/led_functions.c \
					 led_control/src/main.c \
					 led_control/src/screen_capture.c \
					 led_control/src/screen_capture_functions.c \
					 led_control/src/csv_control.c \
					 cJSON/cJSON.c

CAPTURE_SRC = led_control/test/capture_test.c \
							led_control/test/screen_capture_functions.c

# Object Files (replace .c with .o)
SRV_TEST_OBJ = $(SRV_TST_SRC:.c=.o)
CSV_CTRL_TEST_OBJ = $(CSV_CTRL_TEST_SRC:.c=.o)
CAPTURE_OBJ = $(CAPTURE_SRC:.c=.o)
MAIN_OBJ = $(MAIN_SRC:.c=.o)

# Output Executable
SRV_TEST_TARGET = server_test
CSV_CTRL_TEST_TARGET = csv_control_test
CAPTURE_TARGET = test_screen_capture
MAIN_TARGET = server

# Build the executable
$(SRV_TEST_TARGET): $(SRV_TEST_OBJ)

$(CSV_CTRL_TEST_TARGET): $(CSV_CTRL_TEST_OBJ)
	$(CC) $(CSV_CTRL_TEST_OBJ) -o $(CSV_CTRL_TEST_TARGET)

$(CAPTURE_TARGET): $(CAPTURE_OBJ)
	$(CC) $(CAPTURE_OBJ) -o $(CAPTURE_TARGET) $(LDFLAGS)

$(MAIN_TARGET): $(MAIN_OBJ)

all: %(SRV_TEST_TARGET)

# Build server_test
$(SRV_TEST_TARGET): $(SRV_TEST_OBJ)
	$(CC) $(SRV_TEST_OBJ) -o $(SRV_TEST_TARGET) $(LDFLAGS)

# Build server
$(MAIN_TARGET): $(MAIN_OBJ)
	$(CC) $(MAIN_OBJ) -o $(MAIN_TARGET) $(LDFLAGS)

# Compile source files into objects
%.o: %.c 
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(SRV_TEST_OBJ) $(SRV_TEST_TARGET)
	rm -f $(CSV_CTRL_TEST_OBJ) $(CSV_CTRL_TEST_TARGET)
	rm -f $(CAPTURE_OBJ) $(CAPTURE_TARGET)
	rm -f $(MAIN_OBJ) $(MAIN_TARGET)