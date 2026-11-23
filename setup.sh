#!/usr/bin/env bash
# Faulter lightweight setup script
# Initializes only the required submodules (Pico SDK + TinyUSB)
# Skips >350 MB of unused Pico W wireless/Bluetooth/crypto libraries

set -e

echo "==> [Faulter] Initializing lightweight submodules..."
git submodule update --init firmware/lib/pico-sdk
git -C firmware/lib/pico-sdk submodule update --init lib/tinyusb
echo "==> [Faulter] Setup complete! Only required libraries were installed."
