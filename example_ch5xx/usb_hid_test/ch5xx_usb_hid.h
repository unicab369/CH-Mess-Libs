/* USB standard device request code */
#ifndef USB_GET_DESCRIPTOR
	#define USB_GET_STATUS          0x00
	#define USB_CLEAR_FEATURE       0x01
	#define USB_SET_FEATURE         0x03
	#define USB_SET_ADDRESS         0x05
	#define USB_GET_DESCRIPTOR      0x06
	#define USB_SET_DESCRIPTOR      0x07
	#define USB_GET_CONFIGURATION   0x08
	#define USB_SET_CONFIGURATION   0x09
	#define USB_GET_INTERFACE       0x0A
	#define USB_SET_INTERFACE       0x0B
	#define USB_SYNCH_FRAME         0x0C
#endif

/* Bit define for USB request type */
#ifndef USB_REQ_TYP_MASK
	#define USB_REQ_TYP_IN          0x80            /* control IN, device to host */
	#define USB_REQ_TYP_OUT         0x00            /* control OUT, host to device */
	#define USB_REQ_TYP_READ        0x80            /* control read, device to host */
	#define USB_REQ_TYP_WRITE       0x00            /* control write, host to device */
	#define USB_REQ_TYP_MASK        0x60            /* bit mask of request type */
	#define USB_REQ_TYP_STANDARD    0x00
	#define USB_REQ_TYP_CLASS       0x20
	#define USB_REQ_TYP_VENDOR      0x40
	#define USB_REQ_TYP_RESERVED    0x60
	#define USB_REQ_RECIP_MASK      0x1F            /* bit mask of request recipient */
	#define USB_REQ_RECIP_DEVICE    0x00
	#define USB_REQ_RECIP_INTERF    0x01
	#define USB_REQ_RECIP_ENDP      0x02
	#define USB_REQ_RECIP_OTHER     0x03
#endif

/* USB descriptor type */
#ifndef USB_DESCR_TYP_DEVICE
	#define USB_DESCR_TYP_DEVICE    0x01
	#define USB_DESCR_TYP_CONFIG    0x02
	#define USB_DESCR_TYP_STRING    0x03
	#define USB_DESCR_TYP_INTERF    0x04
	#define USB_DESCR_TYP_ENDP      0x05
	#define USB_DESCR_TYP_QUALIF    0x06
	#define USB_DESCR_TYP_SPEED     0x07
	#define USB_DESCR_TYP_OTG       0x09
	#define USB_DESCR_TYP_HID       0x21
	#define USB_DESCR_TYP_REPORT    0x22
	#define USB_DESCR_TYP_PHYSIC    0x23
	#define USB_DESCR_TYP_CS_INTF   0x24
	#define USB_DESCR_TYP_CS_ENDP   0x25
	#define USB_DESCR_TYP_HUB       0x29
#endif

#define DEF_USB_GET_IDLE           0x02			/* get idle for key or mouse */
#define DEF_USB_GET_PROTOCOL       0x03			/* get protocol for bios type */
#define DEF_USB_SET_REPORT         0x09			/* set report for key */
#define DEF_USB_SET_IDLE           0x0A			/* set idle for key or mouse */
#define DEF_USB_SET_PROTOCOL       0x0B			/* set protocol for bios type */

#define USB_INTERFACE_MAX_NUM 1
#define USB_INTERFACE_MAX_INDEX	0
#define DevEP0SIZE 64
#define DevEP1SIZE 64

typedef struct PACKED {
    u8 bRequestType;
    u8 bRequest;
    u16 wValue;
    u16 wIndex;
    u16 wLength;
} USB_SETUP_REQ;

uint8_t *pEP0_RAM_Addr;
uint8_t *pEP1_RAM_Addr;
uint8_t *pEP2_RAM_Addr;
uint8_t *pEP3_RAM_Addr;

