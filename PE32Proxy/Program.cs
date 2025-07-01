using System.Diagnostics;

namespace PE32Proxy;

// Performance test program
internal class Program
{
    static void Main(string[] args)
    {
        Console.WriteLine("=== Ultra-fast IPC test ===");

        using (var client = new UltraFastIPCClient(@"..\..\..\..\Debug\UltraFastIPC.exe"))
        {
            if ( !client.StartBridgeProcess())
            {
                Console.WriteLine("Startup failed");
                return;
            }

            Console.WriteLine("Starting performance test...");

            // Warm up the system
            for (int i = 0; i < 10; i++)
            {
                client.SendRequestUltraFast("warmup");
            }

            // Official performance test
            const int testCount = 1000;
            var stopwatch = Stopwatch.StartNew();
            var stopwatch2 = new Stopwatch();
            var times = new List<double>();
            for (int i = 0; i < testCount; i++)
            {
                stopwatch2.Restart();
                var response = client.SendRequestUltraFast($"test_{i}");
                Console.WriteLine($"Response received: {response}");
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
