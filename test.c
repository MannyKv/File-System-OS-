/*
 * test.c
 *
 *  Modified on: 24/03/2023
 *      Author: Robert Sheehan
 */

#include <stdio.h>
#include <string.h>

#include "fileSystem.h"
#include "CuTest.h"
#define BLOCK_SIZE 64
void TestFormat(CuTest *tc)
{
	char volName[64];

	char *justRight = "1--------10--------20--------30--------40--------50--------60--";
	CuAssertIntEquals(tc, 0, format(justRight));
	CuAssertIntEquals(tc, 0, volumeName(volName));
	CuAssertStrEquals(tc, justRight, volName);

	char *normal = "regular volume name";
	CuAssertIntEquals(tc, 0, format(normal));
	CuAssertIntEquals(tc, 0, volumeName(volName));
	CuAssertStrEquals(tc, normal, volName);
}

void TestRootDirectory(CuTest *tc)
{
	CuAssertIntEquals(tc, 0, format("test root"));
	char listResult[1024];
	char *expectedResult = "/:\n";
	list(listResult, "/");
	CuAssertStrEquals(tc, expectedResult, listResult);
}

void TestCreateFile(CuTest *tc)
{
	format("test create file");
	CuAssertIntEquals(tc, 0, create("/fileA"));
	char listResult[1024];
	char *expectedResult = "/:\nfileA:\t0\n";
	list(listResult, "/");
	CuAssertStrEquals(tc, expectedResult, listResult);
}

void TestCreateFiles(CuTest *tc)
{
	format("test create files");
	CuAssertIntEquals(tc, 0, create("/fileA"));
	CuAssertIntEquals(tc, 0, create("/fileB"));
	CuAssertIntEquals(tc, 0, create("/fileC"));
	char listResult[1024];
	char *expectedResult = "/:\nfileA:\t0\nfileB:\t0\nfileC:\t0\n";
	list(listResult, "/");
	CuAssertStrEquals(tc, expectedResult, listResult);
}

void TestCreateLotsOfFiles(CuTest *tc)
{
	format("test create files");
	CuAssertIntEquals(tc, 0, create("/fileA"));
	CuAssertIntEquals(tc, 0, create("/fileB"));
	CuAssertIntEquals(tc, 0, create("/fileC"));
	CuAssertIntEquals(tc, 0, create("/fileD"));
	CuAssertIntEquals(tc, 0, create("/fileE"));
	CuAssertIntEquals(tc, 0, create("/fileF"));
	CuAssertIntEquals(tc, 0, create("/fileG"));
	CuAssertIntEquals(tc, 0, create("/fileH"));
	char listResult[1024];
	char *expectedResult = "/:\nfileA:\t0\nfileB:\t0\nfileC:\t0\nfileD:\t0\nfileE:\t0\n"
						   "fileF:\t0\nfileG:\t0\nfileH:\t0\n";
	list(listResult, "/");
	CuAssertStrEquals(tc, expectedResult, listResult);
}

void TestCreateFileWithDir(CuTest *tc)
{
	format("test file with dir");
	CuAssertIntEquals(tc, 0, create("/dir1/fileA"));
	char listResult[1024];
	char *expectedResult = "/dir1:\nfileA:\t0\n";
	list(listResult, "/dir1");
	CuAssertStrEquals(tc, expectedResult, listResult);
}

void TestCreateFilesWithDir(CuTest *tc)
{
	format("test file with dir");
	CuAssertIntEquals(tc, 0, create("/dir1/fileA"));
	CuAssertIntEquals(tc, 0, create("/dir1/fileB"));
	char listResult[1024];
	char *expectedResult = "/dir1:\nfileA:\t0\nfileB:\t0\n";
	list(listResult, "/dir1");
	CuAssertStrEquals(tc, expectedResult, listResult);
}

