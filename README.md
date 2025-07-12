# NetAn - Network Analyzer

NetAn is a command-line network analysis tool designed to provide essential network diagnostics on both Linux and Windows operating systems. It aims to be a lightweight, dependency-free executable that can help users identify and troubleshoot network issues.

## Features

NetAn currently supports the following commands:

*   **`ping <host> [port]`**: Performs a TCP-based latency check to a specified host and port (defaults to 80). This is useful for measuring network responsiveness without requiring special privileges.
    ```bash
    ./netan ping google.com
    ./netan ping example.com 443
    ```

*   **`trace <host> [port]`**: Executes a TCP-based traceroute to a specified host and port (defaults to 80). This helps in identifying the path packets take to a destination and potential bottlenecks, even in environments where ICMP is blocked.
    ```bash
    ./netan trace google.com
    ./netan trace example.com 443
    ```

*   **`scan <host> <start_port> [end_port]`**: Performs a TCP connect port scan on a given host for a specified range of ports. Useful for checking open ports on a remote machine.
    ```bash
    ./netan scan example.com 80 100
    ./netan scan 192.168.1.1 22
    ```

*   **`dns <hostname_or_ip>`**: Performs both forward (hostname to IP) and reverse (IP to hostname) DNS lookups.
    ```bash
    ./netan dns google.com
    ./netan dns 8.8.8.8
    ```

## Building from Source

NetAn uses CMake as its build system, making it easy to compile on various platforms.

### Prerequisites

*   **Git**: For cloning the repository.
*   **CMake**: Version 3.10 or higher.
*   **C Compiler**: 
    *   **Linux**: GCC (GNU Compiler Collection)
    *   **Windows**: MinGW-w64 (for GCC on Windows) or MSVC (Visual Studio C++ compiler)

### Compilation Steps

1.  **Clone the repository (if you haven't already):**
    ```bash
    git clone https://github.com/your-repo/netan.git # Replace with actual repo URL
    cd netan
    ```

2.  **Create a build directory and navigate into it:**
    ```bash
    mkdir build
    cd build
    ```

3.  **Configure the project with CMake:**
    *   **Linux:**
        ```bash
        cmake ..
        ```
    *   **Windows (MinGW-w64):**
        ```bash
        cmake .. -G "MinGW Makefiles"
        ```
    *   **Windows (Visual Studio):**
        ```bash
        cmake .. -G "Visual Studio 16 2019" # Adjust generator based on your VS version
        ```

4.  **Build the project:**
    ```bash
    cmake --build .
    ```

    This will compile the source code and create the `netan` executable (or `netan.exe` on Windows) in the `build` directory.

## Usage

After building, you can run the executable from the `build` directory:

```bash
./build/netan <command> [options]
```

For example:

```bash
./build/netan ping example.com
./build/netan trace google.com 443
./build/netan scan localhost 1 1024
./build/netan dns 1.1.1.1
```

## Future Enhancements

*   MTR-like continuous traceroute
*   Throughput/iperf client
*   Bufferbloat testing
*   QoS/DSCP reading/writing
*   Interface statistics & NIC info
*   PCAP dump option
*   JSON log output and Prometheus exporter
*   Unit and integration tests
*   Static analysis and sanitizers

## Contributing

Contributions are welcome! Please feel free to open issues or submit pull requests.
