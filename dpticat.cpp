#define _CRT_SECURE_NO_WARNINGS


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <ctype.h>
#include <time.h>
#include <signal.h>
//#include <random.h>

#include "dpcdecl.h"
#include "dmgr.h"
#include "dpti.h"

#define N_TESTS 65536
#define W_BYTES 8
#define WIDTH (W_BYTES * 8)
#define FINE 10
#define COARSE (WIDTH - FINE)

typedef unsigned char byte;

char dev_name[cchDvcNameMax + 1];
HIF hif = hifInvalid;

void cancel_out(int signum){
  fprintf(stderr, "Ctrl-C pressed, exiting...\n");
  DmgrClose(hif);
  exit(1);
}

int main( int argc, char* argv[] ) {
  srand((unsigned int)time(NULL));
  if(argc < 2) {
    fprintf(stderr, "ERROR: no device specified\n");
    fflush(stderr);
    exit(1);
  }
  
  strcpy(dev_name, argv[1]);
  
  /* Attempt to open the device.
   */
  if ( ! DmgrOpen(&hif, dev_name) ) {
    fprintf(stderr, "ERROR: unable to open device \"%s\"\n", dev_name);
    fflush(stderr);
    exit(2);
  }
  printf("Opened HIF\n");
  signal(SIGINT, &cancel_out);
  int n_ports;
  
  DptiGetPortCount(hif, &n_ports);
  
  printf("Number of ports on %s: %i\n", dev_name, n_ports);
  
  if(argc < 3){
    fprintf(stderr, "ERROR: no port specified");
    DmgrClose(hif);
    fflush(stderr);
    exit(3);
  }
  DPRP port_props;
  int port_num = atoi(argv[2]);
  
  DptiGetPortProperties(hif, port_num, &port_props);
  
  if(port_props & dprpPtiAsynchronous){
    printf("Port %i is asynchronous\n", port_num);
  }
  else{
    printf("Port %i is synchronous\n", port_num);
  }
  
  if (!DptiEnableEx(hif, port_num)){
    fprintf(stderr, "ERROR: failed to enable port %i\n", port_num);
    DmgrClose(hif);
    fflush(stderr);
    exit(4);
  }
  
  
  int n_bytes = 128;
  byte *out_bytes = new byte[1];
  byte *in_bytes = new byte[n_bytes];
  
  for(int test_count = 0; test_count < N_TESTS; test_count++){
    printf("Test %i\n", test_count);
    //int n_bytes = rand() & 0xFF;
    out_bytes[0] = n_bytes;
    //out_bytes[1] = 255 - n_bytes;
    for(int i = 0; i < n_bytes; i++){
      in_bytes[i] = 0xBA;
    }
    
    printf("Requesting %i bytes\n", out_bytes[0]);
    
    DptiIO(hif, out_bytes, 1, NULL, 0, false);
    

    printf("Request Sent\nReceiving %i bytes\n", out_bytes[0]);
    
    DptiIO(hif, NULL, 0, in_bytes, out_bytes[0], false);

    //printf("Data Received\nRequesting Remaining %i bytes\n", out_bytes[1]);
    
    //DptiIO(hif, out_bytes+1, 1, NULL, 0, false);
    
    //printf("Request Sent\nReceiving %i bytes\n", out_bytes[1]);
    
    //DptiIO(hif, NULL, 0, in_bytes+out_bytes[0], out_bytes[1], false);
        
    printf("Data received, printing...\n");
    unsigned long long prev_coarse = 0;
    unsigned int prev_fine = 0;
    double prev_time = 0;
    
    for(int i = 0; i < out_bytes[0]; i+=W_BYTES){
      for(int j = 0; j < W_BYTES; j++){
	printf("%02x", in_bytes[i+j]);
      }
      printf("\n");
      unsigned long long curr_coarse = 0;
      for(int j = 0; j+8 <= COARSE; j += 8){
	curr_coarse = curr_coarse << 8 | in_bytes[i+(j >> 3)];
      }
      int leftover = COARSE & 0x7;
      if(leftover){
	printf("Coarse before: %llu, extra: %x\n", curr_coarse << leftover, (in_bytes[i + (COARSE >> 3)] >> (8-leftover)));
	curr_coarse = curr_coarse << leftover  | (in_bytes[i + (COARSE >> 3)] >> (8-leftover));
      }
      unsigned int curr_fine = 0;
      for(int j = 0; j+8 <= FINE; j += 8){
	curr_fine = curr_fine | (((unsigned int)in_bytes[i+W_BYTES-1-(j>>3)]) << j);
      }
      leftover = FINE & 0x7;
      if(leftover){
	curr_fine = curr_fine | (((unsigned int)(in_bytes[i+W_BYTES-1-(FINE>>3)] & ((1 << leftover) - 1))) << (FINE & (~0x7)));
      }
      double curr_time = ((curr_coarse+1) / 120000000.0) - (curr_fine * 0.000000000015);
      printf("Coarse: %llu, fine: %u, time: %lf\n", curr_coarse, curr_fine, curr_time);
      printf("CDiff: %llu, FDiff: %i, TDiff: %le\n", curr_coarse - prev_coarse, (int)curr_fine - (int)prev_fine,curr_time - prev_time);
      prev_coarse = curr_coarse;
      prev_fine = curr_fine;
      prev_time = curr_time;
    }
  }
  
  DmgrClose(hif);
  return 0;
}