void TestWriteFile(CuTest *tc)
{
	format("test file write");
	create("/fileA");
	char listResult[1024];
	CuAssertIntEquals(tc, 0, a2write("/fileA", "hi ", 4));
	char *expectedResult = "/:\nfileA:\t4\n";
	list(listResult, "/");
	CuAssertStrEquals(tc, expectedResult, listResult);
	char *data = "1--------10--------20--------30--------40--------50--------60--";
	CuAssertIntEquals(tc, 0, a2write("/fileA", data, 64));
	expectedResult = "/:\nfileA:\t68\n";
	list(listResult, "/");
	CuAssertStrEquals(tc, expectedResult, listResult);
}

void TestReadFile(CuTest *tc)
{
	format("test file read");
	create("/fileA");
	a2write("/fileA", "hi ", 4);
	char *data = "1--------10--------20--------30--------40--------50--------60--";
	a2write("/fileA", data, 64);
	char *expectedResult = "hi ";
	char readResult[128];
	CuAssertIntEquals(tc, 0, a2read("/fileA", readResult, 4));
	CuAssertStrEquals(tc, expectedResult, readResult);
	char readData[128];
	expectedResult = readData;
	strncpy(expectedResult, data, 128);
	CuAssertIntEquals(tc, 0, a2read("/fileA", readResult, 64));
	CuAssertStrEquals(tc, expectedResult, readResult);
}

void TestReadLotsOfFiles(CuTest *tc)
{
	format("test lots of reads");
	create("/fileA");
	create("/fileB");
	a2write("/fileA", "I am in fileA", 14);
	a2write("/fileB", "Whereas I am in fileB", 22);
	char readResult[128];
	char *expectedResult = "I am";
	CuAssertIntEquals(tc, 0, a2read("/fileA", readResult, 4));
	readResult[4] = '\0';
	CuAssertStrEquals(tc, expectedResult, readResult);

	expectedResult = "Whereas";
	CuAssertIntEquals(tc, 0, a2read("/fileB", readResult, 7));
	readResult[7] = '\0';
	CuAssertStrEquals(tc, expectedResult, readResult);
	char listResult[1024];

	expectedResult = " in fileA";
	CuAssertIntEquals(tc, 0, a2read("/fileA", readResult, 9));
	readResult[9] = '\0';
	CuAssertStrEquals(tc, expectedResult, readResult);

	expectedResult = " I am";
	CuAssertIntEquals(tc, 0, a2read("/fileB", readResult, 5));
	readResult[5] = '\0';
	CuAssertStrEquals(tc, expectedResult, readResult);
}

void TestWriteAndReadWithDirectories(CuTest *tc)
{
	format("test write and read with directories");
	create("/dir1/dir2/fileA");
	a2write("/dir1/dir2/fileA", "I am in /dir1/dir2/fileA", 25);
	char listResult[1024];
	char *expectedResult = "/dir1/dir2:\nfileA:\t25\n";
	list(listResult, "/dir1/dir2");
	CuAssertStrEquals(tc, expectedResult, listResult);
	char readResult[128];
	expectedResult = "I am in /dir1/dir2/fileA";
	CuAssertIntEquals(tc, 0, a2read("/dir1/dir2/fileA", readResult, 25));
	CuAssertStrEquals(tc, expectedResult, readResult);
}

void TestSeek(CuTest *tc)
{
	format("test seek");
	create("/fileA");
	a2write("/fileA", "aaaaabbbbbwhereddddd", 21);
	CuAssertIntEquals(tc, 0, seek("/fileA", 10));
	char *expectedResult = "where";
	char readResult[128];
	CuAssertIntEquals(tc, 0, a2read("/fileA", readResult, 5));
	readResult[5] = '\0';
	CuAssertStrEquals(tc, expectedResult, readResult);
}
void TestBadVolumeName(CuTest *tc)
{
	char *justMore =
		"1--------10--------20--------30--------40--------50--------60---";
	CuAssertIntEquals(tc, -1, format(justMore));
	CuAssertIntEquals(tc, EBADVOLNAME, file_errno);

	char *justUnder = "";
	CuAssertIntEquals(tc, -1, format(justUnder));
	CuAssertIntEquals(tc, EBADVOLNAME, file_errno);
}

