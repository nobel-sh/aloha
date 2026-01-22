BUILD_DIR := build
CMAKE := cmake

.PHONY: all build clean rebuild test install

all: build

build:
	@mkdir -p $(BUILD_DIR)
	@$(CMAKE) -S . -B $(BUILD_DIR)
	@$(CMAKE) --build $(BUILD_DIR)

install: build
	@$(CMAKE) --install $(BUILD_DIR)

clean:
	@rm -rf $(BUILD_DIR)

rebuild: clean build

test: build
	@bash scripts/test_programs.sh

test_rebuild: build test
