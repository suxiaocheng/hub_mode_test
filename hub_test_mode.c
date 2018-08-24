/*
 * Copyright (C) 2015 Huizhou Desay SV Automotive Co., Ltd
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <libusb.h>
#include <errno.h>

#define USB_REQ_SET_FEATURE	0x03
#define USB_RT_PORT		0x23
#define USB_PORT_FEAT_TEST	21

#if 1
#define USB_TEST_J		0X01
#define USB_TEST_K		0X02
#define USB_TEST_SE0_NAK	0X03
#define USB_TEST_PACKET		0x04
#define USB_TEST_FORCE_ENABLE	0x05
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define CHAR_MAX_LEN	512

int total_devices;

void usage(void)
{
	printf("usage: host mode for hub test\n");
	printf("hub_test_mode vid pid port_num test_mode\n\n");
	printf("vid pid: VendorID ProductID: 0x0000 0x0001\n\n");
	printf("port_num:  1 - port1\n");
	printf("           2 - port2\n");
	printf("           3 - port3\n\n");
	printf("test_mode: 1 - USB_TEST_J\n");
	printf("           2 - USB_TEST_K\n");
	printf("           3 - USB_TEST_SE0_NAK\n");
	printf("           4 - USB_TEST_PACKET\n");
	printf("           5 - USB_TEST_FORCE_ENABLE\n");
}

int get_configuration(libusb_device * dev,
		      struct libusb_config_descriptor *config)
{
	int ret = 0;
	ret = libusb_get_config_descriptor(dev, 0, &config);
	return ret;
}

void dump_altsetting(const struct libusb_interface_descriptor *interface)
{
	printf("    Interface Descriptor:\n"
	       "      bLength             %5u\n"
	       "      bDescriptorType     %5u\n"
	       "      bInterfaceNumber    %5u\n"
	       "      bAlternateSetting   %5u\n"
	       "      bNumEndpoints       %5u\n"
	       "      bInterfaceClass     %5u\n"
	       "      bInterfaceSubClass  %5u\n"
	       "      bInterfaceProtocol  %5u\n",
	       interface->bLength, interface->bDescriptorType,
	       interface->bInterfaceNumber, interface->bAlternateSetting,
	       interface->bNumEndpoints, interface->bInterfaceClass,
	       interface->bInterfaceSubClass, interface->bInterfaceProtocol);
}

void dump_interface(const struct libusb_interface *interface)
{
	int i;

	for (i = 0; i < interface->num_altsetting; i++)
		dump_altsetting(&interface->altsetting[i]);
}

int list_devices(libusb_context * ctx)
{
	libusb_device **list;
	struct libusb_device_descriptor desc;
	struct libusb_config_descriptor *conf;

	libusb_device_handle *handle = NULL;
	int config = 0;
	int ret;

	int status;
	ssize_t num_devs, i, j, k;

	status = 1;		/* 1 device not found, 0 device found */

	total_devices = 0;
	num_devs = libusb_get_device_list(ctx, &list);
	if (num_devs < 0)
		goto error;
	total_devices = num_devs;

	for (i = 0; i < num_devs; ++i) {
		libusb_device *dev = list[i];
		libusb_open(dev, &handle);

		libusb_get_configuration(handle, &config);

		uint8_t bnum = libusb_get_bus_number(dev);
		uint8_t dnum = libusb_get_device_address(dev);

		libusb_get_device_descriptor(dev, &desc);
		status = 0;
		printf("[%zx]device:%04x:%04x\n", i, desc.idVendor,
		       desc.idProduct);
		printf("bDeviceSubClass = %5u\n", desc.bDeviceSubClass);
		printf("bDeviceClass    = %5u\n", desc.bDeviceClass);
		printf("bDeviceProtocol    = %5u\n", desc.bDeviceProtocol);
		
		{
			struct libusb_device_handle *usb_p=NULL;
			unsigned char product[CHAR_MAX_LEN];
			unsigned char sn[CHAR_MAX_LEN];
			ret = libusb_open(list[i],&usb_p);
			if (ret == 0) {
				libusb_get_string_descriptor_ascii(usb_p,desc.iProduct,product,CHAR_MAX_LEN);
				libusb_get_string_descriptor_ascii(usb_p,desc.iSerialNumber,sn,CHAR_MAX_LEN);
				printf("iProduct = %s\n", product);
				printf("iSerialNumber = %s\n", sn);
				libusb_close(usb_p);
			} else {
				printf("[ERROR]libusb open device fail\n");
				continue;
			}
		}
		
		for (j = 0; j < desc.bNumConfigurations; ++j) {
			ret = libusb_get_config_descriptor(dev, j, &conf);
			if (ret) {
				fprintf(stderr, "Couldn't get configuration "
					"descriptor %lu, some information will "
					"be missing\n", j);
			} else {
				printf("bNumberInterfaces = %5u\n",
				       conf->bNumInterfaces);
				printf("bConfigurationValue = %5u\n",
				       conf->bConfigurationValue);

				for (k = 0; k < conf->bNumInterfaces; k++)
					dump_interface(&conf->interface[k]);
				libusb_free_config_descriptor(conf);
			}
		}		

	}

	libusb_free_device_list(list, 0);