#define pSetupReqPak          ((USB_SETUP_REQ*)pEP0_RAM_Addr)
#define pEP0_DataBuf          (pEP0_RAM_Addr)
#define pEP1_OUT_DataBuf      (pEP1_RAM_Addr)
#define pEP1_IN_DataBuf       (pEP1_RAM_Addr + 64)
#define pEP2_OUT_DataBuf      (pEP2_RAM_Addr)
#define pEP2_IN_DataBuf       (pEP2_RAM_Addr + 64)
#define pEP3_OUT_DataBuf      (pEP3_RAM_Addr)
#define pEP3_IN_DataBuf       (pEP3_RAM_Addr + 64)
#define pEP4_OUT_DataBuf      (pEP0_RAM_Addr + 64)
#define pEP4_IN_DataBuf       (pEP0_RAM_Addr + 128)

const uint8_t MyLangDescr[] = {0x04, 0x03, 0x09, 0x04};
const uint8_t MyManuInfo[] = {0x0E, 0x03, 'w', 0, 'c', 0, 'h', 0, '.', 0, 'c', 0, 'N', 0};
const uint8_t MyProdInfo[] = {0x0C, 0x03, 'C', 0, 'h', 0, '5', 0, '8', 0, 'x', 0};
uint8_t HID_Buf[] = {0,0,0,0};

const uint8_t *pDescr;
uint8_t		DevConfig, Ready = 0;
uint8_t		SetupReqCode;
uint16_t	SetupReqLen;
uint8_t		Report_Value[USB_INTERFACE_MAX_INDEX+1] = {0x00};
uint8_t		Idle_Value[USB_INTERFACE_MAX_INDEX+1] = {0x00};
uint8_t		USB_SleepStatus = 0x00;

__attribute__((aligned(4))) uint8_t EP0_Databuf[64 + 64 + 64];
__attribute__((aligned(4))) uint8_t EP1_Databuf[64 + 64];
__attribute__((aligned(4))) uint8_t EP2_Databuf[64 + 64];
__attribute__((aligned(4))) uint8_t EP3_Databuf[64 + 64];

const uint8_t MyDevDescr[] = {
	0x12, 0x01,				// bLength, bDescriptorType (Device Descriptor)
	0x10, 0x01,				// bcdUSB: USB specification version (1.10)
	0x00, 0x00, 0x00,		// bDevieClass, bDeviceSubClass, bDeviceProtocol
	DevEP0SIZE,				// bMaxPacketSize0: Max packet size for endpoint 0 (64 bytes)
	0x3d, 0x41,				// idVendor: 0x4131
	0x07, 0x21,				// idProduct: 0x2107
	0x00, 0x00,				// bcdDevice: Device release number (0.00)
	0x01,					// iManufacturer: Manufacturer string index (1)
	0x02,					// iProduct: Product string index (2)
	0x00,					// iSerialNumber: Serial number string index (0 = none)
	0x01					// bNumConfigurations: Number of configurations (1)
};

const uint8_t MyCfgDescr[] = {
	// Configuration descriptor:
	// [len, type (configuration), totalLenH, totalLenL, numInterfaces, configValue, 
	// 	configIndex, attributes (Bus Powered, Remote Wakeup), maxPower]
	0x09, 0x02, 0x29, 0x00, USB_INTERFACE_MAX_NUM, 0x01, 0x04, 0xA0 , 0x23,

	// Interface descriptor:
	// [len, type (interface), interfaceNumber, alternateSetting, numEndpoints (0x02), 
	// interfaceClass (HID), interfaceSubclass, interfaceProtocol, interfaceNameIndex]
	0x09, 0x04, 0x00, 0x00, 0x02, 0x03, 0x00, 0x00, 0x05,

	// HID class descriptor:
	// [len, type (HID), HIDreleaseH, HIDreleaseL, countryCode,
	// numHIDDescriptors, descriptorType, descriptorLenH, descriptorLenL]
	0x09, 0x21, 0x00, 0x01, 0x00, 0x01, 0x22, 0x22, 0x00,

	// Endpoint descriptor:
	// [len, type (endpoint), endpointAddress (EP1 IN), attributes (Interrupt Transfer), maxPacketSize, 
	// interval (ignore for full speed), refresh (1 uFrame)]
	0x07, 0x05, 0x81, 0x03, DevEP1SIZE, 0x00, 0x01,

	// Endpoint descriptor: EP1 OUT
	0x07, 0x05, 0x01, 0x03, DevEP1SIZE, 0x00, 0x01
};

