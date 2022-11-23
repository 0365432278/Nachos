// exception.cc
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "syscall.h"
#include "ksyscall.h"
//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2.
//
// If you are handling a system call, don't forget to increment the pc
// before returning. (Or else you'll loop making the same system call forever!)
//
//	"which" is the kind of exception.  The list of possible exceptions
//	is in machine.h.
//----------------------------------------------------------------------
#define MAX_READ_STRING_LENGTH 255

/**
 * @brief Convert system string to user string
 *
 * @param str string to convert
 * @param addr addess of user string
 * @param convert_length set max length of string to convert, leave
 * blank to convert all characters of system string
 * @return void
 */

void increaseProCounter()
{
	/* set previous programm counter (debugging only)*/
	kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

	/* set programm counter to next instruction (all Instructions are 4 byte wide)*/
	kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

	/* set next programm counter for brach execution */
	kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);
}

void StringSys2User(char *str, int addr, int convert_length = -1)
{
	int length = convert_length;
	if (convert_length == -1)
	{
		length = strlen(str);
	}

	for (int i = 0; i < length; i++)
	{
		kernel->machine->WriteMem(addr + i, 1,
								  str[i]); // copy characters to user space
	}
	kernel->machine->WriteMem(addr + length, 1, '\0');
}

char *stringUser2System(int addr, int convert_length = -1)
{
	int length = 0;
	bool stop = false;
	char *str;

	do
	{
		int oneChar;
		kernel->machine->ReadMem(addr + length, 1, &oneChar);
		length++;
		// if convert_length == -1, we use '\0' to terminate the process
		// otherwise, we use convert_length to terminate the process
		stop = ((oneChar == '\0' && convert_length == -1) ||
				length == convert_length);
	} while (!stop);

	str = new char[length];
	for (int i = 0; i < length; i++)
	{
		int oneChar;
		kernel->machine->ReadMem(addr + i, 1,
								 &oneChar); // copy characters to kernel space
		str[i] = (unsigned char)oneChar;
	}
	return str;
}

