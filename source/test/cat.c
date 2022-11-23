#include "syscall.h"

int main()
{
	char fileName[256];
    int length, id;
    int fileSize;
    char c;
    int i;

    //Nhập filename
    PrintString("Input filename's length: (<= 255): ");
    length = ReadNum();
    PrintString("Input filename: ");
    ReadString(fileName, length);
    PrintString("Open file: ");
    PrintString(fileName);
    PrintString("\n");
    //Mở file
    id = Open(fileName, 1);

    //Hiển thị nội dung file
    if (id != -1) {
        //Di chuyển con trỏ đến cuối file, đọc kích thước file (fileSize)
		fileSize = Seek(-1, id);
		//Di chuyển con trỏ đến đầu file, tiến hành đọc file
		Seek(0, id);
		
		for (i = 0; i < fileSize; i++)
		{
			Read(&c, 1, id); // Đọc từng kí tự trong file
			PrintChar(c); // In từng kí tự
		}
		Close(id); // Đóng file
    } else
        // Mở file thất bại
        PrintString("Open file failed\n");
}