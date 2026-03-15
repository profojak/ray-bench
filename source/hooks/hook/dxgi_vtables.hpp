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
    GetDevice = 7,
};

enum class IDXGIResource_VTable_ID : int {
    GetSharedHandle = 8,
    GetUsage = 9,
    SetEvictionPriority = 10,
    GetEvictionPriority = 11,
};

enum class IDXGIKeyedMutex_VTable_ID : int {
    AcquireSync = 8,
    ReleaseSync = 9,
};

enum class IDXGISurface_VTable_ID : int {
    GetDesc = 8,
    Map = 9,
    Unmap = 10,
};

enum class IDXGISurface1_VTable_ID : int {
    GetDC = 11,
    ReleaseDC = 12,
};

enum class IDXGIAdapter_VTable_ID : int {
    EnumOutputs = 7,
    GetDesc = 8,
    CheckInterfaceSupport = 9,
};

enum class IDXGIOutput_VTable_ID : int {
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
    EnumAdapters = 7,
    MakeWindowAssociation = 8,
    GetWindowAssociation = 9,
    CreateSwapChain = 10,
    CreateSoftwareAdapter = 11,
};

enum class IDXGIDevice_VTable_ID : int {
    GetAdapter = 7,
    CreateSurface = 8,
    QueryResourceResidency = 9,
    SetGPUThreadPriority = 10,
    GetGPUThreadPriority = 11,
};

enum class IDXGIFactory1_VTable_ID : int {
    EnumAdapters1 = 12,
    IsCurrent = 13,
};

enum class IDXGIAdapter1_VTable_ID : int {
    GetDesc1 = 10,
};

enum class IDXGIDevice1_VTable_ID : int {
    SetMaximumFrameLatency = 12,
    GetMaximumFrameLatency = 13,
};

enum class IDXGIDisplayControl_VTable_ID : int {
    QueryInterface = 0,
    AddRef = 1,
    Release = 2,
    IsStereoEnabled = 3,
    SetStereoEnabled = 4,
};

enum class IDXGIOutputDuplication_VTable_ID : int {
    GetDesc = 7,
    AcquireNextFrame = 8,
    GetFrameDirtyRects = 9,
    GetFrameMoveRects = 10,
    GetFramePointerShape = 11,
    MapDesktopSurface = 12,
    UnMapDesktopSurface = 13,
    ReleaseFrame = 14,
};

enum class IDXGISurface2_VTable_ID : int {
    GetResource = 13,
};

enum class IDXGIResource1_VTable_ID : int {
    CreateSubresourceSurface = 12,
    CreateSharedHandle = 13,
};

enum class IDXGIDevice2_VTable_ID : int {
    OfferResources = 14,
    ReclaimResources = 15,
    EnqueueSetEvent = 16,
};

enum class IDXGISwapChain1_VTable_ID : int {
    GetDesc1 = 18,
    GetFullscreenDesc = 19,
    GetHwnd = 20,
    GetCoreWindow = 21,
    Present1 = 22,
    IsTemporaryMonoSupported = 23,
    GetRestrictToOutput = 24,
    SetBackgroundColor = 25,
    GetBackgroundColor = 26,
    SetRotation = 27,
    GetRotation = 28,
};

enum class IDXGIFactory2_VTable_ID : int {
    IsWindowedStereoEnabled = 14,
    CreateSwapChainForHwnd = 15,
    CreateSwapChainForCoreWindow = 16,
    GetSharedResourceAdapterLuid = 17,
    RegisterStereoStatusWindow = 18,
    RegisterStereoStatusEvent = 19,
    UnregisterStereoStatus = 20,
    RegisterOcclusionStatusWindow = 21,
    RegisterOcclusionStatusEvent = 22,
    UnregisterOcclusionStatus = 23,
    CreateSwapChainForComposition = 24,
};

enum class IDXGIAdapter2_VTable_ID : int {
    GetDesc2 = 11,
};

