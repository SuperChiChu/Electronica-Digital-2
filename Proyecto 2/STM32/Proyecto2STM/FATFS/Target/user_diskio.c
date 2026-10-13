/* USER CODE BEGIN Header */
/**
 ******************************************************************************
  * @file    user_diskio.c
  * @brief   This file includes a diskio driver skeleton to be completed by the user.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
 /* USER CODE END Header */

#ifdef USE_OBSOLETE_USER_CODE_SECTION_0
/*
 * Warning: the user section 0 is no more in use (starting from CubeMx version 4.16.0)
 * To be suppressed in the future.
 * Kept to ensure backward compatibility with previous CubeMx versions when
 * migrating projects.
 * User code previously added there should be copied in the new user sections before
 * the section contents can be deleted.
 */
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */
#endif

/* USER CODE BEGIN DECL */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "ff_gen_drv.h"
#include "main.h"

/*
 * SPI1 ya existe en main.c.
 */
extern SPI_HandleTypeDef hspi1;


/* Card types */
#define CT_MMC      0x01
#define CT_SD1      0x02
#define CT_SD2      0x04
#define CT_BLOCK    0x08


/* SD commands */
#define CMD0        0
#define CMD1        1
#define CMD8        8
#define CMD9        9
#define CMD16       16
#define CMD17       17
#define CMD24       24
#define CMD55       55
#define CMD58       58
#define ACMD41      (0x80 + 41)


/* Disk status */
static volatile DSTATUS Stat = STA_NOINIT;

static uint8_t CardType = 0;


/*
 * =========================================================
 * SPI helpers
 * =========================================================
 */

static uint8_t SD_SPI_TxRx(uint8_t data)
{
    uint8_t received = 0xFF;

    HAL_SPI_TransmitReceive(
        &hspi1,
        &data,
        &received,
        1,
        100
    );

    return received;
}


static void SD_SPI_SetPrescaler(uint32_t prescaler)
{
    /*
     * Esperar a que SPI termine cualquier transmisión.
     */
    while (__HAL_SPI_GET_FLAG(
        &hspi1,
        SPI_FLAG_BSY))
    {
    }

    __HAL_SPI_DISABLE(&hspi1);

    MODIFY_REG(
        hspi1.Instance->CR1,
        SPI_CR1_BR,
        prescaler
    );

    hspi1.Init.BaudRatePrescaler =
        prescaler;

    __HAL_SPI_ENABLE(&hspi1);
}


static void SD_SPI_Slow(void)
{
    /*
     * LCD deseleccionado.
     * SD deseleccionada.
     */
    HAL_GPIO_WritePin(
        LCD_CS_GPIO_Port,
        LCD_CS_Pin,
        GPIO_PIN_SET
    );

    HAL_GPIO_WritePin(
        SD_CS_GPIO_Port,
        SD_CS_Pin,
        GPIO_PIN_SET
    );

    /*
     * 84 MHz / 256
     * ≈ 328 kHz.
     *
     * Necesario durante inicialización SD.
     */
    SD_SPI_SetPrescaler(
        SPI_BAUDRATEPRESCALER_256
    );
}


static void SD_SPI_Fast(void)
{
    HAL_GPIO_WritePin(
        LCD_CS_GPIO_Port,
        LCD_CS_Pin,
        GPIO_PIN_SET
    );

    HAL_GPIO_WritePin(
        SD_CS_GPIO_Port,
        SD_CS_Pin,
        GPIO_PIN_SET
    );

    /*
     * Velocidad conservadora para SD.
     * 84 MHz / 8 ≈ 10.5 MHz.
     */
    SD_SPI_SetPrescaler(
        SPI_BAUDRATEPRESCALER_8
    );
}


