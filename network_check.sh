#!/bin/bash

# Ana dizin ve zaman
LOG_DIR="./logs"
TIMESTAMP=$(date '+%Y-%m-%d_%H-%M')
mkdir -p "$LOG_DIR"

# Ağ Arayüzünü otomatik tespit et
INTERFACE=$(ip route | grep default | awk '{print $5}')
IP=$(ip addr show "$INTERFACE" | grep 'inet ' | awk '{print $2}' | cut -d/ -f1)

echo "[$TIMESTAMP] Analiz Başlatıldı: Arayüz $INTERFACE - IP: $IP"

# 1. PING TESTİ
echo "🏓 Ping Test (Google DNS)" | tee "$LOG_DIR/ping_log_$TIMESTAMP.txt"
ping -c 20 -i 0.2 8.8.8.8 | tee -a "$LOG_DIR/ping_log_$TIMESTAMP.txt"

# 2. TRACEROUTE
echo -e "\n🌐 Traceroute (8.8.8.8)" | tee "$LOG_DIR/traceroute_$TIMESTAMP.txt"
traceroute 8.8.8.8 | tee -a "$LOG_DIR/traceroute_$TIMESTAMP.txt"

# 3. SPEEDTEST
if command -v speedtest &> /dev/null; then
    echo -e "\n🚀 Speedtest" | tee "$LOG_DIR/speedtest_$TIMESTAMP.txt"
    speedtest | tee -a "$LOG_DIR/speedtest_$TIMESTAMP.txt"
else
    echo "⚠️ Speedtest-cli yüklü değil. 'sudo apt install speedtest-cli' ile yükleyin." | tee -a "$LOG_DIR/speedtest_$TIMESTAMP.txt"
fi

# 4. DNS Lookup süresi
echo -e "\n🔎 DNS Sorgu Süresi" | tee "$LOG_DIR/dns_time_$TIMESTAMP.txt"
dig google.com | grep "Query time" | tee -a "$LOG_DIR/dns_time_$TIMESTAMP.txt"

# 5. Trafik Monitörü (İsteğe bağlı)
echo -e "\n📊 Canlı Ağ Trafiği (5s iftop snapshot)"
if command -v iftop &> /dev/null; then
    sudo timeout 5 iftop -i "$INTERFACE" -t > "$LOG_DIR/iftop_$TIMESTAMP.txt"
else
    echo "⚠️ 'iftop' yüklü değil. 'sudo apt install iftop' ile yükleyin." | tee -a "$LOG_DIR/iftop_$TIMESTAMP.txt"
fi

echo -e "\n✅ Analiz tamamlandı. Çıktılar $LOG_DIR içinde."
