// See https://aka.ms/new-console-template for more information
using System.Diagnostics;

using var pe32 = new PE32Proxy.PE32Proxy();

Console.WriteLine("=== UltraFastIPC Performance Test ===");
Console.WriteLine("1. Run Performance Testing");
Console.WriteLine("2. Run Robustness Testing");
Console.WriteLine("Other key to exit");

var input = Console.ReadKey();

if (input.Key == ConsoleKey.D1)
{
    Console.WriteLine();
    Console.WriteLine("Running performance test...");

    // Official performance test
    const int testCount = 100000;
    var stopwatch = Stopwatch.StartNew();
    var stopwatch2 = new Stopwatch();
    var times = new List<double>();
    for (int i = 0; i < testCount; i++)
    {
        stopwatch2.Restart();
        var result = pe32.it_api();
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
}
else if (input.Key == ConsoleKey.D2)
{
    Console.WriteLine();
    // Robustness test
    Console.WriteLine("Running robustness test...");
    // Please input the time you want to run the test (in hours)
    Console.WriteLine("Please input the time you want to run the test (in hours):");
    double time = double.Parse(Console.ReadLine());

    var cycleStopwatch = new Stopwatch();
    var startTime = DateTime.Now;
    ulong testCount = 1;

    while (GetRunTime(startTime) < TimeSpan.FromHours(time))
    {
        cycleStopwatch.Restart();
        var result = pe32.it_api();
        cycleStopwatch.Stop();
        testCount++;
        Console.WriteLine(
            $"Cycle {testCount} || Run Time: {GetRunTime(startTime)} || Result: {result:X} || Delay: {cycleStopwatch.ElapsedTicks * 1000000.0 / Stopwatch.Frequency:F1} us"
        );
    }
}

Console.WriteLine("=== End of test ===");
Console.WriteLine("Press any key to exit...");
Console.ReadKey();

static TimeSpan GetRunTime(DateTime startTime)
{
    return DateTime.Now - startTime;
}
