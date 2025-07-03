// See https://aka.ms/new-console-template for more information
using System.Diagnostics;

using var pe32 = new PE32Proxy.PE32Proxy();

// Official performance test
const int testCount = 100000;
var stopwatch = Stopwatch.StartNew();
var stopwatch2 = new Stopwatch();
var times = new List<double>();
for (int i = 0; i < testCount; i++)
{
    stopwatch2.Restart();
    var result = pe32.pe32_api();
    stopwatch2.Stop();
    times.Add((stopwatch2.ElapsedTicks * 1000000.0) / Stopwatch.Frequency);
    //Console.WriteLine(result.ToString("X"));
}

stopwatch.Stop();

double averageTime = (stopwatch.ElapsedTicks * 1000000.0) / (Stopwatch.Frequency * testCount);

Console.WriteLine(string.Join(" us\n ", times));
Console.WriteLine($"Average time: {times.Average():F1} us");

Console.WriteLine($"Performance test completed:");
Console.WriteLine($"Total number of requests: {testCount}");
Console.WriteLine($"Total time: {stopwatch.ElapsedMilliseconds} ms");
Console.WriteLine($"Average latency: {averageTime:F1} us");
Console.WriteLine($"QPS: {testCount * 1000.0 / stopwatch.ElapsedMilliseconds:F0}");
Console.WriteLine("=== End of test ===");
Console.WriteLine("Press any key to exit...");
Console.ReadKey();