const uint8_t HIDDescr[] = {  
	0x06, 0x00,0xff,		// Usage Page (0xFF00) - Vendor Defined
	0x09, 0x01,				// Usage (0x01) - Vendor Usage 1
	0xa1, 0x01,				// Collection (Application) - Start main collection

	// Input Report (64 bytes)
	0x09, 0x02,				// Usage (0x02) - Vendor Usage 2
	0x15, 0x00,				// Logical Minimum (0)
	0x26, 0x00,0xff,  		// Logical Maximum (255)
	0x75, 0x08,				// Report Size (8 bits per field)
	0x95, 0x40,				// Report Count (64 fields = 64 bytes)
	0x81, 0x06,				// Input (Data, Variable, Relative)
	
	// Output Report (64 bytes)
	0x09, 0x02,				// Usage (0x02) - Vendor Usage 2
	0x15, 0x00,				// Logical Minimum (0)
	0x26, 0x00,0xff,  		// Logical Maximum (255)
	0x75, 0x08,				// Report Size (8 bits per field)
	0x95, 0x40,				// Report Count (64 fields = 64 bytes)
	0x91, 0x06,				// Output (Data, Variable, Relative)

	// End Collection
	0xC0
};

void DevEP1_IN_Deal(uint8_t l) {
    R8_UEP1_T_LEN = l;
    R8_UEP1_CTRL = (R8_UEP1_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_ACK;
}

void DevEP1_OUT_Deal(uint8_t len) {
    uint8_t i;
    for(i = 0; i < len; i++) pEP1_IN_DataBuf[i] = ~pEP1_OUT_DataBuf[i];
    DevEP1_IN_Deal(len);
}

void USB_handleTransfer(void) {
	uint8_t len, chtype;
	uint8_t errflag = 0;

	// Not idle - Check bits 5:4 in interrupt status register to view token PID identifier.
	// If these bits are not 11 (indicating idle), enter if statement
	if((R8_USB_INT_ST & MASK_UIS_TOKEN) != MASK_UIS_TOKEN) {

		// In host mode, bits 3:0 are response PID identifier. Analyze operation token and endpoint number
		switch(R8_USB_INT_ST & (MASK_UIS_TOKEN | MASK_UIS_ENDP)) {
			// IN token: device sends data to host. Endpoint number in bits 3:0 is 0.
			case UIS_TOKEN_IN:
				{
					// Endpoint 0 is bidirectional endpoint, used for control transfer. "|0" operation is omitted
					// This value is assigned when SETUP packet is received. There will be SETUP
					// packet processing later, corresponding to setup stage of control transfer.
					switch(SetupReqCode) {
						case USB_GET_DESCRIPTOR:
							// Packets over 64 bytes are processed in multiple transfers, first few transfers should be full packets.
							len = SetupReqLen >= DevEP0SIZE ? DevEP0SIZE : SetupReqLen;
							memcpy(pEP0_DataBuf, pDescr, len);
							
							// DMA is directly connected to memory, detects memory modification, then can send data from memory without MCU control.
							// If just copying between arrays without involving physical memory matching DMA, cannot trigger DMA.
							SetupReqLen -= len;			// Record remaining data length to be sent
							pDescr += len;				// Update starting address of next data to be sent
							R8_UEP0_T_LEN = len;

							// Synchronization toggle. Toggle DATA0 and DATA1 in PID for IN direction (T direction for MCU)
							// After assigning handshake packet response (ACK, NAK, STALL) to endpoint control register,
							// hardware packages it into compliant packet, DMA automatically sends
							R8_UEP0_CTRL ^= RB_UEP_T_TOG;
							break;
							
						case USB_SET_ADDRESS:
							// USB standard command, host sets unique address for device, range 0-127, 0 is default address
							// 7-bit address + highest bit user-defined address (default 1), OR with "packet transfer length" 
							// (here "packet transfer length" is later assigned as address bits
							R8_USB_DEV_AD = (R8_USB_DEV_AD & RB_UDA_GP_BIT) | SetupReqLen;

							// R responds to OUT transaction with ACK, T responds to IN transaction with NAK.
							// In this CASE branch for IN direction, when DMA responds in memory and MCU has no data update, return NAK handshake packet.
							// In general program OUT transactions, device returns packet to host, does not respond with NAK.
							R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
							break;

						case USB_SET_FEATURE:
							// USB standard command, host requests to activate a feature on device, interface, or endpoint
							break;

						default:
							// Status stage complete interrupt or force upload 0-length data packet to end control transfer
							// (data packet with data field length 0, containing SYNC, PID, EOP fields)
							R8_UEP0_T_LEN = 0;

							// R responds to OUT transaction with ACK, T responds to IN transaction with NAK.
							// In this CASE branch for OUT direction, when DMA responds with updated data in memory
							// and MCU verification is normal, return ACK handshake packet.
							R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
							Ready = 1;
							// PRINT("Ready_STATUS = %d\n",Ready);
							break;
					}
				}
				break;

			case UIS_TOKEN_OUT:
				// OUT token: host sends data to device, bits 5:4 are 00. Endpoint number in bits 3:0 is 0.
				// Endpoint 0 is bidirectional endpoint, used for control transfer. "|0" operation is omitted
				// Read current number of received data bytes stored in USB receive length register
				// Receive length register is shared by endpoints, transmit length registers are separate
				len = R8_USB_RX_LEN;
				break;

			case UIS_TOKEN_OUT | 1:
				{
					// Hardware checks if data packet synchronization toggle is correct.
					if(R8_USB_INT_ST & RB_UIS_TOG_OK) {
						// Unsynchronized data packets will be discarded
						// DATA synchronization toggle for OUT transaction. Set an expected value.
						R8_UEP1_CTRL ^= RB_UEP_R_TOG;
						len = R8_USB_RX_LEN;	// Read number of received data bytes
						DevEP1_OUT_Deal(len);	// Send bytes with length len, automatically return ACK handshake packet. Custom program.
					}
				}
				break;

			case UIS_TOKEN_IN | 1:
				// Toggle DATA for IN transaction. Set PID of packet to be sent.
				R8_UEP1_CTRL ^= RB_UEP_T_TOG;

				// When DMA has no data updated by MCU, set T response to IN transaction as NAK. If updated, send data.
				R8_UEP1_CTRL = (R8_UEP1_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK;
				Ready = 1;
				// PRINT("Ready_IN_EP1 = %d\n",Ready);
				break;
		}
		R8_USB_INT_FG = RB_UIF_TRANSFER;	// Write 1 to clear interrupt flag
	}

	// Setup packet processing
	if(R8_USB_INT_ST & RB_UIS_SETUP_ACT) {
		R8_UEP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_NAK;
		// R expects DATA1 for OUT transaction (PID of data packet received by DMA must be DATA1, 
		// otherwise data error requiring retransmission) and ACK (DMA received data in memory, MCU verification normal)
		// T sets DATA1 for IN transaction (MCU sends data to DMA memory, sent as DATA1) and NAK (MCU not ready with data).

		// Number of bytes in data stage
		// pSetupReqPak: Cast endpoint 0 RAM address to structure address, structure members arranged sequentially
		SetupReqLen = pSetupReqPak->wLength;
		SetupReqCode = pSetupReqPak->bRequest; 	// Command number
		chtype = pSetupReqPak->bRequestType;	// Contains data transfer direction, command type, recipient info

		len = 0;
		errflag = 0;

		// Check command type, if not standard request, enter if statement
		if((pSetupReqPak->bRequestType & USB_REQ_TYP_MASK) != USB_REQ_TYP_STANDARD) {
			/* Non-standard request: Other requests, such as class requests, vendor requests, etc. */
			if(pSetupReqPak->bRequestType & 0x40) {
				/* Vendor request */
			}
			else if(pSetupReqPak->bRequestType & 0x20) {
				// Determined as HID class request
				switch(SetupReqCode) {
					case DEF_USB_SET_IDLE:
						// Host wants to set idle duration for specific HID device input report
						Idle_Value[pSetupReqPak->wIndex] = (uint8_t)(pSetupReqPak->wValue>>8);
						break;

					case DEF_USB_SET_REPORT:
						// Host wants to set HID device report descriptor
						break;

					case DEF_USB_SET_PROTOCOL:
						// Host wants to set protocol currently used by HID device
						Report_Value[pSetupReqPak->wIndex] = (uint8_t)(pSetupReqPak->wValue);
						break;

					case DEF_USB_GET_IDLE:
						// Host wants to read current idle rate of specific HID device input report
						EP0_Databuf[0] = Idle_Value[pSetupReqPak->wIndex];
						len = 1;
						break;

					case DEF_USB_GET_PROTOCOL:
						// Host wants to get protocol currently used by HID device
						EP0_Databuf[0] = Report_Value[pSetupReqPak->wIndex];
						len = 1;
						break;

					default:
						errflag = 0xFF;
				}
			}
		}

		// Determined as standard request, host commands
		else {
			printf("IM HERE CCC\n");

			switch (SetupReqCode) {
				case USB_GET_DESCRIPTOR: {
					printf("IM HERE 1\n");

					// Right shift 8 bits, check if original high 8 bits are 0, if 1 indicates IN direction
					switch ((pSetupReqPak->wValue) >> 8) {
						// Different values represent different commands. Host wants to get device descriptor
						case USB_DESCR_TYP_DEVICE:
							// Place device descriptor string at pDescr address, copy function will send at end of "get standard descriptor" case
							pDescr = MyDevDescr;
							// Protocol specifies 1st byte of device descriptor stores byte length
							len = MyDevDescr[0];
							break;

						case USB_DESCR_TYP_CONFIG:
							// Place configuration descriptor string at pDescr address, will be sent later
							pDescr = MyCfgDescr;
							// Protocol specifies 3rd byte of configuration descriptor stores total length of configuration information
							len = MyCfgDescr[2];
							break;

						case USB_DESCR_TYP_HID:
							switch((pSetupReqPak->wIndex) & 0xff) {
								case 0:
									// Class descriptor location for interface 1, ready to send
									pDescr = (uint8_t *)(&MyCfgDescr[18]);
									len = 9;
									break;

								default:
									/* Unsupported string descriptor */
									errflag = 0xff;
									break;
							}
							break;

						case USB_DESCR_TYP_REPORT:
							{
								if(((pSetupReqPak->wIndex) & 0xff) == 0) {
									pDescr = HIDDescr; 			// Data ready for upload
									len = sizeof(HIDDescr);
								}
								else
									// This program only has 2 interfaces, this statement should not normally execute
									len = 0xff;
							}
							break;

						case USB_DESCR_TYP_STRING: 
							{
								switch((pSetupReqPak->wValue) & 0xff) {
									case 1:
										pDescr = MyManuInfo;
										len = MyManuInfo[0];
										break;
									case 2:
										pDescr = MyProdInfo;
										len = MyProdInfo[0];
										break;
									case 0:
										pDescr = MyLangDescr;
										len = MyLangDescr[0];
										break;
									default:
										errflag = 0xFF; 	// Unsupported string descriptor
										break;
								}
							}
							break;

						default:
							errflag = 0xff;
							break;
					}
					if(SetupReqLen > len) SetupReqLen = len;	  	// Actual total length to upload
					len = (SetupReqLen >= DevEP0SIZE) ? DevEP0SIZE : SetupReqLen;	// Maximum length is 64 bytes
					memcpy(pEP0_DataBuf, pDescr, len);
					pDescr += len;
				}
				break;

				case USB_SET_ADDRESS:
					printf("IM HERE 2\n");
					// Temporarily store host-assigned device address in SetupReqLen
					// Control stage will assign to device address parameter
					SetupReqLen = (pSetupReqPak->wValue) & 0xff;
					break;

				case USB_GET_CONFIGURATION:
					printf("IM HERE 3\n");
					pEP0_DataBuf[0] = DevConfig;	// Place device configuration in RAM
					// Set data stage byte count to 1. Because DevConfig is only one byte.
					if(SetupReqLen > 1) SetupReqLen = 1;
					break;

				case USB_SET_CONFIGURATION:
					printf("IM HERE 4\n");
					DevConfig = (pSetupReqPak->wValue) & 0xff;
					break;

				case USB_GET_INTERFACE:
					printf("IM HERE 5\n");
					pEP0_DataBuf[0] = 0x00;
					// Set data stage byte count to 1. Because data to transmit is only one byte.
					if(SetupReqLen > 1) SetupReqLen = 1;
					break;

				case USB_SET_INTERFACE:
					break;

				// Disable USB device feature/function. Can be at device or endpoint level.
				case USB_CLEAR_FEATURE:
					printf("IM HERE 111\n");
					// Check if endpoint feature (clear endpoint halt/stall status)
					if((pSetupReqPak->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP) {
						switch((pSetupReqPak->wIndex) & 0xff) {
							// Highest bit of 16-bit determines data transfer direction, 0 for OUT, 1 for IN. Lower bits are endpoint number.
							case 0x81:
								// Clear _TOG and _T_RES bits, write latter as _NAK, respond to IN transaction with NAK indicating no data return
								R8_UEP1_CTRL = (R8_UEP1_CTRL & ~(RB_UEP_T_TOG | MASK_UEP_T_RES)) | UEP_T_RES_NAK;
								break;
							case 0x01:
								// Clear _TOG and _R_RES bits, write latter as _ACK, respond to OUT transaction with ACK indicating normal reception
								R8_UEP1_CTRL = (R8_UEP1_CTRL & ~(RB_UEP_R_TOG | MASK_UEP_R_RES)) | UEP_R_RES_ACK;
								break;
							default:
								errflag = 0xFF; 	// Unsupported endpoint
								break;
						}
					}
					// Check if device feature (for device wakeup)
					else if((pSetupReqPak->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_DEVICE) {
						if(pSetupReqPak->wValue == 1) {
							USB_SleepStatus &= ~0x01;   // Clear lowest bit
						}
					}
					else {
						errflag = 0xFF;
					}
					break;

				// Enable USB device feature/function. Can be at device or endpoint level.
				case USB_SET_FEATURE:
					printf("IM HERE 222\n");

					// Check if endpoint feature (halt/stop endpoint)
					if((pSetupReqPak->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP) {
						switch(pSetupReqPak->wIndex) {
							// Highest bit of 16-bit determines data transfer direction, 0 for OUT, 1 for IN. Lower bits are endpoint number.
							case 0x81:
								// Clear _TOG and _T_RES bits, write latter as _STALL, stop endpoint operation according to host command
								R8_UEP1_CTRL = (R8_UEP1_CTRL & ~(RB_UEP_T_TOG | MASK_UEP_T_RES)) | UEP_T_RES_STALL;
								break;
							case 0x01:
								// Clear _TOG and _R_RES bits, write latter as _STALL, stop endpoint operation according to host command
								R8_UEP1_CTRL = (R8_UEP1_CTRL & ~(RB_UEP_R_TOG | MASK_UEP_R_RES)) | UEP_R_RES_STALL;
								break;
							default:
								errflag = 0xFF;		// Unsupported endpoint
								break;
						}
					}

					// Check if device feature (put device to sleep)
					else if((pSetupReqPak->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_DEVICE) {
						if(pSetupReqPak->wValue == 1) {
							USB_SleepStatus |= 0x01;	// Set sleep
						}
					}
					else {
						errflag = 0xFF;
					}
					break;

				case USB_GET_STATUS:
					printf("IM HERE 333\n");

					if((pSetupReqPak->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP) {
						pEP0_DataBuf[0] = 0x00;

						switch(pSetupReqPak->wIndex) {
							case 0x81:
								// Check _TOG and _T_RES bits, if in STALL state, enter if statement
								if((R8_UEP1_CTRL & (RB_UEP_T_TOG | MASK_UEP_T_RES)) == UEP_T_RES_STALL) {
									// Return D0 as 1, indicating endpoint is halted. This bit is configured by SET_FEATURE and CLEAR_FEATURE commands.
									pEP0_DataBuf[0] = 0x01;
								}
								break;

							case 0x01:
								// Check _TOG and _R_RES bits, if in STALL state, enter if statement
								if((R8_UEP1_CTRL & (RB_UEP_R_TOG | MASK_UEP_R_RES)) == UEP_R_RES_STALL) {
									pEP0_DataBuf[0] = 0x01;
								}
								break;
						}
					}
					else if((pSetupReqPak->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_DEVICE) {
						pEP0_DataBuf[0] = 0x00;
						if(USB_SleepStatus) {
							// Lowest bit D0 is 0 for bus-powered device, 1 for self-powered device. D1 bit is 1 for remote wakeup support, 0 for no support.
							pEP0_DataBuf[0] = 0x02;
						}
						else {
							pEP0_DataBuf[0] = 0x00;
						}
					}
					// Return status information format is 16-bit number, upper 8 bits reserved as 0
					pEP0_DataBuf[1] = 0;

					// Set data stage byte count to 2. Because data to transmit is only 2 bytes.
					if(SetupReqLen >= 2) SetupReqLen = 2;
					break;

				default:
					errflag = 0xff;
					break;
			}
		}

		if(errflag == 0xff) {
			// SetupReqCode = 0xFF;
			R8_UEP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL; // STALL
			Ready = 1;
			// PRINT("Ready_Stall = %d\n",Ready);
		}
		else {
			// Upload. Highest bit is 1, data transfer direction is device to host.
			if(chtype & 0x80) {
				len = (SetupReqLen > DevEP0SIZE) ? DevEP0SIZE : SetupReqLen;
				SetupReqLen -= len;
			}
			else
				// Download. Highest bit is 0, data transfer direction is host to device.
				len = 0;
			R8_UEP0_T_LEN = len;
			// Default data packet is DATA1
			R8_UEP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;
		}

		R8_USB_INT_FG = RB_UIF_TRANSFER;	// Write 1 to clear interrupt flag
	}
}

void USB_DevTransProcess(void) {
	uint8_t intflag = R8_USB_INT_FG;

	// Check USB transfer complete interrupt flag 
	if(intflag & RB_UIF_TRANSFER) {
		USB_handleTransfer();
	}

	// Check bus reset flag in _INT_FG, triggered when 1
	else if (intflag & RB_UIF_BUS_RST) {
		// Set device address to 0, waiting for host to assign new address to device
		R8_USB_DEV_AD = 0;

		// Set endpoint 0 control register to: receive response ACK for normal reception, transmit response NAK for no data to return
		R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
		R8_UEP1_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
		R8_UEP2_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
		R8_UEP3_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
		R8_USB_INT_FG = RB_UIF_BUS_RST; 		// Write 1 to clear interrupt flag
	}

	 // Check bus suspend or wakeup event interrupt flag in _INT_FG. Both suspend and wakeup trigger this interrupt
	else if(intflag & RB_UIF_SUSPEND) {
		// Get suspend status bit in miscellaneous status register, 1 means USB bus is in suspend state, 0 means bus is in non-suspend state
		// When device is idle for more than 3ms, host requests device to suspend (similar to computer sleep)
		if(R8_USB_MIS_ST & RB_UMS_SUSPEND) {
			Ready = 0;
			// PRINT("Ready_Sleep = %d\n",Ready);
		}
		else {
			// wakeup. Suspend or wakeup interrupt triggered, but not determined as suspend
			Ready = 1;
			// PRINT("Ready_WeakUp = %d\n",Ready);
		}
		R8_USB_INT_FG = RB_UIF_SUSPEND; 	// Write 1 to clear interrupt flag
	}
	else {
		// No interrupt flag in _INT_FG, write original value back to register
		R8_USB_INT_FG = intflag;
	}
}


void USB_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast"))) __attribute__((section(".highcode")))  __attribute((interrupt));
void USB_IRQHandler(void) {
	USB_DevTransProcess();
}

void DevHIDReport(uint8_t data0, uint8_t data1, uint8_t data2, uint8_t data3) {
    HID_Buf[0] = data0;
    HID_Buf[1] = data1;
    HID_Buf[2] = data2;
    HID_Buf[3] = data3;
    memcpy(pEP1_IN_DataBuf, HID_Buf, sizeof(HID_Buf));
    DevEP1_IN_Deal(DevEP1SIZE);
}

void USB_DeviceInit(void) {
    pEP0_RAM_Addr = EP0_Databuf;
	pEP1_RAM_Addr = EP1_Databuf;
	pEP2_RAM_Addr = EP2_Databuf;
	pEP3_RAM_Addr = EP3_Databuf;

    R8_USB_CTRL = 0x00;

    R8_UEP4_1_MOD = RB_UEP4_RX_EN | RB_UEP4_TX_EN | RB_UEP1_RX_EN | RB_UEP1_TX_EN;
    R8_UEP2_3_MOD = RB_UEP2_RX_EN | RB_UEP2_TX_EN | RB_UEP3_RX_EN | RB_UEP3_TX_EN;

    R16_UEP0_DMA = (uint16_t)(uint32_t)pEP0_RAM_Addr;
    R16_UEP1_DMA = (uint16_t)(uint32_t)pEP1_RAM_Addr;
    R16_UEP2_DMA = (uint16_t)(uint32_t)pEP2_RAM_Addr;
    R16_UEP3_DMA = (uint16_t)(uint32_t)pEP3_RAM_Addr;

    R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
    R8_UEP1_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK | RB_UEP_AUTO_TOG;
    R8_UEP2_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK | RB_UEP_AUTO_TOG;
    R8_UEP3_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK | RB_UEP_AUTO_TOG;
    R8_UEP4_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;

    R8_USB_DEV_AD = 0x00;
    R8_USB_CTRL = RB_UC_DEV_PU_EN | RB_UC_INT_BUSY | RB_UC_DMA_EN;
    R16_PIN_ANALOG_IE |= RB_PIN_USB_IE | RB_PIN_USB_DP_PU;
    R8_USB_INT_FG = 0xFF;
    R8_UDEV_CTRL = RB_UD_PD_DIS | RB_UD_PORT_EN;
    R8_USB_INT_EN = RB_UIE_SUSPEND | RB_UIE_BUS_RST | RB_UIE_TRANSFER;

    NVIC_EnableIRQ( USB_IRQn );
}
