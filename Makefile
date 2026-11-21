.PHONY: all esphome-dependencies esphome-config esphome-build esphome-upload esphome-logs \
	esphome-debug-config esphome-debug-build esphome-debug-upload esphome-debug-ota esphome-debug-logs \
	esphome-secrets clean help

ESPHOME_PORT ?= /dev/cu.usbserial-110
ESPHOME_DEBUG_HOST ?= 192.168.4.1

ESPHOME_DIR := esphome
ESPHOME_PROD := heater-gsm-monitor.yaml
ESPHOME_DEBUG := heater-gsm-monitor.debug.yaml
ESPHOME_SECRETS := $(ESPHOME_DIR)/secrets.yaml

all: esphome-build

esphome-dependencies:
	@command -v esphome >/dev/null 2>&1 || python3 -m pip install -r $(ESPHOME_DIR)/requirements.txt

esphome-secrets:
	@[ -f "$(ESPHOME_SECRETS)" ] || (echo "✗ Missing $(ESPHOME_SECRETS)"; echo "  cp $(ESPHOME_DIR)/secrets.yaml.example $(ESPHOME_SECRETS)"; exit 1)

# ---------------------------------------------------------------------------
# Production firmware (headless, no WiFi)
# ---------------------------------------------------------------------------
esphome-config: esphome-secrets
	cd $(ESPHOME_DIR) && esphome config $(ESPHOME_PROD)

esphome-build: esphome-dependencies esphome-secrets
	cd $(ESPHOME_DIR) && esphome compile $(ESPHOME_PROD)

esphome-upload: esphome-dependencies esphome-secrets
	cd $(ESPHOME_DIR) && esphome upload $(ESPHOME_PROD) --device $(ESPHOME_PORT)
	@echo "✓ ESPHome production upload complete"

esphome-logs: esphome-dependencies esphome-secrets
	cd $(ESPHOME_DIR) && esphome logs $(ESPHOME_PROD) --device $(ESPHOME_PORT)

# ---------------------------------------------------------------------------
# Debug firmware (AP hotspot, web UI, OTA)
# ---------------------------------------------------------------------------
esphome-debug-config: esphome-secrets
	cd $(ESPHOME_DIR) && esphome config $(ESPHOME_DEBUG)

esphome-debug-build: esphome-dependencies esphome-secrets
	cd $(ESPHOME_DIR) && esphome compile $(ESPHOME_DEBUG)

esphome-debug-upload: esphome-dependencies esphome-secrets
	cd $(ESPHOME_DIR) && esphome upload $(ESPHOME_DEBUG) --device $(ESPHOME_PORT)
	@echo "✓ ESPHome debug upload complete"

esphome-debug-ota: esphome-dependencies esphome-secrets
	cd $(ESPHOME_DIR) && esphome upload $(ESPHOME_DEBUG) --device $(ESPHOME_DEBUG_HOST)
	@echo "✓ ESPHome debug OTA upload complete"

esphome-debug-logs: esphome-dependencies esphome-secrets
	cd $(ESPHOME_DIR) && esphome logs $(ESPHOME_DEBUG) --device $(ESPHOME_DEBUG_HOST)

# ---------------------------------------------------------------------------
# Housekeeping
# ---------------------------------------------------------------------------
clean:
	rm -rf $(ESPHOME_DIR)/.esphome
	@echo "✓ Build artifacts removed"

help:
	@echo "Production firmware:"
	@echo "  make esphome-config        - Validate production config"
	@echo "  make esphome-build         - Compile production firmware"
	@echo "  make esphome-upload        - Upload production firmware over USB"
	@echo "  make esphome-logs          - Logs over USB"
	@echo ""
	@echo "Debug firmware:"
	@echo "  make esphome-debug-build   - Compile AP hotspot + web UI build"
	@echo "  make esphome-debug-upload  - Upload debug firmware over USB"
	@echo "  make esphome-debug-ota     - Upload debug firmware over the debug AP (192.168.4.1)"
	@echo "  make esphome-debug-logs    - Logs over the debug AP (192.168.4.1)"
	@echo ""
	@echo "  make clean                 - Remove build artifacts"
