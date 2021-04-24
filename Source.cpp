#include <Windows.h>
#include <tchar.h>
#include "resource.h"

#define WM_USER_SHELLICON (WM_USER + 3)

static bool isSprayControlActivated = false; //����, ������������ ������ ����� ��� ���

static int X_max{};
static int Y_max{};
static int X_min{};
static int Y_min{}; //���������� ��� ��������� ������������ ������

static DWORD   dwThreadId; //������������� ������
static HANDLE  hThread; //���������� ������

static HWND hWnd_info; //���������� ��������������� ����

HINSTANCE hInst; //������������� ����������

//�������� ������� ��������� ��������� �� ����
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
//�������� �������, ������� ����� �����������, ����� ����� ����������
DWORD WINAPI SprayControlAK47(LPVOID lpParam);

//������� �������, ����� �����
int CALLBACK WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR     lpCmdLine,
    _In_ int       nCmdShow
)
{
    //��������� ��������� ������ ����
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = _T("Spray_contol_App");
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

    //����������� ������ ����
    if (!RegisterClassEx(&wcex))
    {
        MessageBox(NULL,
            _T("�� ������� ���������������� ����� ����!"),
            _T("������ ����������� ������"),
            NULL);

        return 1;
    }

    hInst = hInstance;

    //������� ������ ����, ��� ����� ������ ��� ������� ������� ������
    HWND hWnd = CreateWindowEx(WS_EX_TRANSPARENT | WS_EX_NOACTIVATE,
        _T("Spray_contol_App"),
        _T("WindowForHotKey"),
        NULL,
        0,
        0,
        0,
        0,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    //��������: ���� ���� ������� ���������� ���� �������� ��� �����, ����� �������� ����
    if (!hWnd)
    {
        MessageBox(NULL,
            _T("�� ������� ������� ����!"),
            _T("������ �������� ����"),
            NULL);

        return 1;
    }

    //������ hot key ��� ������� ���� (������� "Q")
    if (!RegisterHotKey(hWnd, 42, NULL, 0x51))
    {
        MessageBoxW(NULL,
            _T("�� ������� ���������������� Hot Key ��� ����"),
            _T("������ ����������� Hot Key"),
            MB_OK);
    }

    //������� ���������� ������, ������� ����� ���������� � ������� �����������
    NOTIFYICONDATA data{};
    data.cbSize = sizeof(data);
    data.hWnd = hWnd;
    data.uID = 1;
    data.uFlags = NIF_MESSAGE | NIF_ICON;
    data.uCallbackMessage = WM_USER_SHELLICON;
    data.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    data.uVersion = NOTIFYICON_VERSION;

    //���������� ������ � ����
    Shell_NotifyIcon(NIM_ADD, &data);

    //���� ��������� ��������� ������
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    //�������� ������ �� ����
    Shell_NotifyIcon(NIM_DELETE, &data);

    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps; //���������, ����������� ��� ��������� � ���������� ������� ����
    HDC hdc;

    switch (message) //����������� ����� ������ ��������� ������
    {
    case WM_HOTKEY: //������ ������� �������
        if (isSprayControlActivated == false) //���� - ���� ����� ������
        {
            //������� �����
            hThread = CreateThread(
                NULL,                   
                0,                      
                SprayControlAK47,       
                0,          
                0,                      
                &dwThreadId);   

            //��������: ���� ����� ������ ���������� �� ����� ����
            if (hThread == NULL)
            {
                ExitProcess(3); //� ������ ������ ������ ��������� �����������
            }

            hWnd_info = NULL; //�������� ���������� ��������������� ����

            X_max = GetSystemMetrics(SM_CXVIRTUALSCREEN); //x ���������� ������� ���� ������������ ����
            Y_max = GetSystemMetrics(SM_CYVIRTUALSCREEN); //y ���������� ������� ���� ������������ ����
            X_min = GetSystemMetrics(SM_XVIRTUALSCREEN); //x ���������� ������ ���� ������������ ������
            Y_min = GetSystemMetrics(SM_YVIRTUALSCREEN); //y ���������� ������ ���� ������������ ������

            //������� ����
            hWnd_info = CreateWindowEx(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT,
                _T("Spray_contol_App"),
                _T("WindowForInformation"),
                WS_OVERLAPPEDWINDOW & ~WS_SYSMENU & ~WS_CAPTION,
                X_min,
                Y_min,
                X_max,
                Y_max,
                NULL,
                NULL,
                hInst,
                NULL
            );

            //��������, ���������� �� ������� ����
            if (!hWnd_info)
            {
                MessageBox(NULL,
                    _T("�� ������� ������� ���� ��� ������ ����������"),
                    _T("������ �������� ����"),
                    NULL);

                return 1;
            }

            HRGN hrgn_hot = CreateRectRgn(X_max - 155, Y_min + (Y_max - Y_min) / 3, X_max - 11, Y_min + (Y_max - Y_min) / 3 + 100); //������ ���������� ��� ������� � ����
            SetWindowRgn(hWnd_info, hrgn_hot, TRUE); //������������� ������

            SetLayeredWindowAttributes(hWnd_info, RGB(255, 255, 255), 255, LWA_COLORKEY); //������ ���� ��� ������������

            ShowWindow(hWnd_info, 1); //���������� ����
            UpdateWindow(hWnd_info); //��������� ����

            isSprayControlActivated = true; //������ �������� �����
        }
        else
        {
            TerminateThread(hThread, 0); //���������� �����
            CloseHandle(hThread); //������� ����������
            dwThreadId = NULL; //�������� ������������� ������

            DestroyWindow(hWnd_info); //���������� ����

            isSprayControlActivated = false; //������ �������� �����
        }
        break;
    case WM_PAINT: //���� ������� ����� � �����������, ��� �������� ������ ��-47 �����������
        hdc = BeginPaint(hWnd, &ps);

        TextOut(hdc, X_max - 150, Y_min + (Y_max - Y_min) / 3, _T("Spray control AK-47"), 20); //������� �����

        SetTextColor(hdc, RGB(0, 128, 0)); //�������� ���� ������ �� �������
        TextOut(hdc, X_max - 117, Y_min + (Y_max - Y_min) / 3 + 18, _T("activated"), 10); //������� �����

        EndPaint(hWnd, &ps);
        break;
    case WM_USER_SHELLICON: //������� �� ������ � ���� ����� ��� ������ ������� ����
        if (lParam == WM_RBUTTONDOWN || lParam == WM_LBUTTONDOWN)
            if (MessageBoxW(NULL, _T("��������� ������?"), _T("�������� ������ ��-47"), MB_YESNO | MB_TOPMOST) == IDYES)
                PostQuitMessage(0);
    default: //���� ��������� �� ��������� �� � ����� �� ���� ���������
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }
}

//����������� ������� ������
DWORD WINAPI SprayControlAK47(LPVOID lpParam)
{
    int stage{}; //���������� ��� �����

    INPUT mouseMove{}; //��������� ����� INPUT

    //��������� ��������� INPUT ��� �������� ����������� ����
    mouseMove.type = INPUT_MOUSE;
    mouseMove.mi.dwFlags = MOUSEEVENTF_MOVE;

    while (true) //����������� ����
    {
        while (GetKeyState(VK_RBUTTON) & 0x8000) //��������, ������ �� ������ ������ ���� (������ ��� ��������)
        {
            //1 ����
            if (stage <= 20) //
            {
                Sleep(31); //��������� ����������� ���������� ������

                mouseMove.mi.dx = 1L; //����������� �� x ������������ ������� ��������� ����
                mouseMove.mi.dy = 9L; //����������� �� y ������������ ������� ��������� ����

                SendInput(1, &mouseMove, sizeof(INPUT)); //�������� �����
                stage++; //����������������� ���������� �����
            }

            /*������� ���������� ����������� ������ ����� ����� �� ��� � 1*/

            //2 ����
            if ((stage >= 21) && (stage <= 45))
            {
                Sleep(15);

                mouseMove.mi.dx = -4L;
                mouseMove.mi.dy = 1L;

                SendInput(1, &mouseMove, sizeof(INPUT));

                stage++;
            }

            //3.1 ����
            if ((stage >= 46) && (stage <= 55))
            {
                Sleep(17);

                mouseMove.mi.dx = 5L;
                mouseMove.mi.dy = 1L;

                SendInput(1, &mouseMove, sizeof(INPUT));
                stage++;
            }

            //3.2 ����
            if ((stage >= 56) && (stage <= 70))
            {
                Sleep(12);

                mouseMove.mi.dx = 3L;
                mouseMove.mi.dy = 0L;

                SendInput(1, &mouseMove, sizeof(INPUT));
                stage++;
            }

            //4 ����
            if ((stage >= 71) && (stage <= 75))
            {
                Sleep(32);

                mouseMove.mi.dx = 0L;
                mouseMove.mi.dy = 0L;

                SendInput(1, &mouseMove, sizeof(INPUT));
                stage++;
            }

            //5 ����
            if ((stage >= 76) && (stage <= 100))
            {
                Sleep(13);

                mouseMove.mi.dx = 0L;
                mouseMove.mi.dy = 1L;

                SendInput(1, &mouseMove, sizeof(INPUT));
                stage++;
            }

            //6 ����
            if ((stage >= 101) && (stage <= 125))
            {
                Sleep(13);

                mouseMove.mi.dx = -5L;
                mouseMove.mi.dy = -1L;

                SendInput(1, &mouseMove, sizeof(INPUT));
                stage++;
            }

            if (stage > 125) //���� ������ ��� ��� ������, � ���������� ����� �����������
                Sleep(1); //��������� ��������, ����� �� ��������� ���������
        }

        stage = 1;
        Sleep(1); //���� ��������� ��������, ����� ������� ��������
    }

    return 0;
}