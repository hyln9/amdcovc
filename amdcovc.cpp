/*
 *  AMDCOVC - AMD Console OVerdrive control utility
 *  Copyright (C) 2016 Mateusz Szpakowski
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#define _BSD_SOURCE
#include <iostream>
#include <exception>
#include <dlfcn.h>
#include <fstream>
#include <algorithm>
#include <cstdio>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <string>
#include <memory>
#include <cstdarg>
#include <cmath>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <CL/cl.h>
extern "C" {
#include <pci/pci.h>
}

#ifdef __linux__
#define LINUX 1
#endif
#include "../include/adl_sdk.h"

// Memory allocation function
void* __stdcall ADL_Main_Memory_Alloc (int iSize)
{
    void* lpBuffer = malloc (iSize);
    return lpBuffer;
}

// Optional Memory de-allocation function
void __stdcall ADL_Main_Memory_Free (void** lpBuffer)
{
    if (nullptr != *lpBuffer)
    {
        free (*lpBuffer);
        *lpBuffer = nullptr;
    }
}

class Error: public std::exception
{
private:
    std::string description;
public:
    explicit Error(const char* _description) : description(_description)
    { }
    Error(int error, const char* _description)
    {
        char errorBuf[32];
        snprintf(errorBuf, 32, "code %d: ", error);
        description = errorBuf;
        description += _description;
    }
    virtual ~Error() noexcept
    { }
    const char* what() const noexcept
    { return description.c_str(); }
};

class ATIADLHandle
{
private:
    typedef int (*ADL_ConsoleMode_FileDescriptor_Set_T)(int fileDescriptor);
    typedef int (*ADL_Main_Control_Create_T)(ADL_MAIN_MALLOC_CALLBACK, int);
    typedef int (*ADL_Main_Control_Destroy_T)();
    typedef int (*ADL_Adapter_NumberOfAdapters_Get_T)(int* numAdapters);
    typedef int (*ADL_Adapter_Active_Get_T)(int adapterIndex, int *status);
    typedef int (*ADL_Adapter_AdapterInfo_Get_T)(LPAdapterInfo info, int inputSize);
    
    typedef int (*ADL_Overdrive5_CurrentActivity_Get_T)(int adapterIndex,
                ADLPMActivity* activity);
    typedef int (*ADL_Overdrive5_Temperature_Get_T)(int adapterIndex, int thermalCtrlIndex,
                ADLTemperature *temperature);
    typedef int (*ADL_Overdrive5_FanSpeedInfo_Get_T)(int adapterIndex, int thermalCtrlIndex,
                ADLFanSpeedInfo* fanSpeedInfo);
    typedef int (*ADL_Overdrive5_FanSpeed_Get_T)(int adapterIndex, int thermalCtrlIndex,
                ADLFanSpeedValue* fanSpeedValue);
    typedef int (*ADL_Overdrive5_ODParameters_Get_T)(int adapterIndex,
                ADLODParameters* odParameters);
    typedef int (*ADL_Overdrive5_ODPerformanceLevels_Get_T)(int adapterIndex, int idefault,
                ADLODPerformanceLevels* odPerformanceLevels);
    
    typedef int (*ADL_Overdrive5_FanSpeed_Set_T)(int adapterIndex, int thermalCtrlIndex,
                ADLFanSpeedValue* fanSpeedValue);
    typedef int (*ADL_Overdrive5_FanSpeedToDefault_Set_T)(int adapterIndex,
                int thermalCtrlIndex);
    typedef int (*ADL_Overdrive5_ODPerformanceLevels_Set_T)(int adapterIndex,
                ADLODPerformanceLevels* odPerformanceLevels);
    
    void* handle;
    void* getSym(const char* name);
    
    ADL_Main_Control_Create_T pADL_Main_Control_Create;
    ADL_Main_Control_Destroy_T pADL_Main_Control_Destroy;
    ADL_ConsoleMode_FileDescriptor_Set_T pADL_ConsoleMode_FileDescriptor_Set;
    ADL_Adapter_NumberOfAdapters_Get_T pADL_Adapter_NumberOfAdapters_Get;
    ADL_Adapter_Active_Get_T pADL_Adapter_Active_Get;
    ADL_Adapter_AdapterInfo_Get_T pADL_Adapter_AdapterInfo_Get;
    ADL_Overdrive5_CurrentActivity_Get_T pADL_Overdrive5_CurrentActivity_Get;
    ADL_Overdrive5_Temperature_Get_T pADL_Overdrive5_Temperature_Get;
    ADL_Overdrive5_FanSpeedInfo_Get_T pADL_Overdrive5_FanSpeedInfo_Get;
    ADL_Overdrive5_FanSpeed_Get_T pADL_Overdrive5_FanSpeed_Get;
    ADL_Overdrive5_ODParameters_Get_T pADL_Overdrive5_ODParameters_Get;
    ADL_Overdrive5_ODPerformanceLevels_Get_T pADL_Overdrive5_ODPerformanceLevels_Get;
    ADL_Overdrive5_FanSpeed_Set_T pADL_Overdrive5_FanSpeed_Set;
    ADL_Overdrive5_FanSpeedToDefault_Set_T pADL_Overdrive5_FanSpeedToDefault_Set;
    ADL_Overdrive5_ODPerformanceLevels_Set_T pADL_Overdrive5_ODPerformanceLevels_Set;
    
public:
    ATIADLHandle();
    ~ATIADLHandle();
    
    void Main_Control_Create(ADL_MAIN_MALLOC_CALLBACK callback,
                             int iEnumConnectedAdapters) const;
    
    void Main_Control_Destroy() const;
    void ConsoleMode_FileDescriptor_Set(int fileDescriptor) const;
    void Adapter_NumberOfAdapters_Get(int* number) const;
    void Adapter_Active_Get(int adapterIndex, int* status) const;
    void Adapter_Info_Get(LPAdapterInfo info, int inputSize) const;
    void Overdrive5_CurrentActivity_Get(int adapterIndex, ADLPMActivity* activity) const;
    void Overdrive5_Temperature_Get(int adapterIndex, int thermalCtrlIndex,
                ADLTemperature* temperature) const;
    void Overdrive5_FanSpeedInfo_Get(int adapterIndex, int thermalCtrlIndex,
                ADLFanSpeedInfo* fanSpeedInfo) const;
    void Overdrive5_FanSpeed_Get(int adapterIndex, int thermalCtrlIndex,
                ADLFanSpeedValue* fanSpeedValue) const;
    void Overdrive5_ODParameters_Get(int adapterIndex, ADLODParameters* odParameters) const;
    void Overdrive5_ODPerformanceLevels_Get(int adapterIndex, int idefault,
                ADLODPerformanceLevels* odPerformanceLevels) const;
    void Overdrive5_FanSpeed_Set(int adapterIndex, int thermalCtrlIndex,
                ADLFanSpeedValue* fanSpeedValue) const;
    void Overdrive5_FanSpeedToDefault_Set(int adapterIndex, int thermalCtrlIndex) const;
    void Overdrive5_ODPerformanceLevels_Set(int adapterIndex,
                ADLODPerformanceLevels* odPerformanceLevels) const;
};

ATIADLHandle::ATIADLHandle() 
try : handle(nullptr),
    pADL_Main_Control_Create(nullptr), pADL_Main_Control_Destroy(nullptr),
    pADL_ConsoleMode_FileDescriptor_Set(nullptr),
    pADL_Adapter_NumberOfAdapters_Get(nullptr), pADL_Adapter_Active_Get(nullptr),
    pADL_Adapter_AdapterInfo_Get(nullptr), pADL_Overdrive5_CurrentActivity_Get(nullptr),
    pADL_Overdrive5_Temperature_Get(nullptr), pADL_Overdrive5_FanSpeedInfo_Get(nullptr),
    pADL_Overdrive5_FanSpeed_Get(nullptr), pADL_Overdrive5_ODParameters_Get(nullptr),
    pADL_Overdrive5_ODPerformanceLevels_Get(nullptr),
    pADL_Overdrive5_FanSpeed_Set(nullptr),
    pADL_Overdrive5_FanSpeedToDefault_Set(nullptr),
    pADL_Overdrive5_ODPerformanceLevels_Set(nullptr)
{
    dlerror(); // clear old errors
    handle = dlopen("libatiadlxx.so", RTLD_LAZY|RTLD_GLOBAL);
    if (handle == nullptr)
        throw Error(dlerror());
    
    pADL_Main_Control_Create = (ADL_Main_Control_Create_T)
                getSym("ADL_Main_Control_Create");
    pADL_Main_Control_Destroy = (ADL_Main_Control_Destroy_T)
                getSym("ADL_Main_Control_Destroy");
    pADL_ConsoleMode_FileDescriptor_Set = (ADL_ConsoleMode_FileDescriptor_Set_T)
                getSym("ADL_ConsoleMode_FileDescriptor_Set");
    pADL_Adapter_NumberOfAdapters_Get = (ADL_Adapter_NumberOfAdapters_Get_T)
                getSym("ADL_Adapter_NumberOfAdapters_Get");
    pADL_Adapter_Active_Get = (ADL_Adapter_Active_Get_T)
                getSym("ADL_Adapter_Active_Get");
    pADL_Adapter_AdapterInfo_Get = (ADL_Adapter_AdapterInfo_Get_T)
                getSym("ADL_Adapter_AdapterInfo_Get");
    pADL_Overdrive5_CurrentActivity_Get = (ADL_Overdrive5_CurrentActivity_Get_T)
                getSym("ADL_Overdrive5_CurrentActivity_Get");
    pADL_Overdrive5_Temperature_Get = (ADL_Overdrive5_Temperature_Get_T)
                getSym("ADL_Overdrive5_Temperature_Get");
    pADL_Overdrive5_FanSpeedInfo_Get = (ADL_Overdrive5_FanSpeedInfo_Get_T)
                getSym("ADL_Overdrive5_FanSpeedInfo_Get");
    pADL_Overdrive5_FanSpeed_Get = (ADL_Overdrive5_FanSpeed_Get_T)
                getSym("ADL_Overdrive5_FanSpeed_Get");
    pADL_Overdrive5_ODParameters_Get = (ADL_Overdrive5_ODParameters_Get_T)
                getSym("ADL_Overdrive5_ODParameters_Get");
    pADL_Overdrive5_ODPerformanceLevels_Get = (ADL_Overdrive5_ODPerformanceLevels_Get_T)
                getSym("ADL_Overdrive5_ODPerformanceLevels_Get");
    pADL_Overdrive5_FanSpeed_Set = (ADL_Overdrive5_FanSpeed_Set_T)
                getSym("ADL_Overdrive5_FanSpeed_Set");
    pADL_Overdrive5_FanSpeedToDefault_Set = (ADL_Overdrive5_FanSpeedToDefault_Set_T)
                getSym("ADL_Overdrive5_FanSpeedToDefault_Set");
    pADL_Overdrive5_ODPerformanceLevels_Set = (ADL_Overdrive5_ODPerformanceLevels_Set_T)
                getSym("ADL_Overdrive5_ODPerformanceLevels_Set");
}
catch(...)
{
    if (handle != nullptr)
    {
        dlerror(); // clear old errors
        if (dlclose(handle)) // if closing failed
            throw Error(dlerror());
    }
    throw;
}

ATIADLHandle::~ATIADLHandle()
{
    if (handle != nullptr)
    {
        dlerror();
        if (dlclose(handle))
            throw Error(dlerror());
    }
}

void* ATIADLHandle::getSym(const char* symbolName)
{
    void* symbol = nullptr;
    dlerror(); // clear old errors
    symbol = dlsym(handle, symbolName);
    const char* error = dlerror();
    if (symbol == nullptr && error != nullptr)
        throw Error(error);
    return symbol;
}

void ATIADLHandle::Main_Control_Create(ADL_MAIN_MALLOC_CALLBACK callback,
                            int iEnumConnectedAdapters) const
{
    int error = pADL_Main_Control_Create(callback, iEnumConnectedAdapters);
    if (error != ADL_OK)
        throw Error(error, "ADL_Main_Control_Create error");
}

void ATIADLHandle::Main_Control_Destroy() const
{
    int error = pADL_Main_Control_Destroy();
    if (error != ADL_OK)
        throw Error(error, "ADL_Main_Control_Destroy error");
}

void ATIADLHandle::ConsoleMode_FileDescriptor_Set(int fileDescriptor) const
{
    int error = pADL_ConsoleMode_FileDescriptor_Set(fileDescriptor);
    if (error != ADL_OK)
        throw Error(error, "ADL_ConsoleMode_FileDescriptor_Set error");
}

void ATIADLHandle::Adapter_NumberOfAdapters_Get(int* number) const
{
    int error = pADL_Adapter_NumberOfAdapters_Get(number);
    if (error != ADL_OK)
        throw Error(error, "ADL_Adapter_NumberOfAdapters_Get error");
}

void ATIADLHandle::Adapter_Active_Get(int adapterIndex, int* status) const
{
    int error = pADL_Adapter_Active_Get(adapterIndex, status);
    if (error != ADL_OK)
        throw Error(error, "ADL_Adapter_Active_Get error");
}

void ATIADLHandle::Adapter_Info_Get(LPAdapterInfo info, int inputSize) const
{
    int error = pADL_Adapter_AdapterInfo_Get(info, inputSize);
    if (error != ADL_OK)
        throw Error(error, "ADL_AdapterInfo_Get error");
}

void ATIADLHandle::Overdrive5_CurrentActivity_Get(int adapterIndex,
                ADLPMActivity* activity) const
{
    int error = pADL_Overdrive5_CurrentActivity_Get(adapterIndex, activity);
    if (error != ADL_OK)
        throw Error(error, "ADL_Overdrive5_CurrentActivity_Get error");
}

void ATIADLHandle::Overdrive5_Temperature_Get(int adapterIndex, int thermalCtrlIndex,
                ADLTemperature *temperature) const
{
    int error = pADL_Overdrive5_Temperature_Get(adapterIndex, thermalCtrlIndex,
                    temperature);
    if (error != ADL_OK)
        throw Error(error, "ADL_Overdrive5_Temperature_Get error");
}

void ATIADLHandle::Overdrive5_FanSpeedInfo_Get(int adapterIndex, int thermalCtrlIndex,
                ADLFanSpeedInfo* fanSpeedInfo) const
{
    int error = pADL_Overdrive5_FanSpeedInfo_Get(adapterIndex, thermalCtrlIndex,
                    fanSpeedInfo);
    if (error != ADL_OK)
        throw Error(error, "ADL_Overdrive5_FanSpeedInfo_Get error");
}

void ATIADLHandle::Overdrive5_FanSpeed_Get(int adapterIndex, int thermalCtrlIndex,
                ADLFanSpeedValue* fanSpeedValue) const
{
    int error = pADL_Overdrive5_FanSpeed_Get(adapterIndex, thermalCtrlIndex,
                    fanSpeedValue);
    if (error != ADL_OK)
        throw Error(error, "ADL_Overdrive5_FanSpeed_Get error");
}

void ATIADLHandle::Overdrive5_ODParameters_Get(int adapterIndex,
                ADLODParameters* odParameters) const
{
    int error = pADL_Overdrive5_ODParameters_Get(adapterIndex, odParameters);
    if (error != ADL_OK)
        throw Error(error, "ADL_Overdrive5_ODParameters_Get error");
}

void ATIADLHandle::Overdrive5_ODPerformanceLevels_Get(int adapterIndex, int idefault,
                ADLODPerformanceLevels* odPerformanceLevels) const
{
    int error = pADL_Overdrive5_ODPerformanceLevels_Get(adapterIndex, idefault,
                    odPerformanceLevels);
    if (error != ADL_OK)
        throw Error(error, "ADL_Overdrive5_ODPerformanceLevels_Get error");
}

void ATIADLHandle::Overdrive5_FanSpeed_Set(int adapterIndex, int thermalCtrlIndex,
                ADLFanSpeedValue* fanSpeedValue) const
{
    int error = pADL_Overdrive5_FanSpeed_Set(adapterIndex, thermalCtrlIndex,
                    fanSpeedValue);
    if (error != ADL_OK)
        throw Error(error, "ADL_Overdrive5_FanSpeed_Set error");
}

void ATIADLHandle::Overdrive5_FanSpeedToDefault_Set(int adapterIndex,
                int thermalCtrlIndex) const
{
    int error = pADL_Overdrive5_FanSpeedToDefault_Set(adapterIndex, thermalCtrlIndex);
    if (error != ADL_OK)
        throw Error(error, "ADL_Overdrive5_FanSpeedToDefault_Set error");
}

void ATIADLHandle::Overdrive5_ODPerformanceLevels_Set(int adapterIndex,
                ADLODPerformanceLevels* odPerformanceLevels) const
{
    int error = pADL_Overdrive5_ODPerformanceLevels_Set(adapterIndex, odPerformanceLevels);
    if (error != ADL_OK)
        throw Error(error, "ADL_Overdrive5_ODPerformanceLevels_Set error");
}

class ADLMainControl
{
private:
    const ATIADLHandle& handle;
    int fd;
    bool mainControlCreated;
    bool withX;
public:
    explicit ADLMainControl(const ATIADLHandle& handle, int devId);
    ~ADLMainControl();
    
    int getAdaptersNum() const;
    bool isAdapterActive(int adapterIndex) const;
    void getAdapterInfo(AdapterInfo* infos) const;
    void getCurrentActivity(int adapterIndex, ADLPMActivity& activity) const;
    int getTemperature(int adapterIndex, int thermalCtrlIndex) const;
    void getFanSpeedInfo(int adapterIndex, int thermalCtrlIndex,
            ADLFanSpeedInfo& info) const;
    int getFanSpeed(int adapterIndex, int thermalCtrlIndex) const;
    void getODParameters(int adapterIndex, ADLODParameters& odParameters) const;
    void getODPerformanceLevels(int adapterIndex, bool isDefault, int perfLevelsNum,
            ADLODPerformanceLevel* perfLevels) const;
    void setFanSpeed(int adapterIndex, int thermalCtrlIndex, int fanSpeed) const;
    void setFanSpeedToDefault(int adapterIndex, int thermalCtrlIndex) const;
    void setODPerformanceLevels(int adapterIndex, int perfLevelsNum,
            ADLODPerformanceLevel* perfLevels) const;
};

ADLMainControl::ADLMainControl(const ATIADLHandle& _handle, int devId)
try : handle(_handle), fd(-1), mainControlCreated(false), withX(true)
{
    try
    { handle.Main_Control_Create(ADL_Main_Memory_Alloc, 0); }
    catch(const Error& error)
    {
        if (getuid()!=0)
            std::cout << "IMPORTANT: This program requires root privileges to be "
                    "working correctly\nif no running X11 server." << std::endl;
    
        withX = false;
        char devName[64];
        snprintf(devName, 64, "/dev/ati/card%u", devId);
        errno = 0;
        fd = open(devName, O_RDWR);
        if (fd==-1)
        {
            cl_uint platformsNum;
            /// force initializing these stupid devices
            clGetPlatformIDs(0, nullptr, &platformsNum);
            errno = 0;
            fd = open(devName, O_RDWR);
            if (fd==-1)
                throw Error(errno, "Can't open GPU device");
        }
        
        handle.ConsoleMode_FileDescriptor_Set(fd);
        handle.Main_Control_Create(ADL_Main_Memory_Alloc, 0);
    }
}
catch(...)
{
    if (mainControlCreated)
        handle.Main_Control_Destroy();
    if (fd!=-1)
        close(fd);
    throw;
}

ADLMainControl::~ADLMainControl()
{
    if (fd!=-1)
        close(fd);
}

int ADLMainControl::getAdaptersNum() const
{
    int num = 0;
    handle.Adapter_NumberOfAdapters_Get(&num);
    return num;
}

bool ADLMainControl::isAdapterActive(int adapterIndex) const
{
    if (!withX) return true;
    int status = 0;
    handle.Adapter_Active_Get(adapterIndex, &status);
    return status == ADL_TRUE;
}

void ADLMainControl::getAdapterInfo(AdapterInfo* infos) const
{
    int num;
    handle.Adapter_NumberOfAdapters_Get(&num);
    for (int i = 0; i < num; i++)
        infos[i].iSize = sizeof(AdapterInfo);
    handle.Adapter_Info_Get(infos, num*sizeof(AdapterInfo));
}

void ADLMainControl::getCurrentActivity(int adapterIndex, ADLPMActivity& activity) const
{
    activity.iSize = sizeof(ADLPMActivity);
    handle.Overdrive5_CurrentActivity_Get(adapterIndex, &activity);
}

int ADLMainControl::getTemperature(int adapterIndex, int thermalCtrlIndex) const
{
    ADLTemperature temp;
    temp.iSize = sizeof(ADLTemperature);
    handle.Overdrive5_Temperature_Get(adapterIndex, thermalCtrlIndex, &temp);
    return temp.iTemperature;
}

void ADLMainControl::getFanSpeedInfo(int adapterIndex, int thermalCtrlIndex,
                ADLFanSpeedInfo& info) const
{
    info.iSize = sizeof(ADLFanSpeedInfo);
    handle.Overdrive5_FanSpeedInfo_Get(adapterIndex, thermalCtrlIndex, &info);
}

int ADLMainControl::getFanSpeed(int adapterIndex, int thermalCtrlIndex) const
{
    ADLFanSpeedValue fanSpeedValue;
    fanSpeedValue.iSpeedType = ADL_DL_FANCTRL_SPEED_TYPE_PERCENT;
    fanSpeedValue.iFlags = 0;
    fanSpeedValue.iSize = sizeof(ADLFanSpeedValue);
    handle.Overdrive5_FanSpeed_Get(adapterIndex, thermalCtrlIndex, &fanSpeedValue);
    return fanSpeedValue.iFanSpeed;
}

void ADLMainControl::getODParameters(int adapterIndex, ADLODParameters& odParameters) const
{
    odParameters.iSize = sizeof(ADLODParameters);
    handle.Overdrive5_ODParameters_Get(adapterIndex, &odParameters);
}

void ADLMainControl::getODPerformanceLevels(int adapterIndex, bool isDefault,
            int perfLevelsNum, ADLODPerformanceLevel* perfLevels) const
{
    const size_t odPLBufSize = sizeof(ADLODPerformanceLevels)+
                    sizeof(ADLODPerformanceLevel)*(perfLevelsNum-1);
    std::unique_ptr<char[]> odPlBuf(new char[odPLBufSize]);
    ADLODPerformanceLevels* odPLevels = (ADLODPerformanceLevels*)odPlBuf.get();
    odPLevels->iSize = odPLBufSize;
    handle.Overdrive5_ODPerformanceLevels_Get(adapterIndex, isDefault, odPLevels);
    std::copy(odPLevels->aLevels, odPLevels->aLevels+perfLevelsNum, perfLevels);
}

void ADLMainControl::setFanSpeed(int adapterIndex, int thermalCtrlIndex, int fanSpeed) const
{
    ADLFanSpeedValue fanSpeedValue;
    fanSpeedValue.iSize = sizeof(ADLFanSpeedValue);
    fanSpeedValue.iSpeedType = ADL_DL_FANCTRL_SPEED_TYPE_PERCENT;
    fanSpeedValue.iFanSpeed = fanSpeed;
    handle.Overdrive5_FanSpeed_Set(adapterIndex, thermalCtrlIndex, &fanSpeedValue);
}

void ADLMainControl::setFanSpeedToDefault(int adapterIndex, int thermalCtrlIndex) const
{
    handle.Overdrive5_FanSpeedToDefault_Set(adapterIndex, thermalCtrlIndex);
}

void ADLMainControl::setODPerformanceLevels(int adapterIndex, int perfLevelsNum,
            ADLODPerformanceLevel* perfLevels) const
{
    const size_t odPLBufSize = sizeof(ADLODPerformanceLevels)+
                    sizeof(ADLODPerformanceLevel)*(perfLevelsNum-1);
    std::unique_ptr<char[]> odPlBuf(new char[odPLBufSize]);
    ADLODPerformanceLevels* odPLevels = (ADLODPerformanceLevels*)odPlBuf.get();
    odPLevels->iSize = odPLBufSize;
    odPLevels->iReserved = 0;
    std::copy(perfLevels, perfLevels+perfLevelsNum, odPLevels->aLevels);
    handle.Overdrive5_ODPerformanceLevels_Set(adapterIndex, odPLevels);
}

static pci_access* pciAccess = nullptr;
static pci_filter pciFilter;

static void pciAccessError(char* msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    vprintf(msg, ap);
    va_end(ap);
    exit(-1);
}

static void initializePCIAccess()
{
    pciAccess = pci_alloc();
    if (pciAccess==nullptr)
        throw Error("Can't allocate PCIAccess");
    pciAccess->error = pciAccessError;
    pci_filter_init(pciAccess, &pciFilter);
    pci_init(pciAccess);
    pci_scan_bus(pciAccess);
}

static void getFromPCI(int deviceIndex, AdapterInfo& adapterInfo)
{
    if (pciAccess==nullptr)
        initializePCIAccess();
    char fnameBuf[64];
    snprintf(fnameBuf, 64, "/proc/ati/%u/name", deviceIndex);
    std::string tmp, pciBusStr;
    {
        std::ifstream procNameIs(fnameBuf);
        procNameIs.exceptions(std::ios::badbit|std::ios::failbit);
        procNameIs >> tmp >> tmp >> pciBusStr;
    }
    unsigned int busNum, devNum, funcNum;
    if (pciBusStr.size() < 9)
        throw Error("Wrong PCI Bus string");
    char* pciStrPtr = (char*)pciBusStr.data()+4;
    char* pciStrPtrNew;
    errno  = 0;
    busNum = strtoul(pciStrPtr, &pciStrPtrNew, 10);
    if (errno!=0 || pciStrPtr==pciStrPtrNew)
        throw Error(errno, "Can't parse BusID");
    pciStrPtr = pciStrPtrNew+1;
    errno  = 0;
    devNum = strtoul(pciStrPtr, &pciStrPtr, 10);
    if (errno!=0 || pciStrPtr==pciStrPtrNew)
        throw Error(errno, "Can't parse DevID");
    pciStrPtr = pciStrPtrNew+1;
    errno  = 0;
    funcNum = strtoul(pciStrPtr, &pciStrPtr, 10);
    if (errno!=0 || pciStrPtr==pciStrPtrNew)
        throw Error(errno, "Can't parse FuncID");
    pci_dev* dev = pciAccess->devices;
    for (; dev!=nullptr; dev=dev->next)
        if (dev->bus==busNum && dev->dev==devNum && dev->func==funcNum)
        {
            char deviceBuf[128];
            deviceBuf[0] = 0;
            pci_lookup_name(pciAccess, deviceBuf, 128, PCI_LOOKUP_DEVICE,
                    dev->vendor_id, dev->device_id);
            adapterInfo.iBusNumber = busNum;
            adapterInfo.iDeviceNumber = devNum;
            adapterInfo.iFunctionNumber = funcNum;
            strcpy(adapterInfo.strAdapterName, deviceBuf);
            break;
        }
}

static void getActiveAdaptersIndices(ADLMainControl& mainControl, int adaptersNum,
                    std::vector<int>& activeAdapters)
{
    activeAdapters.clear();
    for (int i = 0; i < adaptersNum; i++)
        if (mainControl.isAdapterActive(i))
            activeAdapters.push_back(i);
}

static void printAdaptersInfo(ADLMainControl& mainControl, int adaptersNum,
            const std::vector<int>& activeAdapters,
            const std::vector<int>& choosenAdapters, bool useChoosen)
{
    std::unique_ptr<AdapterInfo[]> adapterInfos(new AdapterInfo[adaptersNum]);
    ::memset(adapterInfos.get(), 0, sizeof(AdapterInfo)*adaptersNum);
    mainControl.getAdapterInfo(adapterInfos.get());
    int i = 0;
    auto choosenIter = choosenAdapters.begin();
    for (int ai = 0; ai < adaptersNum; ai++)
    {
        if (!mainControl.isAdapterActive(ai))
            continue;
        if (useChoosen && (choosenIter==choosenAdapters.end() || *choosenIter!=i))
        { i++; continue; }
        
        if (adapterInfos[ai].strAdapterName[0]==0)
            getFromPCI(adapterInfos[ai].iAdapterIndex, adapterInfos[ai]);
        
        ADLPMActivity activity;
        mainControl.getCurrentActivity(ai, activity);
        std::cout << "Adapter " << i << ": " << adapterInfos[ai].strAdapterName << "\n"
                "  Core: " << activity.iEngineClock/100.0 << " MHz, "
                "Mem: " << activity.iMemoryClock/100.0 << " MHz, "
                "Vddc: " << activity.iVddc/1000.0 << " V, "
                "Load: " << activity.iActivityPercent << "%, "
                "Temp: " << mainControl.getTemperature(ai, 0)/1000.0 << " C, "
                "Fan: " << mainControl.getFanSpeed(ai, 0) << "%" << std::endl;
        ADLODParameters odParams;
        mainControl.getODParameters(ai, odParams);
        std::cout << "  Max Ranges: Core: " << odParams.sEngineClock.iMin/100.0 << " - " <<
            odParams.sEngineClock.iMax/100.0 << " MHz, "
            "Mem: " << odParams.sMemoryClock.iMin/100.0 << " - " <<
                odParams.sMemoryClock.iMax/100.0 << " MHz, " <<
            "Vddc: " <<  odParams.sVddc.iMin/1000.0 << " - " <<
                odParams.sVddc.iMax/1000.0 << " V\n";
        int levelsNum = odParams.iNumberOfPerformanceLevels;
        std::unique_ptr<ADLODPerformanceLevel[]> odPLevels(
                new ADLODPerformanceLevel[levelsNum]);
        mainControl.getODPerformanceLevels(ai, false, levelsNum, odPLevels.get());
        std::cout << "  PerfLevels: Core: " << odPLevels[0].iEngineClock/100.0 << " - " <<
            odPLevels[levelsNum-1].iEngineClock/100.0 << " MHz, "
            "Mem: " << odPLevels[0].iMemoryClock/100.0 << " - " <<
            odPLevels[levelsNum-1].iMemoryClock/100.0 << " MHz, "
            "Vddc: " << odPLevels[0].iVddc/1000.0 << " - " <<
            odPLevels[levelsNum-1].iVddc/1000.0 << " V\n";
        if (useChoosen)
            ++choosenIter;
        i++;
    }
}

static void printAdaptersInfoVerbose(ADLMainControl& mainControl, int adaptersNum,
            const std::vector<int>& activeAdapters,
            const std::vector<int>& choosenAdapters, bool useChoosen)
{
    std::unique_ptr<AdapterInfo[]> adapterInfos(new AdapterInfo[adaptersNum]);
    ::memset(adapterInfos.get(), 0, sizeof(AdapterInfo)*adaptersNum);
    mainControl.getAdapterInfo(adapterInfos.get());
    int i = 0;
    auto choosenIter = choosenAdapters.begin();
    for (int ai = 0; ai < adaptersNum; ai++)
    {
        if (!mainControl.isAdapterActive(ai))
            continue;
        if (useChoosen && (choosenIter==choosenAdapters.end() || *choosenIter!=i))
        { i++; continue; }
        if (adapterInfos[ai].strAdapterName[0]==0)
            getFromPCI(adapterInfos[ai].iAdapterIndex, adapterInfos[ai]);
        std::cout << "Adapter " << i << ": " << adapterInfos[ai].strAdapterName << "\n";
        ADLFanSpeedInfo fsInfo;
        ADLPMActivity activity;
        mainControl.getCurrentActivity(ai, activity);
        std::cout << "  Current CoreClock: " << activity.iEngineClock/100.0 << " MHz\n"
                "  Current MemoryClock: " << activity.iMemoryClock/100.0 << " MHz\n"
                "  Current Voltage: " << activity.iVddc/1000.0 << " V\n"
                "  GPU Load: " << activity.iActivityPercent << "%\n"
                "  Current PerfLevel: " << activity.iCurrentPerformanceLevel << "\n"
                "  Current BusSpeed: " << activity.iCurrentBusSpeed << "\n"
                "  Current BusLanes: " << activity.iCurrentBusLanes<< "\n";
        
        int temperature = mainControl.getTemperature(ai, 0);
        std::cout << "  Temperature: " << temperature/1000.0 << " C\n";
        mainControl.getFanSpeedInfo(ai, 0, fsInfo);
        std::cout << "  FanSpeed Min: " << fsInfo.iMinPercent << "%\n"
                "  FanSpeed Max: " << fsInfo.iMaxPercent << "%\n"
                "  FanSpeed MinRPM: " << fsInfo.iMinRPM << " RPM\n"
                "  FanSpeed MaxRPM: " << fsInfo.iMaxRPM << " RPM" << "\n";
        std::cout << "  Current FanSpeed: " << mainControl.getFanSpeed(ai, 0) << "%\n";
        ADLODParameters odParams;
        mainControl.getODParameters(ai, odParams);
        std::cout << "  CoreClock: " << odParams.sEngineClock.iMin/100.0 << " - " <<
                odParams.sEngineClock.iMax/100.0 << " MHz, step: " <<
                odParams.sEngineClock.iStep/100.0 << " MHz\n"
                "  MemClock: " << odParams.sMemoryClock.iMin/100.0 << " - " <<
                odParams.sMemoryClock.iMax/100.0 << " MHz, step: " <<
                odParams.sMemoryClock.iStep/100.0 << " MHz\n"
                "  Voltage: " << odParams.sVddc.iMin/1000.0 << " - " <<
                odParams.sVddc.iMax/1000.0 << " V, step: " <<
                odParams.sVddc.iStep/1000.0 << " V\n";
        std::unique_ptr<ADLODPerformanceLevel[]> odPLevels(
                new ADLODPerformanceLevel[odParams.iNumberOfPerformanceLevels]);
        mainControl.getODPerformanceLevels(ai, false, odParams.iNumberOfPerformanceLevels,
                                odPLevels.get());
        std::cout << "  Performance levels: " << odParams.iNumberOfPerformanceLevels << "\n";
        for (int j = 0; j < odParams.iNumberOfPerformanceLevels; j++)
            std::cout << "    Performance Level: " << j << "\n"
                "      CoreClock: " << odPLevels[j].iEngineClock/100.0 << " MHz\n"
                "      MemClock: " << odPLevels[j].iMemoryClock/100.0 << " MHz\n"
                "      Voltage: " << odPLevels[j].iVddc/1000.0 << " V\n";
        mainControl.getODPerformanceLevels(ai, true, odParams.iNumberOfPerformanceLevels,
                                odPLevels.get());
        std::cout << "  Default Performance levels: " <<
                        odParams.iNumberOfPerformanceLevels << "\n";
        for (int j = 0; j < odParams.iNumberOfPerformanceLevels; j++)
            std::cout << "    Performance Level: " << j << "\n"
                "      CoreClock: " << odPLevels[j].iEngineClock/100.0 << " MHz\n"
                "      MemClock: " << odPLevels[j].iMemoryClock/100.0 << " MHz\n"
                "      Voltage: " << odPLevels[j].iVddc/1000.0 << " V\n";
        std::cout.flush();
        if (useChoosen)
            ++choosenIter;
        i++;
    }
}

enum class OVCParamType
{
    CORE_CLOCK,
    MEMORY_CLOCK,
    VDDC_VOLTAGE,
    FAN_SPEED
};

enum: int {
    LAST_PERFLEVEL = -1
};

struct OVCParameter
{
    OVCParamType type;
    int adapterIndex;
    int partId;
    double value;
    bool useDefault;
};

static OVCParameter parseOVCParameter(const char* string)
{
    const char* afterName = strchr(string, ':');
    if (afterName==nullptr)
    {
        afterName = strchr(string, '=');
        if (afterName==nullptr)
            throw Error("This is not parameter");
    }
    std::string name(string, afterName);
    OVCParameter param;
    param.adapterIndex = 0;
    param.partId = 0;
    param.useDefault = false;
    bool partIdSet = false;
    if (name=="coreclk")
    {
        param.type = OVCParamType::CORE_CLOCK;
        param.partId = LAST_PERFLEVEL;
    }
    else if (name=="memclk")
    {
        param.type = OVCParamType::MEMORY_CLOCK;
        param.partId = LAST_PERFLEVEL;
    }
    else if (name=="vcore")
    {
        param.type = OVCParamType::VDDC_VOLTAGE;
        param.partId = LAST_PERFLEVEL;
    }
    else if (name=="fanspeed")
    {
        param.type = OVCParamType::FAN_SPEED;
        partIdSet = false;
    }
    else if (name=="icoreclk")
    {
        param.type = OVCParamType::CORE_CLOCK;
        partIdSet = true;
    }
    else if (name=="imemclk")
    {
        param.type = OVCParamType::MEMORY_CLOCK;
        partIdSet = true;
    }
    else if (name=="ivcore")
    {
        param.type = OVCParamType::VDDC_VOLTAGE;
        partIdSet = true;
    }
    else
        throw Error("Wrong parameter name");
    
    char* next;
    if (*afterName==':')
    {   // if is
        afterName++;
        errno = 0;
        int value = strtol(afterName, &next, 10);
        if (errno!=0)
            throw Error("Can't parse device index");
        if (afterName != next)
            param.adapterIndex = value;
        afterName = next;
    }
    else if (*afterName==0)
        throw Error("Unterminated parameter");
    
    if (*afterName==':' && !partIdSet)
    {
        afterName++;
        errno = 0;
        int value = strtol(afterName, &next, 10);
        if (errno!=0)
            throw Error("Can't parse partId");
        if (afterName != next)
            param.partId = value;
        afterName = next;
    }
    else if (*afterName==0)
        throw Error("Unterminated parameter");
    
    if (*afterName=='=')
    {
        afterName++;
        errno = 0;
        if (::strcmp(afterName, "default")==0)
        {
            param.useDefault = true;
            afterName += 7;
        }
        else
        {
            param.value = strtod(afterName, &next);
            if (errno!=0 || afterName==next)
                throw Error("Can't parse value");
            afterName = next;
        }
        if (*afterName!=0)
            throw Error("Garbages at parameter");
    }
    else
        throw Error("Unterminated parameter");
    /*std::cout << "param: " << int(param.type) << ", dev: " << param.adapterIndex <<
            ", pid: " << param.partId << ", value=" << param.value << std::endl;*/
    return param;
}