static void SD_SPI_RestoreLCD(void)
{
    HAL_GPIO_WritePin(
        SD_CS_GPIO_Port,
        SD_CS_Pin,
        GPIO_PIN_SET
    );

    /*
     * Restaurar exactamente la velocidad
     * que ya utilizamos para la ILI9341.
     */
    SD_SPI_SetPrescaler(
        SPI_BAUDRATEPRESCALER_2
    );
}


/*
 * =========================================================
 * SD helpers
 * =========================================================
 */

static void SD_Deselect(void)
{
    HAL_GPIO_WritePin(
        SD_CS_GPIO_Port,
        SD_CS_Pin,
        GPIO_PIN_SET
    );

    /*
     * Clock extra después de liberar CS.
     */
    SD_SPI_TxRx(0xFF);
}


static uint8_t SD_WaitReady(
    uint32_t timeout)
{
    uint32_t start =
        HAL_GetTick();

    do
    {
        if (SD_SPI_TxRx(0xFF)
            == 0xFF)
        {
            return 1;
        }

    } while (
        (HAL_GetTick() - start)
        < timeout
    );

    return 0;
}


static uint8_t SD_Select(void)
{
    HAL_GPIO_WritePin(
        SD_CS_GPIO_Port,
        SD_CS_Pin,
        GPIO_PIN_RESET
    );

    SD_SPI_TxRx(0xFF);

    if (SD_WaitReady(500))
    {
        return 1;
    }

    SD_Deselect();

    return 0;
}


static uint8_t SD_SendCommand(
    uint8_t cmd,
    uint32_t arg)
{
    uint8_t crc;
    uint8_t response;
    uint8_t packet[6];


    /*
     * ACMD = CMD55 + comando.
     */
    if (cmd & 0x80)
    {
        cmd &= 0x7F;

        response =
            SD_SendCommand(
                CMD55,
                0
            );

        if (response > 1)
        {
            return response;
        }
    }


    SD_Deselect();

    if (!SD_Select())
    {
        return 0xFF;
    }


    packet[0] =
        0x40 | cmd;

    packet[1] =
        (uint8_t)(arg >> 24);

    packet[2] =
        (uint8_t)(arg >> 16);

    packet[3] =
        (uint8_t)(arg >> 8);

    packet[4] =
        (uint8_t)arg;


    /*
     * CRC solamente obligatorio
     * durante CMD0 y CMD8.
     */
    crc = 0x01;

    if (cmd == CMD0)
    {
        crc = 0x95;
    }

    if (cmd == CMD8)
    {
        crc = 0x87;
    }

    packet[5] = crc;


    for (int i = 0; i < 6; i++)
    {
        SD_SPI_TxRx(packet[i]);
    }


    /*
     * Esperar respuesta R1.
     */
    for (int i = 0;
         i < 10;
         i++)
    {
        response =
            SD_SPI_TxRx(0xFF);

        if (!(response & 0x80))
        {
            return response;
        }
    }


    return 0xFF;
}


static uint8_t SD_ReceiveDataBlock(
    uint8_t *buff,
    uint32_t length)
{
    uint8_t token;
    uint32_t start =
        HAL_GetTick();


    /*
     * Esperar token 0xFE.
     */
    do
    {
        token =
            SD_SPI_TxRx(0xFF);

        if (token == 0xFE)
        {
            break;
        }

    } while (
        (HAL_GetTick() - start)
        < 200
    );


    if (token != 0xFE)
    {
        return 0;
    }


    for (uint32_t i = 0;
         i < length;
         i++)
    {
        buff[i] =
            SD_SPI_TxRx(0xFF);
    }


    /*
     * Descartar CRC.
     */
    SD_SPI_TxRx(0xFF);
    SD_SPI_TxRx(0xFF);


    return 1;
}


