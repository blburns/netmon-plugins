# Makefile for Network Utils - Modern Nagios Plugins
# Copyright 2024

# Variables
PROJECT_NAME = netmon-plugins
VERSION = 0.1.0
BUILD_DIR = build
DIST_DIR = dist
PACKAGE_DIR = packaging

# Build options (can be overridden: make build ENABLE_SSL=ON)
ENABLE_SSL ?= ON
ENABLE_SNMP ?= ON
ENABLE_MYSQL ?= ON
ENABLE_PGSQL ?= ON
ENABLE_LDAP ?= ON
ENABLE_TESTS ?= ON
ENABLE_PACKAGING ?= ON

# CMake options string
CMAKE_OPTIONS = -DENABLE_SSL=$(ENABLE_SSL) \
                -DENABLE_SNMP=$(ENABLE_SNMP) \
                -DENABLE_MYSQL=$(ENABLE_MYSQL) \
                -DENABLE_PGSQL=$(ENABLE_PGSQL) \
                -DENABLE_LDAP=$(ENABLE_LDAP) \
                -DENABLE_TESTS=$(ENABLE_TESTS) \
                -DENABLE_PACKAGING=$(ENABLE_PACKAGING)

# Platform detection
UNAME_S := $(shell uname -s)
ifeq ($(OS),Windows_NT)
    PLATFORM = windows
    CXX = cl
    CXXFLAGS = /std:c++17 /W3 /O2 /DNDEBUG /EHsc
    PARALLEL_JOBS = $(shell echo %NUMBER_OF_PROCESSORS%)
    INSTALL_PREFIX = C:/Program Files/$(PROJECT_NAME)
    PLUGINS_DIR = $(INSTALL_PREFIX)/libexec/monitoring-plugins
    RM = del /Q
    RMDIR = rmdir /S /Q
    MKDIR = mkdir
    CP = copy
    ifeq ($(shell echo $$SHELL),/usr/bin/bash)
        RM = rm -rf
        RMDIR = rm -rf
        MKDIR = mkdir -p
        CP = cp -r
        INSTALL_PREFIX = /c/Program\ Files/$(PROJECT_NAME)
        PLUGINS_DIR = /c/Program\ Files/$(PROJECT_NAME)/libexec/monitoring-plugins
    endif
else ifeq ($(UNAME_S),Darwin)
    PLATFORM = macos
    CXX = clang++
    CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -O2 -DNDEBUG
    PARALLEL_JOBS = $(shell sysctl -n hw.ncpu)
    INSTALL_PREFIX = /usr/local
    PLUGINS_DIR = $(INSTALL_PREFIX)/libexec/monitoring-plugins
    RM = rm -rf
    RMDIR = rm -rf
    MKDIR = mkdir -p
    CP = cp -r
else
    PLATFORM = linux
    CXX = g++
    CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -O2 -DNDEBUG
    PARALLEL_JOBS = $(shell nproc)
    INSTALL_PREFIX = /usr/local
    PLUGINS_DIR = $(INSTALL_PREFIX)/libexec/monitoring-plugins
    RM = rm -rf
    RMDIR = rm -rf
    MKDIR = mkdir -p
    CP = cp -r
endif

# Directories
SRC_DIR = src
INCLUDE_DIR = include
PLUGINS_DIR_SRC = plugins
SCRIPTS_DIR = scripts
DEPLOYMENT_DIR = deployment

# Default target
all: build

# Create build directory
$(BUILD_DIR)-dir:
	$(MKDIR) $(BUILD_DIR)

# Build using CMake
build: $(BUILD_DIR)-dir
ifeq ($(PLATFORM),windows)
	cd $(BUILD_DIR) && cmake .. -G "Visual Studio 16 2019" -A x64 $(CMAKE_OPTIONS) && cmake --build . --config Release
else
	cd $(BUILD_DIR) && cmake .. $(CMAKE_OPTIONS) && make -j$(PARALLEL_JOBS)