static void parseAdaptersList(const char* string, std::vector<int>& adapters)
{
    adapters.clear();
    while (true)
    {
        char* endptr;
        errno = 0;
        int adapterIndex = strtol(string, &endptr, 10);
        if (errno!=0 || endptr==string)
            throw Error("Can't parse adapter index");
        adapters.push_back(adapterIndex);
        string = endptr;
        if (*string==0)
            break;
        if (*string==',')
            string++;
        else
            throw Error("Garbages at adapter list");
    }
}

struct FanSpeedSetup
{
    double value;
    bool useDefault;
    bool isSet;
};

static void setOVCParameters(ADLMainControl& mainControl, int adaptersNum,
            const std::vector<int>& activeAdapters,
            const std::vector<OVCParameter>& ovcParams)
{
    std::cout << "WARNING: setting AMD Overdrive parameters!" << std::endl;
    std::cout <<
        "\nIMPORTANT NOTICE: Before any setting of AMD Overdrive parameters,\n"
        "please STOP ANY GPU computations and GPU renderings.\n"
        "Please use this utility CAREFULLY, because it can DAMAGE your hardware!\n" 
        << std::endl;
    
    const int realAdaptersNum = activeAdapters.size();
    std::vector<ADLODParameters> odParams(realAdaptersNum);
    std::vector<std::vector<ADLODPerformanceLevel> > perfLevels(realAdaptersNum);
    std::vector<std::vector<ADLODPerformanceLevel> > defaultPerfLevels(realAdaptersNum);
    std::vector<bool> changedDevices(realAdaptersNum);
    std::fill(changedDevices.begin(), changedDevices.end(), false);
    try
    {
    for (OVCParameter param: ovcParams)
        if (param.adapterIndex>=realAdaptersNum || param.adapterIndex<0)
            throw Error("Some parameters refer to devices that no present!");
    
    // check fanspeed
    for (OVCParameter param: ovcParams)
        if (param.type==OVCParamType::FAN_SPEED)
        {
            if(param.partId!=0)
                throw Error("Some fanspeed parameters have wrong thermalCtrlIndex!");
            if(!param.useDefault && (param.value<0.0 || param.value>100.0))
                throw Error("Some fanspeed parameters have value out of range!");
        }
    
    for (int ai = 0; ai < realAdaptersNum; ai++)
    {
        int i = activeAdapters[ai];
        mainControl.getODParameters(i, odParams[ai]);
        perfLevels[ai].resize(odParams[ai].iNumberOfPerformanceLevels);
        defaultPerfLevels[ai].resize(odParams[ai].iNumberOfPerformanceLevels);
        mainControl.getODPerformanceLevels(i, 0, odParams[ai].iNumberOfPerformanceLevels,
                    perfLevels[ai].data());
        mainControl.getODPerformanceLevels(i, 1, odParams[ai].iNumberOfPerformanceLevels,
                    defaultPerfLevels[ai].data());
    }
    
    // check other params
    for (OVCParameter param: ovcParams)
        if (param.type!=OVCParamType::FAN_SPEED)
        {
            int i = param.adapterIndex;
            int partId = (param.partId!=LAST_PERFLEVEL)?param.partId:
                    odParams[i].iNumberOfPerformanceLevels-1;
            if (partId >= odParams[i].iNumberOfPerformanceLevels || partId < 0)
                throw Error("Some other parameters have wrong performance levels");
            switch(param.type)
            {
                case OVCParamType::CORE_CLOCK:
                    if (!param.useDefault &&
                        (param.value < odParams[i].sEngineClock.iMin/100.0 ||
                        param.value > odParams[i].sEngineClock.iMax/100.0))
                        throw Error("Some coreclocks out of range");
                    break;
                case OVCParamType::MEMORY_CLOCK:
                    if (!param.useDefault &&
                        (param.value < odParams[i].sMemoryClock.iMin/100.0 ||
                         param.value > odParams[i].sMemoryClock.iMax/100.0))
                        throw Error("Some memclocks out of range");
                    break;
                case OVCParamType::VDDC_VOLTAGE:
                    if (!param.useDefault &&
                        (param.value < odParams[i].sVddc.iMin/1000.0 ||
                        param.value > odParams[i].sVddc.iMax/1000.0))
                        throw Error("Some vcores out of range");
                    break;
                default:
                    break;
            }
        }
    }
    catch(const Error& error)
    {
        std::cerr << "NO ANY settings applied. "
                "Error in parameters or some serious errors!\n" << std::endl;
        throw;
    }
    // print what has been changed
    for (OVCParameter param: ovcParams)
        if (param.type==OVCParamType::FAN_SPEED)
        {
            std::cout << "Setting fanspeed to ";
            if (param.useDefault)
                std::cout << "default";
            else
                std::cout << param.value << "%";
            std::cout << " for adapter " <<
                    param.adapterIndex << " at thermal controller " <<
                    param.partId << std::endl;
        }
    for (OVCParameter param: ovcParams)
        if (param.type!=OVCParamType::FAN_SPEED)
        {
            int i = param.adapterIndex;
            int partId = (param.partId!=LAST_PERFLEVEL)?param.partId:
                    odParams[i].iNumberOfPerformanceLevels-1;
            if (partId >= odParams[i].iNumberOfPerformanceLevels || partId < 0)
                throw Error("Some other parameters have wrong performance levels");
            switch(param.type)
            {
                case OVCParamType::CORE_CLOCK:
                    std::cout << "Setting core clock to ";
                    if (param.useDefault)
                        std::cout << "default";
                    else
                        std::cout << param.value << " MHz";
                    std::cout << " for adapter " << param.adapterIndex <<
                            " at performance level " << partId << std::endl;
                    break;
                case OVCParamType::MEMORY_CLOCK:
                    std::cout << "Setting memory clock to ";
                    if (param.useDefault)
                        std::cout << "default";
                    else
                        std::cout << param.value << " MHz";
                    std::cout << " for adapter " << param.adapterIndex <<
                            " at performance level " << partId << std::endl;
                    break;
                case OVCParamType::VDDC_VOLTAGE:
                    std::cout << "Setting Vddc voltage to ";
                    if (param.useDefault)
                        std::cout << "default";
                    else
                        std::cout << param.value << " V";
                    std::cout << " for adapter " << param.adapterIndex <<
                            " at performance level " << partId << std::endl;
                    break;
                default:
                    break;
            }
        }
    std::vector<FanSpeedSetup> fanSpeedSetups(realAdaptersNum);
    std::fill(fanSpeedSetups.begin(), fanSpeedSetups.end(),
              FanSpeedSetup{ 0.0, false, false });
    for (OVCParameter param: ovcParams)
        if (param.type==OVCParamType::FAN_SPEED)
        {
            fanSpeedSetups[param.adapterIndex].value = param.value;
            fanSpeedSetups[param.adapterIndex].useDefault = param.useDefault;
            fanSpeedSetups[param.adapterIndex].isSet = true;
        }
    
    for (OVCParameter param: ovcParams)
        if (param.type!=OVCParamType::FAN_SPEED)
        {
            int i = param.adapterIndex;
            int partId = (param.partId!=LAST_PERFLEVEL)?param.partId:
                    odParams[i].iNumberOfPerformanceLevels-1;
            ADLODPerformanceLevel& perfLevel = perfLevels[i][partId];
            const ADLODPerformanceLevel& defaultPerfLevel = defaultPerfLevels[i][partId];
            switch(param.type)
            {
                case OVCParamType::CORE_CLOCK:
                    if (param.useDefault)
                        perfLevel.iEngineClock = defaultPerfLevel.iEngineClock;
                    else
                        perfLevel.iEngineClock = int(round(param.value*100.0));
                    break;
                case OVCParamType::MEMORY_CLOCK:
                    if (param.useDefault)
                        perfLevel.iMemoryClock = defaultPerfLevel.iMemoryClock;
                    else
                        perfLevel.iMemoryClock = int(round(param.value*100.0));
                    break;
                case OVCParamType::VDDC_VOLTAGE:
                    if (param.useDefault)
                        perfLevel.iVddc = defaultPerfLevel.iVddc;
                    else if (perfLevel.iVddc==0)
                        std::cout << "Voltage for adapter " << i <<
                                    " is not set!" << std::endl;
                    else
                        perfLevel.iVddc = int(round(param.value*1000.0));
                    break;
                default:
                    break;
            }
            changedDevices[i] = true;
        }
    
    /// set fan speeds
    for (int i = 0; i < realAdaptersNum; i++)
        if (fanSpeedSetups[i].isSet)
        {
            if (!fanSpeedSetups[i].useDefault)
                mainControl.setFanSpeed(activeAdapters[i], 0 /* must be zero */,
                                int(round(fanSpeedSetups[i].value)));
            else
                mainControl.setFanSpeedToDefault(activeAdapters[i], 0);
        }
    
    // set od perflevels
    for (int i = 0; i < realAdaptersNum; i++)
        if (changedDevices[i])
            mainControl.setODPerformanceLevels(activeAdapters[i],
                    odParams[i].iNumberOfPerformanceLevels, perfLevels[i].data());
}

