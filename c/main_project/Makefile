CC = gcc

# Directories
INC_DIR = /usr/local/include
LIB_DIR = /use/local/lib 

# Compiler Flags
CFLAGS = -I$(INC_DIR)
LDFLAGS = -I$(LIB_DIR) -lmicrohttpd -lws2811 -pthread -lrt -lgnutls -lm

# Source Files
SRV_TST_SRC = led_control/test/server_test.c \
							led_control/test/led_test.c \
							led_control/test/led_capture_test.c

CAPTURE_SRC = led_control/test/capture_test.c \
							led_control/test/screen_capture_functions.c

# Object Files (replace .c with .o)
SRV_TEST_OBJ = $(SRV_TST_SRC:.c=.o)
CAPTURE_OBJ = $(CAPTURE_SRC:.c=.o)

# Output Executable
SRV_TEST_TARGET = server_test
CAPTURE_TARGET = test_screen_capture

# Build the executable
$(SRV_TEST_TARGET): $(SRV_TEST_OBJ)

$(CAPTURE_TARGET): $(CAPTURE_OBJ)
	$(CC) $(CAPTURE_OBJ) -o $(CAPTURE_TARGET) $(LDFLAGS)

all: %(SRV_TEST_TARGET)

# Build server_test
$(SRV_TEST_TARGET): $(SRV_TEST_OBJ)
	$(CC) $(SRV_TEST_OBJ) -o $(SRV_TEST_TARGET) $(LDFLAGS)

# Compile source files into objects
%.o: %.c 
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(SRV_TEST_OBJ) $(SRV_TEST_TARGET)