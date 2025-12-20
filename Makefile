# ==========================
# Toolchain
# ==========================
CC      := gcc
CFLAGS  := -Wall -Wextra -O2 -Iinclude
LDFLAGS :=

# ==========================
# Directories
# ==========================
SRC_DIR     := src
CLIENT_DIR  := client
TEST_DIR    := tests
BUILD_DIR   := build

# ==========================
# Install paths
# ==========================
PREFIX ?= /usr
BINDIR := $(PREFIX)/bin

# ==========================
# Binaries
# ==========================
SERVER_BIN := ramora-server
CLIENT_BIN := ramora-client
TEST_BIN   := ramora-test

# ==========================
# Sources
# ==========================
SERVER_SRCS := \
	$(SRC_DIR)/server.c \
	$(SRC_DIR)/buf.c \
	$(SRC_DIR)/cmd_proc.c \
	$(SRC_DIR)/config.c \
	$(SRC_DIR)/conn.c \
	$(SRC_DIR)/g_data.c \
	$(SRC_DIR)/heap.c \
	$(SRC_DIR)/hmap.c \
	$(SRC_DIR)/logging_helper.c \
	$(SRC_DIR)/oper.c \
	$(SRC_DIR)/response.c \
	$(SRC_DIR)/timer.c \
	$(SRC_DIR)/vtr.c

CLIENT_SRCS := \
	$(CLIENT_DIR)/client.c

TEST_SRCS := \
	$(TEST_DIR)/test.c

# ==========================
# Objects
# ==========================
SERVER_OBJS := $(SERVER_SRCS:%.c=$(BUILD_DIR)/%.o)
CLIENT_OBJS := $(CLIENT_SRCS:%.c=$(BUILD_DIR)/%.o)
TEST_OBJS   := $(TEST_SRCS:%.c=$(BUILD_DIR)/%.o)

# ==========================
# Default target
# ==========================
.PHONY: all
all: $(SERVER_BIN) $(CLIENT_BIN) $(TEST_BIN)

# ==========================
# Compile rules
# ==========================
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# ==========================
# Link rules
# ==========================
$(SERVER_BIN): $(SERVER_OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

$(CLIENT_BIN): $(CLIENT_OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

$(TEST_BIN): $(TEST_OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

# ==========================
# Cleanup
# ==========================
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	rm -f $(SERVER_BIN) $(CLIENT_BIN) $(TEST_BIN)
