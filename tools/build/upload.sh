#!/bin/bash
set -e

FIRMWARE="${1:-firmware.bin}"
BOSSAC="/home/me/.arduino15/packages/adafruit/tools/bossac/1.8.0-48-gb176eee/bossac"

# Target Vendor ID and Product ID pairs (lowercase, without 0x prefix)
TARGET_VIDS=("0666" "239a" "239a")
TARGET_PIDS=("0666" "801e" "001e")

if [ ! -f "$FIRMWARE" ]; then
    echo "Error: $FIRMWARE not found"
    exit 1
fi

# Function to search ttyACM devices for specific VID/PID pairs
find_trinket_port() {
    for dev in /dev/ttyACM*; do
        [ -e "$dev" ] || continue
        # Check udev properties for specific VID and PID
        udev_info=$(udevadm info --query=property --name="$dev" 2>/dev/null)
        
        for i in "${!TARGET_VIDS[@]}"; do
            v="${TARGET_VIDS[$i]}"
            p="${TARGET_PIDS[$i]}"
            if echo "$udev_info" | grep -qiE "ID_VENDOR_ID=$v" && echo "$udev_info" | grep -qiE "ID_MODEL_ID=$p"; then
                echo "$dev"
                return 0
            fi
        done
    done
    return 1
}

echo "Searching for device with target VID/PID..."
PORT=$(find_trinket_port)

if [ -z "$PORT" ]; then
    echo "Error: No matching device found on /dev/ttyACM*"
    exit 1
fi

echo "Found board on $PORT"

# 1200-bps touch reset to trigger bootloader
echo "Touching port $PORT at 1200 bps to enter bootloader mode..."

# Check if the app port is busy before trying to open it
if fuser -s "$PORT" 2>/dev/null; then
    BUSY_PID=$(fuser "$PORT" 2>/dev/null)
    BUSY_PROC=$(ps -p "$BUSY_PID" -o comm= 2>/dev/null || echo "unknown")
    echo "Error: $PORT is busy (PID $BUSY_PID, process '$BUSY_PROC')." >&2
    echo "Close any program using this port and try again." >&2
    exit 1
fi

python3 -c "
import serial, time
try:
    s = serial.Serial('$PORT', 1200)
    s.dtr = False
    time.sleep(0.1)
    s.close()
except Exception as e:
    print(f'Warning during 1200bps touch: {e}')
"

echo "Waiting for board to reboot into bootloader..."
sleep 0.1

UPLOAD_PORT=""
# Re-scan specifically for the target device (or fallback to any ACM port if ID shifts slightly in bootloader mode)
for i in $(seq 1 40); do
    TEMP_PORT=$(find_trinket_port || ls /dev/ttyACM* 2>/dev/null | head -n 1)
    if [ -n "$TEMP_PORT" ]; then
        UPLOAD_PORT="$TEMP_PORT"
        echo "Bootloader port detected at $UPLOAD_PORT!"
        break
    fi
    sleep 0.25
done

if [ -z "$UPLOAD_PORT" ]; then
    echo "Error: Bootloader port did not appear after reset."
    exit 1
fi

# Check if the bootloader port is busy (e.g. another program has it open)
if fuser -s "$UPLOAD_PORT" 2>/dev/null; then
    BUSY_PID=$(fuser "$UPLOAD_PORT" 2>/dev/null)
    BUSY_PROC=$(ps -p "$BUSY_PID" -o comm= 2>/dev/null || echo "unknown")
    echo "Error: $UPLOAD_PORT is busy (PID $BUSY_PID, process '$BUSY_PROC')." >&2
    echo "Close any program using this port and try again." >&2
    exit 1
fi

BOSSAC_PORT="${UPLOAD_PORT#/dev/}"

echo "Flashing $FIRMWARE on /dev/$BOSSAC_PORT..."
MAX_RETRIES=3
for attempt in $(seq 1 $MAX_RETRIES); do
    echo "Attempt $attempt of $MAX_RETRIES..."
    if "$BOSSAC" -i -d --port="$BOSSAC_PORT" -U -i --offset=0x2000 -w -v "$FIRMWARE" -R; then
        echo "Done!"
        exit 0
    fi
    if [ "$attempt" -lt "$MAX_RETRIES" ]; then
        echo "Flash failed, retrying in 1s..."
        sleep 1
    fi
done
echo "Error: Flash failed after $MAX_RETRIES attempts." >&2
exit 1