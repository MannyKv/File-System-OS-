/*
 * fileSystem.c
 *
 *  Modified on: 02/05/2023
 *      Author: Mlal371
 *
 * Complete this file.
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <libgen.h>
#include "device.h"
#include <string.h>
#include "fileSystem.h"

char **splitString(char *string, char *delim);
int incrementNextPos();
bool search_directory(unsigned char *directory_data, int num_entries, char *target_name, int *next_block);
int intialiseBlock(int blocknum);
int editBlock(unsigned char *data, int blockNum, int start, int size);
int intialiseDataBlock(int blocknum);
int readCurrDirectory(int target_dir, char *write);
int navigateToDirectory(char **toFind);
int findFile(int dirToSearch, int *posFound, char *target_name);

struct superBlock
{
	int next_free;
	int spaces_available;
};

struct directory
{
	unsigned char type[2];
	unsigned char direcName[8];
	int size;
	int subDirectory;
} __attribute__((packed));

struct data
{
	int nextBlock;
	int filePointer;
	unsigned char dataWritten[56];
} __attribute__((packed));

/* The file system error number. */
int file_errno = 0;
/*Next available spot*/
int nextspot = 2;
/*
 * Formats the device for use by this file system.
 * The volume name must be < 64 bytes long.
 * All information previously on the device is lost.
 * Also creates the root directory "/".
 * Returns 0 if no problem or -1 if the call failed.
 */
int format(char *volumeName)
{
	printf("\nformatting is happening");
	// clear each block first
	unsigned char toClear[64];
	memset(toClear, 0, sizeof(toClear));
	for (int x = 0; x < numBlocks(); x++)
	{
		blockWrite(x, toClear);
	}
	if ((strlen(volumeName) > 63) || strlen(volumeName) <= 0)
	{
		file_errno = EBADVOLNAME;
		return -1;
	}

	unsigned char vname[BLOCK_SIZE];
	size_t numChars = strlen(volumeName);
	memcpy(vname, volumeName, numChars);
	memset(vname + numChars, 0, BLOCK_SIZE - numChars);

	int result = blockWrite(0, vname);

	unsigned char sysInfo[BLOCK_SIZE];
	memset(sysInfo, 0, BLOCK_SIZE);
	int nextBlock = 2;
	memcpy(sysInfo, &nextBlock, sizeof(int));

	// int space_avail = numBlocks();
	// memcpy(sysInfo + 4, space_avail, sizeof(int));

	int result2 = blockWrite(1, sysInfo);

	unsigned char rootInfo[BLOCK_SIZE];
	memset(rootInfo, 0, BLOCK_SIZE);
	int intial_space = 3;
	memcpy(rootInfo, &intial_space, sizeof(int));
	int nextBlockRoot = 0; // default for directories not carriny on
	memset(rootInfo + 4, nextBlockRoot, sizeof(int));
	incrementNextPos();
	blockWrite(2, rootInfo);

	// if(result == -1 || result2 == -1){
	//	return -1;
	// }

	return result;
}

/*
 * Returns the volume's name in the result.
 * Returns 0 if no problem or -1 if the call failed.
 */
int volumeName(char *result)
{
	unsigned char resultFromBlock[BLOCK_SIZE];
	int resultOf = blockRead(0, resultFromBlock);
	size_t len = strlen((char *)resultFromBlock);
	char *resultName = malloc(len + 1);
	memcpy(resultName, resultFromBlock, len);
	resultName[len] = '\0';
	strcpy(result, resultName);
	free(resultName);
	return 0;
}

/*
 * Makes a file with a fully qualified pathname starting with "/".
 * It automatically creates all intervening directories.
 * Pathnames can consist of any printable ASCII characters (0x20 - 0x7e)
 * including the space character.
 * The occurrence of "/" is interpreted as starting a new directory
 * (or the file name).
 * Each section of the pathname must be between 1 and 7 bytes long (not
 * counting the "/"s).
 * The pathname cannot finish with a "/" so the only way to create a directory
 * is to create a file in that directory. This is not true for the root
 * directory "/" as this needs to be created when format is called.
 * The total length of a pathname is limited only by the size of the device.
 * Returns 0 if no problem or -1 if the call failed.
 */
