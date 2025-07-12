# NetAn - Ağ Analiz Aracı

NetAn, hem Linux hem de Windows işletim sistemlerinde temel ağ teşhisleri sağlamak üzere tasarlanmış bir komut satırı ağ analiz aracıdır. Kullanıcıların ağ sorunlarını belirlemelerine ve gidermelerine yardımcı olmak için hafif, bağımlılıksız bir yürütülebilir dosya olmayı hedefler.

## Özellikler

NetAn şu anda aşağıdaki komutları desteklemektedir:

*   **`ping <ana bilgisayar> [port]`**: Belirtilen bir ana bilgisayar ve porta (varsayılan olarak 80) TCP tabanlı bir gecikme kontrolü yapar. Bu, özel ayrıcalıklar gerektirmeden ağ yanıt hızını ölçmek için kullanışlıdır.
    ```bash
    ./netan ping google.com
    ./netan ping ornek.com 443
    ```

*   **`trace <ana bilgisayar> [port]`**: Belirtilen bir ana bilgisayar ve porta (varsayılan olarak 80) TCP tabanlı bir traceroute (izleme yolu) yürütür. Bu, paketlerin bir hedefe giderken izlediği yolu ve potansiyel darboğazları belirlemeye yardımcı olur, hatta ICMP'nin engellendiği ortamlarda bile.
    ```bash
    ./netan trace google.com
    ./netan trace ornek.com 443
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
    git clone https://github.com/fbkaragoz/network-shell.git
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

## Kullanım

Derlemeden sonra, yürütülebilir dosyayı `build` dizininden çalıştırabilirsiniz:

```bash
./build/netan <komut> [seçenekler]
```

Örneğin:

```bash
./build/netan ping ornek.com
./build/netan trace google.com 443
./build/netan scan localhost 1 1024
./build/netan dns 1.1.1.1
```

## Gelecek Geliştirmeler

*   MTR benzeri sürekli traceroute
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
