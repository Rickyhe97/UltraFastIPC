// UltraFastIPC.h - High performance IPC header file
#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <iostream>
#include <atomic>
#include <memory>
#include <chrono>
#include <PE32.h>
#include <string>
#include <vector>
#include <sstream> 
using namespace std;

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
};

class UltraFastIPCServer {
private:
	HANDLE hMapFile;
	SharedMemoryLayout* pSharedMemory;
	bool isRunning;
	int parentPid;
	std::string sharedMemoryName;
	bool debugMode;

	// High precision timer - Microsecond-level precision measurement
	uint64_t GetMicroseconds() {
		LARGE_INTEGER frequency, counter;
		QueryPerformanceFrequency(&frequency);
		QueryPerformanceCounter(&counter);
		return (counter.QuadPart * 1000000LL) / frequency.QuadPart;
	}

public:
	UltraFastIPCServer(const std::string& name,int id,bool debugMode)
		: sharedMemoryName(name), parentPid(id), isRunning(false), debugMode(false),hMapFile(nullptr), pSharedMemory(nullptr) {
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

				//// Record the start processing time
				//uint64_t startTime = GetMicroseconds();
				//pSharedMemory->last_request_time = startTime;

				// Atomically mark as processing
				pSharedMemory->request_flag.store(2, std::memory_order_release);

				// Process request - This is your core business logic
				ProcessRequestUltraFast();

				//// Record the end processing time
				//uint64_t endTime = GetMicroseconds();
				//pSharedMemory->last_response_time = endTime;

				// Mark response as ready
				pSharedMemory->response_flag.store(1, std::memory_order_release);
				pSharedMemory->request_flag.store(0, std::memory_order_release);

				// Update statistics
				lastProcessedSequence = currentSequence;

				//// Output performance statistics (optional, may need to be turned off in production)
				//uint64_t processingTime = endTime - startTime;
				//if (processingTime > 1000) {  // Only report if processing time is more than 1000 microseconds
				//	std::cout << "Processing time: " << processingTime << " microseconds" << std::endl;
				//}
			}

			HANDLE hParent = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_INFORMATION, FALSE, (DWORD)parentPid);

			if (hParent == NULL) {
				DWORD err = GetLastError();
				ExitProcess(0);
				std::cout << "Parent process not found, exiting: " << err << std::endl;
				return;
			}
			else {

				DWORD exitCode = 0;

				if (!GetExitCodeProcess(hParent, &exitCode)) {
					DWORD err = GetLastError();
					std::cerr << "GetExitCodeProcess failed£¬error code: " << err << std::endl;
					if (err == ERROR_ACCESS_DENIED) {
						std::cerr << "Please try PROCESS_QUERY_LIMITED_INFORMATION¡£" << std::endl;
					}
					else if (err == ERROR_INVALID_HANDLE) {
						std::cerr << "Unvalued handle." << std::endl;
					}
				}

				CloseHandle(hParent);

				if (exitCode != STILL_ACTIVE) {
					std::cout << "Parent process has exited, exiting server." << std::endl;
					isRunning = false;  // Stop processing loop
				}

			}

			// Extremely short sleep, yield CPU time slice but keep high responsiveness
			// In some cases, you may need to completely remove this Sleep to achieve extreme performance
			Sleep(0);  // Yield time slice but immediately reschedule
		}
	}

private:
	std::vector<std::string> Split(const std::string& s, char delimiter) {
		std::vector<std::string> tokens;
		std::stringstream ss(s);
		std::string item;
		while (getline(ss, item, delimiter)) {
			tokens.push_back(item);
		}
		return tokens;
	}

