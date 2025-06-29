using System;
using System.Diagnostics;
using System.IO;
using System.IO.MemoryMappedFiles;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

// 共享内存布局结构 - 必须与C++端完全一致
[StructLayout(LayoutKind.Sequential)]
public struct SharedMemoryLayout
{
    public uint request_flag;        // 请求标志
    public uint response_flag;       // 响应标志  
    public uint sequence_id;         // 序列号
    public uint request_size;        // 请求大小
    public uint response_size;       // 响应大小

    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4096)]
    public byte[] request_data;      // 请求数据

    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4096)]
    public byte[] response_data;     // 响应数据

    public ulong last_request_time;  // 最后请求时间
    public ulong last_response_time; // 最后响应时间
    public uint total_requests;      // 总请求数
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

    // 高精度计时器
    [DllImport("kernel32.dll")]
    private static extern bool QueryPerformanceCounter(out long lpPerformanceCount);

    [DllImport("kernel32.dll")]
    private static extern bool QueryPerformanceFrequency(out long lpFrequency);

    private long performanceFrequency;

    public UltraFastIPCClient(string bridgeExePath, string sharedMemName = "UltraFastIPC_SharedMem")
    {
        this.bridgeExecutablePath = bridgeExePath;
        this.sharedMemoryName = sharedMemName;

        // 初始化高精度计时器
        QueryPerformanceFrequency(out performanceFrequency);

        Console.WriteLine("超高性能IPC客户端初始化完成");
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
            Console.WriteLine("启动32位桥接进程...");

            ProcessStartInfo startInfo = new ProcessStartInfo
            {
                FileName = bridgeExecutablePath,
                UseShellExecute = false,
                CreateNoWindow = false
            };

            bridgeProcess = Process.Start(startInfo);

            if (bridgeProcess == null)
            {
                throw new InvalidOperationException("无法启动桥接进程");
            }

            // 等待进程初始化
            await Task.Delay(1000);

            // 连接到共享内存
            try
            {
                mmf = MemoryMappedFile.OpenExisting(sharedMemoryName);
                accessor = mmf.CreateViewAccessor(0, Marshal.SizeOf<SharedMemoryLayout>());

                Console.WriteLine("成功连接到共享内存");
                return true;
            }
            catch (FileNotFoundException)
            {
                throw new InvalidOperationException("无法连接到共享内存，请确保32位程序正常运行");
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"启动失败: {ex.Message}");
            return false;
        }
    }

    public async Task<string> SendRequestUltraFastAsync(string request, int timeoutMicroseconds = 1000)
    {
        if (accessor == null)
            throw new InvalidOperationException("IPC客户端未初始化");

        long startTime = GetMicroseconds();

        try
        {
            // 准备请求数据
            byte[] requestBytes = Encoding.UTF8.GetBytes(request);
            if (requestBytes.Length > 4096)
                throw new ArgumentException("请求数据过大");

            uint currentSequence = ++sequenceCounter;

            // 写入请求到共享内存 - 这些操作都是内存级别的，极其快速
            accessor.Write(12, (uint)requestBytes.Length);  // request_size位置
            accessor.WriteArray(20, requestBytes, 0, requestBytes.Length);  // request_data位置

            // 原子性地设置序列号和标志
            accessor.Write(8, currentSequence);   // sequence_id
            accessor.Write(0, (uint)1);          // request_flag = 1

            // 等待响应 - 使用忙等待以获得最低延迟
            long timeoutTime = startTime + timeoutMicroseconds;

            while (GetMicroseconds() < timeoutTime)
            {
                uint responseFlag = accessor.ReadUInt32(4);  // response_flag位置

                if (responseFlag == 1)
                {
                    // 读取响应
                    uint responseSize = accessor.ReadUInt32(16);  // response_size位置
                    byte[] responseBytes = new byte[responseSize];
                    accessor.ReadArray(4116, responseBytes, 0, (int)responseSize);  // response_data位置

                    // 清除响应标志
                    accessor.Write(4, (uint)0);  // response_flag = 0

                    long endTime = GetMicroseconds();
                    long totalTime = endTime - startTime;

                    string response = Encoding.UTF8.GetString(responseBytes);

                    Console.WriteLine($"请求完成，总耗时: {totalTime} 微秒");

                    return response;
                }

                // 极短暂的让出CPU，但保持高响应性
                Thread.Yield();
            }

            throw new TimeoutException($"请求超时 ({timeoutMicroseconds} 微秒)");
        }
        catch (Exception ex)
        {
            Console.WriteLine($"发送请求失败: {ex.Message}");
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

// 性能测试程序
class Program
{
    static async Task Main(string[] args)
    {
        Console.WriteLine("=== 超高性能IPC测试 ===");

        using (var client = new UltraFastIPCClient(@"C:\Repository\Demo\UltraFastIPC\x64\Debug\UltraFastIPC.exe"))
        {
            if (!await client.StartBridgeAsync())
            {
                Console.WriteLine("启动失败");
                return;
            }

            Console.WriteLine("开始性能测试...");

            // 预热系统
            for (int i = 0; i < 10; i++)
            {
                await client.SendRequestUltraFastAsync("warmup");
            }

            // 正式性能测试
            const int testCount = 1000;
            var stopwatch = Stopwatch.StartNew();

            for (int i = 0; i < testCount; i++)
            {
                await client.SendRequestUltraFastAsync($"test_{i}");
            }

            stopwatch.Stop();

            double averageTime = (stopwatch.ElapsedTicks * 1000000.0) / (Stopwatch.Frequency * testCount);

            Console.WriteLine($"性能测试完成:");
            Console.WriteLine($"总请求数: {testCount}");
            Console.WriteLine($"总耗时: {stopwatch.ElapsedMilliseconds} 毫秒");
            Console.WriteLine($"平均延迟: {averageTime:F1} 微秒");
            Console.WriteLine($"QPS: {testCount * 1000.0 / stopwatch.ElapsedMilliseconds:F0}");
        }
    }
}