int create(char *pathName)
{
	// make a copy of the pathName string
	char *ptr = strdup(pathName);

	// split the directory name into parts
	char *delim = "/";
	char **test = splitString(dirname(ptr), delim);

	// iterate through the array of directories
	int currentD = 2;
	int entries;
	int next_block = 2;
	for (int y = 0; test[y] != NULL; y++)
	{
		bool found = false;

		// read whole directory first
		unsigned char currentDirec[64];
		blockRead(currentD, currentDirec);

		while ((next_block > 0) && (!found))
		{ // fix this for no instance found
			// read how many entries the block has
			blockRead(currentD, currentDirec);
			memcpy(&entries, currentDirec, sizeof(int));

			// if the directory is empty break and start creating
			if (entries == 3)
			{
				printf("%s\n", "break!");
				break;
			}

			// Otherwise start searching the directory which returns whether the directory is found
			memcpy(&next_block, currentDirec + 4, sizeof(next_block));
			// Passes in the current directory and num entries along with file to find
			// and the next directory block (if created)
			found = search_directory(currentDirec, entries, test[y], &next_block);
			if (!found && (next_block > 0))
			{
				currentD = next_block;
			}
		}

		if (found)
		{
			// if the directory was found then navigate to that directory and repeat
			currentD = next_block;
		}
		// if not found then create a new directory
		if (!found)
		{

			// if there are no available positions inside the block, create a new block and write to there
			if (entries == 0)
			{

				// find the next free block
				int nextFree = incrementNextPos();
				if (nextFree == -1)
				{
					file_errno = ENOROOM;
					return -1;
				}

				memcpy(currentDirec + 4, &nextFree, sizeof(int));

				blockWrite(currentD, currentDirec);
				// assign the current block next block to next free and load the next block
				currentD = nextFree;
				blockRead(currentD, currentDirec);
				// write the number 3 of slots available and 0 to the next block
				int avail = 3;
				memcpy(currentDirec, &avail, sizeof(int));

				blockWrite(currentD, currentDirec);
			}
			// Setup the new block in the struct
			// append to the end of the last block (can be found using 8 26 44)

			struct directory newDir;

			memcpy(&newDir.direcName, test[y], 8);

			newDir.size = 8;						  // 8 byte initially for sys info
			newDir.subDirectory = incrementNextPos(); // load random block as its directory
			if (newDir.subDirectory == -1)
			{
				file_errno = ENOROOM;
				return -1;
			}
			memcpy(&newDir.type, "t", 2);

			// intilise that new block
			intialiseBlock(newDir.subDirectory);

			// create new directory
			unsigned char newDirectoryBlock[BLOCK_SIZE];

			blockRead(currentD, newDirectoryBlock);

			memcpy(&entries, newDirectoryBlock, sizeof(int));

			memcpy(newDirectoryBlock + 8 + (18 * (3 - entries)), &newDir, sizeof(struct directory));

			entries = entries - 1;

			memcpy(newDirectoryBlock, &entries, sizeof(int));

			int success = blockWrite(currentD, newDirectoryBlock);
			currentD = newDir.subDirectory;
		}
	}

	//  create the file for directory
	char *ptr2 = strdup(pathName);
	char *filename = basename(ptr2);

	// read current direc
	unsigned char currentBlock[64];
	blockRead(currentD, currentBlock);
	int blockSpace = 0;
	while (blockSpace == 0)
	{
		memcpy(&blockSpace, currentBlock, sizeof(int));
		int nextblock = 0;
		memcpy(&nextblock, currentBlock + 4, sizeof(int));
		if (blockSpace == 0 && (nextblock == 0))
		{

			// find the next free block
			int nextFree = incrementNextPos();
			if (nextFree == -1)
			{
				file_errno = ENOROOM;
				return -1;
			}

			intialiseBlock(nextFree);
			memcpy(currentBlock + 4, &nextFree, sizeof(int));
			blockWrite(currentD, currentBlock);
			// assign the current block next block to next free and load the next block
			currentD = nextFree;
		}
		else if (nextblock > 0)
		{
			currentD = nextblock;
			blockRead(currentD, currentBlock);
			memcpy(&blockSpace, currentBlock, sizeof(int));
		}
	}

	struct directory file;
	memcpy(&file.type, "f", 2);

	memcpy(&file.direcName, filename, 8);

	file.size = 0;
	// at this point only contains size block pointer
	file.subDirectory = incrementNextPos(); // we will give it a block right away and intialise that block
	if (file.subDirectory == -1)
	{
		file_errno = ENOROOM;
		return -1;
	}
	intialiseDataBlock(file.subDirectory);

	memcpy(currentBlock + 8 + (18 * (3 - blockSpace)), &file, sizeof(struct directory));

	// subtract one from entry list
	blockSpace = blockSpace - 1;
	memcpy(currentBlock, &blockSpace, sizeof(int));
	blockWrite(currentD, currentBlock);

	return 0;
}