error:
	return status;
}

int test_devices(libusb_context * ctx, int devices)
{
	libusb_device **list;
	struct libusb_device_descriptor desc;
	struct libusb_config_descriptor *conf;

	libusb_device_handle *handle = NULL;
	int config = 0;
	int ret;

	int status;
	ssize_t num_devs, i, j, k;

	status = 1;		/* 1 device not found, 0 device found */
	
	num_devs = libusb_get_device_list(ctx, &list);
	if (num_devs < 0)
		goto error;
	
	if ((devices >= 0) && (devices < num_devs))
	{
		struct libusb_device_handle *usb_p=NULL;		
		ret = libusb_open(list[devices],&usb_p);
		if (ret == 0) {
			ret = libusb_control_transfer(usb_p, USB_RT_PORT,	/* bRequesType */
							  USB_REQ_SET_FEATURE,	/* bRequest */
							  USB_PORT_FEAT_TEST,	/* wValue */
							  (USB_TEST_PACKET << 8) | 0x01,	/* wIndex */
							  NULL,	/* data */
							  0,	/* wLength */
							  1000);	/* timeout */
			if (ret != 0)
				printf("[ERROR] send set_feature message failed\n");
			else
				printf("[INFO] send set_feature message success\n");
			libusb_close(usb_p);
		} else {
			printf("[ERROR]libusb open device fail\n");			
		}
	} else {
		printf("[ERROR] Device out of range total[%zx], current[%d]\n", 
			num_devs, devices);
	}
	
	libusb_free_device_list(list, 0);
error:
	return status;
}

int main(int argc, char **argv)
{
	libusb_context *ctx = NULL;	/* a libusb session */
	int ret;
	unsigned int vid, pid, portnum, test_mode;
	int i;
	int sel_device = 0;
	
	vid = pid = 0;
	portnum = 0;
	test_mode = 0;
	
	printf("[INFO] Argument Number: %d\n", argc);
	for (i=0; i<argc; i++) {
		printf("[INFO] \t arg[%d]: %s\n", i, argv[i]);
	}
	printf("[INFO] End of argument list dump\n\n");

	if ((argc > 1) && (strcmp(argv[1], "-h") == 0 ||
	    strcmp(argv[1], "--help")) == 0) {
		usage();
		return -1;
	}

	if(argc >= 5) {
		/* Parse the parameter data from string to unsigned long */
		vid = strtoul(argv[1], NULL, 0);
		pid = strtoul(argv[2], NULL, 0);
		portnum = strtoul(argv[3], NULL, 0);
		test_mode = strtoul(argv[4], NULL, 0);
	}

	printf("vid[%04x] pid[%04x] port_num[%d] test_mode[%d]\n",
	       vid, pid, portnum, test_mode);

	/* initialize a library session */
	ret = libusb_init(&ctx);
	if (ret < 0) {
		printf("libusb_init fail\n");
		return -1;
	}

	list_devices(ctx);

	if ((vid != 0) && (pid != 0)) {
		libusb_device_handle *udevh;
		udevh = libusb_open_device_with_vid_pid(ctx, vid, pid);
		if (udevh) {
			ret = libusb_control_transfer(udevh, USB_RT_PORT,	/* bRequesType */
							  USB_REQ_SET_FEATURE,	/* bRequest */
							  USB_PORT_FEAT_TEST,	/* wValue */
							  (test_mode << 8) | portnum,	/* wIndex */
							  NULL,	/* data */
							  0,	/* wLength */
							  1000);	/* timeout */
			if (ret != 0)
				printf("send set_feature message failed\n");
			else
				printf("send set_feature message success\n");

			libusb_close(udevh);
		} else {
			printf("opne hub device failed\n");
			return -1;
		}
	} else {		
		int sel_ok = FALSE;
		char sel_string;

		while (TRUE) {
			printf("\n\nPlease manual select the interface[0-%d]:", 
				total_devices-1);
			sel_string = (char)getchar();
			getchar();
			
			if (sel_string == 'q') {
				printf("[INFO] User quit\n");
				sel_device = -1;
			} else {
				printf("[INFO] Input is:%c\n", sel_string);				
				
				if ((sel_string < '0') || (sel_string > '9')) {
					printf("[ERROR] You input is invald: %c\n", sel_string);
					sel_device = 0;
				} else {
					sel_device = sel_string - '0';
					if((sel_device >= 0) && (sel_device < total_devices)) {
						sel_ok = TRUE;
					} else {
						printf("[ERROR] Select device out of range.\n");
						sel_device = 0;
					}
				}
			}
			if (sel_ok == TRUE) {
				break;
			}
			if (sel_device == -1) {
				return 0;
			}
		}
	}
	printf("[INFO] You select device is %d\n", sel_device);
	test_devices(ctx, sel_device);

	return 0;
}
