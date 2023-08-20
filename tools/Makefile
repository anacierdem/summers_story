INSTALLDIR = $(N64_INST)
# Why can't use one level up?
OUT_DIR = .
CFLAGS += -std=gnu99 -O2 -Wall -Werror -Wno-unused-result -I../../include
all: mkterrain

$(OUT_DIR)/mkterrain: mkterrain.c
	$(CC) $(CFLAGS) mkterrain.c -o $(OUT_DIR)/mkterrain -lm

.PHONY: clean

clean:
	rm -rf $(OUT_DIR)/mkterrain