void ExceptionHandler(ExceptionType which)
{
	int type = kernel->machine->ReadRegister(2);

	DEBUG(dbgSys, "Received Exception " << which << " type: " << type << "\n");

	switch (which)
	{
	case NoException: // return control to kernel
		kernel->interrupt->setStatus(SystemMode);
		DEBUG(dbgSys, "Switch to system mode\n");
		break;
	case PageFaultException:
	case ReadOnlyException:
	case BusErrorException:
	case AddressErrorException:
	case OverflowException:
	case IllegalInstrException:
	case NumExceptionTypes:
		cerr << "Error " << which << " occurs\n";
		SysHalt();
		ASSERTNOTREACHED();
		break;
	case SyscallException:
		switch (type)
		{
			int result;
		case SC_Halt:
		{
			DEBUG(dbgSys, "Shutdown, initiated by user program.\n");
			SysHalt();
			ASSERTNOTREACHED();
			break;
		}
		case SC_Add:
		{
			DEBUG(dbgSys, "Add " << kernel->machine->ReadRegister(4) << " + " << kernel->machine->ReadRegister(5) << "\n");

			/* Process SysAdd Systemcall*/
			// int result;
			result = SysAdd(/* int op1 */ (int)kernel->machine->ReadRegister(4),
							/* int op2 */ (int)kernel->machine->ReadRegister(5));

			DEBUG(dbgSys, "Add returning with " << result << "\n");
			/* Prepare Result */
			kernel->machine->WriteRegister(2, (int)result);

			/* Modify return point */
			increaseProCounter();

			return;
		}
		case SC_Sub:
		{
			DEBUG(dbgSys, "Sub " << kernel->machine->ReadRegister(4) << " - " << kernel->machine->ReadRegister(5) << "\n");

			/* Process SysAdd Systemcall*/
			// int result;
			result = SysSub(/* int op1 */ (int)kernel->machine->ReadRegister(4),
							/* int op2 */ (int)kernel->machine->ReadRegister(5));

			DEBUG(dbgSys, "Sub returning with " << result << "\n");
			/* Prepare Result */
			kernel->machine->WriteRegister(2, (int)result);

			/* Modify return point */
			increaseProCounter();

			return;
		}
		case SC_ReadNum:
		{
			result = SysReadNum();
			kernel->machine->WriteRegister(2, result);

			increaseProCounter();
			return;
		}
		case SC_PrintNum:
		{
			int character = kernel->machine->ReadRegister(4);
			SysPrintNum(character);
			increaseProCounter();
			return;
		}
		case SC_ReadChar:
		{
			char result = SysReadChar();
			kernel->machine->WriteRegister(2, (int)result);
			increaseProCounter();
			return;
		}
		case SC_PrintChar:
		{
			char character = (char)kernel->machine->ReadRegister(4);
			SysPrintChar(character);
			increaseProCounter();
			return;
		}
		case SC_RandomNum:
		{
			result = SysRandomNum();
			kernel->machine->WriteRegister(2, result);
			increaseProCounter();
			return;
		}
		case SC_ReadString:
		{
			int memPtr = kernel->machine->ReadRegister(4); // read address of C-string
			int length = kernel->machine->ReadRegister(5); // read length of C-string
			if (length > MAX_READ_STRING_LENGTH)
			{ // avoid allocating large memory
				DEBUG(dbgSys, "String length exceeds " << MAX_READ_STRING_LENGTH);
				SysHalt();
			}
			char *buffer = SysReadString(length);
			StringSys2User(buffer, memPtr);
			delete[] buffer;
			increaseProCounter();
			return;
		}
		case SC_PrintString:
		{
			int memPtr = kernel->machine->ReadRegister(4); // read address of C-string
			char *buffer = stringUser2System(memPtr);

			SysPrintString(buffer, strlen(buffer));
			delete[] buffer;
			increaseProCounter();
			return;
		}
		case SC_Exit:
		{
			cerr << "\n";
			return increaseProCounter();
		}
		case SC_Create:
		{
			// Input: Dia chi tu vung nho user cua ten file
			// Output: -1 = Loi, 0 = Thanh cong
			// Chuc nang: Tao ra file voi tham so la ten file
			int virtAddr;
			char* filename;
			DEBUG('a', "\n SC_CreateFile call ...");
			DEBUG('a', "\n Reading virtual address of filename");

			virtAddr = kernel->machine->ReadRegister(4); //Doc dia chi cua file tu thanh ghi R4
			DEBUG('a', "\n Reading filename.");
			
			//Sao chep khong gian bo nho User sang System, voi do dang toi da la (32 + 1) bytes
			filename = stringUser2System(virtAddr, 32 + 1);
			if (strlen(filename) == 0)
			{
				printf("\n File name is not valid");
				DEBUG('a', "\n File name is not valid");
				kernel->machine->WriteRegister(2, -1); //Return -1 vao thanh ghi R2
				increaseProCounter();
				return;
			}
			
			if (filename == NULL)  //Neu khong doc duoc
			{
				printf("\n Not enough memory in system");
				DEBUG('a', "\n Not enough memory in system");
				kernel->machine->WriteRegister(2, -1); //Return -1 vao thanh ghi R2
				delete filename;
				increaseProCounter();
				return;
			}
			DEBUG('a', "\n Finish reading filename.");
			
			if (!kernel->fileSystem->Create(filename)) //Tao file bang ham Create cua fileSystem, tra ve ket qua
			{
				//Tao file that bai
				printf("\n Error create file '%s'", filename);
				kernel->machine->WriteRegister(2, -1);
				delete filename;
				increaseProCounter();
				return;
			}
			
			//Tao file thanh cong
			kernel->machine->WriteRegister(2, 0);
			delete filename;
			increaseProCounter(); //Day thanh ghi lui ve sau de tiep tuc ghi
			return;
		}
		case SC_Open:
		{
			// Input: arg1: Dia chi cua chuoi name
			// Output: Tra ve OpenFileID neu thanh cong, -1 neu loi
			// Chuc nang: Tra ve ID cua file.
	 
			int virtAddr = kernel->machine->ReadRegister(4); // Lay dia chi cua tham so name tu thanh ghi so 4
			int type = kernel->machine->ReadRegister(5); // Lay tham so type tu thanh ghi so 5
			char* filename;
			filename = stringUser2System(virtAddr, 32); // Copy chuoi tu vung nho User Space sang System Space voi bo dem name dai MaxFileLength
			
			//Tìm slot trống
			int freeSlot = kernel->fileSystem->findFreeSlot();

			if (freeSlot != -1) // Tim duoc slot trong
			{
				if (type == 0 || type == 1) // File được mở để đọc và viết type=0
											// hoặc chỉ đọc type = 1
				{
					
					if ((kernel->fileSystem->openf[freeSlot] = kernel->fileSystem->Open(filename, type)) != NULL) //Mo file thanh cong
					{
						kernel->machine->WriteRegister(2, freeSlot); //Ghi OpenFileID (vị trí slot trống) vào thanh ghi 2

					}
				}
				else if (type == 2) // Xử lí stdin khi type = 2
				{
					kernel->machine->WriteRegister(2, 0); //Ghi OpenFileID vào thanh ghi 2
				}
				else // Xử lí stdout khi type = 3
				{
					kernel->machine->WriteRegister(2, 1); //Ghi OpenFileID vào thanh ghi 2
				}
				delete[] filename;
				increaseProCounter();
				return;
			}
			kernel->machine->WriteRegister(2, -1); //Không mở được file: ghi -1 vào thanh ghi 2
			
			delete[] filename;
			increaseProCounter();
			return;
		}
		case SC_Close:
		{
			//Input id cua file(OpenFileID)
			// Output: 0: thanh cong, -1 that bai
			int fid = kernel->machine->ReadRegister(4); // Lay id cua file tu thanh ghi so 4
			if (fid >= 0 && fid <= 14) //Chi xu li khi fid nam trong [0, 14]
			{
				if (kernel->fileSystem->openf[fid]) //neu mo file thanh cong
				{
					delete kernel->fileSystem->openf[fid]; //Xoa vung nho luu tru file
					kernel->fileSystem->openf[fid] = NULL; //Gan vung nho NULL
					kernel->machine->WriteRegister(2, 0);
					increaseProCounter();
					return;
				}
			}
			kernel->machine->WriteRegister(2, -1);
			increaseProCounter();
			return;
		}
		case SC_Read:
		{
			// Input: buffer(char*), so ky tu(int), id cua file(OpenFileID)
			// Output: -1: Loi, So byte read thuc su: Thanh cong, -2: Thanh cong
			// Cong dung: Doc file voi tham so la buffer, so ky tu cho phep va id cua file
			int virtAddr = kernel->machine->ReadRegister(4); // Lay dia chi cua tham so buffer tu thanh ghi so 4
			int charcount = kernel->machine->ReadRegister(5); // Lay charcount tu thanh ghi so 5
			int id = kernel->machine->ReadRegister(6); // Lay id cua file tu thanh ghi so 6 
			int OldPos;
			int NewPos;
			char *buf;
			// Kiem tra id cua file truyen vao co nam ngoai bang mo ta file khong
			if (id < 0 || id > 14)
			{
				printf("\nKhong the read vi id nam ngoai bang mo ta file.");
				kernel->machine->WriteRegister(2, -1);
				increaseProCounter();
				return;
			}
			// Kiem tra file co ton tai khong
			if (kernel->fileSystem->openf[id] == NULL)
			{
				printf("\nKhong the read vi file nay khong ton tai.");
				kernel->machine->WriteRegister(2, -1);
				increaseProCounter();
				return;
			}
			if (kernel->fileSystem->openf[id]->type == 3) // Xet truong hop doc file stdout (type quy uoc la 3) thi tra ve -1
			{
				printf("\nKhong the read file stdout.");
				kernel->machine->WriteRegister(2, -1);
				increaseProCounter();
				return;
			}
			OldPos = kernel->fileSystem->openf[id]->GetCurrentPos(); // Kiem tra thanh cong thi lay vi tri OldPos
			buf = stringUser2System(virtAddr, charcount); // Copy chuoi tu vung nho User Space sang System Space voi bo dem buffer dai charcount
			// Xet truong hop doc file stdin (type quy uoc la 2)
			if (kernel->fileSystem->openf[id]->type == 2)
			{
				// Su dung ham Read cua lop SynchConsole de tra ve so byte thuc su doc duoc
				int size = kernel->synchConsoleIn->GetString(buf, charcount); 
				StringSys2User(buf, virtAddr, size); // Copy chuoi tu vung nho System Space sang User Space voi bo dem buffer co do dai la so byte thuc su
				kernel->machine->WriteRegister(2, size); // Tra ve so byte thuc su doc duoc
				delete buf;
				increaseProCounter();
				return;
			}
			// Xet truong hop doc file binh thuong thi tra ve so byte thuc su
			if ((kernel->fileSystem->openf[id]->Read(buf, charcount)) > 0)
			{
				// So byte thuc su = NewPos - OldPos
				NewPos = kernel->fileSystem->openf[id]->GetCurrentPos();
				// Copy chuoi tu vung nho System Space sang User Space voi bo dem buffer co do dai la so byte thuc su 
				StringSys2User(buf, virtAddr, NewPos - OldPos); 
				kernel->machine->WriteRegister(2, NewPos - OldPos);
			}
			else
			{
				// Truong hop con lai la doc file co noi dung la NULL tra ve -2
				//printf("\nDoc file rong.");
				kernel->machine->WriteRegister(2, -2);
			}
			delete buf;
			increaseProCounter();
			return;
		}
		case SC_Write:
		{
			// Input: buffer(char*), so ky tu(int), id cua file(OpenFileID)
			// Output: -1: Loi, So byte write thuc su: Thanh cong, -2: Thanh cong
			// Cong dung: Ghi file voi tham so la buffer, so ky tu cho phep va id cua file
			int virtAddr = kernel->machine->ReadRegister(4); // Lay dia chi cua tham so buffer tu thanh ghi so 4
			int charcount = kernel->machine->ReadRegister(5); // Lay charcount tu thanh ghi so 5
			int id = kernel->machine->ReadRegister(6); // Lay id cua file tu thanh ghi so 6
			int OldPos;
			int NewPos;
			char *buf;
			// Kiem tra id cua file truyen vao co nam ngoai bang mo ta file khong
			if (id < 0 || id > 14)
			{
				printf("\nKhong the write vi id nam ngoai bang mo ta file.");
				kernel->machine->WriteRegister(2, -1);
				increaseProCounter();
				return;
			}
			// Kiem tra file co ton tai khong
			if (kernel->fileSystem->openf[id] == NULL)
			{
				printf("\nKhong the write vi file nay khong ton tai.");
				kernel->machine->WriteRegister(2, -1);
				increaseProCounter();
				return;
			}
			// Xet truong hop ghi file only read (type quy uoc la 1) hoac file stdin (type quy uoc la 2) thi tra ve -1
			if (kernel->fileSystem->openf[id]->type == 1 || kernel->fileSystem->openf[id]->type == 2)
			{
				printf("\nKhong the write file stdin hoac file only read.");
				kernel->machine->WriteRegister(2, -1);
				increaseProCounter();
				return;
			}
			OldPos = kernel->fileSystem->openf[id]->GetCurrentPos(); // Kiem tra thanh cong thi lay vi tri OldPos
			buf = stringUser2System(virtAddr, charcount);  // Copy chuoi tu vung nho User Space sang System Space voi bo dem buffer dai charcount
			// Xet truong hop ghi file read & write (type quy uoc la 0) thi tra ve so byte thuc su
			if (kernel->fileSystem->openf[id]->type == 0)
			{
				if ((kernel->fileSystem->openf[id]->Write(buf, charcount)) > 0)
				{
					// So byte thuc su = NewPos - OldPos
					NewPos = kernel->fileSystem->openf[id]->GetCurrentPos();
					kernel->machine->WriteRegister(2, NewPos - OldPos);
					delete buf;
					increaseProCounter();
					return;
				}
			}
			if (kernel->fileSystem->openf[id]->type == 3) // Xet truong hop con lai ghi file stdout (type quy uoc la 3)
			{
				int i = 0;
				while (buf[i] != 0 && buf[i] != '\n') // Vong lap de write den khi gap ky tu '\n'
				{
					kernel->synchConsoleOut->PutString(buf + i, 1); // Su dung ham Write cua lop SynchConsole 
					i++;
				}
				buf[i] = '\n';
				kernel->synchConsoleOut->PutString(buf + i, 1); // Write ky tu '\n'
				kernel->machine->WriteRegister(2, i - 1); // Tra ve so byte thuc su write duoc
				delete buf;
				increaseProCounter();
				return;
			}
		}

		case SC_Seek:
		{
			// Input: Vi tri(int), id cua file(OpenFileID)
			// Output: -1: Loi, Vi tri thuc su: Thanh cong
			// Cong dung: Di chuyen con tro den vi tri thich hop trong file voi tham so la vi tri can chuyen va id cua file
			int pos = kernel->machine->ReadRegister(4); // Lay vi tri can chuyen con tro den trong file
			int id = kernel->machine->ReadRegister(5); // Lay id cua file
			// Kiem tra id cua file truyen vao co nam ngoai bang mo ta file khong
			if (id < 0 || id > 14)
			{
				printf("\nKhong the seek vi id nam ngoai bang mo ta file.");
				kernel->machine->WriteRegister(2, -1);
				increaseProCounter();
				return;
			}
			// Kiem tra file co ton tai khong
			if (kernel->fileSystem->openf[id] == NULL)
			{
				printf("\nKhong the seek vi file nay khong ton tai.");
				kernel->machine->WriteRegister(2, -1);
				increaseProCounter();
				return;
			}
			// Kiem tra co goi Seek tren console khong
			if (id == 0 || id == 1)
			{
				printf("\nKhong the seek tren file console.");
				kernel->machine->WriteRegister(2, -1);
				increaseProCounter();
				return;
			}
			// Neu pos = -1 thi gan pos = Length nguoc lai thi giu nguyen pos
			pos = (pos == -1) ? kernel->fileSystem->openf[id]->Length() : pos;
			if (pos > kernel->fileSystem->openf[id]->Length() || pos < 0) // Kiem tra lai vi tri pos co hop le khong
			{
				printf("\nKhong the seek file den vi tri nay.");
				kernel->machine->WriteRegister(2, -1);
			}
			else
			{
				// Neu hop le thi tra ve vi tri di chuyen thuc su trong file
				kernel->fileSystem->openf[id]->Seek(pos);
				kernel->machine->WriteRegister(2, pos);
			}
			increaseProCounter();
			return;
		}
		case SC_Remove:
		{
			// Input: Ten file (char*)
			// Output: -1: Loi, 0: Thanh cong
			// Cong dung: Xoa file
			int virtAddr;
			char* filename;
			DEBUG('a', "\n SC_Remove call ...");
			DEBUG('a', "\n Reading virtual address of filename");

			virtAddr = kernel->machine->ReadRegister(4); //Doc dia chi cua file tu thanh ghi R4
			DEBUG('a', "\n Reading filename.");
			
			//Sao chep khong gian bo nho User sang System, voi do dang toi da la (32 + 1) bytes
			filename = stringUser2System(virtAddr, 32 + 1);
			if (strlen(filename) == 0)
			{
				printf("\n File name is not valid");
				DEBUG('a', "\n File name is not valid");
				kernel->machine->WriteRegister(2, -1); //Return -1 vao thanh ghi R2
				increaseProCounter();
				return;
			}
			
			if (filename == NULL)  //Neu khong doc duoc
			{
				printf("\n Not enough memory in system");
				DEBUG('a', "\n Not enough memory in system");
				kernel->machine->WriteRegister(2, -1); //Return -1 vao thanh ghi R2
				delete filename;
				increaseProCounter();
				return;
			}
			DEBUG('a', "\n Finish reading filename.");
			
			if (!kernel->fileSystem->Remove(filename)) //Xoa file bang ham Remove cua fileSystem, tra ve ket qua
			{
				//Xoa file that bai
				printf("\n Error remove file '%s'", filename);
				kernel->machine->WriteRegister(2, -1);
				delete filename;
				increaseProCounter();
				return;
			}
			
			//Xoa file thanh cong
			kernel->machine->WriteRegister(2, 0);
			delete filename;
			increaseProCounter(); //Day thanh ghi lui ve sau de tiep tuc ghi
			return;
		}
		case SC_ThreadFork:
		case SC_ThreadYield:
		case SC_ExecV:
		case SC_ThreadExit:
		case SC_ThreadJoin:
		{
			DEBUG(dbgSys, "Not yet implemented syscall " << type << "\n");
			increaseProCounter();
			return;
		}
		default:
			cerr << "Unexpected system call " << type << "\n";
			break;
		}
		break;
	default:
		cerr << "Unexpected user mode exception" << (int)which << "\n";
		break;
	}
	ASSERTNOTREACHED();
}