#include <Windows.h>
#include <tchar.h>
#include "resource.h"

#define WM_USER_SHELLICON (WM_USER + 3)

static bool isSprayControlActivated = false; //флаг, определяющий создан поток или нет

static int X_max{};
static int Y_max{};
static int X_min{};
static int Y_min{}; //переменные для координат виртуального экрана

static DWORD   dwThreadId; //идентификатор потока
static HANDLE  hThread; //дескриптор потока

static HWND hWnd_info; //дескриптор информационного окна

HINSTANCE hInst; //идентификатор приложения

//прототип функции обработки сообщений от окна
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
//прототип функции, которая будет выполняться, когда поток существует
DWORD WINAPI SprayControlAK47(LPVOID lpParam);

//главная функция, точка входа
int CALLBACK WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR     lpCmdLine,
    _In_ int       nCmdShow
)
{
    //заполняем структуру класса окна
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

    //регистрация класса окна
    if (!RegisterClassEx(&wcex))
    {
        MessageBox(NULL,
            _T("Не удалось зарегистрировать класс окна!"),
            _T("Ошибка регистрации класса"),
            NULL);

        return 1;
    }

    hInst = hInstance;

    //создаем пустое окно, оно нужно только для задания горячих клавиш
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

    //проверка: если окно создано дескриптор окна содержит его адрес, иначе содержит ноль
    if (!hWnd)
    {
        MessageBox(NULL,
            _T("Не удалось создать окно!"),
            _T("Ошибка создания окна"),
            NULL);

        return 1;
    }

    //задаем hot key для пустого окна (клавиша "Q")
    if (!RegisterHotKey(hWnd, 42, NULL, 0x51))
    {
        MessageBoxW(NULL,
            _T("Не удалось зарегистрировать Hot Key для окна"),
            _T("Ошибка регистрации Hot Key"),
            MB_OK);
    }

    //задание параметров иконки, которая будет находиться в области уведомлений
    NOTIFYICONDATA data{};
    data.cbSize = sizeof(data);
    data.hWnd = hWnd;
    data.uID = 1;
    data.uFlags = NIF_MESSAGE | NIF_ICON;
    data.uCallbackMessage = WM_USER_SHELLICON;
    data.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    data.uVersion = NOTIFYICON_VERSION;

    //добавление иконки в трей
    Shell_NotifyIcon(NIM_ADD, &data);

    //цикл обработки сообщений потока
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    //удаление иконки из трея
    Shell_NotifyIcon(NIM_DELETE, &data);

    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps; //структура, необходимая для рисования в клиентской области окна
    HDC hdc;

    switch (message) //разбираемся какое именно сообщение пришло
    {
    case WM_HOTKEY: //нажата горячая клавиша
        if (isSprayControlActivated == false) //флаг - если поток создан
        {
            //создаем поток
            hThread = CreateThread(
                NULL,                   
                0,                      
                SprayControlAK47,       
                0,          
                0,                      
                &dwThreadId);   

            //проверка: если поток создан дескриптор не равен нулю
            if (hThread == NULL)
            {
                ExitProcess(3); //в случае ошибки работа программы завершается
            }

            hWnd_info = NULL; //обнуляем дескриптор информационного окна

            X_max = GetSystemMetrics(SM_CXVIRTUALSCREEN); //x координата правого угла виртуального экра
            Y_max = GetSystemMetrics(SM_CYVIRTUALSCREEN); //y координата правого угла виртуального экра
            X_min = GetSystemMetrics(SM_XVIRTUALSCREEN); //x координата левого угла виртуального экрана
            Y_min = GetSystemMetrics(SM_YVIRTUALSCREEN); //y координата левого кгла виртуального экрана

            //создаем окно
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

            //проверка, получилось ли создать окно
            if (!hWnd_info)
            {
                MessageBox(NULL,
                    _T("Не удалось создать окно для вывода информации"),
                    _T("Ошибка создания окна"),
                    NULL);

                return 1;
            }

            HRGN hrgn_hot = CreateRectRgn(X_max - 155, Y_min + (Y_max - Y_min) / 3, X_max - 11, Y_min + (Y_max - Y_min) / 3 + 100); //задаем переменную для региона в окне
            SetWindowRgn(hWnd_info, hrgn_hot, TRUE); //устанавливаем регион

            SetLayeredWindowAttributes(hWnd_info, RGB(255, 255, 255), 255, LWA_COLORKEY); //задаем цвет для прозрачности

            ShowWindow(hWnd_info, 1); //показываем окно
            UpdateWindow(hWnd_info); //обновляем окно

            isSprayControlActivated = true; //меняем значение флага
        }
        else
        {
            TerminateThread(hThread, 0); //уничтожаем поток
            CloseHandle(hThread); //очищаем дескриптор
            dwThreadId = NULL; //обнуляем идентификатор потока

            DestroyWindow(hWnd_info); //уничтожаем окно

            isSprayControlActivated = false; //меняем значение флага
        }
        break;
    case WM_PAINT: //надо вывести текст с информацией, что контроль отдачи АК-47 активирован
        hdc = BeginPaint(hWnd, &ps);

        TextOut(hdc, X_max - 150, Y_min + (Y_max - Y_min) / 3, _T("Spray control AK-47"), 20); //выводим текст

        SetTextColor(hdc, RGB(0, 128, 0)); //изменяем цвет текста на зеленый
        TextOut(hdc, X_max - 117, Y_min + (Y_max - Y_min) / 3 + 18, _T("activated"), 10); //выводим текст

        EndPaint(hWnd, &ps);
        break;
    case WM_USER_SHELLICON: //нажатие на иконку в трее левой или правой кнопкой мыши
        if (lParam == WM_RBUTTONDOWN || lParam == WM_LBUTTONDOWN)
            if (MessageBoxW(NULL, _T("Завершить работу?"), _T("Контроль отдачи АК-47"), MB_YESNO | MB_TOPMOST) == IDYES)
                PostQuitMessage(0);
    default: //если сообщение не совпадает ни с одним из выше указанных
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }
}