static const char* helpAndUsageString =
"amdcovc 0.1 by Mateusz Szpakowski (matszpk@interia.pl)\n"
"Program is distributed under terms of the GPLv2.\n"
"\n"
"Usage: amdcovc [--help|-?] [--verbose|-v] [-a LIST|--adapters=LIST] [PARAM ...]\n"
"Print AMD Overdrive informations if no parameter given.\n"
"Set AMD Overdrive parameters (clocks, fanspeeds,...) if any parameter given.\n"
"\n"
"List of parameters:\n"
"  coreclk[:[ADAPTER][:LEVEL]]=CLOCK     set core clock in MHz\n"
"  memclk[:[ADAPTER][:LEVEL]]=CLOCK      set memory clock in MHz\n"
"  vcore[:[ADAPTER][:LEVEL]]=VOLTAGE     set Vddc voltage in Volts\n"
"  icoreclk[:ADAPTER]=CLOCK              set core clock in MHz for idle level\n"
"  imemclk[:ADAPTER]=CLOCK               set memory clock in MHz for idle level\n"
"  ivcore[:ADAPTER]=VOLTAGE              set Vddc voltage  in Volts for idle level\n"
"  fanspeed[:[ADAPTER][:THID]]=PERCENT   set fanspeed in percents\n"
"Extra specifiers in parameters:\n"
"  ADAPTER                   adapter (device) index (default is 0)\n"
"  LEVEL                     performance level (typically 0 or 1, default is last)\n"
"  THID                      thermal controller index (must be 0)\n"
"You can use 'default' in value place to set default value.\n"
"For fanspeed 'default' value force automatic speed setup.\n"
"\n"
"List of options:\n"
"  -a, --adapters=LIST       print informations only for these adapters\n"
"  -v, --verbose             print verbose informations\n"
"      --version             print version\n"
"  -?, --help                print help\n"
"\n"
"Sample usage:\n"
"amdcovc coreclk:1=900 coreclk=1000\n"
"    set core clock to 900 for adapter 1, set core clock to 1000 for adapter 0\n"
"amdcovc coreclk:1:0=900 coreclk:0:1=1000\n"
"    set core clock to 900 for adapter 1 at performance level 0,\n"
"    set core clock to 1000 for adapter 0 at performance level 1\n"
"amdcovc coreclk:1:0=default coreclk:0:1=default\n"
"    set core clock to default for adapter 0 and 1\n"
"amdcovc fanspeed=75 fanspeed:2=60 fanspeed:1=default\n"
"    set fanspeed to 75% for adapter 0 and set fanspeed to 60% for adapter 2\n"
"    set fanspeed to default for adapter 1\n"
"amdcovc vcore=1.111 vcore::0=0.81\n"
"    set Vddc voltage to 1.111 V for adapter 0\n"
"    set Vddc voltage to 0.81 for adapter 0 for performance level 0\n"
"\n"
"IMPORTANT NOTICE: Before any setting of AMD Overdrive parameters,\n"
"please STOP ANY GPU computations and GPU renderings.\n"
"Please use this utility CAREFULLY, because it can DAMAGE your hardware!\n";

