CC = cc
LDFLAGS = -lpthread -Llib -lzlog -lm
CFLAGS = -I$(HEADER_DIR) -g

SOURCE_DIR = PZFilesExpress_source
HEADER_DIR = PZFilesExpress_header
OBJECT_DIR = obj
# BIN_DIR = bin
SOURCES = $(wildcard $(SOURCE_DIR)/*.c)
OBJECTS = $(SOURCES:$(SOURCE_DIR)/%.c=$(OBJECT_DIR)/%.o)
TARGET = PZFilesExpress


# 默认目标  
all: $(BIN_DIR) $(TARGET)
  
# 链接目标  
$(TARGET): $(OBJECTS)  
	$(CC) -o $(TARGET) $(OBJECTS) $(LDFLAGS)
# 编译规则  
$(OBJECT_DIR)/%.o: $(SOURCE_DIR)/%.c | $(OBJECT_DIR)
	$(CC) -c $< -o $@ $(CFLAGS)
# 创建 obj bin目录  
# $(OBJECT_DIR) $(BIN_DIR):
$(OBJECT_DIR):
	mkdir -p $@
# # 清理目标  
clean:
	rm -rf $(OBJECT_DIR) *.txt *.fdl *.dat

.PHONY: all clean $(OBJECT_DIR)