#pragma once

#define ATA_STATUS_BUSY             0x80
#define ATA_STATUS_READY            0x40
#define ATA_STATUS_WRITE_FAULT      0x20
#define ATA_STATUS_SEEK_COMPLETE    0x10
#define ATA_STATUS_DATA_REQUEST     0x08
#define ATA_STATUS_CORRECTED_DATA   0x04
#define ATA_STATUS_INDEX            0x02
#define ATA_STATUS_ERROR            0x01

#define ATA_ERROR_BAD_BLOCK         0x80
#define ATA_ERROR_UNCORRECTABLE     0x40
#define ATA_ERROR_MEDIA_CHANGE      0x20
#define ATA_ERROR_ID_NOT_FOUND      0x10
#define ATA_ERROR_MEDIA_CHANGE_REQ  0x08
#define ATA_ERROR_ABORTED           0x04
#define ATA_ERROR_TRACK0_NOT_FOUND  0x02
#define ATA_ERROR_NO_ADDR_MARK      0x01

#define ATA_PORT_CMD_READ_PIO       0x20
#define ATA_PORT_CMD_READ_PIO_EXT   0x24
#define ATA_PORT_CMD_READ_DMA       0xC8
#define ATA_PORT_CMD_READ_DMA_EXT   0x25
#define ATA_PORT_CMD_WRITE_PIO      0x30
#define ATA_PORT_CMD_WRITE_PIO_EXT  0x34
#define ATA_PORT_CMD_WRITE_DMA      0xCA
#define ATA_PORT_CMD_WRITE_DMA_EXT  0x35

#define ATA_PORT_CMD_PACKET         0xA0
#define ATA_PORT_CMD_IDENTIFY_PACK  0xA1
#define ATA_PORT_CMD_IDENTIFY       0xEC
#define ATA_PORT_CMD_CACHE_CLEAR        0xE7
#define ATA_PORT_CMD_CACHE_CLEAR_EXT    0xEA

#define ATAPI_CMD_READ              0xA8
#define ATAPI_CMD_EJECT             0x1B

#define ATA_PORT_DATA               0x00
#define ATA_PORT_ERROR              0x01
#define ATA_PORT_FEATURES           0x01
#define ATA_PORT_SECTOR_COUNT0      0x02
#define ATA_PORT_LBA0               0x03
#define ATA_PORT_LBA1               0x04
#define ATA_PORT_LBA2               0x05
#define ATA_PORT_HDDEVSEL           0x06
#define ATA_PORT_COMMAND            0x07
#define ATA_PORT_STATUS             0x07
#define ATA_PORT_SECTOR_COUNT1      0x08
#define ATA_PORT_LBA3               0x09
#define ATA_PORT_LBA4               0x0A
#define ATA_PORT_LBA5               0x0B
#define ATA_PORT_CONTROL            0x0C
#define ATA_PORT_ALTSTATUS          0x0C
#define ATA_PORT_DEVICE_ADDRESS     0x0D

#define ATA_IDENT_DEVICETYPE        0
#define ATA_IDENT_CYLINDERS         2
#define ATA_IDENT_HEADS             6
#define ATA_IDENT_SECTORS           12
#define ATA_IDENT_SERIAL            20
#define ATA_IDENT_MODEL             54
#define ATA_IDENT_CAPABILITIES      98
#define ATA_IDENT_FIELDVALID        106
#define ATA_IDENT_MAX_LBA           120
#define ATA_IDENT_COMMANDSETS       164
#define ATA_IDENT_MAX_LBA_EXT       200

#define IDE_ATA                     0x00
#define IDE_ATAPI                   0x01
#define ATA_MASTER                  0x00
#define ATA_SLAVE                   0x01

#define ATA_PRIMARY                 0x00
#define ATA_SECONDARY               0x01

#define ATA_READ                    0x00
#define ATA_WRITE                   0x01