#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LANGUAGE_MAX_COUNT 5
#define LANGUAGE_MAX_LENGTH 20

int movieCount = 0;

/* struct for movie information */
struct movie
{
	char *title;
	int year;
	char languages[LANGUAGE_MAX_COUNT][LANGUAGE_MAX_LENGTH+1];
	int languageCount;
	float rate;
	struct movie *next;
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

	// Set the next node to NULL in the newly created movie entry
	currMovie->next = NULL;

	return currMovie;
}

/*
* free the movie list.
*/
void freeMovieList(struct movie *list)
{
	struct movie *node;
	while (list != NULL)
	{
		node = list->next;
		if(list->title != NULL)
		{
			free(list->title);
		}
		free(list);
		list = node;
	}
}

/*
* Return a linked list of movies by parsing data from
* each line of the specified file.
*/
struct movie *processFile(char *filePath)
{
	// Open the specified file for reading only
	FILE *movieFile = fopen(filePath, "r");

	char *currLine = NULL;
	size_t len = 0;
	ssize_t nread;
	char *token;

	// The head of the linked list
	struct movie *head = NULL;
	// The tail of the linked list
	struct movie *tail = NULL;

	// Read the file line by line
	while ((nread = getline(&currLine, &len, movieFile)) != -1)
	{
		if(movieCount++ == 0)
		{
			continue;
		}
		// Get a new movie node corresponding to the current line
		struct movie *newNode = createMovie(currLine);

		// Is this the first node in the linked list?
		if (head == NULL)
		{
			// This is the first node in the linked link
			// Set the head and the tail to this node
			head = newNode;
			tail = newNode;
		}
		else
		{
			// This is not the first node.
			// Add this node to the list and advance the tail
			tail->next = newNode;
			tail = newNode;
		}
	}
	movieCount--;
	free(currLine);
	fclose(movieFile);
	return head;
}

/*
* Show movies released in the specified year
*/
void printMovieByYear(struct movie *list, int year)
{
	int hasMatch = 0;
	while (list != NULL)
	{
		if(list->year == year)
		{
			printf("%s\n", list->title);
			hasMatch++;
		}
		list = list->next;
	}
	if(hasMatch == 0)
	{
		printf("No data about movies released in the year %d\n", year);
	}
	printf("\n");
}

/*
* Show highest rated movie for each year
*/
void printHighestRatedMovie(struct movie *list)
{
	int i;
	struct movie **tmpList = malloc(sizeof(struct movie *) * movieCount);
	for(i = 0; i < movieCount; i++)
	{
		tmpList[i] = NULL;
	}
	while (list != NULL)
	{
		for(i = 0; i < movieCount; i++)
		{
			if(tmpList[i] == NULL)
			{
				tmpList[i] = list;
				break;
			}else{
				if(tmpList[i]->year == list->year)
				{
					if(tmpList[i]->rate < list->rate)
					{
						tmpList[i] = list;
					}
					break;
				}else{
					continue;
				}
			}
		}
		list = list->next;
	}
	for(i = 0; i < movieCount && tmpList[i] != NULL; i++)
	{
		printf("%d %.1f %s\n", tmpList[i]->year, tmpList[i]->rate, tmpList[i]->title);
	}
	printf("\n");
	free(tmpList);
}

/*
* Show the title and year of release of all movies in a specific language
*/
void printMovieByLanguage(struct movie *list, char *language)
{
	int i;
	int hasMatch = 0;
	while (list != NULL)
	{
		for(i = 0; i < list->languageCount; i++)
		{
			if(!strcmp(language, list->languages[i]))
			{
				printf("%d %s\n", list->year, list->title);
				hasMatch++;
				break;
			}
		}
		list = list->next;
	}
	if(hasMatch == 0)
	{
		printf("No data about movies released in %s\n", language);
	}
	printf("\n");
}


void printPromote()
{
	printf("1. Show movies released in the specified year\n\
2. Show highest rated movie for each year\n\
3. Show the title and year of release of all movies in a specific language\n\
4. Exit from the program\n\n");
}

int main(int argc, char *argv[])
{
	int index = 0;
	int year = 0;
	char language[LANGUAGE_MAX_LENGTH+1];
	
	if (argc < 2)
	{
		printf("You must provide the name of the file to process\n");
		printf("Example usage: ./movies movies_sample_1.csv\n");
		return EXIT_FAILURE;
	}
	struct movie *list = processFile(argv[1]);
	printf("Processed file %s and parsed data for %d movies\n\n", argv[1], movieCount);
	
	while(1)
	{
		printPromote();
		printf("Enter a choice from 1 to 4: ");
		scanf("%d",&index);
		switch(index)
		{
			case 1:
				printf("Enter the year for which you want to see movies: ");
				scanf("%d",&year);
				printMovieByYear(list, year);
				break;
			case 2:
				printHighestRatedMovie(list);
				break;
			case 3:
				printf("Enter the language for which you want to see movies: ");
				memset(language,0x00,sizeof(language));
				scanf("%s",&language);
				printMovieByLanguage(list, language);
				break;
			case 4:
				goto END;
				break;
			default:
				printf("You entered an incorrect choice. Try again.\n\n");
				break;
		}
	}
END:
	freeMovieList(list);
	return EXIT_SUCCESS;
}

