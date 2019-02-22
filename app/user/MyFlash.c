#include "MyFlash.h"

struct BBA_FlashData flashData;

uint32 ICACHE_FLASH_ATTR ReadMyFlashData(struct BBA_FlashData *p)
{
    spi_flash_read(BBA_PARTITION_ADDR,(uint32 *)p,BBA_BBA_PARTITION_SIZE_BYTES);
    os_printf("Read BBA_data success\r\n");

    if(p->flag_init==1)
    {
        os_printf("device number:%s\r\n",p->deviceNumber);
        os_printf("wifi_ssid:%s\r\n",p->wifi_ssid);
        os_printf("wifi_password:%s\r\n",p->wifi_password);
        os_printf("function World:%s\r\n",p->functionState);
        os_printf("function data:%s\r\n",p->data);
        os_printf("flag:%d\r\n",p->flag_init);  
    }
    
    return 1;
}

uint32 ICACHE_FLASH_ATTR WriteMyFlashData(struct BBA_FlashData *p,uint32 len)
{
        spi_flash_erase_sector(250);	//先擦除某个扇区，再写(16个扇区为一块，即一块等于4K*16=64K) 1MB=256个扇区
		spi_flash_write(BBA_PARTITION_ADDR,(uint32 *)p,BBA_BBA_PARTITION_SIZE_BYTES);
        
        return 1;
}