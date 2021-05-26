#include "usage.h"


void usage()
{
    printf("BitCrackEvo OPTIONS [TARGETS]\n");
    printf("Where TARGETS is one or more addresses\n\n");
	
    printf("-h, --help              Display this message\n");
    printf("-c, --compressed        Use compressed points\n");
    printf("-u, --uncompressed      Use Uncompressed points\n");
    printf("--compression  MODE     Specify compression where MODE is\n");
    printf("                          COMPRESSED or UNCOMPRESSED or BOTH\n");
    printf("-d, --device ID         Use device ID\n");
    printf("-b, --blocks N          N blocks\n");
    printf("-t, --threads N         N threads per block\n");
    printf("-p, --points N          N points per thread\n");
    printf("-i, --in FILE           Read addresses from FILE, one per line\n");
    printf("-o, --out FILE          Write keys to FILE\n");
    printf("-f, --follow            Follow text output\n");
    printf("--list-devices          List available devices\n");
    printf("--keyspace KEYSPACE     Specify the keyspace:\n");
    printf("                          START:END\n");
    printf("                          START:+COUNT\n");
    printf("                          START\n");
    printf("                          :END\n"); 
    printf("                          :+COUNT\n");
    printf("                        Where START, END, COUNT are in hex format\n");
    printf("--stride N              Increment by N keys at a time\n");
    printf("--share M/N             Divide the keyspace into N equal shares, process the Mth share\n");
    printf("--continue FILE         Save/load progress from FILE\n");
}
