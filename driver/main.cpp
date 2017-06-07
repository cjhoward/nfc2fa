#include <cstdio>
#include <wchar.h>
#include <string.h>
#include <cstdlib>
#include <stdint.h>
#include <iostream>
#include <string>
#include "hidapi.h"

#ifdef _WIN32
	#include <windows.h>
	#include <conio.h>
#else
	#include <unistd.h>
#endif

static char get_keystroke(void)
{
	if (_kbhit()) {
		char c = _getch();
		if (c >= 32) return c;
	}
	return 0;
}


#if (defined(WIN32) || defined(WINDOWS) || defined(__WINDOWS__)) 
#include <windows.h>
static void delay_ms(unsigned int msec)
{
	Sleep(msec);
}
#else
#include <unistd.h>
#include <termios.h>
static void delay_ms(unsigned int msec)
{
	usleep(msec * 1000);
}
#endif

void set_clipboard(const char* text, size_t length)
{
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, length + 1);
    memcpy(GlobalLock(hMem), text, length + 1);
    GlobalUnlock(hMem);
    OpenClipboard(0);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();
}

void setEcho(bool enabled)
{
#ifdef WIN32
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE); 
    DWORD mode;
    GetConsoleMode(hStdin, &mode);

    if( !enabled)
        mode &= ~ENABLE_ECHO_INPUT;
    else
        mode |= ENABLE_ECHO_INPUT;

    SetConsoleMode(hStdin, mode );

#else
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    if( !enabled )
        tty.c_lflag &= ~ECHO;
    else
        tty.c_lflag |= ECHO;

    (void) tcsetattr(STDIN_FILENO, TCSANOW, &tty);
#endif
}

#define VENDOR_ID 0x16C0
#define PRODUCT_ID 0x0486
#define PACKET_SIZE 64
#define HID_BUFFER_SIZE PACKET_SIZE + 1
#define CIPHER_SIZE 32

#define COMMAND_READ 0x01
#define COMMAND_WRITE 0x02

#define MESSAGE_TAG_DETECTED 0x01
#define MESSAGE_TAG_READ 0x02
#define MAX_UID_LENGTH 7

hid_device* device = NULL;
unsigned char hid_buffer[HID_BUFFER_SIZE];

void send_command(unsigned char command)
{
	hid_buffer[0] = 0;
	hid_buffer[1] = command;
	hid_write(device, hid_buffer, 2);
}

int main(int argc, char* argv[])
{
	// Init HID API
	if (hid_init())
	{
		std::printf("Failed to init HID API\n");
		return -1;
	}
	
	std::printf("Waiting for NFC controller...");
	while (1)
	{
		// Attempt to open device
		device = hid_open(VENDOR_ID, PRODUCT_ID, NULL);
		if (device == NULL)
		{
			// Retry
			delay_ms(1000);
			continue;
		}
		
		std::printf(" connected!\n");
		
		// Set the hid_read() function to be non-blocking.
		hid_set_nonblocking(device, 1);
		
		// 
		std::printf("Press 'r' to read or 'w' to write\n");
		
		// Listen for packets
		while (1)
		{
			// Listen for commands
			char c;
			while ((c = get_keystroke()) >= 32)
			{
				/*
				printf("got key '%c', sending...\n", c);
				buffer[0] = 0;
				buffer[1] = c;
				for (int i = 2; i < HID_BUFFER_SIZE; ++i)
				{
					buffer[i] = 0x00;
				}
				
				hid_write(device, buffer, BUFFER_SIZE);
				*/
				
				if (c == 'r')
				{
					send_command(COMMAND_READ);
					std::cout << "Hold your NFC tag to the reader\n";
					
					// Tell NFC controller to enter read mode
					// ...
					
					/*
					printf("Hold your NFC tag to the reader\n");
					std::cout << "Enter your secondary password: ";
					
					char secondaryPassword[256 + 1];
					
					setEcho(false);
					std::cin.getline(secondaryPassword, 257);
					setEcho(true);
					
					std::cout << "Your primary password was successfully decrypted and copied to the clipboard" << std::endl;
					
					size_t ciphertext_length = (size_t)buffer[2];
					char* ciphertext = new char[ciphertext_length];
					for (size_t i = 0; i < ciphertext_length; ++i)
					{
						
						ciphertext[i] = buffer[3 + i];
					}
					
					set_clipboard(ciphertext, ciphertext_length);
					
					delete[] ciphertext;
					*/
				}
				else if (c == 'w')
				{
					send_command(COMMAND_WRITE);
					
					/*
					printf("Enter your primary password: ");
					printf("\n");
					printf("Enter your primary password again: ");
					printf("\n");
					printf("Enter your secondary password: ");
					printf("\n");
					printf("Enter your secondary password again: ");
					printf("\n");
					printf("Are you sure you want to overwrite your NFC tag? y/n");
					printf("\n");
					printf("Hold your tag up to the reader");
					printf("\n");*/
				}
				else
				{
					std::printf("Unknown command\n");
				}
			}
			
			// Attempt to read packet
			int size = hid_read_timeout(device, hid_buffer, HID_BUFFER_SIZE, 200);
			if (size < 0)
			{
				// Device disconnected
				break;
			}
			else if (size == 0)
			{
				// Empty packet
				continue;
			}
			else
			{
				if (hid_buffer[2] & MESSAGE_TAG_DETECTED)
				{
					std::cout << "Detected NFC tag ";
					
					uint8_t uidLength = hid_buffer[3];
					uint8_t uid[MAX_UID_LENGTH];
					for (uint8_t i = 0; i < uidLength; ++i)
					{
						uid[i] = hid_buffer[4 + i];
						std::printf("%02X", uid[i]);
						if (i < uidLength - 1)
						{
							std::cout << ":";
						}
					}
					
					std::cout << std::endl;
				}
				else if (hid_buffer[2] & MESSAGE_TAG_READ)
				{
					
				}
				/*
				// Packet received
				printf("\nReceived %d bytes:\n", size);
				for (int i = 0; i < size; ++i)
				{
					printf("%02X ", buffer[i] & 255);
					if (i % 16 == 15 && i < size - 1)
						printf("\n");
				}
				
				printf("\n");
				*/
			}
		}
		
		// Close device
		hid_close(device);
		
		std::printf("NFC controller disconnected.\nWaiting for reconnection...");
	}
	
	// Close HID API
	hid_exit();

	return 0;
}