static uint8_t SD_SendDataBlock(
    const uint8_t *buff)
{
    uint8_t response;


    if (!SD_WaitReady(500))
    {
        return 0;
    }


    /*
     * Token de bloque único.
     */
    SD_SPI_TxRx(0xFE);


    for (int i = 0;
         i < 512;
         i++)
    {
        SD_SPI_TxRx(buff[i]);
    }


    /*
     * CRC dummy.
     */
    SD_SPI_TxRx(0xFF);
    SD_SPI_TxRx(0xFF);


    response =
        SD_SPI_TxRx(0xFF);


    /*
     * 0x05 = Data Accepted.
     */
    if ((response & 0x1F)
        != 0x05)
    {
        return 0;
    }


    if (!SD_WaitReady(500))
    {
        return 0;
    }


    return 1;
}

/* USER CODE END DECL */

/* Private function prototypes -----------------------------------------------*/
DSTATUS USER_initialize (BYTE pdrv);
DSTATUS USER_status (BYTE pdrv);
DRESULT USER_read (BYTE pdrv, BYTE *buff, DWORD sector, UINT count);
#if _USE_WRITE == 1
  DRESULT USER_write (BYTE pdrv, const BYTE *buff, DWORD sector, UINT count);
#endif /* _USE_WRITE == 1 */
#if _USE_IOCTL == 1
  DRESULT USER_ioctl (BYTE pdrv, BYTE cmd, void *buff);
#endif /* _USE_IOCTL == 1 */

Diskio_drvTypeDef  USER_Driver =
{
  USER_initialize,
  USER_status,
  USER_read,
#if  _USE_WRITE
  USER_write,
#endif  /* _USE_WRITE == 1 */
#if  _USE_IOCTL == 1
  USER_ioctl,
#endif /* _USE_IOCTL == 1 */
};

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Initializes a Drive
  * @param  pdrv: Physical drive number (0..)
  * @retval DSTATUS: Operation status
  */
DSTATUS USER_initialize (
	BYTE pdrv           /* Physical drive nmuber to identify the drive */
)
{
  /* USER CODE BEGIN INIT */

	uint8_t type = 0;
	uint8_t response;
	uint8_t ocr[4];

	uint32_t start;


	/*
	 * Solo tenemos drive 0.
	 */
	if (pdrv != 0)
	{
	    return STA_NOINIT;
	}


	/*
	 * SPI lento durante inicialización.
	 */
	SD_SPI_Slow();


	/*
	 * SD debe estar deseleccionada
	 * mientras recibe mínimo 74 clocks.
	 */
	HAL_GPIO_WritePin(
	    SD_CS_GPIO_Port,
	    SD_CS_Pin,
	    GPIO_PIN_SET
	);

	for (int i = 0; i < 10; i++)
	{
	    SD_SPI_TxRx(0xFF);
	}


	/*
	 * CMD0 -> Idle state
	 */
	if (SD_SendCommand(
	        CMD0,
	        0) == 1)
	{
	    /*
	     * SD versión 2?
	     */
	    if (SD_SendCommand(
	            CMD8,
	            0x1AA) == 1)
	    {
	        for (int i = 0;
	             i < 4;
	             i++)
	        {
	            ocr[i] =
	                SD_SPI_TxRx(0xFF);
	        }


	        if (ocr[2] == 0x01 &&
	            ocr[3] == 0xAA)
	        {
	            start =
	                HAL_GetTick();

	            do
	            {
	                response =
	                    SD_SendCommand(
	                        ACMD41,
	                        1UL << 30
	                    );

	                if (response == 0)
	                {
	                    break;
	                }

	            } while (
	                (HAL_GetTick()
	                 - start)
	                < 1500
	            );


	            if (response == 0 &&
	                SD_SendCommand(
	                    CMD58,
	                    0) == 0)
	            {
	                for (int i = 0;
	                     i < 4;
	                     i++)
	                {
	                    ocr[i] =
	                        SD_SPI_TxRx(
	                            0xFF
	                        );
	                }


	                type =
	                    CT_SD2;

	                if (ocr[0] & 0x40)
	                {
	                    type |=
	                        CT_BLOCK;
	                }
	            }
	        }
	    }

	    /*
	     * SD versión 1 / MMC
	     */
	    else
	    {
	        uint8_t command;


	        if (SD_SendCommand(
	                ACMD41,
	                0) <= 1)
	        {
	            type =
	                CT_SD1;

	            command =
	                ACMD41;
	        }
	        else
	        {
	            type =
	                CT_MMC;

	            command =
	                CMD1;
	        }


	        start =
	            HAL_GetTick();

	        do
	        {
	            response =
	                SD_SendCommand(
	                    command,
	                    0
	                );

	            if (response == 0)
	            {
	                break;
	            }

	        } while (
	            (HAL_GetTick() -
	             start)
	            < 1500
	        );


	        if (response != 0 ||
	            SD_SendCommand(
	                CMD16,
	                512) != 0)
	        {
	            type = 0;
	        }
	    }
	}


	CardType = type;

	SD_Deselect();

	SD_SPI_RestoreLCD();


	if (type)
	{
	    Stat &= ~STA_NOINIT;
	}
	else
	{
	    Stat = STA_NOINIT;
	}


	return Stat;

  /* USER CODE END INIT */
}

