#include <popt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>
#include <stdbool.h>
#include <locale.h>

#define DEFAULT_WINDOW_SIZE 3
#define DEFAULT_SCAN_AREA_SIZE 300

#define EXIT_ERR_ARGS -1
#define EXIT_ERR_READ -2
#define EXIT_ERR_MEM -3

//ANSI escape codes
#define AEC_BOLDRED L"\033[01;31m"
#define AEC_DEFAULT L"\033[m"

int main(int argc, char** argv)
{
    //execution environment
    int retVal = 0;
    char* filename = NULL;
    unsigned int windowSize = DEFAULT_WINDOW_SIZE;
    unsigned int scanAreaSize = DEFAULT_SCAN_AREA_SIZE;
    int printVersion = 0;

    //read POPT arguments
    struct poptOption optionsArray[] =
    {
        {"file", 		'f', 	POPT_ARG_STRING, 	&filename, 		0, 	"Name of a file to read", 	"FILENAME"},
        {"window", 		'w', 	POPT_ARG_INT, 		&windowSize, 	0, 	"Scanning window size", 	"WINDOWSIZE"},
        {"scan", 		's', 	POPT_ARG_INT, 		&scanAreaSize, 	0, 	"Scanning area",			"SCANAREA"},
        {"version",		'\0',	POPT_ARG_NONE,		&printVersion,	0,	"Print version and exit",	NULL},
        POPT_AUTOHELP
        POPT_TABLEEND
    };

    //initialise popt context
    poptContext optCon = poptGetContext(NULL, argc, (const char**) argv, optionsArray, 0);

    //read options
    if(poptGetNextOpt(optCon) != -1)
    {
        poptPrintHelp(optCon, stderr, 0);
        retVal = EXIT_ERR_ARGS;
        goto end;
    }
    
    //read filename
    char* leftFile = (char*) poptGetArg(optCon);
	if(leftFile != NULL)
	{
		filename = leftFile;
	}
    
    //check if file given
    if(filename == NULL)
    {
        poptPrintHelp(optCon, stderr, 0);
        retVal = EXIT_ERR_ARGS;
        goto end;
    }
    
    //free context
	poptFreeContext(optCon);
    
    //set locale
    setlocale(LC_ALL, "");

    //open file
    FILE* file = fopen(filename, "rb");
    if(file == NULL)
    {
        perror(filename);
        retVal = EXIT_ERR_READ;
        goto end;
    }

    //find size
    fseek(file, 0, SEEK_END);
    size_t filesize = ftell(file);
	//we are using reopen, since fseek behaves weirdly
	freopen(filename, "rb", file);
    

    //allocate buffer of the wide char, of the size of the file
    //since the number of Unicode charaters will never be greater than number of bytes in file
    wchar_t* buffer = calloc(sizeof(wchar_t), filesize);
    if(buffer == NULL)
    {
        perror("malloc");
        retVal = EXIT_ERR_MEM;
        goto file;
    }

    //read contents
    size_t charCount = 0;
    while(charCount < filesize)
    {
        wint_t readChar = fgetwc(file);
        if(readChar == WEOF)
        {
            if(ferror(file))
            {
                perror(filename);
                retVal = EXIT_ERR_READ;
                goto mainBuffer;
            }
            break;
        }
        buffer[charCount] = (wchar_t)readChar;
        charCount++;
    }

    //allocate buffer of marked characters
    int* markedChars = calloc(sizeof(bool), charCount);
    if(markedChars == NULL)
    {
        perror("malloc");
        retVal = EXIT_ERR_MEM;
        goto mainBuffer;
    }
    
    //main loop
    for(size_t beginning = 0; beginning + windowSize < charCount; beginning++)
	{
		//load window
		bool windowComplete = true;
		for(size_t windowBeginning = 0; windowBeginning < windowSize; windowBeginning++)
		{
			if(!iswalnum(buffer[beginning + windowBeginning]))
			{
				windowComplete = false;
				break;
			}
		}
		if(!windowComplete)
		{
			continue;
		}
		//move window through scan area
		for(size_t scanBeginning = 1; scanBeginning + windowSize < scanAreaSize && beginning + scanBeginning + windowSize < charCount; scanBeginning++)
		{
			//check chars in window
			bool isMatch = true;
			for(size_t windowBeginning = 0; windowBeginning < windowSize; windowBeginning++)
			{
				wchar_t currChar = buffer[beginning + scanBeginning + windowBeginning];
				if(!iswalnum(currChar))
				{
					isMatch = false;
					break;
				}
				if(towlower(currChar) != towlower(buffer[beginning + windowBeginning]))
				{
					isMatch = false;
					break;
				}
			}
			if(isMatch)
			{
				//mark all chars in window and found area
				for(size_t windowBeginning = 0; windowBeginning < windowSize; windowBeginning++)
				{
					markedChars[beginning + windowBeginning] = true;
					markedChars[beginning + scanBeginning + windowBeginning] = true;
				}
			}
		}
	}

    //print file
    for(size_t i = 0; i < charCount; i++)
    {
		if(!markedChars[i])
		{
			wprintf(L"%lc", buffer[i]);
		}
        else
		{
			wprintf(AEC_BOLDRED L"%lc" AEC_DEFAULT, buffer[i]);
		}
    }


    free(markedChars);
mainBuffer:
    free(buffer);
file:
    fclose(file);
end:
    return retVal;
}
