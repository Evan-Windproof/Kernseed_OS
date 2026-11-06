/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#include <kernseed/shell.h>
/* In-place bounded tokenization: whitespace, single/double quotes, backslash.
 * Return -1 for too many arguments, -2 for unterminated quote/escape. */
int shell_parse(char *line,char **argv,size_t capacity){
    char *read=line,*write=line;size_t argc=0;
    while(*read){
        while(*read==' '||*read=='\t')read++;
        if(!*read)break;
        if(argc==capacity)return -1;
        argv[argc++]=write;char quote=0;
        while(*read){
            char c=*read++;
            if(!quote&&(c==' '||c=='\t'))break;
            if(c=='\\'){if(!*read)return -2;*write++=*read++;continue;}
            if(c=='\''||c=='"'){
                if(!quote){quote=c;continue;}
                if(quote==c){quote=0;continue;}
            }
            *write++=c;
        }
        if(quote)return -2;
        *write++=0;
    }
    return (int)argc;
}
