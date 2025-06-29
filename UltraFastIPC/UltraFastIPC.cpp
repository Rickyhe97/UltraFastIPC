// UltraFastIPC.h - ��������IPCͷ�ļ�
#pragma once
#include <windows.h>
#include <iostream>
#include <atomic>
#include <memory>
#include <chrono>

// �����ڴ沼�� - �����������̵�"��ͬ����"
struct SharedMemoryLayout {
    // ���Ʊ�־ - ʹ��ԭ�Ӳ���ȷ���̰߳�ȫ
    std::atomic<uint32_t> request_flag{ 0 };      // 0=������, 1=��������, 2=���ڴ���
    std::atomic<uint32_t> response_flag{ 0 };     // 0=����Ӧ, 1=������Ӧ
    std::atomic<uint32_t> sequence_id{ 0 };       // ���кţ���ֹ�ظ�����

    // �������� - Ԥ������⶯̬�ڴ����
    uint32_t request_size;                      // �������ݳ���
    uint32_t response_size;                     // ��Ӧ���ݳ���
    char request_data[4096];                    // �������ݻ�����
    char response_data[4096];                   // ��Ӧ���ݻ�����

    // ����ͳ�� - ���ڼ�غ��Ż�
    uint64_t last_request_time;                // �������ʱ�䣨΢�룩
    uint64_t last_response_time;               // �����Ӧʱ�䣨΢�룩
    uint32_t total_requests;                   // ��������
};

class UltraFastIPCServer {
private:
    HANDLE hMapFile;
    SharedMemoryLayout* pSharedMemory;
    bool isRunning;
    std::string sharedMemoryName;

    // �߾��ȼ�ʱ�� - ΢�뼶���Ȳ���
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
        // ���������ڴ�ӳ�� - �����������̵�"������̨"
        hMapFile = CreateFileMappingA(
            INVALID_HANDLE_VALUE,           // ʹ��ϵͳҳ���ļ�
            NULL,                           // Ĭ�ϰ�ȫ����
            PAGE_READWRITE,                 // ��д����
            0,                              // ��λ��С
            sizeof(SharedMemoryLayout),     // ��λ��С
            sharedMemoryName.c_str()        // ӳ���������
        );

        if (hMapFile == NULL) {
            std::cerr << "���������ڴ�ʧ��: " << GetLastError() << std::endl;
            return false;
        }

        // ӳ�乲���ڴ浽���̵�ַ�ռ�
        pSharedMemory = (SharedMemoryLayout*)MapViewOfFile(
            hMapFile,                       // ӳ�������
            FILE_MAP_ALL_ACCESS,            // ��д����
            0, 0,                           // ƫ����
            sizeof(SharedMemoryLayout)      // ӳ���С
        );

        if (pSharedMemory == nullptr) {
            std::cerr << "ӳ�乲���ڴ�ʧ��: " << GetLastError() << std::endl;
            CloseHandle(hMapFile);
            return false;
        }

        // ��ʼ�������ڴ�ṹ
        new (pSharedMemory) SharedMemoryLayout();

        std::cout << "�����ڴ�IPC��������ʼ���ɹ�" << std::endl;
        return true;
    }

    void StartProcessing() {
        isRunning = true;
        uint32_t lastProcessedSequence = 0;

        std::cout << "��ʼ�����ٴ���ѭ��..." << std::endl;

        while (isRunning) {
            // ʹ��ԭ�Ӳ�������Ƿ��������� - ���ϵͳ���ÿ�ö�
            uint32_t currentSequence = pSharedMemory->sequence_id.load(std::memory_order_acquire);
            uint32_t requestFlag = pSharedMemory->request_flag.load(std::memory_order_acquire);

            // ֻ������������ʱ�Ŵ��������������CPU����
            if (requestFlag == 1 && currentSequence != lastProcessedSequence) {

                // ��¼��ʼ����ʱ��
                uint64_t startTime = GetMicroseconds();
                pSharedMemory->last_request_time = startTime;

                // ԭ���Եر�����ڴ���
                pSharedMemory->request_flag.store(2, std::memory_order_release);

                // �������� - ��������ĺ���ҵ���߼�
                ProcessRequestUltraFast();

                // ��¼�������ʱ��
                uint64_t endTime = GetMicroseconds();
                pSharedMemory->last_response_time = endTime;

                // �����Ӧ����
                pSharedMemory->response_flag.store(1, std::memory_order_release);
                pSharedMemory->request_flag.store(0, std::memory_order_release);

                // ����ͳ����Ϣ
                pSharedMemory->total_requests++;
                lastProcessedSequence = currentSequence;

                // �������ͳ�ƣ���ѡ�����������п�����Ҫ�رգ�
                uint64_t processingTime = endTime - startTime;
                if (processingTime > 100) {  // ֻ�г���100΢��ű���
                    std::cout << "����ʱ��: " << processingTime << " ΢��" << std::endl;
                }
            }

            // ���̵����ߣ��ó�CPUʱ��Ƭ�����ָ���Ӧ��
            // ��ĳЩ����£��������Ҫ��ȫ�Ƴ����Sleep�Ի�ü�������
            Sleep(0);  // �ó�ʱ��Ƭ���������µ���
        }
    }

private:
    void ProcessRequestUltraFast() {
        // ��ȡ�������� - ע������û���κ��ڴ����
        uint32_t requestSize = pSharedMemory->request_size;
        const char* requestData = pSharedMemory->request_data;

        // ��ĳ���ҵ���߼�������
        // ʾ�����򵥵��ַ�������
        const char* response = "PROCESSED_ULTRA_FAST";
        uint32_t responseLen = strlen(response);

        // ֱ��д�빲���ڴ棬����������
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

// 32λ���̵�������
int main() {
    std::cout << "=== ��������32λIPC������ ===" << std::endl;

    UltraFastIPCServer server("UltraFastIPC_SharedMem");

    if (!server.Initialize()) {
        std::cerr << "��������ʼ��ʧ��" << std::endl;
        return -1;
    }

    // ��ʼ��������������̣߳�
    server.StartProcessing();

    return 0;
}