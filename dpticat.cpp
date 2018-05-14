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
  
  
  byte *out_bytes = new byte[2];
  byte *in_bytes = new byte[255];
  
  for(int test_count = 0; test_count < N_TESTS; test_count++){
    printf("Test %i\n", test_count);
    int n_bytes = rand() & 0xFF;
    out_bytes[0] = n_bytes;
    out_bytes[1] = 255 - n_bytes;
    for(int i = 0; i < 255; i++){
      in_bytes[i] = 0xBA;
    }
    
    printf("Requesting %i bytes\n", out_bytes[0]);
    
    DptiIO(hif, out_bytes, 1, NULL, 0, false);
    

    printf("Request Sent\nReceiving %i bytes\n", out_bytes[0]);
    
    DptiIO(hif, NULL, 0, in_bytes, out_bytes[0], false);

    printf("Data Received\nRequesting Remaining %i bytes\n", out_bytes[1]);
    
    DptiIO(hif, out_bytes+1, 1, NULL, 0, false);
    
    printf("Request Sent\nReceiving %i bytes\n", out_bytes[1]);
    
    DptiIO(hif, NULL, 0, in_bytes+out_bytes[0], out_bytes[1], false);
        
    printf("Data received, checking...\n");
    
    for(int i = 0; i < out_bytes[0]; i++){
      if((out_bytes[0]-i) != in_bytes[i]){
	printf("Verification error: expected: %02x, received: %02x @ %i\n", out_bytes[0]-i, in_bytes[i], i);
      }
    }
    for(int i = 0; i < out_bytes[1]; i++){
      if((out_bytes[1]-i) != in_bytes[i+out_bytes[0]]){
	printf("Verification error: expected: %02x, received: %02x @ %i\n", out_bytes[1]-i, in_bytes[i+out_bytes[0]], i);
      }
    }
    
  }
  
  DmgrClose(hif);
  return 0;
}