void TestCreateLudicrousNumberOfFiles(CuTest *tc)
{
	int iterCount = (BLOCK_SIZE - 2) * (numBlocks() - 1) / 12 - 1;
	char expectedResult[iterCount * 12];
	char listResult[iterCount * 12];
	strcpy(expectedResult, "/:\n");
	char *largeDir = "Large Directory";
	CuAssertIntEquals(tc, 0, format(largeDir));
	for (int i = 0; i < iterCount; i++)
	{
		char file[9] = {0};
		char output[12];
		sprintf(file, "/fi%d", i);
		sprintf(output, "%s:\t0\n", file + 1);
		CuAssertIntEquals(tc, 0, create(file));
		strcat(expectedResult, output);
	}
	list(listResult, "/");
	CuAssertStrEquals(tc, expectedResult, listResult);
}

void TestWriteExactlyBlock(CuTest *tc)
{
	char *exactBlock = "Exact Block";
	CuAssertIntEquals(tc, 0, format(exactBlock));
	CuAssertIntEquals(tc, 0, create("/file"));
	int writeAmount = BLOCK_SIZE - 2;
	char writeData[writeAmount];
	char expectedResult[writeAmount * 2 + 1];
	char outputResult[writeAmount * 2 + 1];
	memset(writeData, 'b', writeAmount);
	memset(expectedResult, 'b', 2 * writeAmount);
	expectedResult[2 * writeAmount] = 0;
	memset(outputResult, '\0', 2 * writeAmount + 1);
	CuAssertIntEquals(tc, 0, a2write("/file", writeData, writeAmount));
	CuAssertIntEquals(tc, 0, a2write("/file", writeData, writeAmount));
	CuAssertIntEquals(tc, 0, a2read("/file", outputResult, writeAmount * 2));
	CuAssertIntEquals(tc, strlen(expectedResult), strlen(outputResult));
	CuAssertStrEquals(tc, expectedResult, outputResult);
}

void TestWriteMaxFile(CuTest *tc)
{
	char *largeFile = "Large File";
	CuAssertIntEquals(tc, 0, format(largeFile));
	CuAssertIntEquals(tc, 0, create("/large"));
	int writeAmount = (BLOCK_SIZE - 2) * (numBlocks() - 2);

	char expectedResult[writeAmount + 1];
	char outputResult[writeAmount + 1];
	memset(expectedResult, 'a', writeAmount);
	memset(expectedResult + writeAmount, '\0', 1);
	memset(outputResult, '\0', writeAmount + 1);
	CuAssertIntEquals(tc, 0, a2write("/large", expectedResult, writeAmount));
	CuAssertIntEquals(tc, 0, a2read("/large", outputResult, writeAmount));
	CuAssertIntEquals(tc, strlen(expectedResult), strlen(outputResult));
	CuAssertStrEquals(tc, expectedResult, outputResult);
}

void TestWriteTooLargeFile(CuTest *tc)
{
	char *tooLargeFile = "Too Large File";
	CuAssertIntEquals(tc, 0, format(tooLargeFile));
	CuAssertIntEquals(tc, 0, create("/large"));
	int writeAmount = (BLOCK_SIZE - 2) * (numBlocks() - 2) + 1;
	char expectedResult[writeAmount + 1];
	char outputResult[writeAmount + 1];
	memset(expectedResult, 'a', writeAmount);
	memset(expectedResult + writeAmount, '\0', 1);
	memset(outputResult, '\0', writeAmount + 1);
	CuAssertIntEquals(tc, -1, a2write("/large", expectedResult, writeAmount));
	CuAssertIntEquals(tc, ENOROOM, file_errno);
	CuAssertIntEquals(tc, 0, a2read("/large", outputResult, writeAmount));
	CuAssertIntEquals(tc, 0, strlen(outputResult));
}

