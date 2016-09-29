#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sendfru.h"

void Send_fru_data( CMD_DATA *cmd_data, int d_count)
{
  int i, j;
  char *cmd_string;
  
  
  for( i = 0 ; i < d_count ; i++ )
  {
    // setup write fru command
    write_fru_cmd fru_cmd;
    fru_cmd.fru_id = (*cmd_data).fru_id[i];
    fru_cmd.fru_ls_offset = 0;    
    fru_cmd.fru_ms_offset = 0;
  
    // 5 bytes for NetFn, CMD, FRU ID, FRU LS offset and FRU MS offset
    // the rest of other are common header and data payload
    if( (*cmd_data).cmd_len[i] + 5 >= IPMI_MAX_REQ_SIZE )
    {
      int j, k;
      unsigned int cmd_send_count = 0;
      int ipmi_cmd_count ;
      
      // calculate how many ipmi command needed
      if((*cmd_data).cmd_len[i] % (IPMI_MAX_REQ_SIZE -5)  == 0)
        ipmi_cmd_count = (*cmd_data).cmd_len[i]/(IPMI_MAX_REQ_SIZE -5);
      else
        ipmi_cmd_count = (*cmd_data).cmd_len[i]/(IPMI_MAX_REQ_SIZE -5) +1;
      
      
      for( j = 0; j < ipmi_cmd_count; j++)
      {
        int data_remaining = (*cmd_data).cmd_len[i] - cmd_send_count;
        fru_cmd.fru_ls_offset = cmd_send_count & 0xff;
        fru_cmd.fru_ms_offset = (cmd_send_count >> 8) & 0xff;
       
        cmd_string = malloc( 38 + MIN(data_remaining,(IPMI_MAX_REQ_SIZE -5)) * 5 + 1 );
        
        // ipmi write fru NET FN, CMD, FRU ID, OFFSETs
        sprintf( cmd_string, "ipmitool raw 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x", \
                 IPMI_WR_FRU_FN, \
                 IPMI_WR_FRU_CMD, \
                 fru_cmd.fru_id, \
                 fru_cmd.fru_ls_offset, \
                 fru_cmd.fru_ms_offset);
                 
        // append ipmi data payload ( totally not exceed 60 data )
        while( cmd_send_count < (*cmd_data).cmd_len[i] )
        {
          sprintf( cmd_string, "%s 0x%02x", \
                   cmd_string, \
                   (*cmd_data).board_info_data[i][cmd_send_count]);
          cmd_send_count++;
          
          // terminate this ipmi raw command since we have 60 args
          if(cmd_send_count % (IPMI_MAX_REQ_SIZE -5)  == 0)
            break;
        }
        
#ifdef DEBUG
        printf("%d\n",cmd_send_count);
        printf("IPMI CMD %d: \n%s\n", j, cmd_string);
#endif

        int res = system(cmd_string);
        fprintf( stdout, "IPMI command execution returnned %d.\n\n\n", res);    
        free(cmd_string);
      }
    }
    else 
    {
      cmd_string = malloc( 38 + ((*cmd_data).cmd_len[i] * 5) + 1  );
      sprintf( cmd_string, "ipmitool raw 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x", \
                IPMI_WR_FRU_FN, \
                IPMI_WR_FRU_CMD, \
                fru_cmd.fru_id, \
                fru_cmd.fru_ls_offset, \
                fru_cmd.fru_ms_offset);
      
      for( j = 0 ; j < (*cmd_data).cmd_len[i] ; j++)
      {        
        sprintf( cmd_string, "%s 0x%02x", cmd_string, (*cmd_data).board_info_data[i][j]);
      }
      
#ifdef DEBUG
      printf("IPMI CMD: \n%s\n", cmd_string);
#endif

      int res = system(cmd_string);
      fprintf( stdout, "IPMI command execution returnned %d.\n\n\n", res);    
      free( cmd_string );
    }
    
  }
}

