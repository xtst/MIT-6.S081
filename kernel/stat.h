typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long uint64;

typedef uint64 pde_t;

#define T_DIR 1	   // Directory
#define T_FILE 2   // File
#define T_DEVICE 3 // Device

struct stat {
	int dev;	 // File system's disk device
	uint ino;	 // Inode number
	short type;	 // Type of file
	short nlink; // Number of links to file
	uint64 size; // Size of file in bytes
};
