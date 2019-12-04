#pragma once

struct crc32 {
	static uint32_t crc_table[256];

	crc32()
	{
		for (uint32_t i = 0; i < 256; i++) {
			uint32_t c = i << 24;
			for (int j = 0; j < 8; j++) {
				c = (c << 1) ^ ((c & 0x80000000) ?
					0x04C11DB7 :
					0);
			}
			crc_table[i] = c;
		}
	}

	uint32_t
	calc(uint8_t *buf, size_t len)
	{
		uint32_t c = 0xffffffff;

		for (int i = 0; i < len; i++) {
			c = (c << 8) ^ crc_table[((c >> 24) ^ buf[i]) & 0xff];
		}
		return c;
	}
};

