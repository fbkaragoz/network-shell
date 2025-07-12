#!/bin/bash

# This script runs a series of network diagnostic commands using NetAn
# and saves the output to a log file.
# It requires NetAn to be built and located in the 'build' directory.
# For 'ping', 'trace', 'mtr' and 'arp_scan' commands, NetAn needs CAP_NET_RAW capability
# (set once with 'sudo setcap cap_net_raw+ep ./build/netan') or the script
# must be run with 'sudo'.

LOG_FILE="netan_diagnostic_$(date +%Y%m%d_%H%M%S).log"
NETAN_PATH="./build/netan"

# Check if NetAn executable exists
if [ ! -f "$NETAN_PATH" ]; then
    echo "Error: NetAn executable not found at $NETAN_PATH."
    echo "Please build the project first (cd build && cmake .. && make)."
    exit 1
fi

echo "Starting NetAn network diagnostic. Output will be saved to $LOG_FILE"
echo "-------------------------------------------------------------------" | tee -a "$LOG_FILE"

# Function to run a command and log its output
run_command() {
    COMMAND_DESC="$1"
    COMMAND="$2"
    echo "\n--- Running: $COMMAND_DESC ---" | tee -a "$LOG_FILE"
    echo "Command: $COMMAND" | tee -a "$LOG_FILE"
    # Check if we need sudo for this command
    if [[ "$COMMAND_DESC" == *"(Requires Admin)"* ]]; then
        if ! sudo -n true 2>/dev/null; then
            echo "Elevated privileges required for this command. Please enter your password if prompted."
            sudo bash -c "$COMMAND" 2>&1 | tee -a "$LOG_FILE"
        else
            sudo bash -c "$COMMAND" 2>&1 | tee -a "$LOG_FILE"
        fi
    else
        bash -c "$COMMAND" 2>&1 | tee -a "$LOG_FILE"
    fi
    echo "--- End of $COMMAND_DESC ---" | tee -a "$LOG_FILE"
}

# --- Diagnostic Commands ---

# 1. Ping to Google (Requires Admin)
run_command "Ping to Google (Requires Admin)" "$NETAN_PATH ping google.com"

# 2. Traceroute to Google (Requires Admin)
run_command "Traceroute to Google (Requires Admin)" "$NETAN_PATH trace google.com"

# 3. MTR to Google (Requires Admin)
run_command "MTR to Google (Requires Admin)" "$NETAN_PATH mtr google.com --count 5"

# 4. DNS Lookup for Google (no admin needed)
run_command "DNS Lookup for Google (no admin needed)" "$NETAN_PATH dns google.com"

# 5. Port Scan on a common service (no admin needed)
run_command "Port Scan on example.com (Port 80) (no admin needed)" "$NETAN_PATH scan example.com 80 80"

# 6. ARP Scan (Linux Only, Requires Admin)
if [[ "$(uname)" == "Linux" ]]; then
    # Attempt to find a common network interface name
    INTERFACE="$(ip -o link show | awk -F': ' '{print $2}' | grep -E '^(eth|enp|wlan|wl|eno)' | head -n 1)"
    if [ -z "$INTERFACE" ]; then
        echo "Warning: Could not automatically determine network interface for ARP scan. Skipping."
    else
        run_command "ARP Scan on $INTERFACE (Linux Only, Requires Admin)" "$NETAN_PATH arp_scan $INTERFACE"
    fi
fi

echo "\nDiagnostic complete. Check $LOG_FILE for full details." | tee -a "$LOG_FILE"