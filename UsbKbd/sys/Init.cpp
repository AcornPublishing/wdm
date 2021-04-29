//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1999 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	UsbKbd example
/////////////////////////////////////////////////////////////////////////////
//	init.cpp:		Driver initialization code
/////////////////////////////////////////////////////////////////////////////
//	DriverEntry		Initialisation entry point
//	UsbKbdUnload		Unload driver routine
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	27-Apr-99	1.0.0	CC	creation
/////////////////////////////////////////////////////////////////////////////

#include "UsbKbd.h"

#pragma code_seg("INIT") // start INIT section

/////////////////////////////////////////////////////////////////////////////
//	DriverEntry:
//
//	Description:
//		This function initializes the driver, and creates
//		any objects needed to process I/O requests.
//
//	Arguments:
//		Pointer to the Driver object
//		Registry path string for driver service key
//
//	Return Value:
//		This function returns STATUS_XXX

extern "C"
NTSTATUS DriverEntry(	IN PDRIVER_OBJECT DriverObject,
						IN PUNICODE_STRING RegistryPath)
{
	NTSTATUS status = STATUS_SUCCESS;

#if DBG
	DebugPrintInit("UsbKbd checked");
#else
	DebugPrintInit("UsbKbd free");
#endif

	DebugPrint("RegistryPath is %T",RegistryPath);

	// Export other driver entry points...
 	DriverObject->DriverExtension->AddDevice = UsbKbdAddDevice;
 	DriverObject->DriverUnload = UsbKbdUnload;

	DriverObject->MajorFunction[IRP_MJ_CREATE] = UsbKbdCreate;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = UsbKbdClose;

	DriverObject->MajorFunction[IRP_MJ_PNP] = UsbKbdPnp;
	DriverObject->MajorFunction[IRP_MJ_POWER] = UsbKbdPower;

	DriverObject->MajorFunction[IRP_MJ_READ] = UsbKbdRead;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = UsbKbdWrite;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = UsbKbdDeviceControl;

	DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = UsbKbdSystemControl;

	DebugPrintMsg("DriverEntry completed");

	return status;
}
#pragma code_seg() // end INIT section

//////////////////////////////////////////////////////////////////////////////
//	UsbKbdUnload
//
//	Description:
//		Unload the driver by removing any remaining objects, etc.
//
//	Arguments:
//		Pointer to the Driver object
//
//	Return Value:
//		None

#pragma code_seg("PAGE") // start PAGE section

VOID UsbKbdUnload(IN PDRIVER_OBJECT DriverObject)
{
	DebugPrintMsg("UsbKbdUnload");
	DebugPrintClose();
}

//////////////////////////////////////////////////////////////////////////////
#pragma code_seg() // end PAGE section
