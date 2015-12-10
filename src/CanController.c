#include <stdio.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <linux/socket.h>
#include <linux/can.h>
#include <linux/can/error.h>
#include <linux/can/raw.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include "cancontroller.h"

#ifndef AF_CAN
#define AF_CAN 29
#endif
#ifndef PF_CAN
#define PF_CAN AF_CAN
#endif

#define DEV_NAME "can0"

int excute_can_rw(int);
int set_can_filter(int);
void handle_read_frame(struct can_frame *);
void handle_err_frame(const struct can_frame *);

int main(int argc, char *argv[]) {
	int sockt;
	int ret;
	struct sockaddr_can addr;
	struct ifreq ifr;

	srand(time(NULL));
	sockt = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if (sockt < 0) {
		perror("socket PF_CAN failed");
		return 1;
	}

	strcpy(ifr.ifr_name, DEV_NAME);
	ret = ioctl(sockt, SIOCGIFINDEX, &ifr);
	if (ret < 0) {
		perror("ioctl failed");
		return 1;
	}

	addr.can_family = PF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	ret = bind(sockt, (struct sockaddr *) &addr, sizeof(addr));
	if (ret < 0) {
		perror("bind failed");
		return 1;
	}

	ret = set_can_filter(sockt);
	if (ret < 0) {
		perror("setsockopt failed");
		return 1;
	}

	excute_can_rw(sockt);

	close(sockt);

	return 0;
}

int set_can_filter(int socket) {
	int ret;

	struct can_filter filter[3];
	filter[0].can_id = 0x200 | CAN_EFF_FLAG;
	filter[0].can_mask = 0xFFF;

	filter[1].can_id = 0x20F | CAN_EFF_FLAG;
	filter[1].can_mask = 0xFFF;

	filter[2].can_id = 0x1F0 | CAN_EFF_FLAG;
	filter[2].can_mask = 0xFFF;

	ret = setsockopt(socket, SOL_CAN_RAW, CAN_RAW_FILTER, &filter,
			sizeof(filter));

	return ret;
}

int excute_can_rw(int fd) {
	int ret, i;
	struct can_frame fr, frdup;
	struct timeval tv;
	fd_set rset;

	while (1) {
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		FD_ZERO(&rset);
		FD_SET(fd, &rset);
		printf("=====\n");

		/* Data read part*/
		printf("------------------------ \n");
		ret = read(fd, &frdup, sizeof(frdup));
		if (ret < sizeof(frdup)) {
			myerr("read failed");
			return -1;
		}

		if (frdup.can_id & CAN_ERR_FLAG) {
			handle_err_frame(&frdup);
			myerr("CAN device error");
			continue;
		}

		handle_read_frame(&frdup);

		// =============================

		frdup.can_id = 0x1fffffff;
		frdup.can_id &= CAN_EFF_MASK;
		frdup.can_id |= CAN_EFF_FLAG;
		frdup.can_dlc = 8;

		printf("send data = ");
		int j = 0xDD;
		for (i = 0; i < frdup.can_dlc; i++) {
			frdup.data[i] = j--;
			printf("%02x ", frdup.data[i]);
		}

		usleep(4000);

		ret = write(fd, &frdup, sizeof(frdup));
		if (ret < 0) {
			myerr("write failed");
			return -1;
		}
	}

	return 0;
}

void handle_read_frame(struct can_frame *fr) {
	int i;
	printf("%08x\n", fr->can_id & CAN_EFF_MASK);
	//printf("%08x\n", fr->can_id);
	printf("dlc = %d\n", fr->can_dlc);
	printf("data = ");
	for (i = 0; i < fr->can_dlc; i++)
		printf("%02x ", fr->data[i]);
	printf("\n");
}

void handle_err_frame(const struct can_frame *fr) {
	if (fr->can_id & CAN_ERR_TX_TIMEOUT) {
		errout("CAN_ERR_TX_TIMEOUT");
	}
	if (fr->can_id & CAN_ERR_LOSTARB) {
		errout("CAN_ERR_LOSTARB");
		errcode(fr->data[0]);
	}
	if (fr->can_id & CAN_ERR_CRTL) {
		errout("CAN_ERR_CRTL");
		errcode(fr->data[1]);
	}
	if (fr->can_id & CAN_ERR_PROT) {
		errout("CAN_ERR_PROT");
		errcode(fr->data[2]);
		errcode(fr->data[3]);
	}
	if (fr->can_id & CAN_ERR_TRX) {
		errout("CAN_ERR_TRX");
		errcode(fr->data[4]);
	}
	if (fr->can_id & CAN_ERR_ACK) {
		errout("CAN_ERR_ACK");
	}
	if (fr->can_id & CAN_ERR_BUSOFF) {
		errout("CAN_ERR_BUSOFF");
	}
	if (fr->can_id & CAN_ERR_BUSERROR) {
		errout("CAN_ERR_BUSERROR");
	}
	if (fr->can_id & CAN_ERR_RESTARTED) {
		errout("CAN_ERR_RESTARTED");
	}
}
