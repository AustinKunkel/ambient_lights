CC = gcc

# Directories
INC_DIR = /usr/local/include
LIB_DIR = /usr/local/lib

# Submodule paths
WS281X_DIR = ../rpi_ws281x
WS281X_BUILD = $(WS281X_DIR)/build
WS281X_LIB = $(WS281X_BUILD)/libws2811.a

LIBWEBSOCKETS_DIR = ../libwebsockets
LIBWEBSOCKETS_BUILD = $(LIBWEBSOCKETS_DIR)/build

# Compiler Flags
CFLAGS = -I$(INC_DIR) \
         -I./cJSON \
         -I$(LIBWEBSOCKETS_DIR)/include \
         -I$(LIBWEBSOCKETS_BUILD)/include \
         -I$(WS281X_DIR)

LDFLAGS = -L$(LIB_DIR) \
          -L$(WS281X_BUILD) \
          -L$(LIBWEBSOCKETS_BUILD)/lib \
          -lwebsockets -lws2811 \
          -pthread -lrt -lgnutls -lmicrohttpd -lm -lssl -lcrypto

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

# Output Executables
SRV_TEST_TARGET = server_test
CSV_CTRL_TEST_TARGET = csv_control_test
CAPTURE_TARGET = test_screen_capture
MAIN_TARGET = server

# Default build
all: $(SRV_TEST_TARGET) $(MAIN_TARGET)

# Build targets
$(SRV_TEST_TARGET): $(SRV_TEST_OBJ)
	$(CC) $(SRV_TEST_OBJ) -o $@ $(LDFLAGS)

$(CSV_CTRL_TEST_TARGET): $(CSV_CTRL_TEST_OBJ)
	$(CC) $(CSV_CTRL_TEST_OBJ) -o $@ $(LDFLAGS)

$(CAPTURE_TARGET): $(CAPTURE_OBJ)
	$(CC) $(CAPTURE_OBJ) -o $@ $(LDFLAGS)

$(MAIN_TARGET): $(MAIN_OBJ)
	$(CC) $(MAIN_OBJ) -o $@ $(LDFLAGS)

# Compile source files into objects
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(SRV_TEST_OBJ) $(SRV_TEST_TARGET)
	rm -f $(CSV_CTRL_TEST_OBJ) $(CSV_CTRL_TEST_TARGET)
	rm -f $(CAPTURE_OBJ) $(CAPTURE_TARGET)
	rm -f $(MAIN_OBJ) $(MAIN_TARGET)
	$(MAKE) -C $(WS281X_BUILD) clean || true
	$(MAKE) -C $(LIBWEBSOCKETS_BUILD) clean || true
