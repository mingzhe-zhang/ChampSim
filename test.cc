#include<stdio.h>
#include<unistd.h>
#include<getopt.h>
int main(int argc, char *argv[])
{
    int opt;
    int digit_optind = 0;
    int option_index = 0;
    char *string = "a::b:c:d";
    static struct option long_options[] =
    {  
        {"reqarg", required_argument,NULL, 'r'},
        {"optarg", optional_argument,NULL, 'n'},
        {"noarg",  no_argument,         NULL,'o'},
        {NULL,     0,                      NULL, 0},
    }; 
    while((opt =getopt_long_only(argc,argv,string,long_options,&option_index))!= -1)
    {
        switch(opt)
        {
            case 'r':
                printf("r1\n");
                break;
            case 'n':
                printf("r2\n");
                break;
            case 'o':
                printf("r3\n");
                break;
            default:
                break;
        }
        printf("opt = %c\t\t", opt);
        printf("optarg = %s\t\t",optarg);
        printf("optind = %d\t\t",optind);
        printf("argv[optind] =%s\t\t", argv[optind]);
        printf("option_index = %d\n",option_index);
    }  
}
