V=1
SOURCE_DIR=src
BUILD_DIR=build
DEVELOPMENT=0
# ADDITIONAL_C_FLAGS = -DNDEBUG
include $(N64_INST)/include/n64.mk

# TODO: use a wildcard here
OBJS = 	$(BUILD_DIR)/summer.o \
		$(BUILD_DIR)/player.o \
		$(BUILD_DIR)/terrain.o \
		$(BUILD_DIR)/tutorial.o \
		$(BUILD_DIR)/dialogue.o \
		$(BUILD_DIR)/menu.o \
		$(BUILD_DIR)/intro.o \
		$(BUILD_DIR)/story.o

assets = $(wildcard assets/*.glb)
assets_ttf = $(wildcard assets/*.ttf)
assets_png = $(wildcard assets/*.png)
assets_wav = $(wildcard assets/*.wav)

assets_conv = 	$(addprefix filesystem/,$(notdir $(assets_ttf:%.ttf=%.font64))) \
				$(addprefix filesystem/,$(notdir $(assets:%.glb=%.model64))) \
				$(addprefix filesystem/,$(notdir $(assets_png:%.png=%.sprite))) \
				filesystem/map.terrain

ifneq ($(DEVELOPMENT), 1)
	assets_conv += $(addprefix filesystem/,$(notdir $(assets_wav:%.wav=%.wav64)))
else
	ADDITIONAL_C_FLAGS += -DDEVELOPMENT
endif

all: summer.z64
.PHONY: all

filesystem/%.wav64: assets/%.wav
	@mkdir -p $(dir $@)
	@echo "    [AUDIO] $@"
	@$(N64_AUDIOCONV) -o filesystem $<

MKSPRITE_FLAGS=--format RGBA32
filesystem/%.sprite: assets/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	@$(N64_MKSPRITE) $(MKSPRITE_FLAGS) -o filesystem "$<"

filesystem/map.terrain: assets/terrain/map.glb
	$(MAKE) -C tools
	@mkdir -p $(dir $@)
	@echo "    [TERRAIN] $@"
	./tools/mkterrain -v -o filesystem "$<"

filesystem/%.model64: assets/%.glb
	@mkdir -p $(dir $@)
	@echo "    [MODEL] $@"
	@$(N64_MKMODEL) -o filesystem "$<"

filesystem/%.font64: assets/%.ttf
	@mkdir -p $(dir $@)
	@echo "    [FONT] $@"
	@$(N64_MKFONT) $(MKFONT_FLAGS) -o filesystem "$<"

filesystem/font_small.font64: MKFONT_FLAGS+=--size 12
filesystem/font_big.font64: MKFONT_FLAGS+=--size 16

$(BUILD_DIR)/summer.dfs: $(assets_conv)
$(BUILD_DIR)/summer.elf: $(OBJS)

summer.z64: N64_ROM_TITLE="Summer"
summer.z64: $(BUILD_DIR)/summer.dfs
summer.z64: CFLAGS+= -Wno-error -Iinclude -Ibox2d/include -Ibox2d/src $(ADDITIONAL_C_FLAGS)

clean:
	rm -f $(BUILD_DIR)/* *.z64 filesystem/*
.PHONY: clean

-include $(wildcard $(BUILD_DIR)/*.d)