/*
 * Returns a list of all files in the named directory.
 * The "result" string is filled in with the output.
 * The result looks like this

/dir1:
file1	42
file2	0

 * The fully qualified pathname of the directory followed by a colon and
 * then each file name followed by a tab "\t" and then its file size.
 * Each file on a separate line.
 * The directoryName is a full pathname.
 */
void list(char *result, char *directoryName)
{
	char *copyOfTarget = strdup(directoryName);
	char resultString[500] = "";
	strcat(resultString, copyOfTarget);
	strcat(resultString, ":\n");
	// make a copy of the pathName string
	char *ptr = strdup(directoryName);

	// split the directory name into parts
	char *delim = "/";
	char **toFind = splitString(ptr, delim);

	if (strcmp(directoryName, "/") == 0)
	{
		readCurrDirectory(2, resultString);
	}
	else
	{
		// Find the directory
		// take down a starting dir
		int currentDirectory = 2;
		int next_block = 2;
		int entries;
		for (int x = 0; toFind[x] != NULL; x++)
		{
			bool found = false;

			// Create a while(!found)
			while ((next_block > 0) && (!found))
			{
				// load current dir
				unsigned char loadedDirectory[64];

				blockRead(currentDirectory, loadedDirectory);
				memcpy(&entries, loadedDirectory, sizeof(int));
				memcpy(&next_block, loadedDirectory + 4, sizeof(int));
				// loop through it to find the target folder
				found = search_directory(loadedDirectory, entries, toFind[x], &next_block);
				if (!found && (next_block > 0))
				{
					currentDirectory = next_block;
				}
				if (found)
				{
					printf("FOUND");
				}
			}
			if (found)
			{
				// if the directory was found then navigate to that directory and repeat
				currentDirectory = next_block;
			}
		}
		// After finding the target folder create a loop to read through all funcs
		readCurrDirectory(currentDirectory, resultString);
	}

	strcpy(result, resultString);
}

/*
 * Writes data onto the end of the file.
 * Copies "length" bytes from data and appends them to the file.
 * The filename is a full pathname.
 * The file must have been created before this call is made.
 * Returns 0 if no problem or -1 if the call failed.
 */
