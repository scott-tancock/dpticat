#define _CRT_SECURE_NO_WARNINGS


#include <iostream>
#include <string>
#include <fstream>
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
#define TRAIN 8
#define COARSE (WIDTH - FINE - TRAIN)
#define TRAIN_SEQ 0x6A

typedef unsigned char byte;

char dev_name[cchDvcNameMax + 1];
HIF hif = hifInvalid;
unsigned char train_seq[TRAIN/8] = {TRAIN_SEQ};

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

  std::string filename;
  std::ofstream file;
  
  if (argc < 4) {
    file = std::ofstream("/tmp/tdc.dat", std::ios_base::trunc);
  }
  else {
    file = std::ofstream(argv[3], std::ios_base::trunc);
  }
  
  int n_bytes = 128;
  byte *out_bytes = new byte[1];
  byte *in_bytes = new byte[n_bytes];
  
  for(int test_count = 0; test_count < N_TESTS; test_count++){
    fprintf(stderr, "Test %i\n", test_count);
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
    
    for(int i = 0; (i+W_BYTES) <= out_bytes[0]; i+=W_BYTES){
      fprintf(stderr, "i = %i\n", i);
      int j;
      for(j = 0; j < W_BYTES; j++){
	printf("%02x", in_bytes[i+j]);
      }
      for(j = 0; j*8 < TRAIN; j++){
	if(in_bytes[i+j] != train_seq[j]){
	  fprintf(stderr, "Miss.\n");
	  i -= W_BYTES-1;
	  break;
	}
      }
      if(j * 8 < TRAIN){
	continue;
      }
      printf("\n");
      unsigned long long curr_coarse = 0;
      for(j = (TRAIN >> 3); (j+1)*8 <= COARSE+TRAIN; j++){
	curr_coarse = curr_coarse << 8 | in_bytes[i+j];
      }
      int leftover = (COARSE+TRAIN) & 0x7;
      if(leftover){
	printf("Coarse before: %llu, extra: %x\n", curr_coarse << leftover, (in_bytes[i + ((COARSE + TRAIN) >> 3)] >> (8-leftover)));
	curr_coarse = curr_coarse << leftover  | (in_bytes[i + ((COARSE+TRAIN) >> 3)] >> (8-leftover));
      }
      unsigned int curr_fine = 0;
      for(j = 0; (j+1)*8 <= FINE; j++){
	curr_fine = curr_fine | (((unsigned int)in_bytes[i+W_BYTES-1-j]) << (j*8));
      }
      leftover = FINE & 0x7;
      if(leftover){
	curr_fine = curr_fine | (((unsigned int)(in_bytes[i+W_BYTES-1-(FINE>>3)] & ((1 << leftover) - 1))) << (FINE & (~0x7)));
      }
      double curr_time = ((curr_coarse+1) / 120000000.0) - (curr_fine * 0.000000000015);
      printf("Coarse: %llu, fine: %u, time: %lf\n", curr_coarse, curr_fine, curr_time);
      printf("CDiff: %llu, FDiff: %i, TDiff: %le\n", curr_coarse - prev_coarse, (int)curr_fine - (int)prev_fine,curr_time - prev_time);
      file << curr_coarse << "," << curr_fine << "," << curr_time << "," << curr_time - prev_time << std::endl;
      prev_coarse = curr_coarse;
      prev_fine = curr_fine;
      prev_time = curr_time;
    }
  }
  file.close();
  DmgrClose(hif);
  return 0;
}
