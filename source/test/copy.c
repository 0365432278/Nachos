#include "syscall.h"

int main()
{
	// char fileNameA[] = 'a.txt';
    // char fileNameB[] = 'b.txt';
    int length, idA, idB;
    int fileSize;
    char c;
    int i;

    // //Nhập file nguồn
    // PrintString("Input name of file to copy!\n");
    // PrintString("Input filename's length: (<= 255): ");
    // length = ReadNum();
    // PrintString("Input filename: ");
    // ReadString(fileNameA, length);
    // PrintString("Open file: ");
    // PrintString(fileNameA);
    // PrintString("\n");
    // //Mở file nguồn
    // idA = Open(fileNameA, 1);

    // //Nhập file đích
    // PrintString("Input name of file to paste!\n");
    // PrintString("Input filename's length: (<= 255): ");
    // length = ReadNum();
    // PrintString("Input filename: ");
    // ReadString(fileNameB, length);
    // PrintString("Open file: ");
    // PrintString(fileNameB);
    // PrintString("\n");
    // //Mở file nguồn
    // idA = Open(fileNameA, 0);

    idA = Open("a.txt", 1); // Mở file nguồn với phương thức chỉ đọc
    idB = Open("b.txt", 0); // Mở file đích với phương thức đọc và ghi

    // Copy & Paste
    if (idA != -1 && idB != -1) {
        //Di chuyển con trỏ đến cuối file, đọc kích thước file nguồn (fileSize)
		fileSize = Seek(-1, idA);
		//Di chuyển con trỏ đến đầu file, tiến hành đọc file nguồn
		Seek(0, idA);
		
		for (i = 0; i < fileSize; i++)
		{
			Read(&c, 1, idA); // Đọc từng kí tự trong file nguồn
            Write(&c, 1, idB);// Ghi từng kí tự vào file đích
			
		}
		Close(idA); // Đóng file nguồn
		Close(idB); // Đóng file đích

    } else
        // Mở file thất bại
        PrintString("Open file failed\n");
}