int a2write(char *fileName, void *data, int length)
{

	// Find the target directory first using dirname()

	char *dir = strdup(fileName);
	// split the directory name into parts

	char *delim = "/";
	char **toFind = splitString(dirname(dir), delim);
	int targetFolder = 2;
	targetFolder = navigateToDirectory(toFind);

	// create a loop to find the file in curr directory or in overflow blocks;
	int nextBlock = 1;
	int fileFound = 0;
	int posFound = 0;
	char *targetFile = strdup(basename(fileName));

	while ((nextBlock > 0) && (fileFound == 0))
	{
		unsigned char currentBlock[64];
		blockRead(targetFolder, currentBlock);
		memcpy(&nextBlock, currentBlock + 4, sizeof(int));
		// search directory
		fileFound = findFile(targetFolder, &posFound, targetFile);
		// returns either 0 for not found or > 0 for the datablock
		if (fileFound == 0)
		{
			// set target folder to next folder
			targetFolder = nextBlock;
		}
	}

	if (fileFound == 0)
	{
		file_errno = ENOSUCHFILE;
		printf("your file does not exist");
		return -1;
	}

	// add the length to the file size first and store it back.
	unsigned char blockWithFile[64];
	blockRead(targetFolder, blockWithFile);
	struct directory file;
	memcpy(&file, blockWithFile + 8 + (18 * (posFound)), sizeof(struct directory));
	int startWrite = 0;
	startWrite = file.size;
	file.size = file.size + length;
	memcpy(blockWithFile + 8 + (18 * (posFound)), &file, sizeof(struct directory));
	blockWrite(targetFolder, blockWithFile);

	// loop through to find the last data block as the existance of nextblock implies current block is full
	int hasNextData = 1;
	int currentDataBlock = fileFound;
	unsigned char dataBlock[64];
	struct data d;
	while (hasNextData > 0)
	{
		blockRead(currentDataBlock, dataBlock);
		memcpy(&d, dataBlock, sizeof(struct data));
		hasNextData = d.nextBlock;
	}
	// create data copy
	char datacopy[length]; // malloc(length);
	memcpy(datacopy, data, length);
	int lengthcpy = length;
	// start reading the data to find last written
	while (lengthcpy > 0)
	{
		int currentlyWritten = startWrite % 56;
		int spaceLeft = 56 - currentlyWritten; // space on current block
		if (spaceLeft > lengthcpy)
		{
			unsigned char toWrite[56] = "";
			memcpy(toWrite, datacopy, lengthcpy);
			memcpy(d.dataWritten + currentlyWritten, toWrite, lengthcpy);

			memcpy(dataBlock, &d, sizeof(struct data));
			blockWrite(currentDataBlock, dataBlock);
			lengthcpy = 0;
		}
		else
		{
			// write whatever you can
			unsigned char toWrite[56] = "";
			memcpy(toWrite, datacopy, spaceLeft);
			strcpy(datacopy, datacopy + (spaceLeft));
			memcpy(d.dataWritten + currentlyWritten, toWrite, lengthcpy);
			lengthcpy = lengthcpy - spaceLeft;
			// make a new block to write to
			int nextB = incrementNextPos();
			if (nextB == -1)
			{
				file_errno = ENOROOM;
				return -1;
			}
			d.nextBlock = nextB;
			intialiseDataBlock(nextB);
			memcpy(dataBlock, &d, sizeof(struct data));
			blockWrite(currentDataBlock, dataBlock);
			currentDataBlock = nextB;
			blockRead(currentDataBlock, dataBlock);
			startWrite = 0;
			memcpy(&d, dataBlock, sizeof(struct data));
		}
	}

	return 0;
}

/*
 * Reads data from the start of the file.
 * Maintains a file position so that subsequent reads continue
 * from where the last read finished.
 * The filename is a full pathname.
 * The file must have been created before this call is made.
 * Returns 0 if no problem or -1 if the call failed.
 */
int a2read(char *fileName, void *data, int length)
{

	fflush(stdout);
	// Find the target directory first using dirname()
	char *dir = strdup(fileName);
	// split the directory name into parts
	char *delim = "/";
	char **toFind = splitString(dirname(dir), delim);
	int targetFolder = 2;
	targetFolder = navigateToDirectory(toFind);
	// create a loop to find the file in curr directory or in overflow blocks;
	int nextBlock = 1;
	int fileFound = 0;
	int posFound = 0;
	char *targetFile = strdup(basename(fileName));

	fflush(stdout);
	while ((nextBlock > 0) && (fileFound == 0))
	{
		unsigned char currentBlock[64];
		blockRead(targetFolder, currentBlock);
		memcpy(&nextBlock, currentBlock + 4, sizeof(int));
		// search directory
		fileFound = findFile(targetFolder, &posFound, targetFile);
		// returns either 0 for not found or > 0 for the datablock
		if (fileFound == 0)
		{
			// set target folder to next folder
			targetFolder = nextBlock;
		}
	}

	if (fileFound == 0)
	{
		file_errno = ENOSUCHFILE;
		printf("your file does not exist");
		return -1;
	}
	int original_block_no = fileFound;
	targetFolder = fileFound;
	unsigned char dataBlock[64];
	blockRead(targetFolder, dataBlock);
	struct data d;
	memcpy(&d, dataBlock, sizeof(struct data));

	// first check where the pointer is pointing to by dividng by 56
	int blockToFind = d.filePointer / 56;
	for (int x = 0; x < blockToFind; x++)
	{
		targetFolder = d.nextBlock;
		blockRead(targetFolder, dataBlock);
		memcpy(&d, dataBlock, sizeof(struct data));
	}
	// then read from pointer%56 for index.
	int start = d.filePointer % 56;

	int lengthcpy = length;

	// start reading the data to find last written

	// char dataToReturn[length + 1];
	memset(data, 0, sizeof(data));

	fflush(stdout);
	while (lengthcpy > 0)
	{

		int EofIn = 56 - start;
		if (EofIn >= lengthcpy)
		{

			int length_of_data = strlen(data);
			memcpy(data + length_of_data, d.dataWritten + start, lengthcpy);
			lengthcpy = 0;
		}
		else
		{

			// read the difference
			int length_of_data = strlen(data);

			memcpy(data + length_of_data, d.dataWritten + start, EofIn);
			memcpy(data + length_of_data + EofIn, "\0", sizeof(char));
			lengthcpy = lengthcpy - EofIn;

			// now load the new directory
			targetFolder = d.nextBlock;

			if (targetFolder == 0)
			{
				file_errno = EOTHER;
				printf("EOF-ERROR");
				return -1;
			}
			blockRead(targetFolder, dataBlock);
			memcpy(&d, dataBlock, sizeof(struct data));
			start = 0;
		}
	}

	unsigned char changePointer[64];
	blockRead(original_block_no, changePointer);
	struct data x;
	memcpy(&x, changePointer, sizeof(struct data));
	x.filePointer = x.filePointer + length;
	memcpy(changePointer, &x, sizeof(struct data));
	blockWrite(original_block_no, changePointer);
	// memcpy(data, dataToReturn, sizeof(dataToReturn));
	return 0;
}

