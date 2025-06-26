THIS_FILE := $(lastword $(MAKEFILE_LIST))

SHELL := /bin/bash

SKETCH_DIR ?= src/main/sketches/esp32-2506-web-relay-th-teleinfo
ARDUINO_CLI ?= arduino-cli
ARDUINO_BOARD ?= esp32:esp32:esp32
ARDUINO_LIBS = "ArduinoJson" "Adafruit HTU21DF Library" "WebSockets"
ARDUINO_PORT ?= /dev/cu.usbserial-*

.PHONY: all build verify upload setup check-arduino-cli list-sketches help

ARDUINO_CLI_OK := $(shell command -v $(ARDUINO_CLI) 2>/dev/null)

check-arduino-cli:
	@if [ -z "$(ARDUINO_CLI_OK)" ]; then \
		echo "Error: $(ARDUINO_CLI) not found."; \
		echo "Install it via: brew install arduino-cli"; \
		echo "Then run: make setup"; \
		exit 1; \
	fi

# ===== SKETCH BUILD

all: verify

verify: check-arduino-cli  ## Compile the sketch to verify it builds
	$(ARDUINO_CLI) compile --fqbn $(ARDUINO_BOARD) $(SKETCH_DIR)

build: verify  ## Build the sketch (alias for verify)

upload: check-arduino-cli  ## Build and upload the sketch to the board
	$(ARDUINO_CLI) compile --fqbn $(ARDUINO_BOARD) $(SKETCH_DIR)
	$(ARDUINO_CLI) upload --fqbn $(ARDUINO_BOARD) --port $(ARDUINO_PORT) $(SKETCH_DIR)

setup: check-arduino-cli  ## Install required cores and libraries
	$(ARDUINO_CLI) core update-index
	$(ARDUINO_CLI) core install esp32:esp32
	$(ARDUINO_CLI) lib install $(ARDUINO_LIBS)
	@if [ ! -d "$(HOME)/Documents/Arduino/libraries/LibTeleinfo" ]; then \
		echo "Installing LibTeleinfo from GitHub (not in Arduino index)..."; \
		git clone https://github.com/hallard/LibTeleinfo.git "$(HOME)/Documents/Arduino/libraries/LibTeleinfo"; \
	else \
		echo "LibTeleinfo already installed"; \
	fi

list-sketches:  ## List available sketches
	@echo "Available sketches:"; \
	for d in src/main/sketches/*/; do \
		name=$$(basename "$$d"); \
		ino=$$(ls "$$d"/*.ino 2>/dev/null | head -1); \
		if [ -n "$$ino" ]; then \
			echo "  $$name"; \
		fi; \
	done

help:  ## Show this help
	@echo "Usage: make <target> [SKETCH_DIR=<path>]"
	@echo ""
	@grep -E '^[a-zA-Z._-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | \
		awk 'BEGIN {FS = ":.*?## "}; {printf "  %-20s %s\n", $$1, $$2}'

# ===== DOCUMENTATION

doc.publishToPDF: 
	source .github/docPublishingScripts.sh && publishPDF

# Builds PDF book
doc.publishToHTML: 
	source .github/docPublishingScripts.sh && publishHTML

# Clean caches
doc.clean:
	rm -rf $(CURDIR)/build
	# docker run --rm -v $(CURDIR):/docs alpine rm -rf /docs/build