#include <Mmdeviceapi.h>
#include <Audioclient.h>
#include <Audiopolicy.h>
#include <windows.h>
#include <stdio.h>
#include <locale.h>
#include <conio.h>
#include <Functiondiscoverykeys_devpkey.h>

#define evt_OnlyMix		1	// ֻ������������
#define evt_MixandMP	2	// ����������+��˷�����
#define evt_OnlyMP		3	// ֻ����˷�����

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);

/// <summary>
///	������Ƶ�ն��豸
/// </summary>
/// <param name="ID">��Ƶ�ն��豸ID</param>
/// <param name="state">����/����:1/0</param>
INT SetEPDev(LPWSTR ID, UINT state)
{
	HKEY hKey;
	WCHAR devPath[1000] = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\MMDevices\\Audio\\Capture\\";
	DWORD devSta = state ? 0x00000001 : 0x10000001;

	lstrcat(devPath, ID);
	RegOpenKeyEx(HKEY_LOCAL_MACHINE, devPath, NULL, KEY_SET_VALUE, &hKey);
	RegSetValueEx(hKey, L"DeviceState", NULL, REG_DWORD, (BYTE*)&devSta, 4);
	RegCloseKey(hKey);

	return S_OK;
}
/// <summary>
///	������Ƶ�����豸
/// </summary>
/// <param name="pCollection">�豸����</param>
/// <param name="evt">�¼����
/// Ĭ��ֻ�����������Ƿ����
/// </param>
INT Proc(IMMDeviceCollection *pCollection, INT evt = 0)
{
	IMMDevice *pEndpoint = NULL;
	IPropertyStore *pProps = NULL;
	LPWSTR pwszID = NULL;
	LPWSTR partID = NULL;
	DWORD pdwstate = NULL;
	BOOL Enable = FALSE;
	HRESULT hr = S_OK;

	// ��ȡ��Ƶ�����豸����
	UINT  count;
	hr = pCollection->GetCount(&count);
	for (UINT i = 0; i < count; i++)
	{
		// ��ȡ������Ƶ�豸
		hr = pCollection->Item(i, &pEndpoint);
		// ��ȡ������Ƶ�豸ID
		hr = pEndpoint->GetId(&pwszID);
		partID = wcsstr(pwszID, L".{") + 1;
		// ��ȡ������Ƶ�豸������Ϣ
		hr = pEndpoint->OpenPropertyStore(STGM_READ, &pProps);
		// ����������Ϣ�豸����
		PROPVARIANT varName;
		PropVariantInit(&varName);
		hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
		switch (evt)
		{
		case 0:
			if (!wcsstr(varName.pwszVal, L"����������"))
				Enable = TRUE;
			// ��ȡ������Ƶ�豸״̬
			hr = pEndpoint->GetState(&pdwstate);
			wprintf(L"%ls %ls\n", pdwstate == DEVICE_STATE_ACTIVE ? L"ON " : L"OFF", varName.pwszVal);
			break;
		case evt_OnlyMix:
			if (wcsstr(varName.pwszVal, L"����������"))
				SetEPDev(partID, 1);
			else
				SetEPDev(partID, 0);
			break;
		case evt_MixandMP:
			SetEPDev(partID, 1);
			break;
		case evt_OnlyMP:
			if (wcsstr(varName.pwszVal, L"����������"))
				SetEPDev(partID, 0);
			else
				SetEPDev(partID, 1);
			break;
		default:
			break;
		}
		// ��ջ�ȡ����Ƶ�豸ID
		CoTaskMemFree(pwszID);
		pwszID = NULL;
		// ��ջ�ȡ����Ƶ�豸����
		PropVariantClear(&varName);
		// �����Ƶ�豸����Ϣ
		pEndpoint->Release();
		pProps->Release();
	}
	// ��ʼȷ�ϲ���ʹ��(������������)
	if (!evt && !Enable)
		return S_FALSE;
	return S_OK;
}
INT main(VOID)
{
	IMMDeviceEnumerator *pEnumerator = NULL;
	IMMDeviceCollection *pCollection = NULL;
	CHAR ch;
	HRESULT hr = S_OK;

	setlocale(LC_ALL, "");
	CoInitialize(NULL);
	hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator);
	// ������Ƶ�����ն�
	pEnumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE | DEVICE_STATE_DISABLED, &pCollection);

	while (TRUE)
	{
		if (Proc(pCollection))
		{
			printf("\n\n������֧��������������\n������˳�����");
			_getch();
			ch = '0';
		}
		else
		{
			printf("\n\nѡ��\n1: ֻ������������\n2: ����������+��˷�����\n3: ֻ����˷�����\n4: ˢ����Ƶ�����ն��豸�б�\n����: �˳�");
			ch = _getch();
		}
		ch -= '0';
		if (ch >= evt_OnlyMix && ch <= evt_OnlyMP)
			Proc(pCollection, ch);
		else if (ch == 4)
		{
			pCollection->Release();
			pEnumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE | DEVICE_STATE_DISABLED, &pCollection);
		}
		else
			break;
		system("cls");
	}

	// ����������ͼ���
	pEnumerator->Release();
	pCollection->Release();

	return 0;
}