/*
 * Repositions the file pointer for the file at the specified location.
 * All positive integers are byte offsets from the start of the file.
 * 0 is the beginning of the file.
 * If the location is after the end of the file, move the location
 * pointer to the end of the file.
 * The filename is a full pathname.
 * The file must have been created before this call is made.
 * Returns 0 if no problem or -1 if the call failed.
 */
int seek(char *fileName, int location)
{
	// Find the target directory first using dirname()
	char *dir = strdup(fileName);
	// split the directory name into parts
	char *delim = "/";
	char **toFind = splitString(dirname(dir), delim);
	int targetFolder = 2;
	targetFolder = navigateToDirectory(toFind);
	// create a loop to find the file in curr directory or in overflow blocks;
	int nextBlock = 1;
	int fileFound = 0;
	int posFound = 0;
	char *targetFile = strdup(basename(fileName));

	while ((nextBlock > 0) && (fileFound == 0))
	{
		unsigned char currentBlock[64];
		blockRead(targetFolder, currentBlock);
		memcpy(&nextBlock, currentBlock, sizeof(int));
		// search directory
		fileFound = findFile(targetFolder, &posFound, targetFile);
		// returns either 0 for not found or > 0 for the datablock
		if (fileFound == 0)
		{
			// set target folder to next folder
			targetFolder = nextBlock;
		}
	}

	// copy the current file size
	unsigned char blockWithFile[64];
	blockRead(targetFolder, blockWithFile);
	struct directory file;
	memcpy(&file, blockWithFile + 8 + (18 * (posFound)), sizeof(struct directory));

	// open up the file data
	int original_block_no = fileFound;
	targetFolder = fileFound;
	unsigned char folder[64];
	unsigned char dataBlock[64];
	blockRead(targetFolder, dataBlock);
	struct data d;
	memcpy(&d, dataBlock, sizeof(struct data));
	if (location > file.size)
	{
		d.filePointer = file.size;
	}
	else
	{
		d.filePointer = location;
	}
	memcpy(dataBlock, &d, sizeof(struct data));
	blockWrite(targetFolder, dataBlock);
	return 0;
}

/* Splits a string into sub strings by a delimiter */
char **splitString(char *string, char *delim)
{
	int i = 0;
	char *p = strtok(string, delim);
	char **array = malloc(sizeof(char *) * (strlen(string) / strlen(delim) + 1));
	while (p != NULL)
	{
		array[i++] = strdup(p);
		p = strtok(NULL, delim);
	}
	array[i] = NULL; // Add null terminator to the end of the array
	return array;
}

/*Function increments the free position in superblock by one and returns the current free position for use by the caller*/
int incrementNextPos()
{
	unsigned char block[64];
	blockRead(1, block);

	int freepos;
	memcpy(&freepos, block, sizeof(int));
	int previous_free = freepos;
	int num_block_available; // do later

	freepos = freepos + 1;

	if (freepos > numBlocks())
	{
		file_errno = ENOROOM;
		return -1;
	}
	memcpy(block, &freepos, sizeof(int));

	blockWrite(1, block);
	return previous_free;
}
bool search_directory(unsigned char *directory_data, int num_entries, char *target_name, int *next_block)
{
	bool found = false;
	int increment = 8;
	for (int z = 0; z < (3 - num_entries); z++)
	{

		struct directory d;

		memcpy(&d, directory_data + increment, 18);

		if (strcmp((char *)d.type, "t") == 0)
		{

			if (strcmp((char *)d.direcName, target_name) == 0)
			{

				memcpy(next_block, &d.subDirectory, sizeof(int));

				found = true;
				break;
			}
		}
		increment += 18;
	}
	if (!found)
	{

		// memcpy(next_block, directory_data + 4, sizeof(int));
	}
	return found;
}

