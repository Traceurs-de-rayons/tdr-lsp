.PHONY: all build copy-binary compile-ts build-extension clean help

UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

ifeq ($(UNAME_S),Linux)
	PLATFORM := linux
else ifeq ($(UNAME_S),Darwin)
	PLATFORM := darwin
else ifeq ($(OS),Windows_NT)
	PLATFORM := win32
else
	PLATFORM := unknown
endif

ifeq ($(UNAME_M),x86_64)
	ARCH := x64
else ifeq ($(UNAME_M),aarch64)
	ARCH := arm64
else ifeq ($(UNAME_M),arm64)
	ARCH := arm64
else
	ARCH := unknown
endif

BUILD_DIR := build
BINARY_NAME := tdr-lsp
BINARY_PATH := $(BUILD_DIR)/$(BINARY_NAME)
BIN_INSTALL_DIR := vscode-ext/bin/$(PLATFORM)-$(ARCH)
BINARY_INSTALL_PATH := $(BIN_INSTALL_DIR)/$(BINARY_NAME)

ifeq ($(OS),Windows_NT)
	BINARY_INSTALL_PATH := $(BINARY_INSTALL_PATH).exe
endif

help:
	@echo "Available targets:"
	@echo "  make build          - Build tdr-lsp C++ binary"
	@echo "  make copy-binary    - Copy binary to extension bin directory"
	@echo "  make install-deps   - Install npm dependencies"
	@echo "  make compile-ts     - Install deps and compile TypeScript extension code"
	@echo "  make build-extension - Package the VS Code extension (.vsix)"
	@echo "  make all            - Build binary, copy it, compile TS, and build extension"
	@echo "  make clean          - Remove build artifacts"
	@echo ""
	@echo "Detected: platform=$(PLATFORM), arch=$(ARCH)"

build:
	@echo "Building tdr-lsp C++ binary..."
	cmake -S . -B $(BUILD_DIR)
	cmake --build $(BUILD_DIR)

copy-binary: build
	@echo "Copying binary to $(BIN_INSTALL_DIR)..."
	@mkdir -p $(BIN_INSTALL_DIR)
	@cp $(BINARY_PATH) $(BINARY_INSTALL_PATH)
	@chmod +x $(BINARY_INSTALL_PATH)
	@echo "Binary copied to: $(BINARY_INSTALL_PATH)"

install-deps:
	@echo "Installing npm dependencies for extension..."
	@cd vscode-ext && npm install
	@echo "Dependencies installed successfully"

compile-ts: install-deps
	@echo "Compiling TypeScript extension..."
	@cd vscode-ext && npm run compile
	@echo "TypeScript compiled successfully"

build-extension: compile-ts
	@echo "Building VS Code extension (.vsix)..."
	@cd vscode-ext && npx @vscode/vsce package
	@echo "Extension built successfully"

all: copy-binary compile-ts build-extension
	@echo ""
	@echo "Full build pipeline completed!"
	@echo " - C++ binary built"
	@echo " - Binary copied to extension bin/"
	@echo " - TypeScript compiled"
	@echo " - Extension packaged (.vsix)"

clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)
	@rm -rf vscode-ext/out
	@rm -f vscode-ext/tdr-scene-language-*.vsix
	@echo "Clean complete"

info:
	@echo "Build Configuration:"
	@echo "  Platform: $(PLATFORM)"
	@echo "  Architecture: $(ARCH)"
	@echo "  Binary path: $(BINARY_PATH)"
	@echo "  Install path: $(BINARY_INSTALL_PATH)"