endif

# Clean build
clean:
	$(RM) $(BUILD_DIR)
	$(RM) $(DIST_DIR)

# Install
install: build
ifeq ($(PLATFORM),windows)
	cd $(BUILD_DIR) && cmake --install . --prefix "$(INSTALL_PREFIX)"
else
	cd $(BUILD_DIR) && sudo make install
endif

# Uninstall
uninstall:
ifeq ($(PLATFORM),windows)
	$(RMDIR) "$(INSTALL_PREFIX)"
else
	sudo rm -rf $(PLUGINS_DIR)/check_*
	sudo rm -rf $(INSTALL_PREFIX)/include/monitoring-utils
	sudo rm -rf $(INSTALL_PREFIX)/etc/netmon-plugins
endif

# Test
test: build
ifeq ($(PLATFORM),windows)
	cd $(BUILD_DIR) && ctest --output-on-failure
else
	cd $(BUILD_DIR) && make test
endif

# Package
package: build
ifeq ($(PLATFORM),macos)
	@echo "Building macOS packages..."
	@mkdir -p $(DIST_DIR)
	cd $(BUILD_DIR) && cpack -G DragNDrop
	cd $(BUILD_DIR) && cpack -G productbuild
	mv $(BUILD_DIR)/$(PROJECT_NAME)-$(VERSION)-*.dmg $(DIST_DIR)/ 2>/dev/null || true
	mv $(BUILD_DIR)/$(PROJECT_NAME)-$(VERSION)-*.pkg $(DIST_DIR)/ 2>/dev/null || true
else ifeq ($(PLATFORM),linux)
	@echo "Building Linux packages..."
	@mkdir -p $(DIST_DIR)
	cd $(BUILD_DIR) && cpack -G RPM
	cd $(BUILD_DIR) && cpack -G DEB
	mv $(BUILD_DIR)/$(PROJECT_NAME)-$(VERSION)-*.rpm $(DIST_DIR)/ 2>/dev/null || true
	mv $(BUILD_DIR)/$(PROJECT_NAME)-$(VERSION)-*.deb $(DIST_DIR)/ 2>/dev/null || true
else ifeq ($(PLATFORM),windows)
	@echo "Building Windows packages..."
	@$(MKDIR) $(DIST_DIR)
	cd $(BUILD_DIR) && cpack -G WIX
	cd $(BUILD_DIR) && cpack -G ZIP
	$(CP) $(BUILD_DIR)/$(PROJECT_NAME)-$(VERSION)-*.msi $(DIST_DIR)/ 2>/dev/null || true
	$(CP) $(BUILD_DIR)/$(PROJECT_NAME)-$(VERSION)-*.zip $(DIST_DIR)/ 2>/dev/null || true
endif

# Development targets
dev-build: $(BUILD_DIR)-dir
ifeq ($(PLATFORM),windows)
	cd $(BUILD_DIR) && cmake .. -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=Debug $(CMAKE_OPTIONS) && cmake --build . --config Debug
else
	cd $(BUILD_DIR) && cmake .. -DCMAKE_BUILD_TYPE=Debug $(CMAKE_OPTIONS) && make -j$(PARALLEL_JOBS)
endif

dev-test: dev-build
ifeq ($(PLATFORM),windows)
	cd $(BUILD_DIR) && ctest --output-on-failure
else
	cd $(BUILD_DIR) && make test
endif

# Dependencies
deps:
ifeq ($(PLATFORM),macos)
	@echo "Installing dependencies on macOS..."
	brew install openssl cmake
else ifeq ($(PLATFORM),linux)
	@echo "Installing dependencies on Linux..."
	sudo apt-get update
	sudo apt-get install -y build-essential cmake libssl-dev
	@echo "Optional dependencies:"
	@echo "  libmysqlclient-dev (for MySQL plugins)"
	@echo "  libpq-dev (for PostgreSQL plugins)"
	@echo "  libldap2-dev (for LDAP plugins)"
	@echo "  libsnmp-dev (for SNMP plugins)"
