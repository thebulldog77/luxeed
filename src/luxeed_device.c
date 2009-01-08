

#include <stdio.h>
#include <stdlib.h> /* memset(), memcpy() */
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "luxeed_device.h"


static int luxeed_dev_id = 0;

static int usb_debug_level = 9;


#if 1
#define RCALL(X) do { result = X; if ( result < 0 || dev->debug ) { fprintf(stderr, "  %s => %d\n", #X, (int) result); } } while ( 0 )
#else
#define RCALL(X) result = X
#endif


/* Parsed from ep01.txt as initialization, minues the leading 0x02.
   Chunks are sent 65 bytes long.
   Checksum appears at 0x14e.
*/
static unsigned char msg_00[] = {
  /* 0000: */       0x02, 0x01, 0x80, 0x00, 0x01, 0x01, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 
  /* 0010: */ 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  /* 0020: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  /* 0030: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  /*     : */ 0x00,
  /* 0040: */       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  /* 0050: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  /* 0060: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  /* 0070: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  /*     : */ 0x00,
  /* 0080: */       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  /* 0090: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  /* 00a0: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  /* 00b0: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  /*     : */ 0x00,
  /* 00c0: */       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  /* 00d0: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  /* 00e0: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  /* 00f0: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  /*     : */ 0x00,
  /* 0100: */       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  /* 0110: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  /* 0120: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  /* 0130: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  /*     : */ 0x00,
  /* 0140: */       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 
  /* 0150: */ 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  /* 0160: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  /* 0170: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  /*     : */ 0x00,
};
static int msg_size = sizeof(msg_00);


static unsigned char msg_ff[] = {
    /*             f     0     1     2     3     4     5     6     7     8     9     a     b     c     d     e  */
    /*                                                aa    bb                                                  */
    /* 0000: */       0x02, 0x01, 0x80, 0x00, 0x00, 0x16, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 
    /* 0010: */ 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0020: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0030: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    /*     : */ 0xff,

    /* 0040: */       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* 0050: */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* 0060: */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* 0070: */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /*     : */ 0xff,

    /* 0080: */       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* 0090: */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* 00a0: */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* 00b0: */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /*     : */ 0xff,

    /* 00c0: */       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* 00d0: */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* 00e0: */ 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
    /* 00f0: */ 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
    /*     : */ 0x00,

    /* 0100: */       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* 0110: */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
    /* 0120: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0130: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /*     : */ 0x00,

    /*                                                                                                      cc,
		  dd											        */
    /* 0140: */       0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe6,
    /* 0150: */ 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0160: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0170: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /*     : */ 0x00,
  };


static unsigned char msg_xx[] = {
    /*             f     0     1     2     3     4     5     6     7     8     9     a     b     c     d     e  */
    /*                                                aa    bb                                                  */
    /* 0000: */       0x02, 0x01, 0x80, 0x00, 0x00, 0x16, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 
    /* 0010: */ 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0020: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0030: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    /*     : */ 0xff,

    /* 0040: */       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* 0050: */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* 0060: */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* 0070: */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /*     : */ 0xff,

    /* 0080: */       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* 0090: */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* 00a0: */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* 00b0: */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /*     : */ 0xff,

    /* 00c0: */       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* 00d0: */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* 00e0: */ 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
    /* 00f0: */ 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
    /*     : */ 0x00,

    /* 0100: */       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* 0110: */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
    /* 0120: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0130: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /*     : */ 0x00,

    /*                                                                                                      cc,
		  dd											        */
    /* 0140: */       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe6,
    /* 0150: */ 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0160: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0170: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /*     : */ 0x00,
  };


luxeed_device *luxeed_device_create()
{
  luxeed_device *dev = malloc(sizeof(*dev));
  
  memset(dev, 0, sizeof(*dev));
  
  return dev;
}


int luxeed_device_destroy(luxeed_device *dev)
{
  if ( ! dev ) {
    return 0;
  }
  luxeed_device_close(dev);
  free(dev->msg);
  memset(dev, 0, sizeof(*dev));
  free(dev);
  return 0;
}