void Collect_data( CMD_DATA *cmd_data, board_info *pcie_board_info, int d_count)
{
  int i, j;
  int p_domain = 0; 
  int p_data;
  int slot_fru_id[] = SLOT_MAP_TO_FRUID;
  
  
  (*cmd_data).board_info_data = (BYTE**)malloc( d_count );
  (*cmd_data).fru_id = (BYTE*)malloc( d_count );
  (*cmd_data).cmd_len = (int*)malloc( d_count );
  
  for( i = 0 ; i < DOMAIN_MAP_TO_SLOT_COUNT ; i++ )
  {
    unsigned int headers_byte_count;
    unsigned int data_byte_count;
    
    // Board Area Format Version 
    if( pcie_board_info[i].b_area_format_version != 0 )
    {
      headers_byte_count = sizeof(common_header);
      data_byte_count = pcie_board_info[i].b_area_length * 8;
      
      (*cmd_data).board_info_data[p_domain] = (BYTE*) malloc( headers_byte_count + data_byte_count);
      (*cmd_data).fru_id[p_domain] = slot_fru_id[i];
      (*cmd_data).cmd_len[p_domain] =  headers_byte_count + data_byte_count;

#ifdef DEBUG
      printf("header + data length = %d\n", headers_byte_count + data_byte_count);
#endif
      
      memset( (*cmd_data).board_info_data[p_domain], 0, headers_byte_count + data_byte_count);
      
      
      // setup common header format
      common_header com_header;
      com_header.header_format_version = 0x01;
      com_header.internal_use_offset = 0;
      com_header.chasis_info_offset = 0;
      com_header.board_info_offset = 0x01;
      com_header.product_info_offset = 0;
      com_header.multi_record_offset = 0;
      com_header.pad = 0;
      com_header.header_checksum = 0xfe;
      
      // write data to array
      p_data = 0;
      memcpy( (*cmd_data).board_info_data[p_domain] + p_data, &com_header, sizeof(common_header));
      p_data += sizeof(common_header);
      
      
      // copy Board Area Format Version to array
      memcpy( (*cmd_data).board_info_data[p_domain] + p_data, \
              &pcie_board_info[i].b_area_format_version, \
              sizeof(BYTE));
      p_data += sizeof(BYTE);
      
      
    }
    else
      continue;
    
    
    // Board Area Length 
    memcpy( (*cmd_data).board_info_data[p_domain] + p_data, \
            &pcie_board_info[i].b_area_length, \
            sizeof(BYTE));
    p_data += sizeof(BYTE);
    
    
    // Language Code
    memcpy( (*cmd_data).board_info_data[p_domain] + p_data, \
            &pcie_board_info[i].b_lang_code, \
            sizeof(BYTE));
    p_data += sizeof(BYTE);
    
    
    // Mfg. Date/Time
    memcpy( (*cmd_data).board_info_data[p_domain] + p_data, \
            pcie_board_info[i].mfg_date_time, \
            sizeof(BYTE)*3);
    p_data += (sizeof(BYTE)*3);
    
    
    // Board Manufacturer    
    memcpy( (*cmd_data).board_info_data[p_domain] + p_data, \
            &pcie_board_info[i].b_manufacturer_tl, \
            sizeof(BYTE));
    p_data += sizeof(BYTE);
    
    if( pcie_board_info[i].b_manufacturer_tl.byte != 0 )
    {  
      memcpy( (*cmd_data).board_info_data[p_domain] + p_data, \
              pcie_board_info[i].b_manufacturer, \
              pcie_board_info[i].b_manufacturer_tl.bits.n_bytes);
      p_data += pcie_board_info[i].b_manufacturer_tl.bits.n_bytes;
    }
    
    
    // Board Product Name
    memcpy( (*cmd_data).board_info_data[p_domain] + p_data, \
            &pcie_board_info[i].b_product_tl, \
            sizeof(BYTE));
    p_data += sizeof(BYTE);
    
    if( pcie_board_info[i].b_product_tl.byte != 0)
    {  
      memcpy( (*cmd_data).board_info_data[p_domain] + p_data, \
              pcie_board_info[i].b_product, \
              pcie_board_info[i].b_product_tl.bits.n_bytes);
      p_data += pcie_board_info[i].b_product_tl.bits.n_bytes;
    }
    
    
    // Board Serial Number 
    memcpy( (*cmd_data).board_info_data[p_domain] + p_data, \
            &pcie_board_info[i].b_serial_tl, \
            sizeof(BYTE));
    p_data += sizeof(BYTE);
    
    if( pcie_board_info[i].b_serial_tl.byte != 0)
    {  
      memcpy( (*cmd_data).board_info_data[p_domain] + p_data, \
              pcie_board_info[i].b_serial, \
              pcie_board_info[i].b_serial_tl.bits.n_bytes);
      p_data += pcie_board_info[i].b_serial_tl.bits.n_bytes;
    }
    
    
    // Board Part Number
    memcpy( (*cmd_data).board_info_data[p_domain] + p_data, \
            &pcie_board_info[i].b_part_tl, \
            sizeof(BYTE));
    p_data += sizeof(BYTE);
    
    if( pcie_board_info[i].b_part_tl.byte != 0)
    {  
      memcpy( (*cmd_data).board_info_data[p_domain] + p_data, \
              pcie_board_info[i].b_part, \
              pcie_board_info[i].b_part_tl.bits.n_bytes);
      p_data += pcie_board_info[i].b_part_tl.bits.n_bytes;
    }
    
    
    // Board FRU File ID 
    memcpy( (*cmd_data).board_info_data[p_domain] + p_data, \
            &pcie_board_info[i].b_fru_file_tl, \
            sizeof(BYTE));
    p_data += sizeof(BYTE);
    
    if( pcie_board_info[i].b_fru_file_tl.byte != 0)
    {
      memcpy( (*cmd_data).board_info_data[p_domain] + p_data, \
              pcie_board_info[i].b_fru_file, \
              pcie_board_info[i].b_fru_file_tl.bits.n_bytes);
      p_data += pcie_board_info[i].b_fru_file_tl.bits.n_bytes;
    }
    
    
    // Custom Mfg. info fields
    for( j = 0 ; j < 8 ; j++ )
    {
      if( pcie_board_info[i].custom_info[j] != 0)
      {
        unsigned int n_bytes = ((TYPE_LEN*)(pcie_board_info[i].custom_info[j]))->bits.n_bytes;
        memcpy( (*cmd_data).board_info_data[p_domain] + p_data, \
                pcie_board_info[i].custom_info[j], \
                1 + n_bytes);
        p_data += (1 + n_bytes);
      }
    }
    
    
    // no more info fields
    memcpy( (*cmd_data).board_info_data[p_domain] + p_data, \
            &pcie_board_info[i].no_more_info, \
            sizeof(BYTE));
    p_data += sizeof(BYTE);
    
    
    // zero append
    p_data += pcie_board_info[i].zero_append_count;
    
    
    // Board Area Checksum
    memcpy( (*cmd_data).board_info_data[p_domain] + p_data, \
            &pcie_board_info[i].b_checksum, \
            sizeof(BYTE));
    p_data += sizeof(BYTE);

#ifdef DEBUG    
    if(headers_byte_count + data_byte_count == p_data)
      printf("BYTE COUNT MATCH!! \n");
    else
      printf("BYTE COUNT = %d\n",p_data);
#endif
      
    p_domain ++;
  }
}

