#include <string>
#include <iostream>
#include<Windows.h>

struct publicComLibVariables
{
	HANDLE hFileMap = NULL;
	void* mData;
	bool exists = false;
	unsigned int mSize = 0;// 1 << 20; // eller 1 << 20;
	//temp char (byt?)
	int offset = 0;
	//temp
	int offsetRead = 0;
	int ID = 0;
};

struct shardInfo
{
	int head;
	int tail;
};

class ComLib
{
public:
    enum TYPE { PRODUCER, CONSUMER };
	publicComLibVariables v;
	shardInfo sInfo;
    // Constructor
    // secret is the known name for the shared memory
    // buffSize is in MEGABYTES (multiple of 1<<20)
    // type is TYPE::PRODUCER or TYPE::CONSUMER
	ComLib(const std::string& secret, const size_t& buffSize, TYPE type);

    // returns "true" if data was sent successfully.
    // false if for ANY reason the data could not be sent.
    // we will not implement an "error handling" mechanism, so we will assume
    // that false means that there was no space in the buffer to put the message.
    // msg is a void pointer to the data.
    // length is the amount of bytes of the message to send.
	bool send(const void* msg, const size_t length);

    // returns: "true" if a message was received.
    // false if there was nothing to read.
    // "msg" is expected to have enough space for the message.
    // use "nextSize()" to check whether our temporary buffer is big enough
    // to hold the next message.
    // @length returns the size of the message just read.
    // @msg contains the actual message read.
	bool recv(char*& msg, size_t& length);

    // return the length of the next message
    // return 0 if no message is available.
    size_t nextSize();

    /* destroy all resources */
	~ComLib();

private:
	TYPE myType;

};