/**
  * @brief  Gets Disk Status
  * @param  pdrv: Physical drive number (0..)
  * @retval DSTATUS: Operation status
  */
DSTATUS USER_status (
	BYTE pdrv       /* Physical drive number to identify the drive */
)
{
  /* USER CODE BEGIN STATUS */

	if (pdrv != 0)
	{
	    return STA_NOINIT;
	}

	return Stat;

  /* USER CODE END STATUS */
}

/**
  * @brief  Reads Sector(s)
  * @param  pdrv: Physical drive number (0..)
  * @param  *buff: Data buffer to store read data
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to read (1..128)
  * @retval DRESULT: Operation result
  */
DRESULT USER_read (
	BYTE pdrv,      /* Physical drive nmuber to identify the drive */
	BYTE *buff,     /* Data buffer to store read data */
	DWORD sector,   /* Sector address in LBA */
	UINT count      /* Number of sectors to read */
)
{
  /* USER CODE BEGIN READ */

	if (pdrv != 0 ||
	    count == 0)
	{
	    return RES_PARERR;
	}


	if (Stat & STA_NOINIT)
	{
	    return RES_NOTRDY;
	}


	SD_SPI_Fast();


	if (!(CardType & CT_BLOCK))
	{
	    sector *= 512;
	}


	UINT remaining = count;


	while (remaining)
	{
	    if (SD_SendCommand(
	            CMD17,
	            sector) != 0)
	    {
	        break;
	    }


	    if (!SD_ReceiveDataBlock(
	            buff,
	            512))
	    {
	        break;
	    }


	    buff += 512;


	    if (CardType & CT_BLOCK)
	    {
	        sector++;
	    }
	    else
	    {
	        sector += 512;
	    }


	    remaining--;
	}


	SD_Deselect();

	SD_SPI_RestoreLCD();


	if (remaining == 0)
	{
	    return RES_OK;
	}


	return RES_ERROR;

  /* USER CODE END READ */
}

/**
  * @brief  Writes Sector(s)
  * @param  pdrv: Physical drive number (0..)
  * @param  *buff: Data to be written
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to write (1..128)
  * @retval DRESULT: Operation result
  */
