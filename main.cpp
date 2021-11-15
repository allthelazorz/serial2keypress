#include <windows.h>
#include <stdio.h>

int ReadByte(LPCWSTR PortSpecifier);

int translatechar(char c) {
//https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
    if (0x30 <= c && c <= 0x39) {  // numbers
        return c;
    } else if (0x61 <= c && c <= 0x7a) {  // letters
        return c - 0x20;
    } else if (c == '+') {
        return VK_OEM_PLUS;
    } else if (c == '.') {
        return VK_OEM_PERIOD;
    } else if (c == '-') {
        return VK_OEM_MINUS;
    } else if (c == ',') {
        return VK_OEM_COMMA;
    } else if (c == '?') {
        return VK_OEM_2;
    } else if (c == '/') {
        return VK_OEM_2;
    } else if (c == ':') {
        return VK_OEM_1;
    } else if (c == ';') {
        return VK_OEM_1;
    } else if (c == '\'') {
        return VK_OEM_7;
    } else if (c == '"') {
        return VK_OEM_7;
    } else if (c == '=') { // =
        return VK_OEM_PLUS;
    } else if (c == '!') { // 1
        return 0x31;
    } else if (c == '@') { // 2
        return 0x32;
    } else if (c == ' ') {
        return VK_SPACE;
    }

    // these don't work    
    else if (c == '_') {   // -   // *
        return VK_OEM_MINUS;
    } else if (c == '(') { // 9   // <kn>
        return 0x39;
    } else if (c == ')') { // 0   // *
        return 0x30;
    } else if (c == '&') { // 7   // <as>
        return 0x37;
    } else if (c == '$') { // 4   // *
        return 0x34;
    }
    
    return -1;
}

int main( ) {
    printf("opening port\n");

    // in cmd: wmic path Win32_SerialPort
    ReadByte(L"\\\\.\\COM5");
}

void sendkey(char c) {
    printf("%c.", c);
    if (int key = translatechar(c)) {
        INPUT ip;
        ip.type = INPUT_KEYBOARD;
        ip.ki.wScan = 0;
        ip.ki.time = 0;
        ip.ki.dwExtraInfo = 0;

        if (c == '+' || c == '?' || c == ':' || c == '"' || c == '_' || c == '(' || c == ')' || c == '!' || c == '&' || c == '$' || c == '@') {

            ip.ki.wVk = VK_LSHIFT;
            ip.ki.dwFlags = 0;
            SendInput(1, &ip, sizeof(INPUT));

            ip.ki.wVk = key;
            ip.ki.dwFlags = 0;
            SendInput(1, &ip, sizeof(INPUT));

            ip.ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, &ip, sizeof(INPUT));

            ip.ki.wVk = VK_LSHIFT;
            ip.ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, &ip, sizeof(INPUT));
        } else {
            ip.ki.wVk = key;
            ip.ki.dwFlags = 0;
            SendInput(1, &ip, sizeof(INPUT));

            ip.ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, &ip, sizeof(INPUT));
        }
    }
}


int ReadByte(LPCWSTR PortSpecifier)
{
    DCB dcb;
    int retVal;
    BYTE Byte;
    DWORD dwBytesTransferred;
    DWORD dwCommModemStatus;

    HANDLE hPort = CreateFile(
        PortSpecifier,
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (!GetCommState(hPort, &dcb)) {
        printf("error\n");
        exit(1);
    }

    printf("setting state\n");

    dcb.BaudRate = CBR_115200;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;

    if (!SetCommState(hPort, &dcb))
        return 0x100;

    printf("so far, so good\n");

    while (true) {
        SetCommMask(hPort, EV_RXCHAR | EV_ERR);
        WaitCommEvent(hPort, &dwCommModemStatus, 0);

        if (dwCommModemStatus & EV_RXCHAR) {
            while (ReadFile(hPort, &Byte, 1, &dwBytesTransferred, 0)) {
                // use the first one if you want to pass through spaces
//                if (Byte < 128) {
                if (Byte != ' ' && Byte < 128) {
                    sendkey(Byte);
                }
            }
        }
        else if (dwCommModemStatus & EV_ERR)
            retVal = 0x101;
    }

    retVal = Byte;
    CloseHandle(hPort);
    return retVal;
}
