#include "syscall.h"

int main() {
    char fileName[256];
    int length;

    //Đọc tên file do người dùng nhập vào
    PrintString("Enter file's name's length: ");
    length = ReadNum();
    PrintString("Enter file's name: ");
    ReadString(fileName, length);

    //Tạo file
    if (Create(fileName) == 0) {
        PrintString("File ");
        PrintString(fileName);
        PrintString(" created successfully!\n");
    } else
        PrintString("Create file failed\n");
}