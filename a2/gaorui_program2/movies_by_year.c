#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>

#define BUFFMAX 21
#define LANGUAGE_MAX_COUNT 5
#define LANGUAGE_MAX_LENGTH 20

/* struct for movie information */
struct movie
{
	char *title;
	int year;
	char languages[LANGUAGE_MAX_COUNT][LANGUAGE_MAX_LENGTH+1];
	int languageCount;
	double rate;
};

/* Parse the current line which is space delimited and create a
*  movie struct with the data in this line
*/
struct movie *createMovie(char *currLine)
{
	struct movie *currMovie = malloc(sizeof(struct movie));

	// For use with strtok_r
	char *saveptr;
	char *saveLanguageptr;
	char *languageToken;
	char *language,*languagePtr;
	int i = 0;

	// The first token is the title
	char *token = strtok_r(currLine, ",", &saveptr);
	currMovie->title = calloc(strlen(token) + 1, sizeof(char));
	strcpy(currMovie->title, token);

	// The next token is the year
	token = strtok_r(NULL, ",", &saveptr);
	currMovie->year = atoi(token);

	// The next token is the languages
	token = strtok_r(NULL, ",", &saveptr);
	// delete "[" and "]"
	language = malloc(strlen(token) - 2 + 1);
	strncpy(language, token + 1, strlen(token) - 2);
	languagePtr = language;
	while ((languageToken = strtok_r(languagePtr, ";", &saveLanguageptr)) != NULL)
	{
		if(i >= LANGUAGE_MAX_COUNT)
		{
			break;
		}
		strcpy(currMovie->languages[i], languageToken);
		i++;
		languagePtr = NULL;
	}
	free(language);
	currMovie->languageCount = i;

	// The next token is the rate
	token = strtok_r(NULL, "\n", &saveptr);
	currMovie->rate = strtod(token,NULL);

	return currMovie;
}

/*
* Parsing data from each line of the specified file.
*/
void processFile(char *dirpath, FILE *movieFile)
{
	char resultFileName[72];
	// Open and append the result file
	int resultfd;
	int movieCount = 0;

	char *currLine = NULL;
	size_t len = 0;
	ssize_t nread;

	// Read the file line by line
	while ((nread = getline(&currLine, &len, movieFile)) != -1)
	{
		if(movieCount++ == 0)
		{
			continue;
		}
		// Get a new movie node corresponding to the current line
		struct movie *newNode = createMovie(currLine);
		memset(resultFileName, 0x00, sizeof(resultFileName));
		sprintf(resultFileName, "%s/%d.txt", dirpath, newNode->year);
		//create or append the random file, file owner rw-r-----
		resultfd = open(resultFileName, O_CREAT | O_RDWR | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP);
		//write the title to the year file
		write(resultfd, newNode->title, strlen(newNode->title));
		write(resultfd, "\n", 1);
		close(resultfd);
		free(newNode);
	}
	movieCount--;
	free(currLine);
}

/*
* process the largest file.
*/
void processLargestFile()
{
	DIR *dirp;
	struct dirent *direntp;
	struct stat stat_buf;
	char *fname = NULL;
	long file_size = 0;
	char dirname[64];
	int var_random = 0;
	// Open the specified file for reading only
	FILE *movieFile = NULL;

	/* open current directory */	
	if((dirp=opendir("./"))==NULL)
	{
		printf("Open current directory ./ Error: %s\n", strerror(errno));
		exit(1);
	}

	/* read the directory */
	while((direntp=readdir(dirp))!=NULL) 
	{
		/* get the file stat */ 
		memset(&stat_buf, 0x00, sizeof(struct stat));
		if(stat(direntp->d_name, &stat_buf)== -1)
		{
			continue;
		}
		// find the largest file
		if(S_ISREG(stat_buf.st_mode) && //file
			!strncmp(direntp->d_name, "movies_", 7) && //the prefix movies_
			!strncmp(direntp->d_name + strlen(direntp->d_name) - 4, ".csv", 4) && //the extension csv
			file_size < stat_buf.st_size)//the largest file
		{
			file_size = stat_buf.st_size;
			fname = direntp->d_name;
		}
	}
	if(fname)
	{
		printf("Now processing the chosen file named %s\n",fname);
		//set the seed
		srandom((unsigned)time(NULL));
		//create random
		var_random = random() % 100000;
		memset(dirname, 0x00, sizeof(dirname));
		sprintf(dirname, "gaorui.movies.%d",var_random);
		//create the random directory, directory owner rwxr-x---
		if(mkdir(dirname, S_IRWXU | S_IRGRP | S_IXGRP)  == -1)
		{
			printf("mkdir directory %s Error: %s\n", dirname, strerror(errno));
			return;
		}
		printf("Created directory with name %s\n\n", dirname);
		if((movieFile = fopen(fname, "r")) != NULL)
		{
			//process the picked file
			processFile(dirname, movieFile);
			fclose(movieFile);
		}
	}
}

