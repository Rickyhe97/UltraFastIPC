// UltraFastIPC.h - High performance IPC header file
#pragma once
#include <windows.h>
#include <iostream>
#include <atomic>
#include <memory>
#include <chrono>
#include "PE32.h"


// Shared memory layout - This is the "common language" between two processes
struct SharedMemoryLayout {
    // Control flags - Use atomic operations to ensure thread safety
    std::atomic<uint32_t> request_flag{ 0 };      // 0=no request, 1=new request, 2=processing
    std::atomic<uint32_t> response_flag{ 0 };     // 0=no response, 1=new response
    std::atomic<uint32_t> sequence_id{ 0 };       // Sequence number, to prevent duplicate processing

    // Data area - Pre-allocated to avoid dynamic memory allocation
    uint32_t request_size;                      // Length of request data
    uint32_t response_size;                     // Length of response data
    char request_data[4096];                    // Request data buffer
    char response_data[4096];                   // Response data buffer

    // Performance statistics - For monitoring and optimization
    uint64_t last_request_time;                // Time of last request (microseconds)
    uint64_t last_response_time;               // Time of last response (microseconds)
    uint32_t total_requests;                   // Total number of requests
};

class UltraFastIPCServer {
private:
    HANDLE hMapFile;
    SharedMemoryLayout* pSharedMemory;
    bool isRunning;
    std::string sharedMemoryName;

    // High precision timer - Microsecond-level precision measurement
    uint64_t GetMicroseconds() {
        LARGE_INTEGER frequency, counter;
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&counter);
        return (counter.QuadPart * 1000000LL) / frequency.QuadPart;
    }

public:
    UltraFastIPCServer(const std::string& name)
        : sharedMemoryName(name), isRunning(false), hMapFile(nullptr), pSharedMemory(nullptr) {
    }

    bool Initialize() {
        // Create a shared memory mapping - This is the "common workspace" between two processes
        hMapFile = CreateFileMappingA(
            INVALID_HANDLE_VALUE,           // Use system pagefile
            NULL,                           // Default security attributes
            PAGE_READWRITE,                 // Read/Write access
            0,                              // High size
            sizeof(SharedMemoryLayout),     // Low size
            sharedMemoryName.c_str()        // Mapping object name
        );

        if (hMapFile == NULL) {
            std::cerr << "Create shared memory failed: " << GetLastError() << std::endl;
            return false;
        }

        // Map shared memory to process address space
        pSharedMemory = (SharedMemoryLayout*)MapViewOfFile(
            hMapFile,                       // Mapping object handle
            FILE_MAP_ALL_ACCESS,            // Read/Write access
            0, 0,                           // Offset
            sizeof(SharedMemoryLayout)      // Mapping size
        );

        if (pSharedMemory == nullptr) {
            std::cerr << "Map shared memory failed: " << GetLastError() << std::endl;
            CloseHandle(hMapFile);
            return false;
        }

        // Initialize shared memory structure
        new (pSharedMemory) SharedMemoryLayout();

        std::cout << "Shared memory IPC server initialization successful" << std::endl;
        return true;
    }

    void StartProcessing() {
        isRunning = true;
        uint32_t lastProcessedSequence = 0;

        std::cout << "Starting ultra-fast processing loop..." << std::endl;

        while (isRunning) {
            // Use atomic operations to check for new requests - This is much faster than system calls
            uint32_t currentSequence = pSharedMemory->sequence_id.load(std::memory_order_acquire);
            uint32_t requestFlag = pSharedMemory->request_flag.load(std::memory_order_acquire);

            // Only process if there is a new request to avoid unnecessary CPU consumption
            if (requestFlag == 1 && currentSequence != lastProcessedSequence) {

                // Record the start processing time
                uint64_t startTime = GetMicroseconds();
                pSharedMemory->last_request_time = startTime;

                // Atomically mark as processing
                pSharedMemory->request_flag.store(2, std::memory_order_release);

                // Process request - This is your core business logic
                ProcessRequestUltraFast();

                // Record the end processing time
                uint64_t endTime = GetMicroseconds();
                pSharedMemory->last_response_time = endTime;

                // Mark response as ready
                pSharedMemory->response_flag.store(1, std::memory_order_release);
                pSharedMemory->request_flag.store(0, std::memory_order_release);

                // Update statistics
                pSharedMemory->total_requests++;
                lastProcessedSequence = currentSequence;

                // Output performance statistics (optional, may need to be turned off in production)
                uint64_t processingTime = endTime - startTime;
                if (processingTime > 100) {  // Only report if processing time is more than 100 microseconds
                    std::cout << "Processing time: " << processingTime << " microseconds" << std::endl;
                }
            }

            // Extremely short sleep, yield CPU time slice but keep high responsiveness
            // In some cases, you may need to completely remove this Sleep to achieve extreme performance
            Sleep(0);  // Yield time slice but immediately reschedule
        }
    }

private:
    void ProcessRequestUltraFast() {
        // Get request data - Note that there is no memory allocation here
        uint32_t requestSize = pSharedMemory->request_size;
        const char* requestData = pSharedMemory->request_data;

        // Your ultra-fast business logic here
        // Example: simple string processing
        const char* response = "PROCESSED_ULTRA_FAST";
        uint32_t responseLen = strlen(response);

        // Directly write to shared memory, no extra allocation needed
        memcpy(pSharedMemory->response_data, response, responseLen);
        pSharedMemory->response_size = responseLen;
    }

public:
    ~UltraFastIPCServer() {
        isRunning = false;

        if (pSharedMemory != nullptr) {
            UnmapViewOfFile(pSharedMemory);
        }

        if (hMapFile != nullptr) {
            CloseHandle(hMapFile);
        }
    }
};

// Main function for a 32-bit process
int main() {

    auto status = pe32_init();

    std::cout << "=== High performance 32-bit IPC server ===" << std::endl;

    UltraFastIPCServer server("UltraFastIPC_SharedMem");

    if (!server.Initialize()) {
        std::cerr << "Server initialization failed" << std::endl;
        return -1;
    }

    // Start processing (this will block the main thread)
    server.StartProcessing();

    return 0;
}
