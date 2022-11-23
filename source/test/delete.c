#include "syscall.h"

int main() {
    char fileName[256];
    int length;

    //Đọc tên file do người dùng nhập vào
    PrintString("Enter file's name's length: ");
    length = ReadNum();
    PrintString("Enter file's name: ");
    ReadString(fileName, length);

    //Xóa file
    if (Remove(fileName) == 0) {
        PrintString("File ");
        PrintString(fileName);
        PrintString(" removed successfully!\n");
    } else
        PrintString("Remove file failed\n");
}