int main(int argc, const char** argv)
try
{
    bool printHelp = false;
    bool printVerbose = false;
    std::vector<OVCParameter> ovcParameters;
    std::vector<int> choosenAdapters;
    bool useAdaptersList = false;
    
    for (int i = 1; i < argc; i++)
        if (::strcmp(argv[i], "--help")==0 || ::strcmp(argv[i], "-?")==0)
            printHelp = true;
        else if (::strcmp(argv[i], "--verbose")==0 || ::strcmp(argv[i], "-v")==0)
            printVerbose = true;
        else if (::strncmp(argv[i], "--adapters=", 11)==0)
        {
            parseAdaptersList(argv[i]+11, choosenAdapters);
            useAdaptersList = true;
        }
        else if (::strcmp(argv[i], "--adapters")==0)
        {
            parseAdaptersList(argv[++i], choosenAdapters);
            useAdaptersList = true;
        }
        else if (::strncmp(argv[i], "-a", 2)==0)
        {
            if (argv[i][2]!=0)
                parseAdaptersList(argv[i]+2, choosenAdapters);
            else
                parseAdaptersList(argv[++i], choosenAdapters);
            useAdaptersList = true;
        }
        else if (::strcmp(argv[i], "--version")==0)
        {
            std::cout << "amdcovc 0.1 by Mateusz Szpakowski (matszpk@interia.pl)\n"
            "Program is distributed under terms of the GPLv2.\n" << std::endl;
            return 0;
        }
        else
            ovcParameters.push_back(parseOVCParameter(argv[i]));
    if (printHelp)
    {
        std::cout << helpAndUsageString;
        std::cout.flush();
        return 0;
    }
    ATIADLHandle handle;
    ADLMainControl mainControl(handle, 0);
    int adaptersNum = mainControl.getAdaptersNum();
    std::vector<int> activeAdapters;
    getActiveAdaptersIndices(mainControl, adaptersNum, activeAdapters);
    
    if (useAdaptersList)
    {   // sort and check adapter list
        std::sort(choosenAdapters.begin(), choosenAdapters.end());
        choosenAdapters.resize(
                std::unique(choosenAdapters.begin(), choosenAdapters.end()) -
                    choosenAdapters.begin());
        for (int adapterIndex: choosenAdapters)
            if (adapterIndex>=int(activeAdapters.size()) || adapterIndex<0)
                throw Error("Some adapter indices out of range");
    }
    
    if (!ovcParameters.empty())
        setOVCParameters(mainControl, adaptersNum, activeAdapters, ovcParameters);
    else
    {
        if (printVerbose)
            printAdaptersInfoVerbose(mainControl, adaptersNum, activeAdapters,
                        choosenAdapters, useAdaptersList);
        else
            printAdaptersInfo(mainControl, adaptersNum, activeAdapters,
                        choosenAdapters, useAdaptersList);
    }
    if (pciAccess!=nullptr)
        pci_cleanup(pciAccess);
    return 0;
}
catch(const std::exception& ex)
{
    if (pciAccess!=nullptr)
        pci_cleanup(pciAccess);
    std::cerr << ex.what() << std::endl;
    return 1;
}