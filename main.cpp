#include <windows.h>
#include <stdio.h>

#include <iostream>

# define COMPORT_LENGTH 100

void wait_exit(int code) {
    printf("press enter to exit\n");
    std::getchar();
    exit(code);
}

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
    
    return 0;
}

bool space_passthrough = false;

int main( ) {
    printf("opening serial2keypress.cfg\n");
    FILE *conffile;
    fopen_s(&conffile, "serial2keypress.cfg", "r");
    if (!conffile) {
        printf("couldn't find config file, creating a new one\n");
        fopen_s(&conffile, "serial2keypress.cfg", "w");
        if (!conffile) {
            printf("couldn't open serial2keypress.cfg in the current location for writing, exiting\n");
            wait_exit(1);
        }
        fprintf(conffile, "\\\\.\\COM5\n");
        fprintf(conffile, "no_space_passthrough\n");
        fclose(conffile);
        printf("wrote conffile with default configuration, please start the program again\n");
        wait_exit(0);
    }

    int comport_length = COMPORT_LENGTH;
    char comport[COMPORT_LENGTH];

    if (!fgets(comport, comport_length, conffile)) {
        printf("configuration file error: first line should be COM port name, e.g.: \\\\.\\COM5\n");
        wait_exit(1);
    }

    comport[strcspn(comport, "\n")] = 0;

    wchar_t wcomport[COMPORT_LENGTH * 2];
    size_t* retval = NULL;
    mbstowcs_s(retval, wcomport, sizeof(wcomport), comport, strlen(comport) + 1);  // not sure about this

    std::wcout << "\"" << wcomport << "\"" << std::endl;


    char sp_passthrough[COMPORT_LENGTH];

    if (!fgets(sp_passthrough, COMPORT_LENGTH, conffile)) {
        printf("configuration file error: second line should be either space_passthrough or no_space_passthrough\n");
        wait_exit(1);
    }

    fclose(conffile);

    sp_passthrough[strcspn(sp_passthrough, "\n")] = 0;

    if (!strcmp(sp_passthrough, "no_space_passthrough")) {
        space_passthrough = false;
    } else if (!strcmp(sp_passthrough, "space_passthrough")) {
        space_passthrough = true;
    } else {
        printf("configuration file error: second line should be either space_passthrough or no_space_passthrough\n");
        wait_exit(1);
    }

    printf("space passthrough: %d\n", space_passthrough);



    printf("opening port %s\n", comport);


    // in cmd: wmic path Win32_SerialPort
//    ReadByte(L"\\\\.\\COM5");
    ReadByte(wcomport);
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
    DWORD dwCommModemStatus = 0;

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
        printf("error getting port state\n");
        wait_exit(1);
    }

    printf("setting state\n");

    dcb.BaudRate = CBR_115200;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;

    if (!SetCommState(hPort, &dcb))
        return 0x100;

    printf("everything seems good, reading characters\n");

    while (true) {
        SetCommMask(hPort, EV_RXCHAR | EV_ERR);
        WaitCommEvent(hPort, &dwCommModemStatus, 0);

        if (dwCommModemStatus & EV_RXCHAR) {
            while (ReadFile(hPort, &Byte, 1, &dwBytesTransferred, 0)) {
                    if (Byte != ' ' || space_passthrough) {
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