//
// Complete board information with format version, length,
// checksum, etc...
//
int Complete_structure( board_info *pcie_board_info)
{
  int i, j, k;
  unsigned int byte_count;
  unsigned int domain_count = 0;
  
  for( i = 0 ; i < DOMAIN_MAP_TO_SLOT_COUNT ; i++ )
  {
    int domains[] = DOMAIN_MAP_TO_SLOT;
    
    byte_count = 0;
    BYTE checksum = 0;
          
    // Board Area Format Version : 1 byte
    if( pcie_board_info[i].b_area_format_version == 0 )
    {
#ifdef DEBUG
      printf("Domain %d have no PCI-E device.\n", domains[i]);  
#endif
      continue;
    }
    else 
    {
      byte_count += 1;
      checksum += pcie_board_info[i].b_area_format_version;
#ifdef DEBUG
      printf("Domain %d have PCI-E device.\n", domains[i]);
#endif
    }
    
    // Board Area Length : 1 byte
    byte_count += 1;
    
    // Language Code : 1 byte
    pcie_board_info[i].b_lang_code = LANGUAGE_CODE;
    byte_count += 1;    
    checksum += pcie_board_info[i].b_lang_code;
    
    // Mfg. Date/Time : 3 bytes
    byte_count += 3;
    for( j = 0 ; j < 3 ; j++)
      checksum += pcie_board_info[i].mfg_date_time[j];
    
    // Board Manufacturer : 1+n bytes  
    byte_count += 1;
    if( pcie_board_info[i].b_manufacturer_tl.byte != 0 )
    {
      unsigned int n_bytes = pcie_board_info[i].b_manufacturer_tl.bits.n_bytes;

#ifdef DEBUG
      printf("Manufacturer information: %d bytes\n", n_bytes);
#endif
      
      byte_count += n_bytes;
      
      checksum += pcie_board_info[i].b_manufacturer_tl.byte;
      for( j = 0; j < n_bytes ; j++ )
        checksum += pcie_board_info[i].b_manufacturer[j];      
    }
    
    // Board Product Name : 1+n bytes
    byte_count += 1;
    if( pcie_board_info[i].b_product_tl.byte != 0)
    {
      unsigned int n_bytes = pcie_board_info[i].b_product_tl.bits.n_bytes;

#ifdef DEBUG
      printf("Product information: %d bytes\n", n_bytes);
#endif
      
      byte_count += n_bytes;
      
      checksum += pcie_board_info[i].b_product_tl.byte;
      for( j = 0; j < n_bytes ; j++ )
        checksum += pcie_board_info[i].b_product[j];
    }
    
    // Board Serial Number : 1+n bytes
    byte_count += 1;
    if( pcie_board_info[i].b_serial_tl.byte != 0)
    {
      unsigned int n_bytes = pcie_board_info[i].b_serial_tl.bits.n_bytes;

#ifdef DEBUG
      printf("Serial Number information: %d bytes\n", n_bytes);
#endif

      byte_count += n_bytes;
      
      checksum += pcie_board_info[i].b_serial_tl.byte;
      for( j = 0; j < n_bytes ; j++ )
        checksum += pcie_board_info[i].b_serial[j];
    }
    
    // Board Part Number : 1+n bytes
    byte_count += 1;
    if( pcie_board_info[i].b_part_tl.byte != 0)
    {
      unsigned int n_bytes = pcie_board_info[i].b_part_tl.bits.n_bytes;

#ifdef DEBUG
      printf("Part Number information: %d bytes\n", n_bytes);  
#endif
      
      byte_count += n_bytes;
      
      checksum += pcie_board_info[i].b_part_tl.byte;
      for( j = 0; j < n_bytes ; j++ )
        checksum += pcie_board_info[i].b_part[j];
    }
    
    // Board FRU File ID : 1+n bytes
    byte_count += 1;
    if( pcie_board_info[i].b_fru_file_tl.byte != 0)
    {
      unsigned int n_bytes = pcie_board_info[i].b_fru_file_tl.bits.n_bytes;

#ifdef DEBUG
      printf("Fru File ID information: %d bytes\n", n_bytes);
#endif
      
      byte_count += n_bytes;
      
      checksum += pcie_board_info[i].b_fru_file_tl.byte;
      for( j = 0; j < n_bytes ; j++ )
        checksum += pcie_board_info[i].b_fru_file[j];
    }
    
    // Custom Mfg. info fields: j*(1+n) bytes
    for( j = 0 ; j < 8 ; j++ )
    {
      if( pcie_board_info[i].custom_info[j] != 0)
      {
        unsigned int n_bytes = ((TYPE_LEN*)(pcie_board_info[i].custom_info[j]))->bits.n_bytes;

#ifdef DEBUG
        printf("Custom information field %d: %d byte\n", j, n_bytes);
#endif
        
        byte_count += 1;
        byte_count += n_bytes;
        
        for( k = 0 ; k < 1+n_bytes ; k++)
          checksum += pcie_board_info[i].custom_info[j][k];
      }
    }
    
    // no more info fields : 1 byte
    byte_count += 1;
    pcie_board_info[i].no_more_info = NO_MORE_FIELDS_BYTE;
    checksum += pcie_board_info[i].no_more_info;
    
    // Board Area Checksum : 1 byte
    byte_count += 1;
    
#ifdef DEBUG
    printf("total %d bytes\n",byte_count);
#endif
    
    // calculate and fill in data length
#ifdef DEBUG
    printf("Board Area Length (Before append): %d\n", byte_count/8);
#endif

    if( byte_count % 8 != 0)
    {
      unsigned int append_byte_count = 8 - (byte_count % 8);
      pcie_board_info[i].zero_append_count = append_byte_count;
      byte_count += append_byte_count;
    }
    pcie_board_info[i].b_area_length = byte_count/8 ;

#ifdef DEBUG
    printf("Board Area Length : %d\n", pcie_board_info[i].b_area_length);
#endif
    
    // fill in Board Area Checksum
    checksum += pcie_board_info[i].b_area_length;
    pcie_board_info[i].b_checksum = -checksum;

#ifdef DEBUG
    printf("checksum : %x\n\n", pcie_board_info[i].b_checksum);    
#endif

    domain_count ++ ;
  }
  
  return domain_count;
}


