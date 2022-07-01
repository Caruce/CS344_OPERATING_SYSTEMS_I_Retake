#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

//An input line will never be longer than 1000 characters (including the line separator)
#define INPUT_MAX_LINE_BUFF 1000
//80 non-line separator characters
#define OUTPUT_MAX_LINE_BUFF 80
//The input for the program will never have more than 49 lines before the stop-processing line
#define MAX_LINE_SIZE 49
//struct for pipe line buffer
typedef struct {
	char buff[INPUT_MAX_LINE_BUFF * MAX_LINE_SIZE + 1];
	int stop;//it's over. default 0
} PipeBuff;

//reads in lines of characters from the standard input
PipeBuff input_buff;
//replaces every line separator in the input by a space
PipeBuff no_separator_buff;
//replaces every pair of plus signs, i.e., "++", by a "^"
PipeBuff no_plus_buff;

// Initialize the mutex for buffer 1
pthread_mutex_t mutex_1 = PTHREAD_MUTEX_INITIALIZER;
// Initialize the mutex for buffer 2
pthread_mutex_t mutex_2 = PTHREAD_MUTEX_INITIALIZER;
// Initialize the mutex for buffer 3
pthread_mutex_t mutex_3 = PTHREAD_MUTEX_INITIALIZER;
// Initialize the condition variable for buffer 1
pthread_cond_t cond_1 = PTHREAD_COND_INITIALIZER;
// Initialize the condition variable for buffer 2
pthread_cond_t cond_2 = PTHREAD_COND_INITIALIZER;
// Initialize the condition variable for buffer 3
pthread_cond_t cond_3 = PTHREAD_COND_INITIALIZER;

/*
*find the oldstr of the str, and replace by newstr
*/
char *strrpc(char *str,char *oldstr,char *newstr)
{
	char bstr[strlen(str) + 1];
	memset(bstr,0x00,sizeof(bstr));
 
	for(int i = 0;i < strlen(str);i++)
	{
		if(!strncmp(str+i,oldstr,strlen(oldstr)))
		{
			strcat(bstr,newstr);
			i += strlen(oldstr) - 1;
		}else{
			strncat(bstr,str + i,1);
		}
	}
	strcpy(str,bstr);
	return str;
}

/*
*Thread 1, called the Input Thread, reads in lines of characters from the standard input
*/
void * get_input(void *args)
{
	//reads in lines of characters from the standard input
    char input_line[INPUT_MAX_LINE_BUFF + 1];
	int i;
	//over flag
	int stop = 0;
	//init the input_buff
	memset(&input_buff, 0x00, sizeof(PipeBuff));
	for(i = 0; i < MAX_LINE_SIZE; i++)
	{
		memset(input_line, 0x00, sizeof(input_line));
		fgets(input_line, INPUT_MAX_LINE_BUFF, stdin);
		// Lock the mutex1 before putting the item in the buffer
		pthread_mutex_lock(&mutex_1);
		if(!strcmp(input_line, "STOP\n"))//STOP, it's over
		{
			input_buff.stop = 1;
			stop = 1;
		}else{
			//load the input_buff
			strcat(input_buff.buff, input_line);
			//The input for the program will never have more than 49 lines before the stop-processing line
			if(i == MAX_LINE_SIZE - 1)
			{
				input_buff.stop = 1;
			}
		}
		// Signal to the consumer that the buffer is no longer empty
		pthread_cond_signal(&cond_1);
		// Unlock the mutex1
		pthread_mutex_unlock(&mutex_1);
		if(stop == 1)
		{
			break;
		}
	}
    return NULL;
}

/*
*Thread 2, called the Line Separator Thread, replaces every line separator in the input by a space.
*/
void * replace_line_separator(void *args)
{
	int stop = 0;
	//init the no_separator_buff
	memset(&no_separator_buff, 0x00, sizeof(PipeBuff));
	for(;;)
	{
		// Lock the mutex1 before putting the item in the buffer
	    pthread_mutex_lock(&mutex_1);
		while(input_buff.buff[0] == '\0' && input_buff.stop == 0)
		{
			// Buffer is empty and not over. Wait for the producer to signal that the buffer has data
			pthread_cond_wait(&cond_1, &mutex_1);
		}
		stop = input_buff.stop;
		// Lock the mutex2 before putting the item in the buffer
		pthread_mutex_lock(&mutex_2);
		no_separator_buff.stop = input_buff.stop;
		//load the no_separator_buff
		strcat(no_separator_buff.buff, input_buff.buff);
		//replaces every line separator in the input by a space.
		strrpc(no_separator_buff.buff, "\n", " ");
		//Signal to the consumer that the buffer is no longer empty
		pthread_cond_signal(&cond_2);
		// Unlock the mutex2
		pthread_mutex_unlock(&mutex_2);
		//init input_buff
		memset(input_buff.buff, 0x00, sizeof(PipeBuff));
		// Unlock the mutex1
		pthread_mutex_unlock(&mutex_1);
		if(stop == 1)
		{
			break;
		}
	}
    return NULL;
}