/*
* process the smallest file.
*/
void processSmallestFile()
{
	DIR *dirp;
	struct dirent *direntp;
	struct stat stat_buf;
	char *fname = NULL;
	char dirname[64];
	long file_size = 0;
	int index = 0;
	int var_random = 0;	
	// Open the specified file for reading only
	FILE *movieFile = NULL;

	/* open current directory */	
	if((dirp=opendir("./"))==NULL)
	{
		printf("Open current directory %s Error: %s\n", strerror(errno));
		exit(1);
	}

	/* read the directory */
	while((direntp=readdir(dirp))!=NULL) 
	{
		/* get the file stat */ 
		memset(&stat_buf, 0x00, sizeof(struct stat));
		if(stat(direntp->d_name, &stat_buf)== -1)
		{
			continue;
		}
		// find the smallest file
		if(S_ISREG(stat_buf.st_mode) && //file
			!strncmp(direntp->d_name, "movies_", 7) && //the prefix movies_
			!strncmp(direntp->d_name + strlen(direntp->d_name) - 4, ".csv", 4))//the extension csv
		{
			if(index == 0 || file_size > stat_buf.st_size)//the smallest file
			{
				file_size = stat_buf.st_size;
				fname = direntp->d_name;
			}
			index++;
		}
	}
	if(fname)
	{
		printf("Now processing the chosen file named %s\n",fname);
		//set the seed
		srandom((unsigned)time(NULL));
		//create random
		var_random = random() % 100000;
		memset(dirname, 0x00, sizeof(dirname));
		sprintf(dirname, "gaorui.movies.%d",var_random);
		//create the random directory, directory owner rwxr-x---
		if(mkdir(dirname, S_IRWXU | S_IRGRP | S_IXGRP)  == -1)
		{
			printf("mkdir directory %s Error: %s\n", dirname, strerror(errno));
			return;
		}
		printf("Created directory with name %s\n\n", dirname);
		if((movieFile = fopen(fname, "r")) != NULL)
		{
			//process the picked file
			processFile(dirname, movieFile);
			fclose(movieFile);
		}
	}
}

/*
* process the required file.
*/
int processRequirementFile(char *filePath)
{
	char dirname[64];
	int var_random = 0;	
	// Open the specified file for reading only
	FILE *movieFile = NULL;

	if((movieFile = fopen(filePath, "r")) != NULL)
	{
		//set the seed
		srandom((unsigned)time(NULL));
		//create random
		var_random = random() % 100000;
		memset(dirname, 0x00, sizeof(dirname));
		sprintf(dirname, "gaorui.movies.%d",var_random);
		//create the random directory, directory owner rwxr-x---
		if(mkdir(dirname, S_IRWXU | S_IRGRP | S_IXGRP)  == -1)
		{
			printf("mkdir directory %s Error: %s\n", dirname, strerror(errno));
			return -2;
		}
		printf("Created directory with name %s\n\n", dirname);
		//process the picked file
		processFile(dirname, movieFile);
		fclose(movieFile);
	}else{
		return -1;
	}
	return 0;
}


void printPromote()
{
	printf("1. Select file to process\n\
2. Exit the program\n\n");
}

void printSubPromote()
{
	printf("\nWhich file you want to process?\n\
Enter 1 to pick the largest file\n\
Enter 2 to pick the smallest file\n\
Enter 3 to specify the name of a file\n\n");
}

int main(int argc, char *argv[])
{
	int index = 0, sub_index = 0;
	char bufIn[BUFFMAX];
	while(1)
	{
		printPromote();
		printf("Enter a choice 1 or 2: ");
		memset(bufIn, 0x00, sizeof(bufIn));
		fgets(bufIn, sizeof(bufIn), stdin);
		index = atoi(bufIn);
		switch(index)
		{
			case 1:
AGAIN:
				printSubPromote();
				printf("Enter a choice from 1 to 3: ");
				memset(bufIn, 0x00, sizeof(bufIn));
				//get input of the stdin
				fgets(bufIn, sizeof(bufIn), stdin);
				sub_index = atoi(bufIn);
				switch(sub_index)
				{
					case 1:
						//pick the largest file
						processLargestFile();
						break;
					case 2:
						//pick the smallest file
						processSmallestFile();
						break;
					case 3:
						//input the file name
						printf("Enter the complete file name: ");
						memset(bufIn, 0x00, sizeof(bufIn));
						fgets(bufIn, sizeof(bufIn), stdin);
						if(bufIn[strlen(bufIn)-1] == '\n')
						{
							bufIn[strlen(bufIn)-1] = '\0';
						}
						if(processRequirementFile(bufIn) == -1)
						{
							printf("The file %s was not found. Try again\n", bufIn);
							goto AGAIN;
						}
						break;
					default:
						printf("You entered an incorrect choice. Try again.\n\n");
						goto AGAIN;
				}
				break;
			case 2:
				goto END;
				break;
			default:
				printf("You entered an incorrect choice. Try again.\n\n");
				break;
		}
	}
END: 
	return EXIT_SUCCESS;
}