ComLib::ComLib(const std::string& secret, const size_t& buffSize, TYPE type)
{
	// 2) API call to create FileMap (use pagefile as a backup)
	v.mSize = buffSize;
	myType = type;
	if (type == TYPE::PRODUCER)
	{

		//v.shardMemName = secret;
		v.hFileMap = CreateFileMapping(
			INVALID_HANDLE_VALUE,
			NULL,
			PAGE_READWRITE,
			(DWORD)0,
			(DWORD)(buffSize + sizeof(sInfo)),
			(LPCWSTR)secret.c_str()); //"myFileMap"
		// 3) check if hFileMap is NULL -> FATAL ERROR
		//if (v.hFileMap == NULL)
		//	std::cout << "error hFileMap = NULL.... \n";
		//// 4) check 
		//if (GetLastError() == ERROR_ALREADY_EXISTS)
		//	std::cout << "already existed \n";
		//if (GetLastError() == NO_ERROR)
		//	std::cout << "now it exist \n";

	}
	else if (type = TYPE::CONSUMER)
	{
		 do
		{
			v.hFileMap = CreateFileMapping(
				INVALID_HANDLE_VALUE,
				NULL,
				PAGE_READWRITE,
				(DWORD)0,
				(DWORD)(buffSize + sizeof(sInfo)),
				(LPCWSTR)secret.c_str()); //"myFileMap"
			// 3) check if hFileMap is NULL -> FATAL ERROR
			//if (v.hFileMap == NULL)
			//	std::cout << "error hFileMap = NULL.... \n";
			// 4) check 
			//if (GetLastError() == ERROR_ALREADY_EXISTS)
			//	std::cout << "already existed \n";
			if (GetLastError() == NO_ERROR)
			{
				//std::cout << "The file is not open try ones more " << " \n";
				CloseHandle(v.hFileMap);
			}

			//v.hFileMap = OpenFileMappingA(
			//	FILE_MAP_ALL_ACCESS,
			//	0,
			//	(LPCSTR)secret.c_str());
			//if (GetLastError() != NO_ERROR)
			//	std::cout <<  "The file is not open try ones more " << " \n";;

			//getchar();
		 } while (GetLastError() == NO_ERROR);
		 //std::cout << "now it's conected " << " \n";
	}
	//else
	//{
	//	std::cout << "it nedds to be a produser or consumer! \n";
	//}
	//    This means that the file map already existed!, but we
	//    still get a handle to it, we share it!
	//    THIS COULD BE USEFUL INFORMATION FOR OUR PROTOCOL.
	// 5) Create one VIEW of the map with a void pointer
	//    This API Call will create a view to ALL the memory
	//    of the File map.
	v.mData = MapViewOfFile(v.hFileMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	//if (v.mData == NULL)
	//	std::cout << "error mData = NULL.... \n";
	if (type == TYPE::PRODUCER)
	{
		memcpy((char*)v.mData + v.mSize, &v.offset, sizeof(int));
		memcpy((char*)v.mData + v.mSize + sizeof(int), &v.offsetRead, sizeof(int));
	}
	Sleep(1000);
	//LAST BUT NOT LEAST
	//Always, close the view, and close the handle, before quiting
	//UnmapViewOfFile((LPCVOID)v.mData);
	//CloseHandle(v.hFileMap);
}

bool ComLib::send(const void * msg, const size_t length)
{
	int chunks = v.mSize / 64;
	int msgChuks = ((length + sizeof(size_t)) / 64) + 1;
	msgChuks *= 64;
	int oldOffset = v.offset;
	if (msgChuks < (v.mSize/2))
	{
		if (v.offset + msgChuks >= v.mSize - sizeof(size_t))
		{
			//säg till läsare att börja om 
			//och startar om
			int restart = 0;
			memcpy((char*)v.mData + v.offset, &restart, sizeof(size_t));
			v.offset = 0;
		}
		// lista ut var the reader är 
		// om det är för lite utryme kvar vänta 
		// när det finns tilräkligt med utryme skicka medelande
		// skicka om det lykades eller inte

		//DWORD dwLength = length;

		///gör bara om length inte är 0 och consumer inte är ivägen
		void * ptr = (char*)v.mData + v.mSize + sizeof(int);
		sInfo.tail = *(int*)ptr;

		if (sInfo.tail == 0 && oldOffset != v.offset) // dont do it (the reder havent moved!)
		{
			//dont send
			v.offset = oldOffset;
		}
		else
		{
			if ((sInfo.tail > oldOffset && sInfo.tail > (oldOffset + msgChuks + 128)) || (oldOffset >= sInfo.tail))
			{
				if (v.offset == oldOffset || (sInfo.tail > v.offset && sInfo.tail > (v.offset + msgChuks + 128)))
				{
					//funkar inte när v.offset behövr bli 0
					//if( oldOffset == v.offset && sInfo.tail > v.offset && sInfo.tail > (v.offset + length + 128)) || oldOffset == sInfo.tail)
					//((sInfo.tail > oldOffset && sInfo.tail > (oldOffset + length + 128))
					//	&& sInfo.tail > v.offset && sInfo.tail > (v.offset + length + 128)
					//	|| (oldOffset >= sInfo.tail))
					//send
					oldOffset = v.offset;

					//v.ID++;
					//memcpy((int*)v.mData + v.offset, &v.ID, sizeof(int));
					//v.offset += sizeof(int);
					int wath = v.mSize - (v.offset + msgChuks);
					memcpy((char*)v.mData + v.offset, &msgChuks, sizeof(size_t));
					//std::cout << "massage on spot" << (char)v.offset << " \n";

					// Du kan skriva struktar inom sizeof() för att få byte storleken
					// 4 bytes
					int bite1 = sizeof(char*);
					// 1 byte
					int bite2 = sizeof(char);
					// 4 byte
					int bite3 = sizeof(int);
					//v.offset = 1;
					memcpy((char*)v.mData + v.offset + sizeof(size_t), msg, length);
					v.offset += msgChuks;

					//__try
					//{
					//	*((LPDWORD)v.mData) = dwLength;
					//}
					//__except (GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR ?
					//	EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
					//{
					//	std::cout << "failed to write from file \n";
					//}
					//std::cout << "the massage sent is: " << dwLength << "\n";
					//std::cout << "head: " << v.offset << "\n";
					//std::cout << "tail: " << sInfo.tail << "\n";
					//std::cout << "-lengh- : " << msgChuks << "\n";
					memcpy((char*)v.mData + v.mSize, &v.offset, sizeof(int));
					return true;
				}
				else
				{
					v.offset = oldOffset;
				}
			}
			else
			{
				v.offset = oldOffset;
			}
		}
	}
	return false;
}

bool ComLib::recv(char *& msg, size_t & length)
{
	void * ptr = (char*)v.mData + v.mSize;
	sInfo.head = *(int*)ptr;
	size_t dLength;
	// ändra sen
	if (v.offsetRead != sInfo.head )
	{
		ptr = (char*)v.mData + v.offsetRead;
		//dLength = *((size_t*)v.mData + v.offsetRead);
		//char temp = *((char*)v.mData + v.offsetRead);
		dLength = *(size_t*)ptr;
		if (dLength == 0)
		{
			v.offsetRead = 0;
			ptr = (char*)v.mData;
			dLength = *(size_t*)ptr;
		}
		//if(v.offsetRead == 0)
		//dwLength = *((LPDWORD)v.mData + (char)v.offsetRead);
		//v.offsetRead +=  sizeof(size_t);
		//memcpy(msg, (char*)v.mData, t);
		msg = ((char*)v.mData + v.offsetRead + sizeof(size_t));
		//*msg = msg;
		//memcpy(msg, (char*)v.mData + v.offsetRead + sizeof(int), length);
		//msg = ((char*)v.mData + v.offsetRead + sizeof(int));
		v.mSize;
		v.offsetRead += dLength;
		//int size = msg;
		//v.offsetRead = msg;
		length = dLength;
		//std::cout << "the massage read is: " << &msg << "\n";
		//std::cout << "the massage read is: " << msg << "\n";
		//std::cout << "head: " << sInfo.head << "\n";
		//std::cout << "tail: " << v.offsetRead << "\n";
		//std::cout << "-lengh- : " << length << "\n";
		memcpy((char*)v.mData + v.mSize + sizeof(int), &v.offsetRead, sizeof(int));
		return true;
	}
	else
	{
		return false;
	}
}

size_t ComLib::nextSize()
{
	void * ptr = (char*)v.mData + v.mSize;
	sInfo.head = *(int*)ptr;
	size_t dLength;
	// ändra sen
	if (v.offsetRead != sInfo.head)
	{
		ptr = (char*)v.mData + v.offsetRead;
		//dLength = *((size_t*)v.mData + v.offsetRead);
		//char temp = *((char*)v.mData + v.offsetRead);
		dLength = *(size_t*)ptr;
		if (dLength == 0)
		{
			v.offsetRead = 0;
			ptr = (char*)v.mData;
			dLength = *(size_t*)ptr;
		}
		return dLength;
	}
	else
	{
		return 0;
	}
}

ComLib::~ComLib()
{
	UnmapViewOfFile((LPCVOID)v.mData);
	//if(myType == TYPE::PRODUCER)
	CloseHandle(v.hFileMap);
}




