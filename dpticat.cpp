#define _CRT_SECURE_NO_WARNINGS


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <ctype.h>
#include <time.h>
//#include <random.h>

#include "dpcdecl.h"
#include "dmgr.h"
#include "dpti.h"

#define N_TESTS 256

typedef unsigned char byte;

char dev_name[cchDvcNameMax + 1];

int main( int argc, char* argv[] ) {
  HIF hif = hifInvalid;
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
  
  int n_bytes = 32;
  byte *out_bytes = new byte[n_bytes*4];
  byte *in_bytes = new byte[n_bytes*4];
  
  for(int test_count = 0; test_count < N_TESTS; test_count++){
    printf("Test %i\n", test_count);
    for(int i = 0; i < n_bytes*4; i++){
      out_bytes[i] = rand();
      in_bytes[i] = 0xBA;
    }
    
    printf("Sending 3/4 Data\n");
    
    for(int c = 0; c < 3; c++){
      printf("c = %i\n", c);
      DptiIO(hif, out_bytes+(c*n_bytes), n_bytes, NULL, 0, false);
    }

    printf("Data Sent\nReceiving 1/2 Data\n");
    
    for(int c = 0; c < 2; c++){
      printf("c = %i\n", c);
      DptiIO(hif, NULL, 0, in_bytes+(c*n_bytes), n_bytes, false);
    }

    printf("Data Received\nSending 1/4 Data\n");
    
    for(int c = 3; c < 4; c++){
      printf("c = %i\n", c);
      DptiIO(hif, out_bytes+(c*n_bytes), n_bytes, NULL, 0, false);
    }
    
    printf("Data Sent\nReceiving 1/2 Data\n");
    
    for(int c = 2; c < 4; c++){
      printf("c = %i\n", c);
      DptiIO(hif, NULL, 0, in_bytes+(c*n_bytes), n_bytes, false);
    }
    
    printf("Data received, checking...\n");
    
    for(int i = 0; i < n_bytes*4; i++){
      if(out_bytes[i] != in_bytes[i]){
	printf("Verification error: out: %02x, in: %02x @ %i\n", out_bytes[i], in_bytes[i], i);
      }
    }
  }
  
  DmgrClose(hif);
  return 0;
}