int editBlock(unsigned char *data, int blockNum, int start, int size)
{
	// create new block
	unsigned char copyBlock[64];
	blockRead(blockNum, copyBlock);
	memcpy(copyBlock + start, data, size);
	blockWrite(blockNum, copyBlock);
	return 0;
}

int intialiseBlock(int blocknum)
{
	int free = 3;
	int nextblock = 0;
	unsigned char copyBlock[64];
	blockRead(blocknum, copyBlock);
	memcpy(copyBlock, &free, sizeof(int));
	memcpy(copyBlock + 4, &nextblock, sizeof(int));
	blockWrite(blocknum, copyBlock);
	return 0;
}

int intialiseDataBlock(int blocknum)
{
	// assign first 4 to next block, rest is for data
	int nextblock = 0;
	int pointer = 0;
	unsigned char copyBlock[64];
	blockRead(blocknum, copyBlock);
	memcpy(copyBlock, &nextblock, sizeof(int));
	memcpy(copyBlock + 4, &pointer, sizeof(int));
	blockWrite(blocknum, copyBlock);
	return 0;
}

int readCurrDirectory(int target_dir, char *write)
{
	int currentDirectory = target_dir;
	int entries;
	int next = target_dir;
	unsigned char loadedDirectory[BLOCK_SIZE];
	blockRead(currentDirectory, loadedDirectory);

	while (next > 0)
	{
		blockRead(currentDirectory, loadedDirectory);
		memcpy(&entries, loadedDirectory, sizeof(int));
		int increment = 8;
		for (int x = 0; x < (3 - entries); x++)
		{
			struct directory d;
			memcpy(&d, loadedDirectory + increment, 18);
			char temp[sizeof(d.size)];
			strcat(write, d.direcName);
			strcat(write, ":");
			strcat(write, "\t");
			sprintf(temp, "%d", d.size);
			strcat(write, temp);
			strcat(write, "\n");
			increment += 18;
		}
		memcpy(&next, loadedDirectory + 4, sizeof(int));
		currentDirectory = next;
	}
	strcat(write, "\0");
	return 0;
}

int navigateToDirectory(char **toFind)
{
	int currentDirectory = 2;
	int next_block = 2;
	int entries;
	for (int x = 0; toFind[x] != NULL; x++)
	{
		bool found = false;

		// Create a while(!found)
		while ((next_block > 0) && (!found))
		{
			// load current dir
			unsigned char loadedDirectory[64];

			blockRead(currentDirectory, loadedDirectory);
			memcpy(&entries, loadedDirectory, sizeof(int));
			memcpy(&next_block, loadedDirectory + 4, sizeof(int));
			// loop through it to find the target folder
			found = search_directory(loadedDirectory, entries, toFind[x], &next_block);
			if (!found && (next_block > 0))
			{
				currentDirectory = next_block;
			}
			if (found)
			{
				printf("FOUND");
			}
		}
		if (found)
		{
			// if the directory was found then navigate to that directory and repeat
			currentDirectory = next_block;
		}
	}
	return currentDirectory;
}

int findFile(int dirToSearch, int *posFound, char *target_name)
{
	unsigned char directory_data[64];
	blockRead(dirToSearch, directory_data);
	int num_space;
	memcpy(&num_space, directory_data, sizeof(int));
	bool found = false;
	int increment = 8;
	int subDirec = 0;
	for (int z = 0; z < (3 - num_space); z++)
	{
		struct directory d;
		memcpy(&d, directory_data + increment, sizeof(struct directory));

		if (strcmp((char *)d.type, "f") == 0)
		{

			if (strcmp((char *)d.direcName, target_name) == 0)
			{

				memcpy(&subDirec, &d.subDirectory, sizeof(int));
				*posFound = z;

				break;
			}
		}
		increment += 18;
	}

	return subDirec;
}