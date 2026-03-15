// ============================================================================

/// @brief DXGI VTable method indices

#pragma once

enum class IDXGIObject_VTable_ID : int {
    QueryInterface = 0,
    AddRef = 1,
    Release = 2,
    SetPrivateData = 3,
    SetPrivateDataInterface = 4,
    GetPrivateData = 5,
    GetParent = 6,
};

enum class IDXGIDeviceSubObject_VTable_ID : int {
    QueryInterface = 0,
    AddRef = 1,
    Release = 2,
    SetPrivateData = 3,
    SetPrivateDataInterface = 4,
    GetPrivateData = 5,
    GetParent = 6,
    GetDevice = 7,
};

enum class IDXGIResource_VTable_ID : int {
    QueryInterface = 0,
    AddRef = 1,
    Release = 2,
    SetPrivateData = 3,
    SetPrivateDataInterface = 4,
    GetPrivateData = 5,
    GetParent = 6,
    GetDevice = 7,
    GetSharedHandle = 8,
    GetUsage = 9,
    SetEvictionPriority = 10,
    GetEvictionPriority = 11,
};

enum class IDXGIKeyedMutex_VTable_ID : int {
    QueryInterface = 0,
    AddRef = 1,
    Release = 2,
    SetPrivateData = 3,
    SetPrivateDataInterface = 4,
    GetPrivateData = 5,
    GetParent = 6,
    GetDevice = 7,
    AcquireSync = 8,
    ReleaseSync = 9,
};

enum class IDXGISurface_VTable_ID : int {
    QueryInterface = 0,
    AddRef = 1,
    Release = 2,
    SetPrivateData = 3,
    SetPrivateDataInterface = 4,
    GetPrivateData = 5,
    GetParent = 6,
    GetDevice = 7,
    GetDesc = 8,
    Map = 9,
    Unmap = 10,
};

enum class IDXGISurface1_VTable_ID : int {
    QueryInterface = 0,
    AddRef = 1,
    Release = 2,
    SetPrivateData = 3,
    SetPrivateDataInterface = 4,
    GetPrivateData = 5,
    GetParent = 6,
    GetDevice = 7,
    GetDesc = 8,
    Map = 9,
    Unmap = 10,
    GetDC = 11,
    ReleaseDC = 12,
};

enum class IDXGIAdapter_VTable_ID : int {
    QueryInterface = 0,
    AddRef = 1,
    Release = 2,
    SetPrivateData = 3,
    SetPrivateDataInterface = 4,
    GetPrivateData = 5,
    GetParent = 6,
    EnumOutputs = 7,
    GetDesc = 8,
    CheckInterfaceSupport = 9,
};

enum class IDXGIOutput_VTable_ID : int {
    QueryInterface = 0,
    AddRef = 1,
    Release = 2,
    SetPrivateData = 3,
    SetPrivateDataInterface = 4,
    GetPrivateData = 5,
    GetParent = 6,
    GetDesc = 7,
    GetDisplayModeList = 8,
    FindClosestMatchingMode = 9,
    WaitForVBlank = 10,
    TakeOwnership = 11,
    ReleaseOwnership = 12,
    GetGammaControlCapabilities = 13,
    SetGammaControl = 14,
    GetGammaControl = 15,
    SetDisplaySurface = 16,
    GetDisplaySurfaceData = 17,
    GetFrameStatistics = 18,
};

enum class IDXGISwapChain_VTable_ID : int {
    QueryInterface = 0,
    AddRef = 1,
    Release = 2,
    SetPrivateData = 3,
    SetPrivateDataInterface = 4,
    GetPrivateData = 5,
    GetParent = 6,
    GetDevice = 7,
    Present = 8,
    GetBuffer = 9,
    SetFullscreenState = 10,
    GetFullscreenState = 11,
    GetDesc = 12,
    ResizeBuffers = 13,
    ResizeTarget = 14,
    GetContainingOutput = 15,
    GetFrameStatistics = 16,
    GetLastPresentCount = 17,
};

enum class IDXGIFactory_VTable_ID : int {
    QueryInterface = 0,
    AddRef = 1,
    Release = 2,
    SetPrivateData = 3,
    SetPrivateDataInterface = 4,
    GetPrivateData = 5,
    GetParent = 6,
    EnumAdapters = 7,
    MakeWindowAssociation = 8,
    GetWindowAssociation = 9,
    CreateSwapChain = 10,
    CreateSoftwareAdapter = 11,
};

enum class IDXGIDevice_VTable_ID : int {
    QueryInterface = 0,
    AddRef = 1,
    Release = 2,
    SetPrivateData = 3,
    SetPrivateDataInterface = 4,
    GetPrivateData = 5,
    GetParent = 6,
    GetAdapter = 7,
    CreateSurface = 8,
    QueryResourceResidency = 9,
    SetGPUThreadPriority = 10,
    GetGPUThreadPriority = 11,
};

enum class IDXGIFactory1_VTable_ID : int {
    QueryInterface = 0,
    AddRef = 1,
    Release = 2,
    SetPrivateData = 3,
    SetPrivateDataInterface = 4,
    GetPrivateData = 5,
    GetParent = 6,
    EnumAdapters = 7,
    MakeWindowAssociation = 8,
    GetWindowAssociation = 9,
    CreateSwapChain = 10,
    CreateSoftwareAdapter = 11,
    EnumAdapters1 = 12,
    IsCurrent = 13,
};

enum class IDXGIAdapter1_VTable_ID : int {
    QueryInterface = 0,
    AddRef = 1,
    Release = 2,
    SetPrivateData = 3,
    SetPrivateDataInterface = 4,
    GetPrivateData = 5,
    GetParent = 6,
    EnumOutputs = 7,
    GetDesc = 8,
    CheckInterfaceSupport = 9,
    GetDesc1 = 10,
};

enum class IDXGIDevice1_VTable_ID : int {
    QueryInterface = 0,
    AddRef = 1,
    Release = 2,
    SetPrivateData = 3,
    SetPrivateDataInterface = 4,
    GetPrivateData = 5,
    GetParent = 6,
    GetAdapter = 7,
    CreateSurface = 8,
    QueryResourceResidency = 9,
    SetGPUThreadPriority = 10,
    GetGPUThreadPriority = 11,
    SetMaximumFrameLatency = 12,
    GetMaximumFrameLatency = 13,
};

// ----------------------------------------------------------------------------