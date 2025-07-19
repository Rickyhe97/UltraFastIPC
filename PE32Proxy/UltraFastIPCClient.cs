using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO.MemoryMappedFiles;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace PE32Proxy;

// Shared memory layout structure - must be exactly the same as the C++ end
[StructLayout(LayoutKind.Sequential)]
public struct SharedMemoryLayout
{
    public uint request_flag; // Offset: 0
    public uint response_flag; // Offset: 4
    public uint sequence_id; // Offset: 8
    public uint request_size; // Offset: 12
    public uint response_size; // Offset: 16

    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4096)]
    public byte[] request_data; // Offset: 20

    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4096)]
    public byte[] response_data; // Offset: 4116

    public ulong last_request_time; // Offset: 8212
    public ulong last_response_time; // Offset: 8216
}

internal partial class UltraFastIPCClient : IDisposable
{
    private readonly string sharedMemoryName;
    private readonly string bridgeExecutablePath;
    private MemoryMappedFile? mmf;
    private MemoryMappedViewAccessor? accessor;
    private Process? bridgeProcess;
    private uint sequenceCounter = 1;
    private bool disposed = false;

    // High precision timer
    [LibraryImport("kernel32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static partial bool QueryPerformanceCounter(out long lpPerformanceCount);

    [LibraryImport("kernel32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static partial bool QueryPerformanceFrequency(out long lpFrequency);

    private long performanceFrequency;

    internal bool DebugMode { get; init; }

    internal UltraFastIPCClient(
        string bridgeExePath,
        string sharedMemName = "UltraFastIPC_SharedMem"
    )
    {
        this.bridgeExecutablePath = bridgeExePath;
        this.sharedMemoryName = sharedMemName;

        // Initialize high precision timer
        QueryPerformanceFrequency(out performanceFrequency);

        Console.WriteLine("Ultra-fast IPC client initialization completed");
    }

    private long GetMicroseconds()
    {
        QueryPerformanceCounter(out long counter);
        return (counter * 1000000L) / performanceFrequency;
    }

    [System.Diagnostics.CodeAnalysis.SuppressMessage(
        "Interoperability",
        "CA1416:Validate platform compatibility",
        Justification = "<Pending>"
    )]
    public bool StartBridgeProcess()
    {
        try
        {
            Console.WriteLine("Starting 32-bit bridge process...");

            ProcessStartInfo startInfo =
                new()
                {
                    FileName = bridgeExecutablePath,
                    Arguments = string.Join(
                        " ",
                        [Environment.ProcessId.ToString(), DebugMode ? "1" : "0"]
                    ),
                    UseShellExecute = DebugMode,
                    CreateNoWindow = !DebugMode,
                };

            bridgeProcess =
                Process.Start(startInfo)
                ?? throw new InvalidOperationException("Failed to start bridge process");

            // Wait for process initialization
            Thread.Sleep(1000);

            // Connect to shared memory
            try
            {
                mmf = MemoryMappedFile.OpenExisting(sharedMemoryName);
                accessor = mmf.CreateViewAccessor(0, Marshal.SizeOf<SharedMemoryLayout>());

                Console.WriteLine("Successfully connected to shared memory");
                return true;
            }
            catch (FileNotFoundException)
            {
                throw new InvalidOperationException(
                    "Failed to connect to shared memory, please ensure the 32-bit program is running"
                );
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Startup failed: {ex.Message}");
            return false;
        }
    }

    public string SendRequestUltraFast(string request, int timeoutMicroseconds = 1000000)
    {
        if (accessor == null)
            throw new InvalidOperationException("IPC client is not initialized");

        long startTime = GetMicroseconds();

        try
        {
            // Prepare request data
            byte[] requestBytes = Encoding.UTF8.GetBytes(request);
            if (requestBytes.Length > 4096)
                throw new ArgumentException("Request data is too large");

            uint currentSequence = ++sequenceCounter;

            // Write request to shared memory - these operations are memory level and extremely fast
            accessor.Write(12, (uint)requestBytes.Length); // request_size position
            accessor.WriteArray(20, requestBytes, 0, requestBytes.Length); // request_data position

            // Atomically set sequence number and flag
            accessor.Write(8, currentSequence); // sequence_id
            accessor.Write(0, (uint)1); // request_flag = 1

            // Wait for response - use busy waiting to get the lowest latency
            long timeoutTime = startTime + timeoutMicroseconds;

            while (GetMicroseconds() < timeoutTime || DebugMode)
            {
                uint responseFlag = accessor.ReadUInt32(4); // response_flag position
                uint requestFlag = accessor.ReadUInt32(0); // requestFlag position

                if (responseFlag == 1 && requestFlag == 0)
                {
                    // Read response
                    uint responseSize = accessor.ReadUInt32(16); // response_size position
                    byte[] responseBytes = new byte[responseSize];
                    accessor.ReadArray(4116, responseBytes, 0, (int)responseSize); // response_data position

                    // Clear response flag
                    accessor.Write(4, (uint)0); // response_flag = 0

                    // Clear request data
                    byte[] emptyRequestBytes = new byte[requestBytes.Length];
                    accessor.WriteArray(20, emptyRequestBytes, 0, requestBytes.Length);

                    string response = Encoding.UTF8.GetString(responseBytes);

                    return response;
                }

                // Extremely short CPU yield, but maintains high responsiveness
                Thread.Yield();
            }

            throw new TimeoutException($"Request timed out ({timeoutMicroseconds / 1000_000.0} s)");
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Request sending failed: {ex.Message}");
            throw;
        }
    }

    public void Dispose()
    {
        if (!disposed)
        {
            accessor?.Dispose();
            mmf?.Dispose();

            if (bridgeProcess != null && !bridgeProcess.HasExited)
            {
                bridgeProcess.Kill();
                bridgeProcess.WaitForExit(1000);
            }
            bridgeProcess?.Dispose();

            disposed = true;
        }
    }
}
