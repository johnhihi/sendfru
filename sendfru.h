
// indicate the number of PHBs
#define DOMAIN_COUNT 6

// PHBs will map to different domains
#define PCIE_SLOT0_DOMAIN 0
#define PCIE_SLOT1_DOMAIN 5
#define PCIE_SLOT2_DOMAIN 4
#define PCIE_MEZZ0_DOMAIN 2
#define PCIE_MEZZ1_DOMAIN 3

// FRU ID for each slot, defined in BMC
#define PCIE_SLOT0_FRUID 0x37
#define PCIE_SLOT1_FRUID 0x38
#define PCIE_SLOT2_FRUID 0x39
#define PCIE_MEZZ0_FRUID 0x3A
#define PCIE_MEZZ1_FRUID 0x3B
#define SLOT_MAP_TO_FRUID {  PCIE_SLOT0_FRUID, \
                             PCIE_SLOT1_FRUID, \
                             PCIE_SLOT2_FRUID, \
                             PCIE_MEZZ0_FRUID, \
                             PCIE_MEZZ1_FRUID }

// Macro for finding the domains respectively
#define DOMAIN_MAP_TO_SLOT_COUNT 5
#define DOMAIN_MAP_TO_SLOT {  PCIE_SLOT0_DOMAIN, \
                              PCIE_SLOT1_DOMAIN, \
                              PCIE_SLOT2_DOMAIN, \
                              PCIE_MEZZ0_DOMAIN, \
                              PCIE_MEZZ1_DOMAIN }

#define IPMI_WR_FRU_FN 0x0a
#define IPMI_WR_FRU_CMD 0x12

// max opal supported
#define IPMI_MAX_REQ_SIZE		60

#define MIN(a, b)	((a) < (b) ? (a) : (b))
#define MAX(a, b)	((a) > (b) ? (a) : (b))
  
typedef unsigned char BYTE;

// TYPE/LENGTH BYTE FORMAT
// 7:6 - type code
//   00 - binary or unspecified
//   01 - BCD plus
//   10 - 6-bit ASCII, packed (overrides Language Codes)
//   11 - Interpretation depends on Language Codes.
// 5:0 - number of data bytes
typedef union
{
  struct
  {
    unsigned char n_bytes : 6;
    unsigned char t_code  : 2;
  }bits;
  BYTE byte;
}TYPE_LEN;

// max data length is n_bytes[5:0] = 11_1111b
#define MAX_DATA_LENGTH 0x3F

#define FORMAT_VERSION_NUMBER 0x01
#define FORMAT_VERSION_NUMBER_MASK 0x0F
#define LANGUAGE_CODE 0x00
#define DEFAULT_TYPE_CODE 0x3
#define NO_MORE_FIELDS_BYTE 0xC1


struct board_information
{
  
  // 7:4 - reserved, write as 0000b
  // 3:0 - format version number = 1h 
  BYTE b_area_format_version;
  
  // Board Area Length (in multiples of 8 bytes)
  BYTE b_area_length;
  
  // Language Code ( 0x0 indicate English )
  BYTE b_lang_code;
  
  // Number of minutes from 0:00 hrs 1/1/96
  // LSbyte first (lettle endian)
  // 00_00_00h = unspecified.
  BYTE mfg_date_time[3];
  
  // TYPE/LENGTH BYTE FORMAT
  // 7:6 - type code
  //   00 - binary or unspecified
  //   01 - BCD plus
  //   10 - 6-bit ASCII, packed (overrides Language Codes)
  //   11 - Interpretation depends on Language Codes.
  // 5:0 - number of data bytes
  TYPE_LEN b_manufacturer_tl;
  BYTE *b_manufacturer;
  
  TYPE_LEN b_product_tl;
  BYTE *b_product;
  
  TYPE_LEN b_serial_tl;
  BYTE *b_serial;
  
  TYPE_LEN b_part_tl;
  BYTE *b_part;
  
  TYPE_LEN b_fru_file_tl;
  BYTE *b_fru_file;
  
  // Additional custom Mfg. Info fields. Defined by manufacturing.
  // Each field must be preceded by a type/length byte
  BYTE *custom_info[8];
  
  // C1h (type/length byte encoded to indicate no more info fields)
  BYTE no_more_info;
  
  // 00h - any remaining unused space
  unsigned int zero_append_count;
  
  // Board Area Checksum (zero checksum)
  BYTE b_checksum;
};
typedef struct board_information board_info;


struct write_fru_data_command
{
  // FRU Device ID.
  BYTE fru_id;
  
  // FRU Inventory Offset to write, LS Byte
  BYTE fru_ls_offset;
  
  // FRU Inventory Offset to write, MS Byte
  BYTE fru_ms_offset;
   
};
typedef struct write_fru_data_command write_fru_cmd;


struct common_header_format
{
  // Common Header Format Version
  BYTE header_format_version;
  
  // Internal Use Area Starting Offset (in multiples of 8 bytes)
  BYTE internal_use_offset;
  
  // Chassis Info Area Starting Offset (in multiples of 8 bytes)
  BYTE chasis_info_offset;
  
  // Board Area Starting Offset (in multiples of 8 bytes)
  BYTE board_info_offset;
  
  // Product Info Area Offset (in multiples of 8 bytes)
  BYTE product_info_offset;
  
  // MultiRecord Area Offset (in multiples of 8 bytes)
  BYTE multi_record_offset;
  
  // PAD, write as 00h
  BYTE pad;
  
  // Common Header Checksum (zero checksum)
  BYTE header_checksum;
};
typedef struct common_header_format common_header;

struct ipmi_cmd_data
{
  unsigned int *cmd_len;
  BYTE *fru_id;
  BYTE **board_info_data;
};
typedef struct ipmi_cmd_data CMD_DATA;