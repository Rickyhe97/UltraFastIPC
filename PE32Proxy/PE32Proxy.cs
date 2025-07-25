﻿using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;

namespace PE32Proxy;

public class PE32Proxy : IDisposable
{
    private bool disposed = false;

    private readonly UltraFastIPCClient client;

    public int SerialNumber { get; private set; }

    public PE32Proxy(bool debugMode)
    {
        string exePath = "UltraFastIPC.exe";

        if (!File.Exists(exePath))
        {
            exePath = Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles),
                @"InfiniTest\PE32Proxy",
                "UltraFastIPC.exe"
            );
        }

        client = new UltraFastIPCClient(exePath) { DebugMode = debugMode };

        if (!client.StartBridgeProcess())
        {
            Console.WriteLine("Startup failed");
            return;
        }

        // Warm up the system
        for (int i = 0; i < 10; i++)
        {
            client.SendRequestUltraFast("warmup");
        }
    }

    public PE32Proxy()
        : this(false) { }

    public void Dispose()
    {
        Dispose(true);
        GC.SuppressFinalize(this);
    }

    protected virtual void Dispose(bool disposing)
    {
        if (!disposed)
        {
            if (disposing)
            {
                client?.Dispose();
            }
            disposed = true;
        }
    }

    private string SendRequest(string request, params object[] args)
    {
        string cmdStr = $"{request} {string.Join(" ", args)}".Trim();
        var response = client.SendRequestUltraFast(cmdStr);
        return response;
    }

    public void TestCommunication(string msg = "test")
    {
        string response = SendRequest(msg);
    }

    #region PE32 API

    public int Initialize()
    {
        it_api();
        int num = it_init();
        if (num <= 0)
        {
            throw new Exception("Failed to initialize the PE32H. Can't find any DigitalCard!");
        }

        for (int bdno = 1; bdno <= num; bdno++)
        {
            it_reset(bdno);
            SerialNumber = it_rd_pesno(bdno);
            it_set_pxi(bdno, 0);
            long num2 = 200L;
            long num3 = 0L;
            long num4 = 0L;
            it_set_rz(bdno, 0, 0);
            it_set_ro(bdno, 0, 0);
            it_set_tp(bdno, 1, (int)num2);
            num3 = 1L;
            num4 = 5L;
            it_set_tstart(bdno, 1, 1, (int)num3);
            it_set_tstop(bdno, 1, 1, (int)num4);
            num3 = 0L;
            num4 = num2;
            it_set_tstart(bdno, 2, 1, (int)num3);
            it_set_tstop(bdno, 2, 1, (int)num4);
            num3 = 0L;
            num4 = num2;
            it_set_tstart(bdno, 3, 1, (int)num3);
            it_set_tstop(bdno, 3, 1, (int)num4);
            it_set_vil(bdno, 0, 0.0);
            it_set_vih(bdno, 0, 1.8);
            it_cpu_df(bdno, 0, 0, 0);
            it_pmufv(bdno, 0, 0.0, 10.0);
            it_con_pmu(bdno, 0, 0);
            it_set_logmode(bdno, 1);
            it_set_addsyn(bdno, 0);
        }
        return num;
    }

    public int Initialize(int bdno)
    {
        it_api();
        int num = it_init();
        if (num <= 0)
        {
            throw new Exception("Failed to initialize the PE32H. Can't find any DigitalCard!");
        }

        if (bdno > num)
        {
            throw new Exception(
                "Failed to initialize the PE32H. The board number is out of range!"
            );
        }

        it_reset(bdno);
        SerialNumber = it_rd_pesno(bdno);
        it_set_pxi(bdno, 0);
        long num2 = 200L;
        long num3 = 0L;
        long num4 = 0L;
        it_set_rz(bdno, 0, 0);
        it_set_ro(bdno, 0, 0);
        it_set_tp(bdno, 1, (int)num2);
        num3 = 1L;
        num4 = 5L;
        it_set_tstart(bdno, 1, 1, (int)num3);
        it_set_tstop(bdno, 1, 1, (int)num4);
        num3 = 0L;
        num4 = num2;
        it_set_tstart(bdno, 2, 1, (int)num3);
        it_set_tstop(bdno, 2, 1, (int)num4);
        num3 = 0L;
        num4 = num2;
        it_set_tstart(bdno, 3, 1, (int)num3);
        it_set_tstop(bdno, 3, 1, (int)num4);
        it_set_vil(bdno, 0, 0.0);
        it_set_vih(bdno, 0, 1.8);
        it_cpu_df(bdno, 0, 0, 0);
        it_pmufv(bdno, 0, 0.0, 10.0);
        it_con_pmu(bdno, 0, 0);
        it_set_logmode(bdno, 1);
        it_set_addsyn(bdno, 0);
        return num;
    }

    public int it_api()
    {
        return int.Parse(SendRequest("pe32_api"));
    }

    public int it_init()
    {
        return int.Parse(SendRequest("pe32_init"));
    }

    public void it_reset(int bdno)
    {
        SendRequest("pe32_reset", bdno);
    }

    public void it_set_ftcnt(int bdno, int cnt)
    {
        SendRequest("pe32_set_ftcnt", bdno, cnt);
    }

    public void it_set_addbeg(int bdno, int add)
    {
        SendRequest("pe32_set_addbeg", bdno, add);
    }

    public void it_set_addend(int bdno, int add)
    {
        SendRequest("pe32_set_addend", bdno, add);
    }

    public void it_set_addif(int bdno, int add)
    {
        SendRequest("pe32_set_addif", bdno, add);
    }

    public void it_set_addsyn(int bdno, int add)
    {
        SendRequest("pe32_set_addsyn", bdno, add);
    }

    public void it_set_lmsyn_enb(int bdno, int onoff)
    {
        SendRequest("pe32_set_lmsyn_enb", bdno, onoff);
    }

    public void it_set_lmsyn_ch(int bdno, int ch)
    {
        SendRequest("pe32_set_lmsyn_ch", bdno, ch);
    }

    public void it_set_lmsyn_active_high(int bdno, int onoff)
    {
        SendRequest("pe32_set_lmsyn_active_high", bdno, onoff);
    }

    public void it_fstart(int bdno, int onoff)
    {
        SendRequest("pe32_fstart", bdno, onoff);
    }

    public void it_set_trigmode(int bdno, int onoff)
    {
        SendRequest("pe32_set_trigmode", bdno, onoff);
    }

    public int it_check_tprun(int bdno)
    {
        return int.Parse(SendRequest("pe32_check_tprun", bdno));
    }

    public int it_check_tpass(int bdno)
    {
        return int.Parse(SendRequest("pe32_check_tpass", bdno));
    }

    public void it_cycle(int bdno, int onoff)
    {
        SendRequest("pe32_cycle", bdno, onoff);
    }

    public int it_check_sync(int bdno)
    {
        return int.Parse(SendRequest("pe32_check_sync", bdno));
    }

    public int it_check_testbeg(int bdno)
    {
        return int.Parse(SendRequest("pe32_check_testbeg", bdno));
    }

    public int it_check_ftend(int bdno)
    {
        return int.Parse(SendRequest("pe32_check_ftend", bdno));
    }

    public int it_check_lend(int bdno)
    {
        return int.Parse(SendRequest("pe32_check_lend", bdno));
    }

    public void it_set_pxi(int bdno, int data)
    {
        SendRequest("pe32_set_pxi", bdno, data);
    }

    public void it_pxi_fstart(int bdno, int ch, int onoff)
    {
        SendRequest("pe32_pxi_fstart", bdno, ch, onoff);
    }

    public void it_pxi_cfail(int bdno, int ch, int onoff)
    {
        SendRequest("pe32_pxi_cfail", bdno, ch, onoff);
    }

    public void it_pxi_lmsyn(int bdno, int ch, int onoff)
    {
        SendRequest("pe32_pxi_lmsyn", bdno, ch, onoff);
    }

    public void it_set_seq(int bdno, int data)
    {
        SendRequest("pe32_set_seq", bdno, data);
    }

    public void it_set_lmf(int bdno, int data)
    {
        SendRequest("pe32_set_lmf", bdno, data);
    }

    public long it_rd_seq(int bdno)
    {
        return long.Parse(SendRequest("pe32_rd_seq", bdno));
    }

    public long it_rd_lmf(int bdno)
    {
        return long.Parse(SendRequest("pe32_rd_lmf", bdno));
    }

    public long it_rd_lmadd(int bdno)
    {
        return long.Parse(SendRequest("pe32_rd_lmadd", bdno));
    }

    public uint it_rd_fccnt(int bdno)
    {
        return uint.Parse(SendRequest("pe32_rd_fccnt", bdno));
    }

    public uint it_rd_flcnt(int bdno)
    {
        return uint.Parse(SendRequest("pe32_rd_flcnt", bdno));
    }

    public uint it_rd_ftcnt(int bdno)
    {
        return uint.Parse(SendRequest("pe32_rd_ftcnt", bdno));
    }

    public int it_check_checkmode(int bdno)
    {
        return int.Parse(SendRequest("pe32_check_checkmode", bdno));
    }

    public int it_check_dataready(int bdno)
    {
        return int.Parse(SendRequest("pe32_check_dataready", bdno));
    }

    public void it_set_checkmode(int bdno, int onoff)
    {
        SendRequest("pe32_set_checkmode", bdno, onoff);
    }

    public void it_set_logmode(int bdno, int onoff)
    {
        SendRequest("pe32_set_logmode", bdno, onoff);
    }

    public int it_rd_clog(int bdno, int addr)
    {
        return int.Parse(SendRequest("pe32_rd_clog", bdno, addr));
    }

    public int it_rd_alog(int bdno, int addr)
    {
        return int.Parse(SendRequest("pe32_rd_alog", bdno, addr));
    }

    public int it_rd_logadd(int bdno)
    {
        return int.Parse(SendRequest("pe32_rd_logadd", bdno));
    }

    public int it_rd_logcnt(int bdno)
    {
        return int.Parse(SendRequest("pe32_rd_logcnt", bdno));
    }

    public int it_rd_pesno(int bdno)
    {
        return int.Parse(SendRequest($"pe32_rd_pesno {bdno}"));
    }

    public double it_get_temp(int bdno, int cno)
    {
        return double.Parse(SendRequest("pe32_get_temp", bdno, cno));
    }

    public void it_dc_range(int bdno, int range)
    {
        SendRequest("pe32_dc_range", bdno, range);
    }

    public void it_set_tp(int bdno, int ts, int data)
    {
        SendRequest("pe32_set_tp", bdno, ts, data);
    }

    public void it_set_tstart(int bdno, int pno, int ts, int data)
    {
        SendRequest("pe32_set_tstart", bdno, pno, ts, data);
    }

    public void it_set_tstop(int bdno, int pno, int ts, int data)
    {
        SendRequest("pe32_set_tstop", bdno, pno, ts, data);
    }

    public void it_set_tstrob(int bdno, int pno, int ts, int data)
    {
        SendRequest("pe32_set_tstrob", bdno, pno, ts, data);
    }

    public void it_set_rz(int bdno, int fs, int data)
    {
        SendRequest("pe32_set_rz", bdno, fs, data);
    }

    public void it_set_ro(int bdno, int fs, int data)
    {
        SendRequest("pe32_set_ro", bdno, fs, data);
    }

    public int it_lmload(int begbdno, int boardwidth, int begadd, string patternfile)
    {
        if (!File.Exists(patternfile))
        {
            throw new FileNotFoundException(
                $"The Digital pattern PEZ file {patternfile} not found."
            );
        }

        return int.Parse(SendRequest("pe32_lmload", begbdno, boardwidth, begadd, patternfile));
    }

    public void it_set_qmode(int bdno, int onoff)
    {
        SendRequest("pe32_set_qmode", bdno, onoff);
    }

    public int it_check_qfail(int bdno, int cno)
    {
        return int.Parse(SendRequest("pe32_check_qfail", bdno, cno));
    }

    public void it_con_2k2vtt(int bdno, int pno, int onoff, double vtt)
    {
        SendRequest("pe32_con_2k2vtt", bdno, pno, onoff, vtt);
    }

    public void it_set_vih(int bdno, int pno, double rv)
    {
        SendRequest("pe32_set_vih", bdno, pno, rv);
    }

    public void it_set_vil(int bdno, int pno, double rv)
    {
        SendRequest("pe32_set_vil", bdno, pno, rv);
    }

    public void it_set_voh(int bdno, int pno, double rv)
    {
        SendRequest("pe32_set_voh", bdno, pno, rv);
    }

    public void it_set_vol(int bdno, int pno, double rv)
    {
        SendRequest("pe32_set_vol", bdno, pno, rv);
    }

    public void it_cpu_df(int bdno, int pno, int donoff, int fonoff)
    {
        SendRequest("pe32_cpu_df", bdno, pno, donoff, fonoff);
    }

    public void it_pmufv(int bdno, int chip, double rv, double clampi)
    {
        SendRequest("pe32_pmufv", bdno, chip, rv, clampi);
    }

    public void it_pmufi(int bdno, int chip, double ri, double cvh, double cvl)
    {
        SendRequest("pe32_pmufi", bdno, chip, ri, cvh, cvl);
    }

    public void it_pmufir(int bdno, int cno, double ri, double cvh, double cvl, int rang)
    {
        SendRequest("pe32_pmufir", bdno, cno, ri, cvh, cvl, rang);
    }

    public void it_pmucv(int bdno, int cno, double cvh, double cvl)
    {
        SendRequest("pe32_pmucv", bdno, cno, cvh, cvl);
    }

    public void it_pmuci(int bdno, int cno, double cih, double cil)
    {
        SendRequest("pe32_pmuci", bdno, cno, cih, cil);
    }

    public void it_con_pmu(int bdno, int pno, int onoff)
    {
        SendRequest("pe32_con_pmu", bdno, pno, onoff);
    }

    public void it_con_pmus(int bdno, int pno, int onoff)
    {
        SendRequest("pe32_con_pmus", bdno, pno, onoff);
    }

    public int it_check_pmu(int bdno, int cno)
    {
        return int.Parse(SendRequest("pe32_check_pmu", bdno, cno));
    }

    public double it_vmeas(int bdno, int pno)
    {
        return double.Parse(SendRequest("pe32_vmeas", bdno, pno));
    }

    public double it_imeas(int bdno, int pno)
    {
        return double.Parse(SendRequest("pe32_imeas", bdno, pno));
    }

    public int it_cal_load(int bdno, string calfile)
    {
        return int.Parse(SendRequest("pe32_cal_load", bdno, calfile));
    }

    public int it_cal_load_auto(int bdno, string calfile)
    {
        return int.Parse(SendRequest("pe32_cal_load_auto", bdno, calfile));
    }

    public void it_cal_reset(int bdno)
    {
        SendRequest("pe32_cal_reset", bdno);
    }

    public void it_set_rffemode(int bdno, int port, int onoff)
    {
        SendRequest("pe32_set_rffemode", bdno, port, onoff);
    }

    public void it_rffe_ftp(int bdno, int wtp, int rtp)
    {
        SendRequest("pe32_rffe_ftp", bdno, wtp, rtp);
    }

    public void it_rffe_wr(int bdno, int port, int sadd, int add, short data)
    {
        SendRequest("pe32_rffe_wr", bdno, port, sadd, add, data);
    }

    public int it_rffe_rd(int bdno, int port, int sadd, int add)
    {
        return int.Parse(SendRequest("pe32_rffe_rd", bdno, port, sadd, add));
    }

    public void it_rffe_ewr(int bdno, int port, int sadd, int add, int data, int Bcnt)
    {
        SendRequest("pe32_rffe_ewr", bdno, port, sadd, add, data, Bcnt);
    }

    public int it_rffe_erd(int bdno, int port, int sadd, int add, int Bcnt)
    {
        return int.Parse(SendRequest("pe32_rffe_erd", bdno, port, sadd, add, Bcnt));
    }

    public int it_rffe_getword(int bdno, int port)
    {
        return int.Parse(SendRequest("pe32_rffe_getword", bdno, port));
    }

    public void it_set_driver(int bdno, int pno, int onoff)
    {
        SendRequest("pe32_set_driver", bdno, pno, onoff);
    }

    public void it_set_rcvskew(int bdno, int pno, int rt)
    {
        SendRequest("pe32_set_rcvskew", bdno, pno, rt);
    }

    public void it_set_deskew(int bdno, int pno, int rt)
    {
        SendRequest("pe32_set_deskew", bdno, pno, rt);
    }

    public void it_rffe_wr0(int bdno, int port, int sadd, short data)
    {
        SendRequest("pe32_rffe_wr0", bdno, port, sadd, data);
    }

    #endregion
}
