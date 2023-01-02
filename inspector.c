#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <endian.h>
#include <ncurses.h>
#include <hidapi/hidapi.h>

#define AIR_VID 0x3318
#define AIR_PID 0x0424

int rows, cols;

static struct imu_report {
	uint16_t unknown1; // 01 02
	uint16_t unknown2;
	uint8_t unknown3;
	uint32_t some_counter1;
	uint8_t unknown4;
	uint64_t unknown5; // 00 00 a0 0f 00 00 00 01 
	uint8_t unknown6;
	int16_t rate_pitch;
	uint8_t unknown7;
	int16_t rate_roll;
	uint8_t unknown8;
	int16_t rate_yaw;
	uint16_t unknown9; // 20 00
	uint32_t unknown10; // 00 00 00 01
	uint8_t unknown11;
	int16_t gyro_roll;
	uint8_t unknown13;
	int16_t gyro_pitch1;
	uint8_t unknown14;
	int16_t gyro_pitch2; // not sure what is going on here
	uint16_t unknown15; // 00 80
	uint32_t unknown16; // 00 04 00 00
	int16_t mag1;
	int16_t mag2;
	int16_t mag3;
	uint32_t some_counter2;
	uint32_t unknown17; // 00 00 00 00
	uint8_t unknown18;
	uint8_t unknown19;
} __attribute__((__packed__)) report;

static void fix_report() {
	report.unknown1 = le32toh(report.unknown1);
	report.some_counter1 = le32toh(report.some_counter1);

	report.rate_pitch = le16toh(report.rate_pitch);
	report.rate_roll = le16toh(report.rate_roll);
	report.rate_yaw = le16toh(report.rate_yaw);

	report.gyro_roll = le16toh(report.gyro_roll);
	report.gyro_pitch1 = le16toh(report.gyro_pitch1);
	report.gyro_pitch2 = le16toh(report.gyro_pitch2);

	report.mag1 = le16toh(report.mag1);
	report.mag2 = le16toh(report.mag2);
	report.mag3 = le16toh(report.mag3);

	report.some_counter2 = le32toh(report.some_counter2);
}

static void print_report() {
	const int HEIGHT = 3;
	const int WIDTH = 20;
	int x = (cols - WIDTH) / 2;
	int y = (rows - HEIGHT) / 2;

	mvprintw(y++, x, "Rate: %04hx %04hx %04hx", report.rate_roll, report.rate_pitch, report.rate_yaw);
	mvprintw(y++, x, "Gyro: %04hx %04hx %04hx", report.gyro_roll, report.gyro_pitch1, report.gyro_pitch2);
	mvprintw(y++, x, "Mag:  %04hx %04hx %04hx", report.mag1, report.mag2, report.mag3);
}

static void print_bytes(uint8_t* buf, size_t len) {
	const int WIDTH = 16;
	int y = (rows - len / (WIDTH + 1)) / 2;
	int x = (cols - WIDTH * 2 - WIDTH + 1) / 2;

	for (int i = 0; i < len; i++) {
		if (i % WIDTH == 0) y++;
		mvprintw(y, x + 2 * (i % WIDTH) + (i % WIDTH), "%02x", buf[i]);
	}
}

static void print_line(char* s) {
	int width = (width = strlen(s)) > cols ? cols : width;
	mvprintw(rows / 2, (cols - width) / 2, "%s", s);
}

static hid_device* get_device() {
	struct hid_device_info* devs = hid_enumerate(AIR_VID, AIR_PID);
	struct hid_device_info* cur_dev = devs;
	hid_device* device = NULL;

	while (devs) {
		if (cur_dev->interface_number == 3) {
			device = hid_open_path(cur_dev->path);
			break;
		}

		cur_dev = cur_dev->next;
	}

	hid_free_enumeration(devs);
	return device;
}

int main() {
	// Open device
	hid_device* device = get_device();
	if (!device) {
		printf("Unable to open device\n");
		return 1;
	}

	// Open the floodgates
	uint8_t magic_payload[] = { 0xaa, 0xc5, 0xd1, 0x21, 0x42, 0x04, 0x00, 0x19, 0x01 };
	int res = hid_write(device, magic_payload, sizeof(magic_payload));
	if (res < 0) {
		printf("Unable to write to device\n");
		return 1;
	}

	initscr();
	cbreak();
	noecho();
	curs_set(0);
	do {
		getmaxyx(stdscr, rows, cols);
		erase();

		res = hid_read(device, (void*)&report, sizeof(report));
		if (res < 0) {
			print_line("Unable to get feature report");
			refresh();
			break;
		} else fix_report();

		if (report.unknown1 != 0x0201) {
			print_bytes((void*)&report, res);
			refresh();
			getch();
			continue;
		}

		print_report(report);
		refresh();
	} while (res);
	getch();
	endwin();

	hid_close(device);
	res = hid_exit();

	return 0;
}
