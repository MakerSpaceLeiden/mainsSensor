#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "Arduino.h"

#include "esp32-hal.h"

#define GPIO_INPUT_PIN (21)

rmt_obj_t* rmt_recv = NULL;

#define MAXREC (4)
mainsnode_datagram_t recbuff[MAXREC];
int recn = 0;
int hdr_errs = 0, state_errs = 0, max_errs = 0, sem_wr_errs = 0, sem_rd_errs = 0;

SemaphoreHandle_t recvSemaphore = NULL;

void proc_datagram(size_t n, unsigned char * data) {
  mainsnode_datagram_t m = *(mainsnode_datagram_t*)data;

  if (m.hdr != MAINSNODE_HDR) {
    hdr_errs ++;
    return;
  };
  if (m.state != MAINSNODE_STATE_ON && m.state != MAINSNODE_STATE_OFF) {
    state_errs ++;
    return;
  };

  // chuck if we're too far behind.
  if (recn >= MAXREC) {
    max_errs++;
    return;
  };

  if (max_errs &&  xSemaphoreTake( recvSemaphore, ( TickType_t ) 10 ) == pdTRUE ) {
    recbuff[recn++] = m;
    xSemaphoreGive( recvSemaphore );
  } else {
    sem_wr_errs++;
  };
  /*
    Serial.printf("\n===>Sensor %08x: %x - state: %s (%x)\n\n", m.val,
                  ntohs(m.node_id), (m.state == MAINSNODE_STATE_ON) ? "On" : "Off", m.state);
  */
};

extern "C" void receive_data(uint32_t *data, size_t len)
{
  if (len == 0)
    return;
  proc(len, data);
}

void setup()
{
  Serial.begin(115200);
  vSemaphoreCreateBinary( recvSemaphore );


  // Initialize the channel to capture up to 192 items
  if ((rmt_recv = rmtInit(GPIO_INPUT_PIN false, RMT_MEM_192)) == NULL)
  {
    Serial.println("init receiver failed\n");
  }

  // Setup 1us tick
  float realTick = rmtSetTick(rmt_recv, 1000);

  // Ask to start reading
  rmtRead(rmt_recv, receive_data);
}

void loop()
{
  while (recn) {
    if (recvSemaphore && xSemaphoreTake( recvSemaphore, ( TickType_t ) 10 ) == pdTRUE ) {
      recn--;
      Serial.printf("Node %d is %s\n",
                    recbuff[recn].node_id, recbuff[recn].state ? "on" : "off");
      xSemaphoreGive( recvSemaphore );
    } else {
      sem_rd_errs++;
    }
  }
  {
    static int n = 0;
    int m = hdr_errs + state_errs + max_errs + sem_wr_errs + sem_rd_errs;
    if (m != n) {
      n = m;
      Serial.printf("Error: HDR: %04d STATE %04d MAX %04d WR %04D RD %04D\n",
                    hdr_errs, state_errs, max_errs, sem_wr_errs, sem_rd_errs);
    }
  }
  delay(500);
}