enum class IDXGIOutput1_VTable_ID : int {
    GetDisplayModeList1 = 19,
    FindClosestMatchingMode1 = 20,
    GetDisplaySurfaceData1 = 21,
    DuplicateOutput = 22,
};

enum class IDXGIDevice3_VTable_ID : int {
    Trim = 17,
};

enum class IDXGISwapChain2_VTable_ID : int {
    SetSourceSize = 29,
    GetSourceSize = 30,
    SetMaximumFrameLatency = 31,
    GetMaximumFrameLatency = 32,
    GetFrameLatencyWaitableObject = 33,
    SetMatrixTransform = 34,
    GetMatrixTransform = 35,
};

enum class IDXGIOutput2_VTable_ID : int {
    SupportsOverlays = 23,
};

enum class IDXGIFactory3_VTable_ID : int {
    GetCreationFlags = 25,
};

enum class IDXGIDecodeSwapChain_VTable_ID : int {
    QueryInterface = 0,
    AddRef = 1,
    Release = 2,
    PresentBuffer = 3,
    SetSourceRect = 4,
    SetTargetRect = 5,
    SetDestSize = 6,
    GetSourceRect = 7,
    GetTargetRect = 8,
    GetDestSize = 9,
    SetColorSpace = 10,
    GetColorSpace = 11,
};

enum class IDXGIFactoryMedia_VTable_ID : int {
    QueryInterface = 0,
    AddRef = 1,
    Release = 2,
    CreateSwapChainForCompositionSurfaceHandle = 3,
    CreateDecodeSwapChainForCompositionSurfaceHandle = 4,
};

enum class IDXGISwapChainMedia_VTable_ID : int {
    QueryInterface = 0,
    AddRef = 1,
    Release = 2,
    GetFrameStatisticsMedia = 3,
    SetPresentDuration = 4,
    CheckPresentDurationSupport = 5,
};

enum class IDXGIOutput3_VTable_ID : int {
    CheckOverlaySupport = 24,
};

enum class IDXGISwapChain3_VTable_ID : int {
    GetCurrentBackBufferIndex = 36,
    CheckColorSpaceSupport = 37,
    SetColorSpace1 = 38,
    ResizeBuffers1 = 39,
};

enum class IDXGIOutput4_VTable_ID : int {
    CheckOverlayColorSpaceSupport = 25,
};

enum class IDXGIFactory4_VTable_ID : int {
    EnumAdapterByLuid = 26,
    EnumWarpAdapter = 27,
};

enum class IDXGIAdapter3_VTable_ID : int {
    RegisterHardwareContentProtectionTeardownStatusEvent = 12,
    UnregisterHardwareContentProtectionTeardownStatus = 13,
    QueryVideoMemoryInfo = 14,
    SetVideoMemoryReservation = 15,
    RegisterVideoMemoryBudgetChangeNotificationEvent = 16,
    UnregisterVideoMemoryBudgetChangeNotification = 17,
};

enum class IDXGIOutput5_VTable_ID : int {
    DuplicateOutput1 = 26,
};

enum class IDXGISwapChain4_VTable_ID : int {
    SetHDRMetaData = 40,
};

enum class IDXGIDevice4_VTable_ID : int {
    OfferResources1 = 18,
    ReclaimResources1 = 19,
};

enum class IDXGIFactory5_VTable_ID : int {
    CheckFeatureSupport = 28,
};

enum class IDXGIAdapter4_VTable_ID : int {
    GetDesc3 = 18,
};

enum class IDXGIOutput6_VTable_ID : int {
    GetDesc1 = 27,
    CheckHardwareCompositionSupport = 28,
};

enum class IDXGIFactory6_VTable_ID : int {
    EnumAdapterByGpuPreference = 29,
};

enum class IDXGIFactory7_VTable_ID : int {
    RegisterAdaptersChangedEvent = 30,
    UnregisterAdaptersChangedEvent = 31,
};

// ----------------------------------------------------------------------------