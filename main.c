#include <popt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <stdbool.h>
#include <locale.h>

#define DEFAULT_WINDOW_SIZE 3
#define DEFAULT_SCAN_AREA_SIZE 100

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
    char* filename = "";
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

    //check if file given
    if(strcmp(filename, "") == 0)
    {
        poptPrintHelp(optCon, stderr, 0);
        retVal = EXIT_ERR_ARGS;
        goto end;
    }
    
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
    bool* markedChars = calloc(sizeof(bool), charCount);
    if(markedChars == NULL)
    {
        perror("malloc");
        retVal = EXIT_ERR_MEM;
        goto mainBuffer;
    }


    //FIXME Debug
    markedChars[4] = true;
	markedChars[5] = true;
	markedChars[11] = true;

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
