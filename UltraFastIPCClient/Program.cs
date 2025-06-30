using System.Diagnostics;
using System.IO.MemoryMappedFiles;
using System.Runtime.InteropServices;
using System.Text;

// Shared memory layout structure - must be exactly the same as the C++ end
[StructLayout(LayoutKind.Sequential)]
public struct SharedMemoryLayout
{
    public uint request_flag;        // Request flag
    public uint response_flag;       // Response flag  
    public uint sequence_id;         // Sequence number
    public uint request_size;        // Request size
    public uint response_size;       // Response size

    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4096)]
    public byte[] request_data;      // Request data

    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4096)]
    public byte[] response_data;     // Response data

    public ulong last_request_time;  // Last request time
    public ulong last_response_time; // Last response time
    public uint total_requests;      // Total number of requests
}

public class UltraFastIPCClient : IDisposable
{
    private readonly string sharedMemoryName;
    private readonly string bridgeExecutablePath;
    private MemoryMappedFile mmf;
    private MemoryMappedViewAccessor accessor;
    private Process bridgeProcess;
    private uint sequenceCounter = 1;
    private bool disposed = false;

    // High precision timer
    [DllImport("kernel32.dll")]
    private static extern bool QueryPerformanceCounter(out long lpPerformanceCount);

    [DllImport("kernel32.dll")]
    private static extern bool QueryPerformanceFrequency(out long lpFrequency);

    private long performanceFrequency;

    public UltraFastIPCClient(string bridgeExePath, string sharedMemName = "UltraFastIPC_SharedMem")
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

    public async Task<bool> StartBridgeAsync()
    {
        try
        {
            Console.WriteLine("Starting 32-bit bridge process...");

            //ProcessStartInfo startInfo = new ProcessStartInfo
            //{
            //    FileName = bridgeExecutablePath,
            //    UseShellExecute = false,
            //    CreateNoWindow = false
            //};

            //bridgeProcess = Process.Start(startInfo);

            //if (bridgeProcess == null)
            //{
            //    throw new InvalidOperationException("Failed to start bridge process");
            //}

            // Wait for process initialization
            await Task.Delay(1000);

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
                throw new InvalidOperationException("Failed to connect to shared memory, please ensure the 32-bit program is running");
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Startup failed: {ex.Message}");
            return false;
        }
    }

    public async Task<string> SendRequestUltraFastAsync(string request, int timeoutMicroseconds = 1000)
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
            accessor.Write(12, (uint)requestBytes.Length);  // request_size position
            accessor.WriteArray(20, requestBytes, 0, requestBytes.Length);  // request_data position

            // Atomically set sequence number and flag
            accessor.Write(8, currentSequence);   // sequence_id
            accessor.Write(0, (uint)1);          // request_flag = 1

            // Wait for response - use busy waiting to get the lowest latency
            long timeoutTime = startTime + timeoutMicroseconds;

            while (GetMicroseconds() < timeoutTime)
            {
                uint responseFlag = accessor.ReadUInt32(4);  // response_flag position

                if (responseFlag == 1)
                {
                    // Read response
                    uint responseSize = accessor.ReadUInt32(16);  // response_size position
                    byte[] responseBytes = new byte[responseSize];
                    accessor.ReadArray(4116, responseBytes, 0, (int)responseSize);  // response_data position

                    // Clear response flag
                    accessor.Write(4, (uint)0);  // response_flag = 0

                    long endTime = GetMicroseconds();
                    long totalTime = endTime - startTime;

                    string response = Encoding.UTF8.GetString(responseBytes);

                    Console.WriteLine($"Request completed, total time: {totalTime} microseconds");

                    return response;
                }

                // Extremely short CPU yield, but maintains high responsiveness
                Thread.Yield();
            }

            throw new TimeoutException($"Request timed out ({timeoutMicroseconds} microseconds)");
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

// Performance test program
class Program
{
    static async Task Main(string[] args)
    {
        Console.WriteLine("=== Ultra-fast IPC test ===");

        using (var client = new UltraFastIPCClient(@"..\..\..\..\..\x64\Release\UltraFastIPC.exe"))
        {
            if (!await client.StartBridgeAsync())
            {
                Console.WriteLine("Startup failed");
                return;
            }

            Console.WriteLine("Starting performance test...");

            // Warm up the system
            for (int i = 0; i < 10; i++)
            {
                await client.SendRequestUltraFastAsync("warmup");
            }

            // Official performance test
            const int testCount = 1000;
            var stopwatch = Stopwatch.StartNew();
            var stopwatch2=new Stopwatch();
            var times = new List<double>();
            for (int i = 0; i < testCount; i++)
            {
                stopwatch2.Restart();
                await client.SendRequestUltraFastAsync($"test_{i}");
                stopwatch2.Stop();
                times.Add((stopwatch2.ElapsedTicks * 1000000.0) / Stopwatch.Frequency);
            }

            stopwatch.Stop();

            double averageTime = (stopwatch.ElapsedTicks * 1000000.0) / (Stopwatch.Frequency * testCount);

            Console.WriteLine(string.Join(" us\n ", times));

            Console.WriteLine($"Performance test completed:");
            Console.WriteLine($"Total number of requests: {testCount}");
            Console.WriteLine($"Total time: {stopwatch.ElapsedMilliseconds} ms");
            Console.WriteLine($"Average latency: {averageTime:F1} us");
            Console.WriteLine($"QPS: {testCount * 1000.0 / stopwatch.ElapsedMilliseconds:F0}");
            Console.WriteLine("=== End of test ===");
            Console.ReadLine();
        }
    }
}