int luxeed_device_find(luxeed_device *dev, uint16_t vendor, uint16_t product)
{
  struct usb_bus *u_bus;
  struct usb_device *u_dev;
  struct usb_bus *u_busses;

  if ( ! vendor ) {
    vendor  = LUXEED_USB_VENDOR;
  }
  if ( ! product ) {
    product = LUXEED_USB_PRODUCT;
  }

  usb_init();
  usb_find_busses();
  usb_find_devices();
  u_busses = usb_get_busses();

  usb_set_debug(usb_debug_level);
  
  for ( u_bus = u_busses; u_bus; u_bus = u_bus->next ) {
    for ( u_dev = u_bus->devices; u_dev; u_dev = u_dev->next ) {
      if ( (u_dev->descriptor.idVendor == vendor) && (u_dev->descriptor.idProduct == product) ) {
	dev->id = luxeed_dev_id ++;

	dev->u_bus = u_bus;
	dev->u_dev = u_dev;

	dev->msg_size = msg_size;
	dev->msg_len = msg_size;
	dev->msg = malloc(sizeof(dev->msg[0]) * dev->msg_size);
	memset(dev->msg, 0, sizeof(dev->msg[0]) * dev->msg_size);

	if ( 0 ) {
	  fprintf(stderr, "  bus %s 0x%x\n", (char*) u_bus->dirname, (int) u_bus->location);
	  fprintf(stderr, "  dev %s 0x%x\n", (char*) u_dev->filename, (int) u_dev->devnum);
	}

        return 0;
      }
    }
  }

  return -1;
}


int luxeed_device_open(luxeed_device *dev)
{
  int result = -1;

  do {
    if ( dev->opened ) {
      result = 0;
      break;
    }
    if ( dev->opening ) {
      result = 0;
      break;
    }
    dev->opening = 1;
    dev->opened = 0;

    fprintf(stderr, "dev opening\n");

    /* Locate the USB device. */
    if ( ! dev->u_dev ) {
      if ( (result = luxeed_device_find(dev, 0, 0)) < 0 ) {
	break;
      }
    }

    /* Open the USB device. */
    dev->u_dh = usb_open(dev->u_dev);
    if ( ! dev->u_dh ) {
      result = -1;
      break;
    }
    
    /* Reset the device */
    RCALL(usb_reset(dev->u_dh));
    
    /* Detach any kernel drivers for the endpoint */
    RCALL(usb_detach_kernel_driver_np(dev->u_dh, 2));
    
    /* Claim the interface. */
    RCALL(usb_claim_interface(dev->u_dh, 2));

    /* Wait for a bit before initializing the device. */
    usleep(100000);

    /* Mark device opened. */
    dev->opening = 0;
    dev->opened = 1;

    fprintf(stderr, "dev opened\n");

    result = 0;

  } while ( 0 );

  return result;
}


int luxeed_device_close(luxeed_device *dev)
{
  int result = 0;

  do {
    if ( dev->u_dh ) {
      RCALL(usb_release_interface(dev->u_dh, 1));
      RCALL(usb_release_interface(dev->u_dh, 2));

      RCALL(usb_close(dev->u_dh));
      if ( result ) break;

      dev->u_dh = 0;
    }

    /* Force device search on open(). */
    dev->u_dev = 0;

    dev->opened = 0;
    dev->opening = 0;
    dev->inited = 0;
    dev->initing = 0;
  } while ( 0 );

  return result;
}


static int dump_buf(unsigned char *bytes, int size)
{
  int i;

  fprintf(stderr, "\n      ");
  for ( i = 0; i < 0x10; ++ i ) {
    fprintf(stderr, "%2x ", (int) i);
  }
  for ( i = 0; i < size; ++ i ) {
    if ( i % 0x10 == 0 ) {
      fprintf(stderr, "\n%04x: ", (int) i);
    }
    fprintf(stderr, "%02x ", (int) bytes[i]);
  }
  fprintf(stderr, "\n\n");

  return 0;
}


#define LUXEED_BLOCK_SIZE 64

// #define luxeed_send luxeed_send_buffered
#define luxeed_send luxeed_send_chunked


int luxeed_send_buffered (luxeed_device *dev, int ep, unsigned char *bytes, int size)
{
  int result = 0;
  int timeout = 1000;

  unsigned char *buf = alloca(sizeof(buf) * ((size / LUXEED_BLOCK_SIZE) * (LUXEED_BLOCK_SIZE + 1)));
  unsigned char *s = buf;
  int buf_size;

  luxeed_device_msg_checksum(dev, bytes, size);

  {
    int i = 0;
    while ( i < size ) {
      *(s ++) = 0x02;
      memcpy(s, bytes + i, LUXEED_BLOCK_SIZE);
      s += LUXEED_BLOCK_SIZE;
      i += LUXEED_BLOCK_SIZE;
    }

    buf_size = s - buf;
  }

  fprintf(stderr, "size %d => %d\n", (int) size, (int) buf_size);
  dump_buf(buf, buf_size);

  RCALL(usb_interrupt_write(dev->u_dh, ep, (void*) buf, buf_size, timeout));
  usleep(750);

  /* Try again later? */
  if ( result < 0 ) {
    luxeed_device_close(dev);
    return 0;
  }

  if ( result >= 0 ) {
    result = 0;
  }

  return result;
}


