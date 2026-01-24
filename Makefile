.PHONY: all build upload clean help install-cli monitor dependencies

# Configuration
ARDUINO_PORT ?= /dev/cu.usbserial-110
BOARD := esp8266:esp8266:nodemcuv2
BUILD_DIR := build
SKETCH_DIR := heater-gsm-monitor
SECRETS_FILE := $(SKETCH_DIR)/arduino_secrets.h
ARDUINO_CLI := arduino-cli

# Targets
all: build

install-cli:
	@command -v $(ARDUINO_CLI) >/dev/null 2>&1 || brew install arduino-cli

dependencies: install-cli
	$(ARDUINO_CLI) core install esp8266:esp8266
	$(ARDUINO_CLI) lib install "TinyGSM" "DallasTemperature" "OneWire" "Adafruit GFX Library" "Adafruit SSD1306"

build: install-cli
	@[ -f "$(SECRETS_FILE)" ] || (echo "✗ Missing $(SECRETS_FILE)"; exit 1)
	@mkdir -p $(BUILD_DIR)
	$(ARDUINO_CLI) compile --fqbn $(BOARD) --output-dir $(BUILD_DIR) $(SKETCH_DIR)

upload: build
	$(ARDUINO_CLI) upload -p $(ARDUINO_PORT) --fqbn $(BOARD) --input-dir $(BUILD_DIR)
	@echo "✓ Upload complete"

monitor:
	screen $(ARDUINO_PORT) 96000

clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "Targets:"
	@echo "  dependencies  - Install ESP32 board and libraries (one-time)"
	@echo "  build         - Compile sketch (checks secrets file exists)"
	@echo "  upload        - Compile and upload to board"
	@echo "  monitor       - Open serial monitor"
	@echo "  clean         - Remove build directory"
	@echo ""
	@echo "Configuration:"
	@echo "  ARDUINO_PORT = $(ARDUINO_PORT)"