void TestCreateLotsSameOfFiles(CuTest *tc)
{
	format("test create same files");
	CuAssertIntEquals(tc, 0, create("/fileA"));
	CuAssertIntEquals(tc, -1, create("/fileA"));
	CuAssertIntEquals(tc, -1, create("/fileA"));
	CuAssertIntEquals(tc, -1, create("/fileA"));
	CuAssertIntEquals(tc, -1, create("/fileA"));
	CuAssertIntEquals(tc, -1, create("/fileA"));
	CuAssertIntEquals(tc, -1, create("/fileA"));
	CuAssertIntEquals(tc, -1, create("/fileA"));
	char listResult[1024];
	char *expectedResult = "/:\nfileA:\t0\n";
	list(listResult, "/");
	CuAssertStrEquals(tc, expectedResult, listResult);
}

void TestCreateFilesInDifferentDirs(CuTest *tc)
{
	format("test create same files");
	CuAssertIntEquals(tc, 0, create("/fileA"));
	CuAssertIntEquals(tc, 0, create("/fileB"));
	CuAssertIntEquals(tc, 0, create("/fileC"));
	CuAssertIntEquals(tc, 0, create("/dir/file1"));
	CuAssertIntEquals(tc, 0, create("/dir/file2"));
	CuAssertIntEquals(tc, 0, create("/dir/file3"));
	CuAssertIntEquals(tc, 0, create("/dir/file4"));
	CuAssertIntEquals(tc, 0, create("/fileD"));
	CuAssertIntEquals(tc, 0, create("/fileE"));
	CuAssertIntEquals(tc, 0, create("/fileF"));
	CuAssertIntEquals(tc, 0, create("/fileG"));
	CuAssertIntEquals(tc, 0, create("/fileH"));
	CuAssertIntEquals(tc, 0, create("/dir/file5"));
	CuAssertIntEquals(tc, 0, create("/dir/file6"));
	CuAssertIntEquals(tc, 0, create("/fileI"));
	CuAssertIntEquals(tc, 0, a2write("/fileA", "aaaa", 4));
	CuAssertIntEquals(tc, 0, a2write("/fileB", "aaaa", 4));
	CuAssertIntEquals(tc, 0, a2write("/fileC", "aaaa", 4));
	CuAssertIntEquals(tc, 0, a2write("/fileD", "aaaa", 4));
	CuAssertIntEquals(tc, 0, a2write("/fileE", "aaaa", 4));
	CuAssertIntEquals(tc, 0, a2write("/fileF", "aaaa", 4));
	CuAssertIntEquals(tc, 0, a2write("/fileG", "aaaa", 4));
	CuAssertIntEquals(tc, 0, a2write("/fileH", "aaaa", 4));
	CuAssertIntEquals(tc, 0, a2write("/fileI", "aaaa", 4));
	// These cannot be here unless block size is larger
	// CuAssertIntEquals(tc, 0, a2write("/dir/file1", "aaaa", 4));
	// CuAssertIntEquals(tc, 0, a2write("/dir/file2", "aaaa", 4));
	// CuAssertIntEquals(tc, 0, a2write("/dir/file3", "aaaa", 4));
	// CuAssertIntEquals(tc, 0, a2write("/dir/file4", "aaaa", 4));
	// CuAssertIntEquals(tc, 0, a2write("/dir/file5", "aaaa", 4));
	// CuAssertIntEquals(tc, 0, a2write("/dir/file6", "aaaa", 4));
	char listResult[1024];
	char *expectedResult =
		"/:\nfileA:\t4\nfileB:\t4\nfileC:\t4\ndir:\t72\nfileD:\t4\n"
		"fileE:\t4\nfileF:\t4\nfileG:\t4\nfileH:\t4\nfileI:\t4\n";
	list(listResult, "/");
	CuAssertStrEquals(tc, expectedResult, listResult);
	// expectedResult =
	// "/dir:\nfile1:\t4\nfile2:\t4\nfile3:\t4\nfile4:\t4\nfile5:"
	//                  "\t4\nfile6:\t4\n";
	// list(listResult, "/dir");
	// CuAssertStrEquals(tc, expectedResult, listResult);
}