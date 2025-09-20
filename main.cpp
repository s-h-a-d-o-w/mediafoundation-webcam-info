#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <string>
#include <algorithm>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <iostream>
#include <combaseapi.h>

static std::wstring SubtypeToString(const GUID& g) {
  const USHORT baseData2 = 0x0000;
  const USHORT baseData3 = 0x0010;
  const BYTE baseData4[8] = {0x80,0x00,0x00,0xAA,0x00,0x38,0x9B,0x71};
  if (g.Data2 == baseData2 && g.Data3 == baseData3 && std::equal(std::begin(baseData4), std::end(baseData4), g.Data4)) {
    DWORD fcc = g.Data1;
    wchar_t s[5];
    s[0] = (wchar_t)(fcc & 0xFF);
    s[1] = (wchar_t)((fcc >> 8) & 0xFF);
    s[2] = (wchar_t)((fcc >> 16) & 0xFF);
    s[3] = (wchar_t)((fcc >> 24) & 0xFF);
    s[4] = 0;
    for (int i = 0; i < 4; ++i) if (s[i] < 32 || s[i] > 126) s[i] = L'?';
    return std::wstring(s);
  }
  wchar_t buf[64] = {};
  int n = StringFromGUID2(g, buf, 64);
  if (n > 0) return std::wstring(buf);
  return L"Unknown";
}

// Based on: https://stackoverflow.com/a/75863860/5040168
void getInfo(IMFActivate* device)
{
  IMFMediaSourceEx* mediaSource = nullptr;
  device->ActivateObject(__uuidof(IMFMediaSourceEx), (void**)&mediaSource);

  // enumerate available streams
  IMFPresentationDescriptor* presentationDesc = nullptr;
  DWORD cDesc = 0;
  mediaSource->CreatePresentationDescriptor(&presentationDesc);
  presentationDesc->GetStreamDescriptorCount(&cDesc);
  // std::wcout << L"Number of streams: " << cDesc << L" " << std::endl;
  for (DWORD i = 0; i < cDesc; i++)
  {
    std::wcout << L"   -- Stream " << i << L" --" << std::endl;

    IMFStreamDescriptor* pStream = nullptr;
    BOOL selected = FALSE;
    presentationDesc->GetStreamDescriptorByIndex(i, &selected, &pStream);
    presentationDesc->DeselectStream(i); // just making sure no stream is selected

    // enumerate media types per stream
    IMFMediaTypeHandler* pHandler = nullptr;
    pStream->GetMediaTypeHandler(&pHandler);
    DWORD cMediaType = 0;
    pHandler->GetMediaTypeCount(&cMediaType);
    // std::wcout << L"Number of media types: " << cMediaType << L" " << std::endl;
    for (DWORD j = 0; j < cMediaType; j++)
    {
      IMFMediaType* pMediaType = nullptr;
      pHandler->GetMediaTypeByIndex(j, &pMediaType);

      UINT32 imageWidth = 0;
      UINT32 imageHeight = 0;
      UINT32 fpsNumerator = 0;
      UINT32 fpsDenominator = 0;
      GUID imageFormat = GUID_NULL;
      MFGetAttributeSize(pMediaType, MF_MT_FRAME_SIZE, &imageWidth, &imageHeight);
      MFGetAttributeRatio(pMediaType, MF_MT_FRAME_RATE, &fpsNumerator, &fpsDenominator);
      pMediaType->GetGUID(MF_MT_SUBTYPE, &imageFormat);

      std::wcout << imageWidth << L"x" << imageHeight << L" (" << fpsNumerator << "/" << fpsDenominator << L" fps), Media type: " << SubtypeToString(imageFormat) << L" " << std::endl;
    }
  }
}

IMFActivate* getDevice() {
  HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  if (hr == RPC_E_CHANGED_MODE) {
    hr = S_OK;
  }
  if (FAILED(hr)) {
    return nullptr;
  }

  if (FAILED(MFStartup(MF_VERSION))) {
    return nullptr;
  }

  IMFAttributes* attributes = nullptr;
  if (FAILED(MFCreateAttributes(&attributes, 1))) {
    return nullptr;
  }

  attributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);

  IMFActivate** devices = nullptr;
  UINT32 count = 0;
  IMFActivate* chosen = nullptr;

  if (SUCCEEDED(MFEnumDeviceSources(attributes, &devices, &count))) {
    for (UINT32 i = 0; i < count; ++i) {
      LPWSTR name = nullptr;
      UINT32 nameLen = 0;
      if (SUCCEEDED(devices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &name, &nameLen)) && name) {
        std::wcout << name << std::endl;
        std::wcout << L"=====================================" << std::endl;
        getInfo(devices[i]);
        CoTaskMemFree(name);
        std::wcout << std::endl;
      }
    }

    for (UINT32 i = 0; i < count; ++i) {
      if (devices[i] != chosen) {
        devices[i]->Release();
      }
    }
    CoTaskMemFree(devices);
  }

  if (attributes) {
    attributes->Release();
  }

  return chosen;
}

int wmain()
{
  getDevice();
  return 0;
}