int luxeed_send_chunked (luxeed_device *dev, int ep, unsigned char *bytes, int size)
{
  int result = -1;
  int timeout = 1000;

  usb_dev_handle *dh;

  luxeed_device_msg_checksum(dev, bytes, size);

  if ( dev->debug ) {
    fprintf(stderr, "send_bytes(%d, %d)...", (int) ep, (int) size);
    dump_buf(bytes, size);
  }


  {
    unsigned char *buf = bytes;
    int left = size;
    int blksize = LUXEED_BLOCK_SIZE;

    while ( left > 0 ) {
      /* Chunk of 0x02 + 64 bytes */
      unsigned char xbuf[1 + blksize];

      xbuf[0] = 0x02;
      memcpy(xbuf + 1, buf, blksize);

      int wsize = blksize < left ? blksize : left;
      wsize += 1;

      if ( dev->debug > 1 ) {
	dump_buf(xbuf, wsize);
      }

      RCALL(usb_interrupt_write(dev->u_dh, ep, (void*) xbuf, wsize, timeout));
      // RCALL(usb_bulk_write(dh, ep, xbuf, wsize, timeout));
      usleep(750);

      /* Try again next time. */
      if ( result < 0 ) {
	luxeed_device_close(dev);
	return 0;
      }

      if ( result != wsize ) {
	result = -1;
	break;
      }
      buf += blksize;
      left -= blksize;
    }
  }
  if ( result >= 0 ) {
    result = 0;
  }
  

  /* Read result */
  if ( 0 ) {
    char buf[1024];
    int buf_size = sizeof(buf);
    RCALL(usb_bulk_read(dh, ep, buf, buf_size, timeout));
    if ( result < size ) {
      result = -1;
    }
  }

  if ( 0 ) {
    RCALL(usb_clear_halt(dh, ep));
  }

  return result;
}




int luxeed_device_msg_checksum(luxeed_device *dev, unsigned char *buf, int size)
{
  int sum = 0;
  int i;
  int sum_save;
  int chksum_i = 0x14e; // 0x154 in total msg output.

  // return 0;

  if ( dev->debug ) {
    if ( buf == msg_ff || buf == msg_00 ) {
      dump_buf(buf, size);
    }
  }
  
  sum_save = buf[chksum_i];
  buf[chksum_i] = 0;

  for ( i = 0; i < size; ++ i ) {
    sum += buf[i];
  }
  sum -= 5;
  sum &= 0xff;

  buf[chksum_i] = sum;

  if ( buf == msg_ff || buf == msg_00 ) {
    if ( sum != sum_save ) {
      dump_buf(buf, size);
      fprintf(stderr, "%p sum %02x, expected %02x\n", (void*) buf, (int) sum, (int) sum_save);
      assert(sum == sum_save);
    }
  } else {
    buf[chksum_i - 1] = 0;
  }

  return sum;
}


/* Initialize all LEDS */
int luxeed_device_init(luxeed_device *dev)
{
  int result = -1;

  do {
    int slp = 100000;
    int i;
    int n = 2;

    if ( dev->initing ) {
      result = 0;
      break;
    }
    if ( dev->inited ) {
      result = 0;
      break;
    }

    fprintf(stderr, "dev initing\n");

    dev->initing = 1;
    dev->inited = 0;

    usleep(slp);
    for ( i = 0; i < n; ++ i ) {

      RCALL(luxeed_send (dev, LUXEED_USB_ENDPOINT_DATA, msg_00, sizeof(msg_00)));
      if ( result ) return result;
      usleep(slp);
      
      RCALL(luxeed_send (dev, LUXEED_USB_ENDPOINT_DATA, msg_ff, sizeof(msg_ff)));
      if ( result ) return result;
      usleep(slp);
    }

    RCALL(luxeed_send (dev, LUXEED_USB_ENDPOINT_DATA, msg_00, sizeof(msg_00)));
    if ( result ) return result;
    usleep(slp);
      
    dev->initing = 0;
    dev->inited = 1;

    fprintf(stderr, "dev: inited\n");

  } while ( 0 );

  return result;
}


unsigned char *luxeed_device_pixel(luxeed_device *dev, int key)
{
  if ( key < 0 || key >= LUXEED_NUM_OF_KEYS ) {
    return 0;
  }
  return &dev->key_data[key * 3];
}


