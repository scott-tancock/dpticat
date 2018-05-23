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
  fprintf(stderr, "Opened HIF\n");
  signal(SIGINT, &cancel_out);
  int n_ports;
  
  DptiGetPortCount(hif, &n_ports);
  
  fprintf(stderr, "Number of ports on %s: %i\n", dev_name, n_ports);
  
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
    fprintf(stderr, "Port %i is asynchronous\n", port_num);
  }
  else{
    fprintf(stderr, "Port %i is synchronous\n", port_num);
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
  
  for(int test_count = 0; true; test_count++){
    fprintf(stderr, "Test %i\n", test_count);
    //int n_bytes = rand() & 0xFF;
    out_bytes[0] = n_bytes;
    //out_bytes[1] = 255 - n_bytes;
    for(int i = 0; i < n_bytes; i++){
      in_bytes[i] = 0xBA;
    }
    
    fprintf(stderr, "Requesting %i bytes\n", out_bytes[0]);
    
    DptiIO(hif, out_bytes, 1, NULL, 0, false);
    

    fprintf(stderr, "Request Sent\nReceiving %i bytes\n", out_bytes[0]);
    
    DptiIO(hif, NULL, 0, in_bytes, out_bytes[0], false);

    //printf("Data Received\nRequesting Remaining %i bytes\n", out_bytes[1]);
    
    //DptiIO(hif, out_bytes+1, 1, NULL, 0, false);
    
    //printf("Request Sent\nReceiving %i bytes\n", out_bytes[1]);
    
    //DptiIO(hif, NULL, 0, in_bytes+out_bytes[0], out_bytes[1], false);
        
    fprintf(stderr, "Data received, printing...\n");
    int bytes_written = 0;
    while(bytes_written < out_bytes[0]){
      int bytes_add = write(1, in_bytes, out_bytes[0]);
      if(bytes_add <= 0){
	break;
      }
      bytes_written += bytes_add;
    }
  }
  DmgrClose(hif);
  return 0;
}
