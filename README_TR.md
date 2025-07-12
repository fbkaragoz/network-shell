# NetAn - Ağ Analiz Aracı

NetAn, hem Linux hem de Windows işletim sistemlerinde temel ağ teşhisleri sağlamak üzere tasarlanmış bir komut satırı ağ analiz aracıdır. Kullanıcıların ağ sorunlarını belirlemelerine ve gidermelerine yardımcı olmak için hafif, bağımlılıksız bir yürütülebilir dosya olmayı hedefler.

## Özellikler

NetAn şu anda aşağıdaki komutları desteklemektedir:

*   **`ping <ana bilgisayar>`**: Belirtilen bir ana bilgisayara ICMP tabanlı bir ping yapar, gecikme (min/ort/maks), paket kaybı ve jitter dahil olmak üzere ayrıntılı istatistikler sağlar. Bu komut yükseltilmiş ayrıcalıklar (root/Yönetici) gerektirir.
    ```bash
    # Linux'ta (bir kez 'sudo setcap cap_net_raw+ep ./build/netan' çalıştırdıktan sonra):
    ./netan ping google.com
    # Linux'ta (setcap kullanılmazsa veya tek çalıştırma için):
    sudo ./netan ping google.com
    # Windows'ta (Komut İstemi/PowerShell'i Yönetici olarak çalıştırın):
    .\netan.exe ping google.com
    ```

*   **`trace <ana bilgisayar>`**: Belirtilen bir ana bilgisayara ICMP tabanlı bir traceroute (izleme yolu) yürütür. Bu komut, ham ICMP paketlerini göndermek ve almak için yükseltilmiş ayrıcalıklar (root/Yönetici) gerektirir. Paketlerin bir hedefe giderken izlediği yolu ve potansiyel darboğazları belirlemeye yardımcı olur, ara durak IP adreslerini gösterir.
    ```bash
    # Linux'ta (bir kez 'sudo setcap cap_net_raw+ep ./build/netan' çalıştırdıktan sonra):
    ./netan trace google.com
    # Linux'ta (setcap kullanılmazsa veya tek çalıştırma için):
    sudo ./netan trace google.com
    # Windows'ta (Komut İstemi/PowerShell'i Yönetici olarak çalıştırın):
    .\netan.exe trace google.com
    ```

*   **`mtr <ana bilgisayar> [--interval <ms>] [--count <sayi>]`**: Belirtilen bir ana bilgisayara ICMP tabanlı bir MTR (My Traceroute) yapar, her durak için sürekli gecikme ve paket kaybı istatistikleri sağlar. Bu komut da yükseltilmiş ayrıcalıklar (root/Yönetici) gerektirir. Aralıklı ağ sorunlarını teşhis etmek için çok değerlidir.
    ```bash
    # Linux'ta (bir kez 'sudo setcap cap_net_raw+ep ./build/netan' çalıştırdıktan sonra):
    ./netan mtr google.com
    # Linux'ta (setcap kullanılmazsa veya tek çalıştırma için):
    sudo ./netan mtr google.com --count 10
    # Windows'ta (Komut İstemi/PowerShell'i Yönetici olarak çalıştırın):
    .\netan.exe mtr google.com --interval 500
    ```

*   **`scan <ana bilgisayar> <baslangic_portu> [bitis_portu]`**: Belirli bir port aralığı için verilen bir ana bilgisayarda TCP bağlantı noktası taraması yapar. Uzak bir makinedeki açık portları kontrol etmek için kullanışlıdır.
    ```bash
    ./netan scan ornek.com 80 100
    ./netan scan 192.168.1.1 22
    ```

*   **`dns <ana_bilgisayar_adi_veya_ip>`**: Hem ileri (ana bilgisayar adından IP'ye) hem de ters (IP'den ana bilgisayar adına) DNS aramaları yapar.
    ```bash
    ./netan dns google.com
    ./netan dns 8.8.8.8
    ```

*   **`arp_scan <arayuz_adi>`**: (Yalnızca Linux) ARP istekleri göndererek yerel ağdaki aktif cihazları tarar. Bu komut yükseltilmiş ayrıcalıklar (root) gerektirir.
    ```bash
    # Linux'ta (bir kez 'sudo setcap cap_net_raw+ep ./build/netan' çalıştırdıktan sonra):
    ./netan arp_scan eth0 # eth0 yerine ağ arayüzünüzün adını yazın (örn. wlan0, enp0s3)
    # Linux'ta (setcap kullanılmazsa veya tek çalıştırma için):
    sudo ./netan arp_scan eth0
    ```

## Kaynaktan Derleme

NetAn, CMake'i derleme sistemi olarak kullanır ve bu da çeşitli platformlarda derlemeyi kolaylaştırır.

### Önkoşullar

*   **Git**: Depoyu klonlamak için.
*   **CMake**: Sürüm 3.10 veya üzeri.
*   **C Derleyici**: 
    *   **Linux**: GCC (GNU Derleyici Koleksiyonu)
    *   **Windows**: MinGW-w64 (Windows'ta GCC için) veya MSVC (Visual Studio C++ derleyicisi)

### Derleme Adımları

1.  **Depoyu klonlayın (daha önce yapmadıysanız):**
    ```bash
    git clone https://github.com/fbkaragoz/network-shell.git # Gerçek depo URL'si ile değiştirin
    cd network-shell
    ```

2.  **Bir derleme dizini oluşturun ve içine girin:**
    ```bash
    mkdir build
    cd build
    ```

3.  **Projeyi CMake ile yapılandırın:**
    *   **Linux:**
        ```bash
        cmake ..
        ```
    *   **Windows (MinGW-w64 kullanarak):**
        ```bash
        cmake .. -G "MinGW Makefiles"
        ```
    *   **Windows (Visual Studio kullanarak):**
        ```bash
        cmake .. -G "Visual Studio 16 2019" # VS sürümünüze göre jeneratörü ayarlayın
        ```

4.  **Projeyi derleyin:**
    ```bash
    cmake --build .
    ```

    Bu, kaynak kodunu derleyecek ve `build` dizininde `netan` yürütülebilir dosyasını (Windows'ta `netan.exe`) oluşturacaktır.

### Derleme Sonrası Kurulum (Yalnızca Linux için `ping`, `trace`, `mtr` ve `arp_scan`)

Linux'ta `ping`, `trace`, `mtr` ve `arp_scan` komutlarının her seferinde `sudo` kullanmadan çalışması için, `netan` yürütülebilir dosyasına `CAP_NET_RAW` yetkisi vermeniz gerekir. Bu, derlemeden sonra yalnızca bir kez yapılmalıdır:

```bash
sudo setcap cap_net_raw+ep ./build/netan
```

## Kullanım

Derlemeden sonra, yürütülebilir dosyayı `build` dizininden çalıştırabilirsiniz:

```bash
./build/netan <komut> [seçenekler]
```

Örneğin:

```bash
./build/netan ping ornek.com
./netan trace google.com
./netan scan localhost 1 1024
./netan dns 1.1.1.1
./netan arp_scan eth0
```

## Gelecek Geliştirmeler

*   Verim/iperf istemcisi
*   Bufferbloat testi
*   QoS/DSCP okuma/yazma
*   Arayüz istatistikleri ve NIC bilgisi
*   PCAP döküm seçeneği
*   JSON günlük çıktısı ve Prometheus dışa aktarıcı
*   Birim ve entegrasyon testleri
*   Statik analiz ve temizleyiciler

## Katkıda Bulunma

Katkılar memnuniyetle karşılanır! Lütfen sorunları açmaktan veya çekme istekleri göndermekten çekinmeyin.