/*
*Thread, 3 called the Plus Sign thread, replaces every pair of plus signs, i.e., "++", by a "^".
*/
void * replace_plus_sign(void *args)
{
	int stop = 0;
	//init the no_plus_buff
	memset(&no_plus_buff, 0x00, sizeof(PipeBuff));
	for(;;)
	{
		// Lock the mutex2 before putting the item in the buffer
	    pthread_mutex_lock(&mutex_2);
		while(no_separator_buff.buff[0] == '\0' && no_separator_buff.stop == 0)
		{
			// Buffer is empty and not over. Wait for the producer to signal that the buffer has data
			pthread_cond_wait(&cond_2, &mutex_2);
		}
		stop = no_separator_buff.stop;
		// Lock the mutex3 before putting the item in the buffer
		pthread_mutex_lock(&mutex_3);
		no_plus_buff.stop = no_separator_buff.stop;
		//load the no_plus_buff
		strcat(no_plus_buff.buff, no_separator_buff.buff);
		//replaces every pair of plus signs, i.e., "++", by a "^"
		strrpc(no_plus_buff.buff, "++", "^");
		//Signal to the consumer that the buffer is no longer empty
		pthread_cond_signal(&cond_3);
		// Unlock the mutex3
		pthread_mutex_unlock(&mutex_3);
		//init no_separator_buff
		memset(no_separator_buff.buff, 0x00, sizeof(PipeBuff));
		// Unlock the mutex2
		pthread_mutex_unlock(&mutex_2);
		if(stop == 1)
		{
			break;
		}
	}
    return NULL;
}

/*
*Thread 4, called the Output Thread, write this processed data to standard output as lines of exactly 80 characters.
*/
void * write_output(void *args)
{
	int i;
	char output[OUTPUT_MAX_LINE_BUFF + 1];
	int count = 0;
	int stop = 0;
	for(;;)
	{
		// Lock the mutex3 before putting the item in the buffer
	    pthread_mutex_lock(&mutex_3);
		while((no_plus_buff.buff[0] == '\0' || strlen(no_plus_buff.buff) < OUTPUT_MAX_LINE_BUFF)
			&& no_plus_buff.stop == 0)
		{
			//output only lines with 80 characters (with a line separator after each line)
			// Buffer is empty and not over. Wait for the producer to signal that the buffer has data
			pthread_cond_wait(&cond_3, &mutex_3);
		}
		stop = no_plus_buff.stop;
		count = strlen(no_plus_buff.buff) / OUTPUT_MAX_LINE_BUFF;
		for(i = 0; i < count; i++)
		{
			memset(output, 0x00, sizeof(output));
			strncpy(output, no_plus_buff.buff, OUTPUT_MAX_LINE_BUFF);
			printf("%s\n", output);
			sprintf(no_plus_buff.buff, "%s", no_plus_buff.buff + OUTPUT_MAX_LINE_BUFF);
		}
		// Unlock the mutex3
		pthread_mutex_unlock(&mutex_3);
		if(stop == 1)
		{
			break;
		}
	}
    return NULL;
}

int main()
{
    pthread_t input_t, line_separator_t, plus_sign_t, output_t;
    // Create the threads
    pthread_create(&input_t, NULL, get_input, NULL);
    pthread_create(&line_separator_t, NULL, replace_line_separator, NULL);
    pthread_create(&plus_sign_t, NULL, replace_plus_sign, NULL);
    pthread_create(&output_t, NULL, write_output, NULL);
    // Wait for the threads to terminate
    pthread_join(input_t, NULL);
    pthread_join(line_separator_t, NULL);
    pthread_join(plus_sign_t, NULL);
    pthread_join(output_t, NULL);
    return EXIT_SUCCESS;
}