int luxeed_device_update(luxeed_device *dev)
{
  int result = 0;

  do {
    if ( luxeed_device_open(dev) < 0 ) {
      result = -1;
      break;
    }

    if ( luxeed_device_init(dev) < 0 ) {
      result = -1;
      break;
    }

    /* Send data */
    assert(dev->msg_size == sizeof(msg_xx));

    /* Start with basic msg. */
    memcpy(dev->msg, msg_xx, dev->msg_size);

    /* Copy key pixel data into place. */
    memcpy(dev->msg + 0x37, dev->key_data, sizeof(dev->key_data));

    /* Compute checksum and send in chunks. */
    result = luxeed_send (dev, LUXEED_USB_ENDPOINT_DATA, dev->msg, dev->msg_size);
    if ( result ) break;

  } while ( 0 );

  return result;
}


static struct {
  int offset;
  const char *map;
  int shift;
} _key_maps[] = {
  { 0 , "`123456789" },
  { 0 , "~!@#$%^&*(", 1 }, /* SHIFT */
  { 10, "0-=\01BKSP\01TAB qwert" },
  { 10, ")_+\01BKSP\01TAB QWERT", 1 },  /* SHIFT */
  { 20, "yuiop[]\\\01CPLKa" },
  { 20, "YUIOP{}|\01CPLKA", 1 },  /* SHIFT */
  { 30, "sdfghjkl;" },
  { 30, "SDFGHJKL:", 1 },  /* SHIFT */
  { 40, "\01ENTR\01LSFTzxcvbnm," },
  { 40, "\01ENTR\01LSFTZXCVBNM<", 1 },  /* SHIFT */
  { 50, "./\01RSFT\01LCTR\01RWIN\01LALT\02RALT\01LWIN\01MENU\01RCTR" },
  { 50, ">?", 1 }, /* SHIFT */
  { -1, 0 }
};

static luxeed_key _keys[LUXEED_NUM_OF_KEYS];

int luxeed_init_keys()
{
  static int initialized = 0;
  int i;
  const char *s;

  if ( initialized )
    return 0;
  
  initialized ++;

  for ( i = 0; (s = _key_maps[i].map); ++ i ) {
    int k = _key_maps[i].offset;

    while ( *s ) {
      luxeed_key *key;
      char name[5];

      memset(name, 0, sizeof(name));

      /* "\01TAB " or "\01LSFT" */
      if ( *s == '\01' ) {
	++ s;
	memcpy(name, s, 4);
	if ( name[3] == ' ' ) 
	  name[3] = '\0';
	s += 4;
      } else {
	name[0] = *(s ++);
      }

      key = &_keys[k];
      key->id = k;
      {
	int j = 0;
	while ( key->name[j] ) {
	  ++ j;
	}
	key->name[j] = strdup(name);
      }

      k ++;
    }
  }

  return 0;
}


luxeed_key *luxeed_device_key_id(luxeed_device *dev, int id)
{
  luxeed_key *key = 0;
  int key_i;

  luxeed_init_keys();

  for ( key_i = 0; key_i < LUXEED_NUM_OF_KEYS; ++ key_i ) {
    if ( _keys[key_i].id == id ) {
      key = &_keys[key_i];
      break;
    }
  }

  return key;
}


luxeed_key *luxeed_device_key_name(luxeed_device *dev, const char *keyname)
{
  luxeed_key *key = 0;

  luxeed_init_keys();

  if ( *keyname == '#' ) {
    key = luxeed_device_key_id(dev, atoi(keyname + 1));
  } else {
    int key_i;
    for ( key_i = 0; key_i < LUXEED_NUM_OF_KEYS; ++ key_i ) {
      int j;
      const char *kn;
      for ( j = 0; (kn = _keys[key_i].name[j]); ++ j ) {
	if ( strcmp(kn, keyname) == 0 ) {
	  key = &_keys[key_i];
	  goto done;
	}
      }
    }
  }

 done:
  return key;
}


luxeed_key *luxeed_device_key_ascii(luxeed_device *dev, int c)
{
  luxeed_key *key = 0;
  int key_i;
  
  luxeed_init_keys();
  
  for ( key_i = 0; key_i < LUXEED_NUM_OF_KEYS; ++ key_i ) {
    int j;
    const char *kn;
    for ( j = 0; (kn = _keys[key_i].name[j]); ++ j ) {
      if ( kn[0] == c && kn[1] == '\0' ) {
	key = &_keys[key_i];
	goto done;
      }
    }
  }

 done:
  return key;
}


/* EOF */