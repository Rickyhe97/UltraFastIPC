// UltraFastIPC.h - 超高性能IPC头文件
#pragma once
#include <windows.h>
#include <iostream>
#include <atomic>
#include <memory>
#include <chrono>

// 共享内存布局 - 这是两个进程的"共同语言"
struct SharedMemoryLayout {
    // 控制标志 - 使用原子操作确保线程安全
    std::atomic<uint32_t> request_flag{ 0 };      // 0=无请求, 1=有新请求, 2=正在处理
    std::atomic<uint32_t> response_flag{ 0 };     // 0=无响应, 1=有新响应
    std::atomic<uint32_t> sequence_id{ 0 };       // 序列号，防止重复处理

    // 数据区域 - 预分配避免动态内存分配
    uint32_t request_size;                      // 请求数据长度
    uint32_t response_size;                     // 响应数据长度
    char request_data[4096];                    // 请求数据缓冲区
    char response_data[4096];                   // 响应数据缓冲区

    // 性能统计 - 用于监控和优化
    uint64_t last_request_time;                // 最后请求时间（微秒）
    uint64_t last_response_time;               // 最后响应时间（微秒）
    uint32_t total_requests;                   // 总请求数
};

class UltraFastIPCServer {
private:
    HANDLE hMapFile;
    SharedMemoryLayout* pSharedMemory;
    bool isRunning;
    std::string sharedMemoryName;

    // 高精度计时器 - 微秒级精度测量
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
        // 创建共享内存映射 - 这是两个进程的"共享工作台"
        hMapFile = CreateFileMappingA(
            INVALID_HANDLE_VALUE,           // 使用系统页面文件
            NULL,                           // 默认安全属性
            PAGE_READWRITE,                 // 读写访问
            0,                              // 高位大小
            sizeof(SharedMemoryLayout),     // 低位大小
            sharedMemoryName.c_str()        // 映射对象名称
        );

        if (hMapFile == NULL) {
            std::cerr << "创建共享内存失败: " << GetLastError() << std::endl;
            return false;
        }

        // 映射共享内存到进程地址空间
        pSharedMemory = (SharedMemoryLayout*)MapViewOfFile(
            hMapFile,                       // 映射对象句柄
            FILE_MAP_ALL_ACCESS,            // 读写访问
            0, 0,                           // 偏移量
            sizeof(SharedMemoryLayout)      // 映射大小
        );

        if (pSharedMemory == nullptr) {
            std::cerr << "映射共享内存失败: " << GetLastError() << std::endl;
            CloseHandle(hMapFile);
            return false;
        }

        // 初始化共享内存结构
        new (pSharedMemory) SharedMemoryLayout();

        std::cout << "共享内存IPC服务器初始化成功" << std::endl;
        return true;
    }

    void StartProcessing() {
        isRunning = true;
        uint32_t lastProcessedSequence = 0;

        std::cout << "开始超高速处理循环..." << std::endl;

        while (isRunning) {
            // 使用原子操作检查是否有新请求 - 这比系统调用快得多
            uint32_t currentSequence = pSharedMemory->sequence_id.load(std::memory_order_acquire);
            uint32_t requestFlag = pSharedMemory->request_flag.load(std::memory_order_acquire);

            // 只有在有新请求时才处理，避免无意义的CPU消耗
            if (requestFlag == 1 && currentSequence != lastProcessedSequence) {

                // 记录开始处理时间
                uint64_t startTime = GetMicroseconds();
                pSharedMemory->last_request_time = startTime;

                // 原子性地标记正在处理
                pSharedMemory->request_flag.store(2, std::memory_order_release);

                // 处理请求 - 这里是你的核心业务逻辑
                ProcessRequestUltraFast();

                // 记录处理完成时间
                uint64_t endTime = GetMicroseconds();
                pSharedMemory->last_response_time = endTime;

                // 标记响应就绪
                pSharedMemory->response_flag.store(1, std::memory_order_release);
                pSharedMemory->request_flag.store(0, std::memory_order_release);

                // 更新统计信息
                pSharedMemory->total_requests++;
                lastProcessedSequence = currentSequence;

                // 输出性能统计（可选，生产环境中可能需要关闭）
                uint64_t processingTime = endTime - startTime;
                if (processingTime > 100) {  // 只有超过100微秒才报告
                    std::cout << "处理时间: " << processingTime << " 微秒" << std::endl;
                }
            }

            // 极短的休眠，让出CPU时间片但保持高响应性
            // 在某些情况下，你可能需要完全移除这个Sleep以获得极致性能
            Sleep(0);  // 让出时间片但立即重新调度
        }
    }

private:
    void ProcessRequestUltraFast() {
        // 获取请求数据 - 注意这里没有任何内存分配
        uint32_t requestSize = pSharedMemory->request_size;
        const char* requestData = pSharedMemory->request_data;

        // 你的超快业务逻辑在这里
        // 示例：简单的字符串处理
        const char* response = "PROCESSED_ULTRA_FAST";
        uint32_t responseLen = strlen(response);

        // 直接写入共享内存，无需额外分配
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

// 32位进程的主函数
int main() {
    std::cout << "=== 超高性能32位IPC服务器 ===" << std::endl;

    UltraFastIPCServer server("UltraFastIPC_SharedMem");

    if (!server.Initialize()) {
        std::cerr << "服务器初始化失败" << std::endl;
        return -1;
    }

    // 开始处理（这会阻塞主线程）
    server.StartProcessing();

    return 0;
}