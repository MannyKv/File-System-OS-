/*
 * afterTest.c
 *
 *  Modified on: 24/03/2023
 *      Author: Robert Sheehan
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fileSystem.h"

int main(void)
{

	printf("\n !!@#ASKJ READ THIS PLEASE FOR THE LOVE OF FUCKING GOD\n");
	char data[128];
	char data1[128];

	a2read("/fileA", data, 6);

	a2read("/dir1/fileA", data1, 7);

	char listResult[1024];
	char listResult1[1024];

	list(listResult, "/");

	list(listResult1, "/dir1");
	printf("test");
	printf("\nData from /fileA: %s\n", data);
	printf("\nData from /dir1/fileA: %s\n", data1);
	printf("\n%s", listResult);
	printf("\n%s", listResult1);
	return EXIT_SUCCESS;
}
