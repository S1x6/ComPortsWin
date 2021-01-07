#include <stdio.h>
#include "stdlib.h"
#include <windows.h>
#include "structs.h"
#include "time.h"

Specs *readArgs(int, char **);

void printHelpAndExit();

void readHexToBytes(char *, Specs *);

unsigned long writeToPort(HANDLE, unsigned char *, int);

void readFromPortAndPrint(HANDLE);

void printBytesAsHex(unsigned char *bytes, unsigned long length);

int main(int argc, char *argv[]) {
    HANDLE hComm;
    SYSTEMTIME startTime, startWriteTime, endWriteTime;
    GetSystemTime(&startTime);
    Specs *specs = readArgs(argc, argv);

    hComm = CreateFileA(specs->portName,                            //port name
                        GENERIC_READ | GENERIC_WRITE, //Read/Write
                        0,                              // No Sharing
                        NULL,                        // No Security
                        OPEN_EXISTING,                              // Open existing port only
                        0,                         // Non Overlapped I/O
                        NULL);                          // Null for Comm Devices

    if (hComm == INVALID_HANDLE_VALUE) {
        printf("Error in opening serial port");
        exit(0);
    }

    printf("opening serial port successful\n");

    // set timeouts
    COMMTIMEOUTS timeouts;
    GetCommTimeouts(hComm, &timeouts);
    timeouts.ReadIntervalTimeout = specs->timeout; // wait maximum t between getting data
    timeouts.ReadTotalTimeoutConstant = 0; // no matter how much it will take in total
    SetCommTimeouts(hComm, &timeouts);

    // write to port
    GetSystemTime(&startWriteTime);
    unsigned long written = writeToPort(hComm, specs->payload, specs->payloadLength);
    GetSystemTime(&endWriteTime);
    printf("Written %lu byte(s)\n", written);

    // read the response from port and print
    readFromPortAndPrint(hComm);

    // log time
    printf("Start: %d\nWriteStart: %d\nWriteEnd: %d\n",
           (startTime.wSecond * 1000) + startTime.wMilliseconds,
           (startWriteTime.wSecond * 1000) + startWriteTime.wMilliseconds,
           (endWriteTime.wSecond * 1000) + endWriteTime.wMilliseconds);

    // free resources
    CloseHandle(hComm); // Closing the Serial Port
    free(specs->portName);
    free(specs->payload);
    free(specs);
    return 0;
}

unsigned long writeToPort(HANDLE port, unsigned char *payload, int payloadLength) {
    unsigned long bytesWritten;
    WriteFile(port,        // Handle to the Serial port
              payload,     // Data to be written to the port
              payloadLength,  //No of bytes to write
              &bytesWritten, //Bytes written
              NULL);
    return bytesWritten;
}

void readFromPortAndPrint(HANDLE port) {
    unsigned char *receive = malloc(4096);
    SYSTEMTIME startReadTime, endReadTime;
    unsigned long bytesRead;

    GetSystemTime(&startReadTime);
    ReadFile(port, receive, 4096, &bytesRead, NULL);
    GetSystemTime(&endReadTime);

    int readStart = startReadTime.wSecond * 1000 + startReadTime.wMilliseconds;
    int readEnd = endReadTime.wSecond * 1000 + endReadTime.wMilliseconds;
    printf("%lu bytes read. Started at %d ms, finished at %d ms\n", bytesRead, readStart, readEnd);
    printBytesAsHex(receive, bytesRead);
}

void printBytesAsHex(unsigned char *bytes, unsigned long length) {
    printf("Response: ");
    for (int i = 0; i < length; i++)
    {
        printf("%02X", bytes[i]);
    }
    printf("\n");
}

Specs *readArgs(int argc, char *argv[]) {
    Specs *specs = (Specs *) malloc(sizeof(Specs));
    specs->payload = NULL;
    specs->payloadLength = 0;
    specs->timeout = -1;
    specs->portName = NULL;

    for (int i = 1; i < argc; ++i) {
        char *arg = argv[i];
        if (strcmp(arg, "-t") == 0) {
            ++i;
            if (argc <= i) printHelpAndExit();
            specs->timeout = atoi(argv[i]);
        } else if (strcmp(arg, "-p") == 0) {
            ++i;
            if (argc <= i) printHelpAndExit();
            specs->portName = (char *) malloc(sizeof(char) * strlen(argv[i]) + 1); // for \0
            strcpy_s(specs->portName, sizeof(argv[i]) + 1, argv[i]);
        } else if (strcmp(arg, "-w") == 0) {
            ++i;
            if (argc <= i) printHelpAndExit();
            readHexToBytes(argv[i], specs);
        } else printHelpAndExit();
    }

    if (specs->payload == 0 || specs->portName == NULL || specs->timeout == -1) printHelpAndExit();
    return specs;
}

void printHelpAndExit() {
    printf("args (all required):\n\t-t\tвремя ожидания ответа в мс\n\t-p\tимя порта\n\t-w\tотправляемые hex данные (например 00ABC8DF) ");
    exit(0);
}

void readHexToBytes(char *hex, Specs *specs) {
    int length = strlen(hex) / 2;
    unsigned char *val = (unsigned char *) malloc(length);
    const char *pos = hex;

    /* WARNING: no sanitization or error-checking whatsoever */
    for (size_t count = 0; count < length; count++) {
        sscanf(pos, "%2hhX", &val[count]);
        pos += 2;
    }

    specs->payload = val;
    specs->payloadLength = length;
}
