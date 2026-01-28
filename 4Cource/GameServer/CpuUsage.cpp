#include "stdafx.h"

#include "CpuUsage.h"

CCpuUsage::CCpuUsage(HANDLE hProcess) {
    if (hProcess == INVALID_HANDLE_VALUE) {
        _hProcess = GetCurrentProcess();
    }
    else {
        _hProcess = hProcess;
    }

    SYSTEM_INFO SystemInfo;
    GetSystemInfo(&SystemInfo);
    _iNumberOfProcessors = SystemInfo.dwNumberOfProcessors;

    _fProcessorTotal = _fProcessorUser = _fProcessorKernel = 0;
    _fProcessTotal = _fProcessUser = _fProcessKernel = 0;

    _ftProcessor_LastKernel.QuadPart = 0;
    _ftProcessor_LastUser.QuadPart = 0;
    _ftProcessor_LastIdle.QuadPart = 0;
    _ftProcess_LastKernel.QuadPart = 0;
    _ftProcess_LastUser.QuadPart = 0;
    _ftProcess_LastTime.QuadPart = 0;

    UpdateCpuTime();
}

void CCpuUsage::UpdateCpuTime() {
    // 시스템 전체 CPU 사용률 계산
    ULARGE_INTEGER Idle, Kernel, User;
    if (!GetSystemTimes((PFILETIME)&Idle, (PFILETIME)&Kernel, (PFILETIME)&User)) {
        return;
    }

    ULONGLONG KernelDiff = Kernel.QuadPart - _ftProcessor_LastKernel.QuadPart;
    ULONGLONG UserDiff = User.QuadPart - _ftProcessor_LastUser.QuadPart;
    ULONGLONG IdleDiff = Idle.QuadPart - _ftProcessor_LastIdle.QuadPart;
    ULONGLONG Total = KernelDiff + UserDiff;

    if (Total == 0) {
        _fProcessorTotal = _fProcessorUser = _fProcessorKernel = 0.0f;
    }
    else {
        _fProcessorTotal = (float)((double)(Total - IdleDiff) / Total * 100.0f);
        _fProcessorUser = (float)((double)UserDiff / Total * 100.0f);
        _fProcessorKernel = (float)((double)(KernelDiff - IdleDiff) / Total * 100.0f);
    }

    _ftProcessor_LastKernel = Kernel;
    _ftProcessor_LastUser = User;
    _ftProcessor_LastIdle = Idle;

    // 특정 프로세스의 CPU 사용률 계산
    ULARGE_INTEGER None, NowTime;
    GetSystemTimeAsFileTime((LPFILETIME)&NowTime);
    GetProcessTimes(_hProcess, (LPFILETIME)&None, (LPFILETIME)&None, (LPFILETIME)&Kernel, (LPFILETIME)&User);

    ULONGLONG TimeDiff = NowTime.QuadPart - _ftProcess_LastTime.QuadPart;
    ULONGLONG ProcessUserDiff = User.QuadPart - _ftProcess_LastUser.QuadPart;
    ULONGLONG ProcessKernelDiff = Kernel.QuadPart - _ftProcess_LastKernel.QuadPart;
    ULONGLONG ProcessTotal = ProcessKernelDiff + ProcessUserDiff;

    if (TimeDiff > 0) {
        _fProcessTotal = (float)(ProcessTotal / (double)_iNumberOfProcessors / (double)TimeDiff * 100.0f);
        _fProcessKernel = (float)(ProcessKernelDiff / (double)_iNumberOfProcessors / (double)TimeDiff * 100.0f);
        _fProcessUser = (float)(ProcessUserDiff / (double)_iNumberOfProcessors / (double)TimeDiff * 100.0f);
    }

    _ftProcess_LastTime = NowTime;
    _ftProcess_LastKernel = Kernel;
    _ftProcess_LastUser = User;
}