else ifeq ($(PLATFORM),windows)
	@echo "Installing dependencies on Windows..."
	@echo "Please install vcpkg and required packages"
endif

# Development dependencies
dev-deps: deps
ifeq ($(PLATFORM),macos)
	brew install cppcheck
else ifeq ($(PLATFORM),linux)
	sudo apt-get install -y clang-format cppcheck
endif

# Format code
format:
	@if command -v clang-format >/dev/null 2>&1; then \
		find $(SRC_DIR) $(INCLUDE_DIR) $(PLUGINS_DIR_SRC) -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i; \
		echo "Code formatting completed"; \
	else \
		echo "clang-format not found. Install it first."; \
	fi

# Build with SSL support (convenience target)
build-ssl: ENABLE_SSL=ON
build-ssl: build

# Build without SSL support (convenience target)
build-no-ssl: ENABLE_SSL=OFF
build-no-ssl: build

# Build with all optional dependencies
build-all: ENABLE_SSL=ON ENABLE_SNMP=ON ENABLE_MYSQL=ON ENABLE_PGSQL=ON ENABLE_LDAP=ON
build-all: build

# Build with minimal dependencies (no optional libs)
build-minimal: ENABLE_SSL=OFF ENABLE_SNMP=OFF ENABLE_MYSQL=OFF ENABLE_PGSQL=OFF ENABLE_LDAP=OFF
build-minimal: build

# Help
help:
	@echo "NetMon Plugins - Modern Monitoring Plugins"
	@echo "====================================="
	@echo ""
	@echo "Essential targets:"
	@echo "  all              - Build the project (default)"
	@echo "  build            - Build using CMake"
	@echo "  clean            - Clean build files"
	@echo "  install          - Install plugins system-wide"
	@echo "  uninstall        - Uninstall plugins"
	@echo "  test             - Run tests"
	@echo "  package          - Build platform-specific packages"
	@echo ""
	@echo "Build options (can be set as variables):"
	@echo "  ENABLE_SSL       - Enable SSL/TLS support (default: ON)"
	@echo "  ENABLE_SNMP      - Enable SNMP support (default: ON)"
	@echo "  ENABLE_MYSQL     - Enable MySQL support (default: ON)"
	@echo "  ENABLE_PGSQL     - Enable PostgreSQL support (default: ON)"
	@echo "  ENABLE_LDAP      - Enable LDAP support (default: ON)"
	@echo "  ENABLE_TESTS     - Enable tests (default: ON)"
	@echo "  ENABLE_PACKAGING - Enable packaging (default: ON)"
	@echo ""
	@echo "Convenience build targets:"
	@echo "  build-ssl         - Build with SSL support"
	@echo "  build-no-ssl      - Build without SSL support"
	@echo "  build-all         - Build with all optional dependencies"
	@echo "  build-minimal     - Build with minimal dependencies (no optional libs)"
	@echo ""
	@echo "Development targets:"
	@echo "  dev-build        - Build in debug mode"
	@echo "  dev-test         - Run tests in debug mode"
	@echo "  format           - Format source code"
	@echo ""
	@echo "Dependency management:"
	@echo "  deps             - Install dependencies"
	@echo "  dev-deps         - Install development tools"
	@echo ""
	@echo "Examples:"
	@echo "  make build                    - Build all plugins with default options"
	@echo "  make build ENABLE_SSL=OFF     - Build without SSL support"
	@echo "  make build-ssl                - Build with SSL support"
	@echo "  make build-no-ssl             - Build without SSL support"
	@echo "  make build-minimal            - Build with minimal dependencies"
	@echo "  make test                     - Build and run tests"
	@echo "  make install                  - Install plugins to system"

.PHONY: all build build-ssl build-no-ssl build-all build-minimal clean install uninstall test package dev-build dev-test deps dev-deps format help

.DEFAULT_GOAL := all