private:
	void ProcessRequestUltraFast() {
		// Get request data - Note that there is no memory allocation here
		uint32_t requestSize = pSharedMemory->request_size;
		const char* requestData = pSharedMemory->request_data;

		std::string response = "0";
		auto tokens = Split(requestData, ' ');
		
		try {
			if (tokens[0] == "pe32_init") {
				response = std::to_string(pe32_init());
			}
			else if (tokens[0] == "pe32_usb") {
				response = std::to_string(pe32_usb());
			}
	/*		else if (tokens[0] == "pe32_CSenter") {
				pe32_CSenter();
			}
			else if (tokens[0] == "pe32_CSleave") {
				pe32_CSleave();
			}
			else if (tokens[0] == "pe32_mdelay") {
				pe32_mdelay(std::stoi(tokens[1]));
			}*/
			else if (tokens[0] == "pe32_readl") {
				int bdn = std::stoi(tokens[1]);
				int offset = std::stoi(tokens[2]);
				int buffer = 0;
				response = std::to_string(pe32_readl(bdn, offset, &buffer));
			}
			else if (tokens[0] == "pe32_writel") {
				int bdn = std::stoi(tokens[1]);
				int offset = std::stoi(tokens[2]);
				int buf = std::stoi(tokens[3]);
				pe32_writel(bdn, offset, buf);
			}
			else if (tokens[0] == "pe32_set_sctl") {
				int bdn = std::stoi(tokens[1]);
				int data = std::stoi(tokens[2]);
				pe32_set_sctl(bdn, data);
			}
			else if (tokens[0] == "pe32_set_sdata") {
				int bdn = std::stoi(tokens[1]);
				int data = std::stoi(tokens[2]);
				pe32_set_sdata(bdn, data);
			}
			else if (tokens[0] == "pe32_rd_sio") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_rd_sio(bdn));
			}
			else if (tokens[0] == "pe32_wr_pe") {
				int bdn = std::stoi(tokens[1]);
				int chip = std::stoi(tokens[2]);
				int port = std::stoi(tokens[3]);
				int data = std::stoi(tokens[4]);
				pe32_wr_pe(bdn, chip, port, data);
			}
			else if (tokens[0] == "pe32_rd_pe") {
				int bdn = std::stoi(tokens[1]);
				int chip = std::stoi(tokens[2]);
				int port = std::stoi(tokens[3]);
				response = std::to_string(pe32_rd_pe(bdn, chip, port));
			}
			else if (tokens[0] == "pe32_rst_pe") {
				int bdn = std::stoi(tokens[1]);
				pe32_rst_pe(bdn);
			}
			else if (tokens[0] == "pe32_usleep") {
				int usec = std::stoi(tokens[1]);
				pe32_usleep(usec);
			}
			//Common functions
			else if (tokens[0] == "pe32_init") {
				response = std::to_string(pe32_init());
			}
			else if (tokens[0] == "pe32_api") {
				response = std::to_string(pe32_api());
			}
			else if (tokens[0] == "pe32_reset") {
				int bdn = std::stoi(tokens[1]);
				pe32_reset(bdn);
			}
			else if (tokens[0] == "pe32_fdiag") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_fdiag(bdn));
			}
			else if (tokens[0] == "pe32_fstart") {
				int bdn = std::stoi(tokens[1]);
				int onoff = std::stoi(tokens[2]);
				pe32_fstart(bdn, onoff);
			}
			else if (tokens[0] == "pe32_diag_fstart") {
				int bdn = std::stoi(tokens[1]);
				int onoff = std::stoi(tokens[2]);
				pe32_diag_fstart(bdn, onoff);
			}
			else if (tokens[0] == "pe32_cycle") {
				int bdn = std::stoi(tokens[1]);
				int onoff = std::stoi(tokens[2]);
				pe32_cycle(bdn, onoff);
			}
			else if (tokens[0] == "pe32_check_reset") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_check_reset(bdn));
			}
			else if (tokens[0] == "pe32_check_fstart") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_check_fstart(bdn));
			}
			else if (tokens[0] == "pe32_check_cycle") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_check_cycle(bdn));
			}
			else if (tokens[0] == "pe32_check_tprun") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_check_tprun(bdn));
			}
			else if (tokens[0] == "pe32_check_sync") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_check_sync(bdn));
			}
			else if (tokens[0] == "pe32_check_testbeg") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_check_testbeg(bdn));
			}
			else if (tokens[0] == "pe32_check_tpass") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_check_tpass(bdn));
			}
			else if (tokens[0] == "pe32_check_ftend") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_check_ftend(bdn));
			}
			else if (tokens[0] == "pe32_check_lend") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_check_lend(bdn));
			}
			else if (tokens[0] == "pe32_set_pxi") {
				int bdn = std::stoi(tokens[1]);
				int data = std::stoi(tokens[2]);
				pe32_set_pxi(bdn, data);
			}
			else if (tokens[0] == "pe32_pxi_fstart") {
				int bdn = std::stoi(tokens[1]);
				int ch = std::stoi(tokens[2]);
				int onoff = std::stoi(tokens[3]);
				pe32_pxi_fstart(bdn, ch, onoff);
			}
			else if (tokens[0] == "pe32_pxi_cfail") {
				int bdn = std::stoi(tokens[1]);
				int ch = std::stoi(tokens[2]);
				int onoff = std::stoi(tokens[3]);
				pe32_pxi_cfail(bdn, ch, onoff);
			}
			else if (tokens[0] == "pe32_pxi_lmsyn") {
				int bdn = std::stoi(tokens[1]);
				int ch = std::stoi(tokens[2]);
				int onoff = std::stoi(tokens[3]);
				pe32_pxi_lmsyn(bdn, ch, onoff);
			}
			else if (tokens[0] == "pe32_set_addbeg") {
				int bdn = std::stoi(tokens[1]);
				long add = std::stol(tokens[2]);
				pe32_set_addbeg(bdn, add);
			}
			else if (tokens[0] == "pe32_set_addend") {
				int bdn = std::stoi(tokens[1]);
				long cnt = std::stol(tokens[2]);
				pe32_set_addend(bdn, cnt);
			}
			else if (tokens[0] == "pe32_set_ftcnt") {
				int bdn = std::stoi(tokens[1]);
				long cnt = std::stol(tokens[2]);
				pe32_set_ftcnt(bdn, cnt);
			}
			else if (tokens[0] == "pe32_set_addsyn") {
				int bdn = std::stoi(tokens[1]);
				long add = std::stol(tokens[2]);
				pe32_set_addsyn(bdn, add);
			}
			else if (tokens[0] == "pe32_set_addif") {
				int bdn = std::stoi(tokens[1]);
				long add = std::stol(tokens[2]);
				pe32_set_addif(bdn, add);
			}
			else if (tokens[0] == "pe32_set_logadd") {
				int bdn = std::stoi(tokens[1]);
				long add = std::stol(tokens[2]);
				pe32_set_logadd(bdn, add);
			}
			else if (tokens[0] == "pe32_set_seq") {
				int bdn = std::stoi(tokens[1]);
				long data = std::stol(tokens[2]);
				pe32_set_seq(bdn, data);
			}
			else if (tokens[0] == "pe32_set_lmf") {
				int bdn = std::stoi(tokens[1]);
				long data = std::stol(tokens[2]);
				pe32_set_lmf(bdn, data);
			}
			//else if (tokens[0] == "pe32_set_lmd") {
			//    int bdn = std::stoi(tokens[1]);
			//    long data = std::stol(tokens[2]);
			//    pe32_set_lmd(bdn, data);
			//}
			//else if (tokens[0] == "pe32_set_lmm") {
			//    int bdn = std::stoi(tokens[1]);
			//    long data = std::stol(tokens[2]);
			//    pe32_set_lmm(bdn, data);
			//}
			else if (tokens[0] == "pe32_set_mmsk") {
				int bdn = std::stoi(tokens[1]);
				long data = std::stol(tokens[2]);
				pe32_set_mmsk(bdn, data);
			}
			else if (tokens[0] == "pe32_set_mmsk") {
				int bdn = std::stoi(tokens[1]);
				long data = std::stol(tokens[2]);
				pe32_set_mmsk(bdn, data);
			}
			else if (tokens[0] == "pe32_set_tp") {
				int bdn = std::stoi(tokens[1]);
				int ts = std::stoi(tokens[2]);
				long data = std::stol(tokens[3]);
				pe32_set_tp(bdn, ts, data);
			}
			else if (tokens[0] == "pe32_set_tstrob") {
				int bdn = std::stoi(tokens[1]);
				int pno = std::stoi(tokens[2]);
				int ts = std::stoi(tokens[3]);
				long data = std::stol(tokens[4]);
				pe32_set_tstrob(bdn, pno, ts, data);
			}
			else if (tokens[0] == "pe32_set_tstart") {
				int bdn = std::stoi(tokens[1]);
				int pno = std::stoi(tokens[2]);
				int ts = std::stoi(tokens[3]);
				long data = std::stol(tokens[4]);
				pe32_set_tstart(bdn, pno, ts, data);
			}
			else if (tokens[0] == "pe32_set_tstop") {
				int bdn = std::stoi(tokens[1]);
				int pno = std::stoi(tokens[2]);
				int ts = std::stoi(tokens[3]);
				long data = std::stol(tokens[4]);
				pe32_set_tstop(bdn, pno, ts, data);
			}
			else if (tokens[0] == "pe32_set_rz") {
				int bdn = std::stoi(tokens[1]);
				int fs = std::stoi(tokens[2]);
				long data = std::stol(tokens[3]);
				pe32_set_rz(bdn, fs, data);
			}
			else if (tokens[0] == "pe32_set_ro") {
				int bdn = std::stoi(tokens[1]);
				int ts = std::stoi(tokens[2]);
				long data = std::stol(tokens[3]);
				pe32_set_ro(bdn, ts, data);
			}
			else if (tokens[0] == "pe32_set_io") {
				int bdn = std::stoi(tokens[1]);
				int ts = std::stoi(tokens[2]);
				long data = std::stol(tokens[3]);
				pe32_set_io(bdn, ts, data);
			}
			else if (tokens[0] == "pe32_set_mk") {
				int bdn = std::stoi(tokens[1]);
				int ts = std::stoi(tokens[2]);
				long data = std::stol(tokens[3]);
				pe32_set_mk(bdn, ts, data);
			}
			else if (tokens[0] == "pe32_set_dstrob") {
				int bdn = std::stoi(tokens[1]);
				int pno = std::stoi(tokens[2]);
				int ts = std::stoi(tokens[3]);
				long data1 = std::stol(tokens[4]);
				long data2 = std::stol(tokens[5]);
				pe32_set_dstrob(bdn, pno, ts, data1, data2);
			}
			else if (tokens[0] == "pe32_rd_actseq") {
				int bdn = std::stoi(tokens[1]);
				pe32_rd_actseq(bdn);
			}
			else if (tokens[0] == "pe32_rd_actlmf") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_rd_actlmf(bdn));
			}
			else if (tokens[0] == "pe32_rd_actlmd") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_rd_actlmd(bdn));
			}
			else if (tokens[0] == "pe32_rd_actlmm") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_rd_actlmm(bdn));
			}
			else if (tokens[0] == "pe32_rd_actlmadd") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_rd_actlmadd(bdn));
			}
			else if (tokens[0] == "pe32_rd_pxibus") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_rd_pxibus(bdn));
			}
			else if (tokens[0] == "pe32_rd_id") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_rd_id(bdn));
			}
			else if (tokens[0] == "pe32_rd_vc") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_rd_vc(bdn));
			}
			else if (tokens[0] == "pe32_rd_seq") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_rd_seq(bdn));
			}
			else if (tokens[0] == "pe32_rd_lmf") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_rd_lmf(bdn));
			}
			else if (tokens[0] == "pe32_rd_lmd") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_rd_lmd(bdn));
			}
			else if (tokens[0] == "pe32_rd_lmm") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_rd_lmm(bdn));
			}
			else if (tokens[0] == "pe32_rd_lmadd") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_rd_lmadd(bdn));
			}
			else if (tokens[0] == "pe32_lmload") {
				int begbdno = std::stoi(tokens[1]);
				int boardwidth = std::stoi(tokens[2]);
				long begadd = std::stol(tokens[3]);
				char* patternfileChar = new char[tokens[4].size() + 1];
				strcpy(patternfileChar, tokens[4].c_str());
				response = std::to_string(pe32_lmload(begbdno, boardwidth, begadd, patternfileChar));
				delete[] patternfileChar;
			}
			else if (tokens[0] == "pe32_lmsave") {
				int begbdno = std::stoi(tokens[1]);
				int boardwidth = std::stoi(tokens[2]);
				long begadd = std::stol(tokens[3]);
				long endadd = std::stol(tokens[4]);
				char* patternfileChar = new char[tokens[4].size() + 1];
				strcpy(patternfileChar, tokens[4].c_str());
				response = std::to_string(pe32_lmsave(begbdno, boardwidth, begadd, endadd, patternfileChar));
				delete[] patternfileChar;
			}
			else if (tokens[0] == "pe32_rd_cmph") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_rd_cmph(bdn));
			}
			else if (tokens[0] == "pe32_rd_cmpl") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_rd_cmpl(bdn));
			}
			else if (tokens[0] == "pe32_rd_creg") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_rd_creg(bdn));
			}
			else if (tokens[0] == "pe32_rd_ftcnt") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_rd_ftcnt(bdn));
			}
			else if (tokens[0] == "pe32_rd_fccnt") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_rd_fccnt(bdn));
			}
			else if (tokens[0] == "pe32_rd_flcnt") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_rd_flcnt(bdn));
			}
			else if (tokens[0] == "pe32_rd_clog") {
				int bdn = std::stoi(tokens[1]);
				int addr = std::stoi(tokens[2]);
				response = std::to_string(pe32_rd_clog(bdn, addr));
			}
			else if (tokens[0] == "pe32_rd_alog") {
				int bdn = std::stoi(tokens[1]);
				int addr = std::stoi(tokens[2]);
				response = std::to_string(pe32_rd_alog(bdn, addr));
			}
			else if (tokens[0] == "pe32_rd_logadd") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_rd_logadd(bdn));
			}
			else if (tokens[0] == "pe32_rd_alogclog") {
				int bdn = std::stoi(tokens[1]);
				int addr = std::stoi(tokens[2]);
				int alog;
				int clog;
				response = std::to_string(pe32_rd_alogclog(bdn, addr, &alog, &clog));
			}
			else if (tokens[0] == "pe32_dump_alogclog") {
				int bdn = std::stoi(tokens[1]);
				int Ksize = std::stoi(tokens[2]);
				int alog;
				int clog;
				response = std::to_string(pe32_dump_alogclog(bdn, Ksize, &alog, &clog));
			}
			else if (tokens[0] == "pe32_set_dumpmode") {
				int bdn = std::stoi(tokens[1]);
				int onoff = std::stoi(tokens[2]);
				pe32_set_dumpmode(bdn, onoff);
			}
			else if (tokens[0] == "pe32_dump_getclog") {
				int bdn = std::stoi(tokens[1]);
				int addr = std::stoi(tokens[2]);
				response = std::to_string(pe32_dump_getclog(bdn, addr));
			}
			else if (tokens[0] == "pe32_dump_getalog") {
				int bdn = std::stoi(tokens[1]);
				int addr = std::stoi(tokens[2]);
				response = std::to_string(pe32_dump_getalog(bdn, addr));
			}
			else if (tokens[0] == "pe32_dump_getalogclog") {
				int bdn = std::stoi(tokens[1]);
				int add = std::stoi(tokens[2]);
				int alog;
				int clog;
				response = std::to_string(pe32_dump_getalogclog(bdn, add, &alog, &clog));
			}
			else if (tokens[0] == "pe32_check_dataready") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_check_dataready(bdn));
			}
			else if (tokens[0] == "pe32_check_checkmode") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_check_checkmode(bdn));
			}
			else if (tokens[0] == "pe32_check_logmode") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_check_logmode(bdn));
			}
			else if (tokens[0] == "pe32_check_trigmode") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_check_trigmode(bdn));
			}
			else if (tokens[0] == "pe32_check_dualmode") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_check_dualmode(bdn));
			}
			else if (tokens[0] == "pe32_set_trigmode") {
				int bdn = std::stoi(tokens[1]);
				int onoff = std::stoi(tokens[2]);
				pe32_set_trigmode(bdn, onoff);
			}
			else if (tokens[0] == "pe32_set_logmode") {
				int bdn = std::stoi(tokens[1]);
				int onoff = std::stoi(tokens[2]);
				pe32_set_logmode(bdn, onoff);
			}
		/*	else if (tokens[0] == "pe32_set_208M") {
				int bdn = std::stoi(tokens[1]);
				int onoff = std::stoi(tokens[2]);
				pe32_set_208M(bdn, onoff);
			}*/
			else if (tokens[0] == "pe32_check_ucnt") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_check_ucnt(bdn));
			}
			else if (tokens[0] == "pe32_set_checkmode") {
				int bdn = std::stoi(tokens[1]);
				int onoff = std::stoi(tokens[2]);
				pe32_set_checkmode(bdn, onoff);
			}
			else if (tokens[0] == "pe32_set_vih") {
				int bdn = std::stoi(tokens[1]);
				int pno = std::stoi(tokens[2]);
				double rv = std::stod(tokens[3]);
				pe32_set_vih(bdn, pno, rv);
			}
			else if (tokens[0] == "pe32_set_vil") {
				int bdn = std::stoi(tokens[1]);
				int pno = std::stoi(tokens[2]);
				double rv = std::stod(tokens[3]);
				pe32_set_vil(bdn, pno, rv);
			}
			else if (tokens[0] == "pe32_set_voh") {
				int bdn = std::stoi(tokens[1]);
				int pno = std::stoi(tokens[2]);
				double rv = std::stod(tokens[3]);
				pe32_set_voh(bdn, pno, rv);
			}
			else if (tokens[0] == "pe32_set_vol") {
				int bdn = std::stoi(tokens[1]);
				int pno = std::stoi(tokens[2]);
				double rv = std::stod(tokens[3]);
				pe32_set_vol(bdn, pno, rv);
			}
			else if (tokens[0] == "pe32_set_driver") {
				int bdno = std::stoi(tokens[1]);
				int pno = std::stoi(tokens[2]);
				int onoff = std::stoi(tokens[3]);
				pe32_set_driver(bdno, pno, onoff);
			}
			else if (tokens[0] == "pe32_cpu_df") {
				int bdn = std::stoi(tokens[1]);
				int pno = std::stoi(tokens[2]);
				int donoff = std::stoi(tokens[3]);
				int fonoff = std::stoi(tokens[4]);
				pe32_cpu_df(bdn, pno, donoff, fonoff);
			}
			else if (tokens[0] == "pe32_pmufv") {
				int bdn = std::stoi(tokens[1]);
				int chip = std::stoi(tokens[2]);
				double rv = std::stod(tokens[3]);
				double clamp = std::stod(tokens[4]);
				pe32_pmufv(bdn, chip, rv, clamp);
			}
			else if (tokens[0] == "pe32_pmufi") {
				int bdn = std::stoi(tokens[1]);
				int chip = std::stoi(tokens[2]);
				double ri = std::stod(tokens[3]);
				double cvh = std::stod(tokens[4]);
				double cvl = std::stod(tokens[5]);
				pe32_pmufi(bdn, chip, ri, cvh, cvl);
			}
			else if (tokens[0] == "pe32_pmufir") {
				int bdn = std::stoi(tokens[1]);
				int chip = std::stoi(tokens[2]);
				double ri = std::stod(tokens[3]);
				double cvh = std::stod(tokens[4]);
				double cvl = std::stod(tokens[5]);
				int rang = std::stoi(tokens[6]);
				pe32_pmufir(bdn, chip, ri, cvh, cvl, rang);
			}
			else if (tokens[0] == "pe32_vmeas") {
				int bdn = std::stoi(tokens[1]);
				int pno = std::stoi(tokens[2]);
				pe32_vmeas(bdn, pno);
			}
			else if (tokens[0] == "pe32_imeas") {
				int bdn = std::stoi(tokens[1]);
				int pno = std::stoi(tokens[2]);
				pe32_imeas(bdn, pno);
			}
			else if (tokens[0] == "pe32_pmucv") {
				int bdn = std::stoi(tokens[1]);
				int chip = std::stoi(tokens[2]);
				int cvh = std::stoi(tokens[3]);
				int cvl = std::stoi(tokens[4]);
				pe32_pmucv(bdn, chip, cvh, cvl);
			}
			else if (tokens[0] == "pe32_pmuci") {
				int bdn = std::stoi(tokens[1]);
				int chip = std::stoi(tokens[2]);
				int cih = std::stoi(tokens[3]);
				int cil = std::stoi(tokens[4]);
				pe32_pmuci(bdn, chip, cih, cil);
			}
			else if (tokens[0] == "pe32_con_pmu") {
				int bdn = std::stoi(tokens[1]);
				int pno = std::stoi(tokens[2]);
				int onoff = std::stoi(tokens[3]);
				pe32_con_pmu(bdn, pno, onoff);
			}
			else if (tokens[0] == "pe32_con_pmus") {
				int bdn = std::stoi(tokens[1]);
				int pno = std::stoi(tokens[2]);
				int onoff = std::stoi(tokens[3]);
				pe32_con_pmus(bdn, pno, onoff);
			}
			else if (tokens[0] == "pe32_con_receiver") {
				int bdn = std::stoi(tokens[1]);
				int pno = std::stoi(tokens[2]);
				int onoff = std::stoi(tokens[3]);
				pe32_con_receiver(bdn, pno, onoff);
			}
			else if (tokens[0] == "pe32_check_pmu") {
				int bdn = std::stoi(tokens[1]);
				int chip = std::stoi(tokens[2]);
				response = std::to_string(pe32_check_pmu(bdn, chip));
			}
			else if (tokens[0] == "pe32_pmuch") {
				int bdn = std::stoi(tokens[1]);
				int chip = std::stoi(tokens[2]);
				response = std::to_string(pe32_pmuch(bdn, chip));
			}
			else if (tokens[0] == "pe32_pmucl") {
				int bdn = std::stoi(tokens[1]);
				int chip = std::stoi(tokens[2]);
				response = std::to_string(pe32_pmucl(bdn, chip));
			}
			else if (tokens[0] == "pe32_cal_load") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_cal_load(bdn, tokens[2].c_str()));
			}
			else if (tokens[0] == "pe32_cal_save") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_cal_save(bdn, tokens[2].c_str()));
			}
			else if (tokens[0] == "pe32_cal_load_auto") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_cal_load_auto(bdn, tokens[2].c_str()));
			}
			else if (tokens[0] == "pe32_cal_save_auto") {
				int bdn = std::stoi(tokens[1]);
				response = std::to_string(pe32_cal_save_auto(bdn, tokens[2].c_str()));
			}
			else if (tokens[0] == "pe32_cal_reset") {
				int bdn = std::stoi(tokens[1]);
				pe32_cal_reset(bdn);
			}
			else if (tokens[0] == "pe32_con_esense") {
				int bdn = std::stoi(tokens[1]);
				int pno = std::stoi(tokens[2]);
				int onoff = std::stoi(tokens[3]);
				pe32_con_esense(bdn, pno, onoff);
			}
			else if (tokens[0] == "pe32_con_eforce") {
				int bdn = std::stoi(tokens[1]);
				int pno = std::stoi(tokens[2]);
				int onoff = std::stoi(tokens[3]);
				pe32_con_eforce(bdn, pno, onoff);
			}
			else if (tokens[0] == "pe32_con_ext") {
				int bdn = std::stoi(tokens[1]);
				int pno = std::stoi(tokens[2]);
				int onoff = std::stoi(tokens[3]);
				pe32_con_ext(bdn, pno, onoff);
			}
			else if (tokens[0] == "pe32_set_deskew") {
				int bdn = std::stoi(tokens[1]);
				int onoff = std::stoi(tokens[2]);
				int rt = std::stoi(tokens[3]);
				pe32_set_deskew(bdn, onoff, rt);
			}
			else {
				ProcessCommonAPI(tokens, response);
			}

		}
		catch (...) {
			response = "error";
		}
		if (debugMode) {
			cout << requestData << endl;
		}
		
		// Directly write to shared memory, no extra allocation needed
		memcpy(pSharedMemory->response_data, response.c_str(), response.size());
		pSharedMemory->response_size = response.size();

		// Clear request data after finishing response
		memset(pSharedMemory->request_data, 0, requestSize);
	}

	void ProcessCommonAPI(std::vector<std::string>& tokens, std::string& response) {
		if (tokens[0] == "pe32_set_fallingskew") {
			int bdn = std::stoi(tokens[1]);
			int pno = std::stoi(tokens[2]);
			int rt = std::stoi(tokens[3]);
			pe32_set_fallingskew(bdn, pno, rt);
		}
		else if (tokens[0] == "pe32_set_rcvskew") {
			int bdn = std::stoi(tokens[1]);
			int pno = std::stoi(tokens[2]);
			int rt = std::stoi(tokens[3]);
			pe32_set_rcvskew(bdn, pno, rt);
		}
		else if (tokens[0] == "pe32_set_rcvfallingskew") {
			int bdn = std::stoi(tokens[1]);
			int pno = std::stoi(tokens[2]);
			int rt = std::stoi(tokens[3]);
			pe32_set_rcvfallingskew(bdn, pno, rt);
		}
		else if (tokens[0] == "pe32_getch") {
			int bdn = std::stoi(tokens[1]);
			int pno = std::stoi(tokens[2]);
			response = std::to_string(pe32_getch(bdn, pno));
		}
		else if (tokens[0] == "pe32_getcl") {
			int bdn = std::stoi(tokens[1]);
			int pno = std::stoi(tokens[2]);
			response = std::to_string(pe32_getcl(bdn, pno));
		}
		else if (tokens[0] == "pemu32_rst_pe") {
			int bdn = std::stoi(tokens[1]);
			pemu32_rst_pe(bdn);
		}
		else if (tokens[0] == "pemu32_set_driver") {
			int bdn = std::stoi(tokens[1]);
			int pno = std::stoi(tokens[2]);
			int onoff = std::stoi(tokens[3]);
			pemu32_set_driver(bdn, pno, onoff);
		}
		else if (tokens[0] == "pe32_counter_ctp") {
			int bdn = std::stoi(tokens[1]);
			long data = std::stol(tokens[2]);
			pe32_counter_ctp(bdn, data);
		}
		else if (tokens[0] == "pe32_counter_start") {
			int bdn = std::stoi(tokens[1]);
			int onoff = std::stoi(tokens[2]);
			pe32_counter_start(bdn, onoff);
		}
		else if (tokens[0] == "pe32_counter_select_ch") {
			int bdn = std::stoi(tokens[1]);
			int ch = std::stoi(tokens[2]);
			pe32_counter_select_ch(bdn, ch);
		}
		else if (tokens[0] == "pe32_counter_rd") {
			int bdn = std::stoi(tokens[1]);
			response = std::to_string(pe32_counter_rd(bdn));
		}
		else if (tokens[0] == "pe32_counter_rdfrq") {
			int bdn = std::stoi(tokens[1]);
			response = std::to_string(pe32_counter_rdfrq(bdn));
		}
		else if (tokens[0] == "pe32_counter_tmmode") {
			int bdn = std::stoi(tokens[1]);
			int onoff = std::stoi(tokens[2]);
			pe32_counter_tmmode(bdn, onoff);
		}
		else if (tokens[0] == "pe32_tmu_cstart_inv") {
			int bdn = std::stoi(tokens[1]);
			int onoff = std::stoi(tokens[2]);
			pe32_tmu_cstart_inv(bdn, onoff);
		}
		else if (tokens[0] == "pe32_tmu_cstop_inv") {
			int bdn = std::stoi(tokens[1]);
			int onoff = std::stoi(tokens[2]);
			pe32_tmu_cstop_inv(bdn, onoff);
		}
		else if (tokens[0] == "pe32_tmu_select_cstart") {
			int bdn = std::stoi(tokens[1]);
			int ch = std::stoi(tokens[2]);
			pe32_tmu_select_cstart(bdn, ch);
		}
		else if (tokens[0] == "pe32_tmu_select_cstop") {
			int bdn = std::stoi(tokens[1]);
			int ch = std::stoi(tokens[2]);
			pe32_tmu_select_cstop(bdn, ch);
		}
		else if (tokens[0] == "pe32_rd_pesno") {
			int bdn = std::stoi(tokens[1]);
			response = std::to_string(pe32_rd_pesno(bdn));
		}
		else if (tokens[0] == "pe32_get_temp") {
			int bdn = std::stoi(tokens[1]);
			int cno = std::stoi(tokens[2]);
			response = std::to_string(pe32_get_temp(bdn, cno));
		}
		else if (tokens[0] == "pe32_set_srdmode") {
			int bdn = std::stoi(tokens[1]);
			int onoff = std::stoi(tokens[2]);
			pe32_set_srdmode(bdn, onoff);
		}
		else if (tokens[0] == "pe32_set_logadd") {
			int bdn = std::stoi(tokens[1]);
			long add = std::stol(tokens[2]);
			pe32_set_logadd(bdn, add);
		}
		else if (tokens[0] == "pe32_srd_select_ch") {
			int bdn = std::stoi(tokens[1]);
			int ch = std::stoi(tokens[2]);
			pe32_srd_select_ch(bdn, ch);
		}
		else if (tokens[0] == "pe32_srd_getword") {
			int bdn = std::stoi(tokens[1]);
			response = std::to_string(pe32_srd_getword(bdn));
		}
		else if (tokens[0] == "pe32_srd_getword2") {
			int bdn = std::stoi(tokens[1]);
			response = std::to_string(pe32_srd_getword2(bdn));
		}
		else if (tokens[0] == "pe32_srd_getsrword") {
			int bdn = std::stoi(tokens[1]);
			int ch = std::stoi(tokens[2]);
			response = std::to_string(pe32_srd_getsrword(bdn, ch));
		}
		else if (tokens[0] == "pe32_srd_rdblock32") {
			int bdn = std::stoi(tokens[1]);
			long add = std::stol(tokens[2]);
			int rdblock32;
			pe32_srd_rdblock32(bdn, add, &rdblock32);
			response = std::to_string(rdblock32);
		}
		else if (tokens[0] == "pe32_setReg") {
			int bdn = std::stoi(tokens[1]);
			int pno = std::stoi(tokens[2]);
			int dacno = std::stoi(tokens[3]);
			int rv = std::stoi(tokens[4]);
			pe32_setReg(bdn, pno, dacno, rv);
		}
		else if (tokens[0] == "pe32_dc_range") {
			int bdn = std::stoi(tokens[1]);
			int range = std::stoi(tokens[2]);
			pe32_dc_range(bdn, range);
		}
		else if (tokens[0] == "pe32_set_lmsyn_active_high") {
			int bdn = std::stoi(tokens[1]);
			int onoff = std::stoi(tokens[2]);
			pe32_set_lmsyn_active_high(bdn, onoff);
		}
		else if (tokens[0] == "pe32_set_lmsyn_ch") {
			int bdn = std::stoi(tokens[1]);
			int ch = std::stoi(tokens[2]);
			pe32_set_lmsyn_ch(bdn, ch);
		}
		else if (tokens[0] == "pe32_rd_logcnt") {
			int bdn = std::stoi(tokens[1]);
			response = std::to_string(pe32_rd_logcnt(bdn));
		}
		else if (tokens[0] == "pe32_reset_lmiomk") {
			int bdn = std::stoi(tokens[1]);
			pe32_reset_lmiomk(bdn);
		}
		else if (tokens[0] == "pe32_con_2k2vtt") {
			int bdn = std::stoi(tokens[1]);
			int pno = std::stoi(tokens[2]);
			int onoff = std::stoi(tokens[3]);
			double vtt = std::stod(tokens[4]);
			pe32_con_2k2vtt(bdn, pno, onoff, vtt);
		}
		else if (tokens[0] == "pe32_get_msg") {
			response = pe32_get_msg();
		}
		else if (tokens[0] == "pe32_set_rffemode") {
			int bdn = std::stoi(tokens[1]);
			int port = std::stoi(tokens[2]);
			int onoff = std::stoi(tokens[3]);
			pe32_set_rffemode(bdn, port, onoff);
		}
		else if (tokens[0] == "pe32_rffe_ftp") {
			int bdn = std::stoi(tokens[1]);
			int wtp = std::stoi(tokens[2]);
			int rtp = std::stoi(tokens[3]);
			pe32_rffe_ftp(bdn, wtp, rtp);
		}
		else if (tokens[0] == "pe32_rffe_pclk") {
			int bdn = std::stoi(tokens[1]);
			int pclk = std::stoi(tokens[2]);
			pe32_rffe_pclk(bdn, pclk);
		}
		else if (tokens[0] == "pe32_rffe_wr") {
			int bdn = std::stoi(tokens[1]);
			int port = std::stoi(tokens[2]);
			int sadd = std::stoi(tokens[3]);
			int add = std::stoi(tokens[4]);
			short data = std::stoi(tokens[5]);
			pe32_rffe_wr(bdn, port, sadd, add, data);
		}
		else if (tokens[0] == "pe32_rffe_rd") {
			int bdn = std::stoi(tokens[1]);
			int port = std::stoi(tokens[2]);
			int sadd = std::stoi(tokens[3]);
			int add = std::stoi(tokens[4]);
			response = std::to_string(pe32_rffe_rd(bdn, port, sadd, add));
		}
		else if (tokens[0] == "pe32_rffe_ewr") {
			int bdn = std::stoi(tokens[1]);
			int port = std::stoi(tokens[2]);
			int sadd = std::stoi(tokens[3]);
			int add = std::stoi(tokens[4]);
			short data = std::stoi(tokens[5]);
			int bcnt = std::stoi(tokens[6]);
			pe32_rffe_ewr(bdn, port, sadd, add, data, bcnt);
		}
		else if (tokens[0] == "pe32_rffe_erd") {
			int bdn = std::stoi(tokens[1]);
			int port = std::stoi(tokens[2]);
			int sadd = std::stoi(tokens[3]);
			int add = std::stoi(tokens[4]);
			int bcnt = std::stoi(tokens[5]);
			response = std::to_string(pe32_rffe_erd(bdn, port, sadd, add, bcnt));
		}
		else if (tokens[0] == "pe32_rffe_getword") {
			int bdn = std::stoi(tokens[1]);
			int port = std::stoi(tokens[2]);
			int add = std::stoi(tokens[3]);
			response = std::to_string(pe32_rffe_getword(bdn, port));
		}
		else if (tokens[0] == "pe32_rffe_wr0") {
			int bdn = std::stoi(tokens[1]);
			int port = std::stoi(tokens[2]);
			int sadd = std::stoi(tokens[3]);
			short data = std::stoi(tokens[4]);
			pe32_rffe_wr0(bdn, port, sadd, data);
		}
		else if (tokens[0] == "pe32_rffe_elwr") {
			int bdn = std::stoi(tokens[1]);
			int port = std::stoi(tokens[2]);
			int sadd = std::stoi(tokens[3]);
			int add = std::stoi(tokens[4]);
			int data = std::stoi(tokens[5]);
			int bcnt = std::stoi(tokens[6]);
			pe32_rffe_elwr(bdn, port, sadd, add, data, bcnt);
		}
		else if (tokens[0] == "pe32_rffe_elrd") {
			int bdn = std::stoi(tokens[1]);
			int port = std::stoi(tokens[2]);
			int sadd = std::stoi(tokens[3]);
			int add = std::stoi(tokens[4]);
			int bcnt = std::stoi(tokens[5]);
			response = std::to_string(pe32_rffe_elrd(bdn, port, sadd, add, bcnt));
		}
		else if (tokens[0] == "pe32_rffe_cmdwr") {
			int bdn = std::stoi(tokens[1]);
			int port = std::stoi(tokens[2]);
			int sadd = std::stoi(tokens[3]);
			int cmd = std::stoi(tokens[4]);
			int add = std::stoi(tokens[5]);
			int data = std::stoi(tokens[6]);
			int bcnt = std::stoi(tokens[7]);
			pe32_rffe_cmdwr(bdn, port, sadd, cmd, add, data, bcnt);
		}
		else if (tokens[0] == "pe32_rffe_cmdrd") {
			int bdn = std::stoi(tokens[1]);
			int port = std::stoi(tokens[2]);
			int sadd = std::stoi(tokens[3]);
			int cmd = std::stoi(tokens[4]);
			int add = std::stoi(tokens[5]);
			int data = std::stoi(tokens[6]);
			int bcnt = std::stoi(tokens[7]);
			pe32_rffe_cmdrd(bdn, port, sadd, cmd, add, data, bcnt);
		}
	/*	else if (tokens[0] == "pe32_rffe_set_word") {
			int bdn = std::stoi(tokens[1]);
			int port = std::stoi(tokens[2]);
			int data = std::stoi(tokens[3]);
			int add = std::stoi(tokens[4]);
			pe32_rffe_set_word(bdn, port, data, add);
		}
		else if (tokens[0] == "pe32_rffe_mipi2") {
			int bdn = std::stoi(tokens[1]);
			int onoff = std::stoi(tokens[2]);
			pe32_rffe_mipi2(bdn, onoff);
		}*/
		else if (tokens[0] == "pe32_set_qmode") {
			int bdn = std::stoi(tokens[1]);
			int onoff = std::stoi(tokens[2]);
			pe32_set_qmode(bdn, onoff);
		}
		else if (tokens[0] == "pe32_check_qfail") {
			int bdn = std::stoi(tokens[1]);
			int cno = std::stoi(tokens[2]);
			response = std::to_string(pe32_check_qfail(bdn, cno));
		}
		else if (tokens[0] == "pe32_set_rodvhdvl") {
			int bdn = std::stoi(tokens[1]);
			int pno = std::stoi(tokens[2]);
			int rodvh = std::stoi(tokens[3]);
			int rodvl = std::stoi(tokens[4]);
			pe32_set_rodvhdvl(bdn, pno, rodvh, rodvl);
		}
		else if (tokens[0] == "pe32_rd_PciRevId") {
			int bdn = std::stoi(tokens[1]);
			response = std::to_string(pe32_rd_PciRevId(bdn));
		}
		else if (tokens[0] == "pe32_rd_PciDevId") {
			int bdn = std::stoi(tokens[1]);
			response = std::to_string(pe32_rd_PciDevId(bdn));
		}
		else if (tokens[0] == "pe32_rd_PciSubId") {
			int bdn = std::stoi(tokens[1]);
			response = std::to_string(pe32_rd_PciSubId(bdn));
		}
		else if (tokens[0] == "pe32_trig_mv") {
			int bdn = std::stoi(tokens[1]);
			int pno = std::stoi(tokens[2]);
			int pxitrg = std::stoi(tokens[3]);
			pe32_trig_mv(bdn, pno, pxitrg);
		}
		else if (tokens[0] == "pe32_trig_mi") {
			int bdn = std::stoi(tokens[1]);
			int pno = std::stoi(tokens[2]);
			int pxitrg = std::stoi(tokens[3]);
			pe32_trig_mi(bdn, pno, pxitrg);
		}
		else if (tokens[0] == "pe32_trig_imeas") {
			int bdn = std::stoi(tokens[1]);
			int pno = std::stoi(tokens[2]);
			response = std::to_string(pe32_trig_imeas(bdn, pno));
		}
		else if (tokens[0] == "pe32_trig_vmeas") {
			int bdn = std::stoi(tokens[1]);
			int pno = std::stoi(tokens[2]);
			response = std::to_string(pe32_trig_vmeas(bdn, pno));
		}
		else if (tokens[0] == "pe32_user_fram_save") {
			int bdn = std::stoi(tokens[1]);
			int ADD = std::stoi(tokens[2]);
			const char* data = tokens[3].c_str();
			int size = std::stoi(tokens[4]);
			pe32_user_fram_save(bdn, ADD, data, size);
		}
		else if (tokens[0] == "pe32_user_fram_load") {
			int bdn = std::stoi(tokens[1]);
			int ADD = std::stoi(tokens[2]);
			char* data = new char[tokens[3].size() + 1];
			int size = std::stoi(tokens[4]);
			response = std::to_string(pe32_user_fram_load(bdn, ADD, data, size));
			delete[] data;
		}
		else {
			response = "Unknown command :"+ tokens[0];
			cout << response << endl;
		}

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
int main(int argc, char* argv[]) {

	if (argc < 2)
	{
		std::cerr << "Please provide at least 2 arguments: process ID and debug mode (0 or 1)" << std::endl;
		return 1;
	}

	auto id = (DWORD)std::stoi(argv[1]);
	char ch = argv[2][0];
	bool debugMode = (ch == '1');

	if (debugMode) {
		std::cout << "Debug mode is ON" << std::endl;
	}
	else {
		std::cout << "Debug mode is OFF" << std::endl;
	}

	std::cout << "Parent process ID: " << id << std::endl;
	std::cout << "Current Process ID: " << GetCurrentProcessId() << std::endl;
	std::cout << "=== High performance 32-bit IPC server ===" << std::endl;

	UltraFastIPCServer server("UltraFastIPC_SharedMem", id, debugMode);

	if (!server.Initialize()) {
		std::cerr << "Server initialization failed" << std::endl;
		return -1;
	}

	// Start processing (this will block the main thread)
	server.StartProcessing();

	return 0;
}

