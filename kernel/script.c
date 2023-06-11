#include <script.h>
#include <terminal.h>
#include <kutils.h>

int script_parse(char* str)
{
    char* start = str;
    int line = 0, ret;

    if(*str == 0){
        return -1;
    }

    /* This assumes that the given string is \0 terminated. */
    do {
        if(*str == '\n'){
            *str = 0;
            
            ret = exec_cmd(start);
            if(ret < 0){
                twritef("script: error on '%s' line %d\n", start, line);
                return -1;
            }
        
            line++;
            start = str+1;
        }
        str++;
    } while (*str != 0);
    
    /* Try to execute the last line incase it ended with a \0 */
    ret = exec_cmd(start);
    if(ret < 0){
        twritef("script: error on '%s' line %d\n", start, line);
        return -1;
    }
        

    return 0;
}