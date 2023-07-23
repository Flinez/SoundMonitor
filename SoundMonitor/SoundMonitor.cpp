#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <vector>
#include <iostream>
#include <functiondiscoverykeys_devpkey.h> 

class SoundDetection
{
public:
    SoundDetection()
    {
        CoInitialize(NULL);
        InitializeDevices();
    }

    ~SoundDetection()
    {
        CoUninitialize();
    }

    void InitializeDevices()
    {
        IMMDeviceEnumerator* pEnumerator = nullptr;
        IMMDeviceCollection* pCollection = nullptr;

        HRESULT hr = CoCreateInstance(
            __uuidof(MMDeviceEnumerator),
            NULL,
            CLSCTX_ALL,
            __uuidof(IMMDeviceEnumerator),
            (void**)&pEnumerator
        );

        if (SUCCEEDED(hr))
        {
            hr = pEnumerator->EnumAudioEndpoints(
                eRender,
                DEVICE_STATE_ACTIVE,
                &pCollection
            );

            if (SUCCEEDED(hr))
            {
                UINT count;
                hr = pCollection->GetCount(&count);

                if (SUCCEEDED(hr))
                {
                    for (UINT i = 0; i < count; i++)
                    {
                        IMMDevice* pDevice;
                        hr = pCollection->Item(i, &pDevice);

                        if (SUCCEEDED(hr))
                        {
                            m_devices.push_back(pDevice);
                        }
                    }
                }

                pCollection->Release();
            }

            pEnumerator->Release();
        }
    }

    std::wstring ReplaceUnknownCharacters(const std::wstring& str, wchar_t fallback)
    {
        std::wstring result;
        for (wchar_t ch : str)
        {
            if (ch >= 0 && ch <= 127)
            {
                result.push_back(ch);
            }
            else
            {
                result.push_back(fallback);
            }
        }
        return result;
    }

    std::wstring GetDeviceName(int deviceIndex)
    {
        std::wstring deviceName;
        if (deviceIndex >= 0 && deviceIndex < m_devices.size())
        {
			IMMDevice* pDevice = m_devices[deviceIndex];
            if (pDevice)
            {
				IPropertyStore* pPropertyStore = nullptr;
                HRESULT hr = pDevice->OpenPropertyStore(
					STGM_READ,
					&pPropertyStore
				);

                if (SUCCEEDED(hr))
                {
					PROPVARIANT friendlyName;
					PropVariantInit(&friendlyName);
                    hr = pPropertyStore->GetValue(
						PKEY_Device_FriendlyName,
						&friendlyName
					);

                    if (SUCCEEDED(hr))
                    {
						deviceName = friendlyName.pwszVal;
                        deviceName = ReplaceUnknownCharacters(deviceName, L'?');
						PropVariantClear(&friendlyName);
					}

					pPropertyStore->Release();
				}
			}
		}

        return deviceName;
    }

    int GetSoundLevel(int deviceIndex)
    {
        if (deviceIndex >= 0 && deviceIndex < m_devices.size())
        {
            IMMDevice* pDevice = m_devices[deviceIndex];
            if (pDevice)
            {
                IAudioMeterInformation* pMeterInfo = nullptr;
                HRESULT hr = pDevice->Activate(
                    __uuidof(IAudioMeterInformation),
                    CLSCTX_ALL,
                    NULL,
                    (void**)&pMeterInfo
                );

                if (SUCCEEDED(hr))
                {
                    float peakValue;
                    hr = pMeterInfo->GetPeakValue(&peakValue);
                    pMeterInfo->Release();

                    if (SUCCEEDED(hr))
                    {
                        int value = static_cast<int>(peakValue * 100);
                        return value;
                    }
                }
            }
        }

        return -1;
    }

    int GetDeviceCount()
    {
        return static_cast<int>(m_devices.size());
    }

private:
    std::vector<IMMDevice*> m_devices;
};


int main()
{
    SoundDetection detection;
    int deviceCount = detection.GetDeviceCount();

    if (deviceCount == 0)
    {
        std::cout << "No audio devices found." << std::endl;
        return 0;
    }

    std::wstring deviceName;
    int soundLevel;
    while (true) {
        for (int deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++)
        {
            deviceName = detection.GetDeviceName(deviceIndex);
            soundLevel = detection.GetSoundLevel(deviceIndex);

            if (!deviceName.empty())
            {
                std::wcout << "Device Name: " << deviceName << std::endl;
            }
            else
            {
                std::wcout << "Unable to retrieve device name." << std::endl;
            }

            if (soundLevel != -1)
            {
                std::cout << "Sound level: " << soundLevel << std::endl;
            }
            else
            {
                std::cout << "Unable to retrieve sound level." << std::endl;
            }
        }

        Sleep(100);
        system("cls");
    }

    return 0;
}