//определение функции потока
DWORD WINAPI SprayControlAK47(LPVOID lpParam)
{
    int stage{}; //переменная для этапа

    INPUT mouseMove{}; //структура ввода INPUT

    //заполняем структуру INPUT для указания перемещения мыши
    mouseMove.type = INPUT_MOUSE;
    mouseMove.mi.dwFlags = MOUSEEVENTF_MOVE;

    while (true) //бесконечный цикл
    {
        while (GetKeyState(VK_RBUTTON) & 0x8000) //проверка, нажата ли правая кнопка мыши (кнопка для выстрела)
        {
            //1 этап
            if (stage <= 20) //
            {
                Sleep(31); //временная приостанвка выполнения потока

                mouseMove.mi.dx = 1L; //перемещение по x относительно текущих координат мыши
                mouseMove.mi.dy = 9L; //перемещение по y относительно текущих координат мыши

                SendInput(1, &mouseMove, sizeof(INPUT)); //отправка ввода
                stage++; //инкрементирование переменной этапа
            }

            /*принцип выполнения последующих этапов точно такой же как и 1*/

            //2 этап
            if ((stage >= 21) && (stage <= 45))
            {
                Sleep(15);

                mouseMove.mi.dx = -4L;
                mouseMove.mi.dy = 1L;

                SendInput(1, &mouseMove, sizeof(INPUT));

                stage++;
            }

            //3.1 этап
            if ((stage >= 46) && (stage <= 55))
            {
                Sleep(17);

                mouseMove.mi.dx = 5L;
                mouseMove.mi.dy = 1L;

                SendInput(1, &mouseMove, sizeof(INPUT));
                stage++;
            }

            //3.2 этап
            if ((stage >= 56) && (stage <= 70))
            {
                Sleep(12);

                mouseMove.mi.dx = 3L;
                mouseMove.mi.dy = 0L;

                SendInput(1, &mouseMove, sizeof(INPUT));
                stage++;
            }

            //4 этап
            if ((stage >= 71) && (stage <= 75))
            {
                Sleep(32);

                mouseMove.mi.dx = 0L;
                mouseMove.mi.dy = 0L;

                SendInput(1, &mouseMove, sizeof(INPUT));
                stage++;
            }

            //5 этап
            if ((stage >= 76) && (stage <= 100))
            {
                Sleep(13);

                mouseMove.mi.dx = 0L;
                mouseMove.mi.dy = 1L;

                SendInput(1, &mouseMove, sizeof(INPUT));
                stage++;
            }

            //6 этап
            if ((stage >= 101) && (stage <= 125))
            {
                Sleep(13);

                mouseMove.mi.dx = -5L;
                mouseMove.mi.dy = -1L;

                SendInput(1, &mouseMove, sizeof(INPUT));
                stage++;
            }

            if (stage > 125) //если кнопка все еще зажата, а предыдущие этапы завершились
                Sleep(1); //небольшая задержка, чтобы не нагружать процессор
        }

        stage = 1;
        Sleep(1); //тоже небольшая задержка, чтобы снизить нагрузку
    }

    return 0;
}