//
// Parse lspci output and fill fru field of board information 
// 
int PCIE_parser(board_info *pcie_board_info)
{
  FILE *fp;
  int i, rd;
  int domains[] = DOMAIN_MAP_TO_SLOT;
  char filename[32], line[256];
  char *str_token;
  char *q1, *q2;
  unsigned int len;
  
  for( i = 0 ; i < DOMAIN_MAP_TO_SLOT_COUNT ; i++ )
  {
    sprintf(filename, "/tmp/domain%d.txt", domains[i]);
    fp = fopen(filename,"r");
    if( fp == NULL )
    {
      fprintf( stdout, "Fail to open /tmp/domain%d.\n", domains[i]);
      continue;
    }
    
    // first line skipped, i.e. skip host birdge
    fgets( line, 256, fp );   
    
    // only read second line if exist
    if( fgets( line, 256, fp) != NULL )  
    {
      printf("%s",line);
      
      // found pci device on slot, fill in board area
      // format versions marked 
      pcie_board_info[i].b_area_format_version = \
      FORMAT_VERSION_NUMBER & FORMAT_VERSION_NUMBER_MASK;
      
      //
      // first one pair of quotation mark indicate Class
      //
      q1 = strchr( line , '\"');
      q2 = strchr( q1+1 , '\"');
      *q2 = '\0';
      
      
      //
      // second one pair of quotation mark indicate Vendor
      //
      q1 = strchr( q2+1 , '\"');
      q2 = strchr( q1+1 , '\"');
      *q2 = '\0';
      
      // count string length
      len = (unsigned)strlen(q1+1);     
      
      // truncate data to MAX_DATA_LENGTH if exceed
      if( len > MAX_DATA_LENGTH )   
        len = MAX_DATA_LENGTH;
      
      pcie_board_info[i].b_manufacturer_tl.bits.t_code = DEFAULT_TYPE_CODE;
      pcie_board_info[i].b_manufacturer_tl.bits.n_bytes = len & MAX_DATA_LENGTH;
      
      pcie_board_info[i].b_manufacturer = (BYTE*) malloc( len );
      if(pcie_board_info[i].b_manufacturer == NULL)
      {
        printf("Error! Cannot allocate memory.\n");
        return -1;
      }
      
      // copy string without null-character
      strncpy( pcie_board_info[i].b_manufacturer, q1+1, len);  
      
      
      
      //
      // third one pair of quotation mark indicate Device
      //
      q1 = strchr( q2+1 , '\"');
      q2 = strchr( q1+1 , '\"');
      *q2 = '\0';
      
      // count string length
      len = (unsigned)strlen(q1+1);     
      
      // truncate data to MAX_DATA_LENGTH if exceed
      if( len > MAX_DATA_LENGTH )   
        len = MAX_DATA_LENGTH;
      
      pcie_board_info[i].b_product_tl.bits.t_code = DEFAULT_TYPE_CODE;
      pcie_board_info[i].b_product_tl.bits.n_bytes = len & MAX_DATA_LENGTH;
      
      pcie_board_info[i].b_product = (BYTE*) malloc( len );
      if(pcie_board_info[i].b_product == NULL)
      {
        printf("Error! Cannot allocate memory.\n");
        return -1;
      }
      
      // copy string without null-character
      strncpy( pcie_board_info[i].b_product, q1+1, len);  
      
      
      //
      // fourth one pair of quotation mark indicate Subsystem Vendor
      //
      q1 = strchr( q2+1 , '\"');
      q2 = strchr( q1+1 , '\"');
      *q2 = '\0';

      
      //
      // fifth one pair of quotation mark indicate Subsystem
      //
      q1 = strchr( q2+1 , '\"');
      q2 = strchr( q1+1 , '\"');
      *q2 = '\0';

      
      
      //
      // Merge Manufacture and Product string to custom info field 0
      //
      
      len = pcie_board_info[i].b_manufacturer_tl.bits.n_bytes +  \
            pcie_board_info[i].b_product_tl.bits.n_bytes + 1;
            
      // truncate data to MAX_DATA_LENGTH if exceed
      if( len > MAX_DATA_LENGTH )   
        len = MAX_DATA_LENGTH;
        
      TYPE_LEN custom_field_1_tl;
      custom_field_1_tl.bits.t_code = DEFAULT_TYPE_CODE;
      custom_field_1_tl.bits.n_bytes = len & MAX_DATA_LENGTH;
      

      
      pcie_board_info[i].custom_info[0] = malloc( sizeof(TYPE_LEN) + len );
      if(pcie_board_info[i].custom_info[0] == NULL)
      {
        printf("Error! Cannot allocate memory.\n");
        return -1;
      }
      
      BYTE *write_indicator = pcie_board_info[i].custom_info[0];
      
      // copy type/len to custom info
      memcpy( write_indicator, &custom_field_1_tl, sizeof(TYPE_LEN));
      write_indicator += sizeof(TYPE_LEN);
      
      // copy string without null-character
      // first, copy vendor to custom info      
      strncpy( write_indicator, \
               pcie_board_info[i].b_manufacturer, \
               pcie_board_info[i].b_manufacturer_tl.bits.n_bytes);  
      write_indicator +=  pcie_board_info[i].b_manufacturer_tl.bits.n_bytes;
      
      // then, append white space
      *write_indicator = ' ';
      write_indicator += 1;
      
      // finally, copy device to custom info
      strncpy( write_indicator, \
               pcie_board_info[i].b_product, \
               pcie_board_info[i].b_product_tl.bits.n_bytes);  
      write_indicator +=  pcie_board_info[i].b_product_tl.bits.n_bytes;
      
#ifdef DEBUG
      char *dbg_string = malloc( len + 1 );
      strncpy( dbg_string, pcie_board_info[i].custom_info[0] + sizeof(TYPE_LEN), len);
      dbg_string[len] = '\0';
      printf("%s\n",dbg_string);
#endif
    }
    
    fclose(fp);
  }
  return 0;
}

int main(int argc, char *argv[])
{
  int res,i,j;
  char cmd[256];
  board_info pcie_board_info[DOMAIN_COUNT];
  CMD_DATA cmd_data;

  
  // clean all of pcie info strcuture
  memset(pcie_board_info, 0, sizeof(board_info) * DOMAIN_COUNT);
  
  // scan all domain for pci devices
  for( i = 0 ; i < DOMAIN_COUNT ; i++ )
  {
    sprintf( cmd, "lspci -s %d:*: -m > /tmp/domain%d.txt", i, i);
    res = system(cmd);
    fprintf( stdout, "domain%d execution returnned %d.\n", i, res);
  }
  
  res = PCIE_parser( pcie_board_info);
  if( res == -1 )
    return res;
  
  int d_count = Complete_structure( pcie_board_info);
  Collect_data( &cmd_data, pcie_board_info, d_count);
  
  Send_fru_data( &cmd_data, d_count);
  return res;
}