#if _USE_WRITE == 1
DRESULT USER_write (
	BYTE pdrv,          /* Physical drive nmuber to identify the drive */
	const BYTE *buff,   /* Data to be written */
	DWORD sector,       /* Sector address in LBA */
	UINT count          /* Number of sectors to write */
)
{
  /* USER CODE BEGIN WRITE */

	if (pdrv != 0 ||
	    count == 0)
	{
	    return RES_PARERR;
	}


	if (Stat & STA_NOINIT)
	{
	    return RES_NOTRDY;
	}


	SD_SPI_Fast();


	if (!(CardType & CT_BLOCK))
	{
	    sector *= 512;
	}


	UINT remaining = count;


	while (remaining)
	{
	    if (SD_SendCommand(
	            CMD24,
	            sector) != 0)
	    {
	        break;
	    }


	    if (!SD_SendDataBlock(buff))
	    {
	        break;
	    }


	    buff += 512;


	    if (CardType & CT_BLOCK)
	    {
	        sector++;
	    }
	    else
	    {
	        sector += 512;
	    }


	    remaining--;
	}


	SD_Deselect();

	SD_SPI_RestoreLCD();


	if (remaining == 0)
	{
	    return RES_OK;
	}


	return RES_ERROR;

  /* USER CODE END WRITE */
}
#endif /* _USE_WRITE == 1 */

/**
  * @brief  I/O control operation
  * @param  pdrv: Physical drive number (0..)
  * @param  cmd: Control code
  * @param  *buff: Buffer to send/receive control data
  * @retval DRESULT: Operation result
  */
#if _USE_IOCTL == 1
DRESULT USER_ioctl (
	BYTE pdrv,      /* Physical drive nmuber (0..) */
	BYTE cmd,       /* Control code */
	void *buff      /* Buffer to send/receive control data */
)
{
  /* USER CODE BEGIN IOCTL */

	DRESULT res =
	    RES_ERROR;


	if (pdrv != 0)
	{
	    return RES_PARERR;
	}


	if (Stat & STA_NOINIT)
	{
	    return RES_NOTRDY;
	}


	SD_SPI_Fast();


	switch (cmd)
	{
	    case CTRL_SYNC:
	    {
	        if (SD_Select())
	        {
	            res = RES_OK;
	        }

	        break;
	    }


	    case GET_SECTOR_SIZE:
	    {
	        *(WORD *)buff =
	            512;

	        res =
	            RES_OK;

	        break;
	    }


	    case GET_BLOCK_SIZE:
	    {
	        /*
	         * Para nuestro uso simple
	         * basta una unidad de sector.
	         */
	        *(DWORD *)buff =
	            1;

	        res =
	            RES_OK;

	        break;
	    }


	    case GET_SECTOR_COUNT:
	    {
	        uint8_t csd[16];


	        if (SD_SendCommand(
	                CMD9,
	                0) == 0 &&
	            SD_ReceiveDataBlock(
	                csd,
	                16))
	        {
	            if ((csd[0] >> 6)
	                == 1)
	            {
	                /*
	                 * CSD v2 - SDHC/SDXC
	                 */
	                uint32_t csize =
	                    ((uint32_t)
	                     (csd[7] &
	                      0x3F)
	                     << 16) |

	                    ((uint32_t)
	                     csd[8]
	                     << 8) |

	                    csd[9];


	                *(DWORD *)buff =
	                    (csize + 1)
	                    << 10;
	            }
	            else
	            {
	                /*
	                 * CSD v1
	                 */
	                uint32_t n;

	                uint32_t csize;


	                n =
	                    (csd[5] &
	                     15) +

	                    ((csd[10] &
	                      128) >> 7) +

	                    ((csd[9] &
	                      3) << 1) +

	                    2;


	                csize =
	                    (csd[8] >> 6) +

	                    ((uint32_t)
	                     csd[7] << 2) +

	                    ((uint32_t)
	                     (csd[6] & 3)
	                     << 10) +

	                    1;


	                *(DWORD *)buff =
	                    csize <<
	                    (n - 9);
	            }


	            res =
	                RES_OK;
	        }

	        break;
	    }


	    default:
	    {
	        res =
	            RES_PARERR;

	        break;
	    }
	}


	SD_Deselect();

	SD_SPI_RestoreLCD();


	return res;

  /* USER CODE END IOCTL */
}
#endif /* _USE_IOCTL == 1 */

